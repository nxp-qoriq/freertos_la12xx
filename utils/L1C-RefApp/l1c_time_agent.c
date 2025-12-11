/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "tbgen_new.h"
#include "la12xx_tbgen.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "l1c_defs.h"
#include "l1c_time_agent.h"
#include "l1c_time_utils.h"
#include "l1c_fwk_tasks.h"
#include "l1c_axiq.h"
#include "semphr.h"

uint8_t time_slots;
QueueHandle_t time_agent_queue = NULL;
time_agent_msg_t time_msg[TIME_AGENT_NUM_Q_ENTRIES];
SemaphoreHandle_t list_mutex;
time_actions_node_t *deferred_actions = NULL;

void cal_spin_loop(int cycles_mul_of_5)
{
	// ~20 cycles fixed overhead, then it's ~5.019 cycles per iteration
#define CAL_LOOP_OVH 20
#define CAL_LOOP_MUL 1000
#define CAL_LOOP_DIV 5019
	volatile int cnt = (cycles_mul_of_5 > CAL_LOOP_OVH) ? cycles_mul_of_5 : CAL_LOOP_OVH;

	cnt = (cycles_mul_of_5 - CAL_LOOP_OVH) * CAL_LOOP_MUL / CAL_LOOP_DIV;

	while (cnt-- > 0) {}
}

void tbgen_wait_master_counter(uint8_t tbgen_no, uint64_t mc)
{
	/* yield to other tasks while waiting for time to pass */
	while (mc >= ullTbgenGetMasterCounter(tbgen_no))
		vTaskDelay(0);
}

void l1c_periodical_tick(uint8_t eTbgenInstance, TimerInstance_t eInstance, void *data)
{
	uint8_t i;
	UNUSED(eTbgenInstance);
	UNUSED(eInstance);
	UNUSED(data);

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	BaseType_t xHigherPriorityTaskWokenAny = pdFALSE;

	no_irqs[crt_core_id]++;

	/* unlock tasks waiting for tick */
	for (i = 1; i < L1C_TASK_MAX_ID; i++)
	{
		if (tasks_map.tasks[i].core_id == crt_core_id) 
			if (tasks_map.tasks[i].tick == TICK_ENABLE)
				xSemaphoreGiveFromISR(tasks_map.tasks[i].tick_sem, &xHigherPriorityTaskWoken);

		xHigherPriorityTaskWokenAny |= xHigherPriorityTaskWoken;
	}

	portYIELD_FROM_ISR( xHigherPriorityTaskWokenAny );
}

void l1c_setup_periodical_tick(uint8_t eTbgenInstance, uint64_t uStartOffset, uint64_t uDuration, uint8_t e200_dst_core)
{
	TimerParams_t * pxTimerParams = (TimerParams_t *) pvPortMalloc(sizeof(TimerParams_t));

	if (!pxTimerParams)
		return;

	/* Configure Timed Interrupt */
	pxTimerParams->ePolarity = STROBE_POL_RISING;   /* Polarity Rising */
	pxTimerParams->uOffset = uStartOffset;          /* When to start */
	pxTimerParams->uInterval = uDuration;           /* Timer interval */
	pxTimerParams->eSm = STROBE_MODE_PULSE;         /* Pulse Mode */
	pxTimerParams->ePw = PULSE_WIDTH_CLK_CYCLE_1;   /* If PulseMode = STROBE_MODE_PULSE */
	pxTimerParams->eTrigMode = TM_REPETITIVE;
	pxTimerParams->pvCb = l1c_periodical_tick;      /* Timer callback */

	/* Program RX_ALIGNMENT Timer Instance 0 */
	iTbgenProgramTimer( eTbgenInstance, RX_ALIGNMENT, TIMER_INSTANCE_0 + e200_dst_core, pxTimerParams );
	iTbgenEnableTimer( eTbgenInstance, RX_ALIGNMENT, TIMER_INSTANCE_0 + e200_dst_core );

	// PRINTF("l1c_setup_periodical_tick: tbgen=%d, start=%#x%08x, duration=%#x%08x, e200_dst_core=%d\r\n",
	// 	   eTbgenInstance,
	// 	   (u32)(uStartOffset >> 32), (u32)uStartOffset,
	// 	   (u32)(uDuration >> 32), (u32)uDuration,
	// 	   e200_dst_core);
	vPortFree(pxTimerParams);
}

void l1c_stop_periodical_tick(uint8_t eTbgenInstance, uint8_t e200_dst_core)
{
	iTbgenDisableTimer(eTbgenInstance, RX_ALIGNMENT, TIMER_INSTANCE_0 + e200_dst_core);
}

inline void l1c_gpio_init(gpio_output_t out)
{
	if (exGpioInit(out.gpio, out.pin, GPIO_OUTPUT))
		PRINTF("\n GPIO init failed");
}

inline void l1c_gpio_set(gpio_time_action_t action)
{
	//PRINTF("gpio = %d, pin = %d, val = %d\r\n",  action.output.gpio, action.output.pin, action.value);
	if (exGpioSetData(action.output.gpio, action.output.pin, action.value))
		PRINTF("\n GPIO set data failed");
}

bool_t wait_pps_sync(uint8_t eTbgenInstance)
{
	u64 ts;
	int retries = 1100;

	ts = ullTbgenGet10MSCounter(eTbgenInstance);

	/* wait for 1s to pass - align to time boundary */
	while (--retries && (ts == ullTbgenGet10MSCounter(eTbgenInstance)))
		vTaskDelay(1);
	
	return !!retries;
}

void l1c_time_agent_enqueue_msg(time_action_t *t)
{
	time_agent_msg_t *msg;
	static uint8_t pool_idx = 0;

	if (!time_agent_queue)
		return;

	/* not thread-safe. enqueue API is only used from a VSPA procedure */
	msg = &time_msg[pool_idx];
	memcpy(&msg->msg, t, sizeof(time_action_t));
	pool_idx++;
	pool_idx %= TIME_AGENT_NUM_Q_ENTRIES;

	xQueueSend(time_agent_queue, &msg, (TickType_t)0);
}

void l1c_time_agent_deferred_main(void *pvParameters)
{
	task_id_t task_id = *(task_id_t *)pvParameters;
	time_actions_node_t *current;

	for( ;; )
	{
		/* tick unlocks loop, thus saving cpu time and limiting access to deferred_list */
		wait_for_tick(task_id);

		/* check if there's something to do; the events are sorted by time */
		for (current = deferred_actions; current != NULL; current = current->next)
		{
			tbgen_wait_master_counter(TBGEN_1, deferred_actions->action.when);
			if (current->action.type == TIME_ACTION_GPIO)
			{
				/* save time when setting GPIOs by doing it NOW */
				l1c_gpio_set(current->action.action.gpio_action);
			}
			else
			{
				/* everything else that was postponed will be sent back to agent to be executed ASAP */
				current->action.when = 0;
				l1c_time_agent_enqueue_msg(&current->action);
			}
			current->action.executed = 1;
		}

		xSemaphoreTake(list_mutex, portMAX_DELAY);
		remove_executed_from_list(&deferred_actions);
		xSemaphoreGive(list_mutex);
	}
}

void destroy_deferred_actions()
{
	remove_all_list(&deferred_actions);
}

void l1c_time_agent_main(void *pvParameters)
{
	time_agent_msg_t *recv;

	UNUSED(pvParameters);

	if (!list_mutex)
		list_mutex = xSemaphoreCreateMutex();

	/* create receive queue at the first run */
	if (!time_agent_queue)
		time_agent_queue = xQueueCreate(TIME_AGENT_NUM_Q_ENTRIES, sizeof(time_agent_msg_t *));

	if (!time_agent_queue)
	{
		/* Queue was not created and must not be used. */
		PRINTF("Error creating Time Agent message queue!\r\n");
		return;
	}

	/* wait for messages in a loop */
	for( ;; )
	{
		xQueueReceive(time_agent_queue, (void *)&recv, portMAX_DELAY);

		if (recv->msg.when)
		{
			/* add to "execution" time the time_offset */
			recv->msg.when += recv->msg.action.generic_action.time_offset;
			time_actions_node_t *new = new_node(recv->msg);
			xSemaphoreTake(list_mutex, portMAX_DELAY);
			add_to_list_sorted(&deferred_actions, new);
			xSemaphoreGive(list_mutex);
			continue;
		}

		/* execute actions based on their types */
		switch (recv->msg.type)
		{
			case TIME_ACTION_TDD:
				if (recv->msg.action.tdd_action.update)
				{
					l1c_tdd_timer_update_steps(recv->msg.action.tdd_action.output.timer,
											   recv->msg.action.tdd_action.output.steps,
											   recv->msg.action.tdd_action.output.steps_count,
											   recv->msg.action.tdd_action.output.steps_count + 1);
					if(recv->msg.action.tdd_action.output.timer_lp_wp)
						l1c_timer_lp_wp_update_steps(recv->msg.action.tdd_action.output.timer_lp_wp,
												recv->msg.action.tdd_action.output.steps,
												recv->msg.action.tdd_action.output.steps_count,
												recv->msg.action.tdd_action.output.steps_count + 1);
				}
				else
				{
					l1c_tdd_timer_program(recv->msg.action.tdd_action.output.timer,
										  recv->msg.action.tdd_action.time_offset,
										  recv->msg.action.tdd_action.output.steps,
										  recv->msg.action.tdd_action.output.steps_count,
										  recv->msg.action.tdd_action.repetitive,
										  recv->msg.action.tdd_action.idle_txrx_mode);

					if(recv->msg.action.tdd_action.output.timer_lp_wp)
						l1c_timer_lp_wp_program(recv->msg.action.tdd_action.output.timer_lp_wp,
											recv->msg.action.tdd_action.time_offset,
											recv->msg.action.tdd_action.output.steps,
											recv->msg.action.tdd_action.output.steps_count,
											recv->msg.action.tdd_action.repetitive);
				}
				break;

			case TIME_ACTION_RF:
				l1c_rf_ctrl_sig_transition(recv->msg.action.rf_ctrl_action.output.rf_gpio,
										   recv->msg.action.rf_ctrl_action.time_offset,
										   recv->msg.action.rf_ctrl_action.output.switch_txrx);
				break;

			case TIME_ACTION_GPIO:
				l1c_gpio_set(recv->msg.action.gpio_action);
				break;

			case TIME_ACTION_TASK:
				l1c_create_task(recv->msg.action.task_action.task_details->core_id,
								recv->msg.action.task_action.task_details->task_id,
								recv->msg.action.task_action.task_details->task_name,
								recv->msg.action.task_action.task_details->tick_en,
								recv->msg.action.task_action.task_details->task_prio,
								DEFAULT_STACK_SIZE,
								recv->msg.action.task_action.task_details->task_code);
				break;

			default:
				/* should never get here */
				break;
		}
	}
}
