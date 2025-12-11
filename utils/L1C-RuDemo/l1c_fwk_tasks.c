/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

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
#include "l1c_fwk_tasks.h"

extern uint8_t crt_core_id;

/* Task database */
task_map_t tasks_map __attribute__ ((section (".smem")));

void l1c_create_task(uint32_t core_id, uint32_t task_id, const char *const task_name, tick_type_t tick_en, UBaseType_t task_priority, const configSTACK_DEPTH_TYPE stack_size, TaskFunction_t pxTaskCode)
{
	tasks_map.tasks[task_id].core_id = core_id;
	tasks_map.tasks[task_id].task_id = task_id;
	tasks_map.tasks[task_id].tick    = tick_en;

	if (core_id == crt_core_id)
	{
		if (!tasks_map.tasks[task_id].task_handle)
		{
			if (tick_en)
			{
				tasks_map.tasks[task_id].tick_sem = xSemaphoreCreateBinary();

				if (! tasks_map.tasks[task_id].tick_sem)
					PRINTF("Could not create tick semaphore for task %s\r\n", task_name);
			}
#ifdef L1C_REFAPP_DEBUG
			PRINTF("\r\nCreating task %s with id %d on core %d, having stack size %#x\r\n",
				   task_name,
				   task_id,
				   core_id,
				   stack_size);
#endif
			xTaskCreate(pxTaskCode,
						task_name,
						stack_size,
						&tasks_map.tasks[task_id].task_id,
						task_priority,
						&tasks_map.tasks[task_id].task_handle);
			return;
		}

#ifdef L1C_REFAPP_DEBUG_ADVANCED
		PRINTF("Task with id %d and handle 0x%x on core %d already exists !\r\n",
				task_id,
				tasks_map.tasks[task_id].task_handle,
				core_id);
#endif
	}
}

task_id_t get_crt_task_id(void)
{
	TaskHandle_t crt_task_handle;
	task_id_t ret_task_id = L1C_UNKNOWN_TASK;
	uint32_t i;

	crt_task_handle = xTaskGetCurrentTaskHandle();

	for (i = 0; i < L1C_TASK_MAX_ID; i++)
	{
		if ((tasks_map.tasks[i].task_handle == crt_task_handle) && (tasks_map.tasks[i].core_id == crt_core_id))
		{
			ret_task_id = tasks_map.tasks[i].task_id;
			break;
		}
	}

	return ret_task_id;
}

void wait_for_tick(task_id_t recv_task_id)
{
	xSemaphoreTake(tasks_map.tasks[recv_task_id].tick_sem, portMAX_DELAY);
}

void l1c_task_suspend(uint32_t task_id)
{
	xTaskHandle t = tasks_map.tasks[task_id].task_handle;

	if (t)
		vTaskSuspend(t);
}

void l1c_task_resume(uint32_t task_id)
{
	xTaskHandle t = tasks_map.tasks[task_id].task_handle;

	if (t)
		vTaskResume(t);
}
