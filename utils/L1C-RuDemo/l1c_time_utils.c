/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2022-2023 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "ppu_intrinsics.h"
#include "l1c_defs.h"
#include "l1c_time_utils.h"
#include "l1c_time_proc.h"

extern time_actions_node_t **time_table;

/*
 * Example of linked lists used by time actions/procedures
 *
 * Feel free to do your own, use static buffers instead of heap
 * RefApp should not be impacted by implementation
 */

time_actions_node_t* new_node(time_action_t time_action_data)
{
	time_actions_node_t *new_node = (time_actions_node_t *)pvPortMalloc(sizeof(time_actions_node_t));

	/* put in the data  */
	new_node->action = time_action_data;
	new_node->next = NULL;

	return new_node;
}

void add_to_list_sorted(time_actions_node_t **head_ref, time_actions_node_t *new_node)
{
	time_actions_node_t* current;

	if (*head_ref == NULL || (*head_ref)->action.when >= new_node->action.when)
	{
		new_node->next = *head_ref;
		*head_ref = new_node;
	}
    else
	{
		current = *head_ref;
		while ((current->next != NULL) && (current->next->action.when < new_node->action.when))
		{
			current = current->next;
		}
		new_node->next = current->next;
		current->next = new_node;
	}
}

void add_to_list(time_actions_node_t **node, time_action_t *to_add)
{
	time_actions_node_t *iter;

	if (!node)
		return;

	/* create head if empty */
	if (!*node) {
		*node = (time_actions_node_t *) pvPortMalloc(sizeof(time_actions_node_t));
		(*node)->action = *to_add;
		(*node)->next = NULL;
		return;
	}

	iter = *node;
	while (iter->next)
		iter = iter->next;

	iter->next = (time_actions_node_t *) pvPortMalloc(sizeof(time_actions_node_t));
	iter = iter->next;

	iter->action = *to_add;
	iter->next = NULL;
}

void remove_node_from_list(time_actions_node_t **head_ref, time_actions_node_t *node)
{
	time_actions_node_t *current, *to_remove, *prev;

	if ((*head_ref == NULL) || (!node))
		return;

	if (*head_ref == node)
	{
		to_remove = node;
		*head_ref = (*head_ref)->next;
		vPortFree(to_remove);
		return;
	}

	prev = *head_ref;
	current = (*head_ref)->next;
	while (current != node)
	{
		current = current->next;
		prev = prev->next;
	}

	to_remove = current;
	prev->next = current->next;
	vPortFree(to_remove);
}

void remove_from_list(time_actions_node_t **node, time_action_type_t to_remove_type)
{
	time_actions_node_t *iter_curr = NULL, *iter_prev = NULL, *to_remove = NULL;

	/* if the list is not initialized */
	if (!node)
		return;

	/* if the list is empty */
	if (!*node)
		return;

	while(*node)
	{
		if ((*node)->action.type == to_remove_type)
		{
			to_remove = *node;
			*node = (*node)->next;
			vPortFree(to_remove);
		}
		else
		{
			break;
		}
	}

	iter_curr = iter_prev = *node;
	while (iter_curr)
	{
		if (iter_curr->action.type == to_remove_type)
		{
			iter_prev->next = iter_curr->next;
			to_remove = iter_curr;
			iter_curr = iter_curr->next;
			vPortFree(to_remove);
		}
		else {
			iter_prev = iter_curr;
			iter_curr = iter_curr->next;
		}
	}
}

void remove_executed_from_list(time_actions_node_t **node)
{
	time_actions_node_t *iter_curr = NULL, *iter_prev = NULL, *to_remove = NULL;

	/* if the list is not initialized */
	if (!node)
		return;

	/* if the list is empty */
	if (!*node)
		return;

	while(*node)
	{
		if ((*node)->action.executed) {
			to_remove = *node;
			*node = (*node)->next;
			vPortFree(to_remove);
		}
		else
		{
			break;
		}
	}

	iter_curr = iter_prev = *node;
	while (iter_curr)
	{
		if (iter_curr->action.executed)
		{
			iter_prev->next = iter_curr->next;
			to_remove = iter_curr;
			iter_curr = iter_curr->next;
			vPortFree(to_remove);
		}
		else {
			iter_prev = iter_curr;
			iter_curr = iter_curr->next;
		}
	}
}

void remove_all_list(time_actions_node_t **node)
{
	time_actions_node_t *to_remove = NULL;

	/* if the list is not initialized */
	if (!node)
		return;

	/* if the list is empty */
	if (!*node)
		return;

	while(*node)
	{
		to_remove = *node;
		*node = (*node)->next;
		vPortFree(to_remove);
	}
}

void insert_rf_ctrl_gpio_actions(time_actions_node_t **node, rf_ctrl_gpio_signal_t *controls, u64 transition_time, bool_t tx_to_rx)
{
	time_action_t tmp_action;
	u32 i;

	/* check if rf_fem ctrl list is empty and exit early */
	if (!controls || !controls[0].in_use)
		return;

	memset(&tmp_action, 0, sizeof(time_action_t));

	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++)
	{
		/* no more signals to control */
		if (!controls[i].in_use)
			break;

		tmp_action.type = TIME_ACTION_GPIO;
		tmp_action.executed = 0;
		tmp_action.when = transition_time + ((tx_to_rx) ? controls[i].tx_rx_delta : controls[i].rx_tx_delta);
		/* Compensate the time to actualy set the GPIOs and all the polling of TBGEN MasterCounter
		 * When having everything on a single core, we must loose some of the RX window for a clean TX.
		 * Additional to time adances, we shall loose ~2.5uS from guard period (that's ok),
		 * respectively ~3.7uS from the end of RX (might be good enough if the UL-DL-gap is large enough
		 */
		tmp_action.when += (uint64_t)((tx_to_rx) ? +600 : -900);
		tmp_action.action.gpio_action.output.gpio = controls[i].gpio;
		tmp_action.action.gpio_action.output.pin = controls[i].pin;
		tmp_action.action.gpio_action.value = ((tx_to_rx) ? !controls[i].polarity : controls[i].polarity);
		tmp_action.action.gpio_action.time_offset = 0;

		add_to_list(node, &tmp_action);
	}

	return;
}

void insert_rf_time_actions(uint32_t *next_time_slot)
{
	u64 tick_len = tick_interval[config_common.scs];
	u64 duration = 0;
	u64 total_duration = 0;
	time_action_t tmp_action;
	uint32_t curr_idx = *next_time_slot;
	uint32_t prev_mode = ~0;
	uint32_t i, k, switch_txrx, gpio_idx = 0;

	PRINTF("Inside insert_rf_time_actions\r\n");

	/* non-TDD time actions are not supported for SCS > 60kHz */
	if (config_common.scs > SCS_kHz60)
		return;

	memset(&tmp_action, 0, sizeof(time_action_t));
	tmp_action.type = TIME_ACTION_RF;

//#ifdef L1C_REFAPP_DEBUG
#define DEBUG_RF_TA(x) \
	PRINTF("Add time action on slot %d: tx =%s, time offset relative to slot %#x, interface %d\r\n", \
		curr_idx, \
		tmp_action.action.rf_ctrl_action.output.switch_txrx ? " OFF" : " ON ", \
		(uint32_t) tmp_action.action.rf_ctrl_action.time_offset, \
		(x));
//#else
//#define DEBUG_RF_TA(x)
//#endif

	/* look into trx_allow for computing the time_slots where action is needed.
	 * avoid recomputing everything again.
	 */
	for (k = 0; k < trx_allow_steps; k++)
	{
		total_duration += trx_allow[k].duration;

		/* check if RX */
		switch_txrx = !(trx_allow[k].tx_rx_allowed & TDD_MODE_10);

		if (prev_mode == switch_txrx)
		{
			duration += trx_allow[k].duration;
			continue;
		}
		else
		{
			prev_mode = switch_txrx;
			duration = trx_allow[k].duration;
		}

		/* check if TX should be ON or OFF */
		if (switch_txrx)
		{
			tmp_action.action.rf_ctrl_action.output.switch_txrx = 1;
			tmp_action.action.rf_ctrl_action.time_offset = (total_duration - duration) % tick_len;
		}
		else
		{
			tmp_action.action.rf_ctrl_action.output.switch_txrx = 0;
			tmp_action.action.rf_ctrl_action.time_offset = 0;
		}

		for (i = 0; i < MAX_USED_INTERFACES; i++)
		{
			if (!config_common.interfaces[i].in_use)
				continue;

			//PRINTF("cur_idx = %d\r\n", curr_idx);
			tmp_action.action.rf_ctrl_action.output.rf_gpio = config_common.interfaces[i].rf_fem_ctrl_ptr;
			add_to_list(&time_table[curr_idx], &tmp_action);

			/* gpio actions must be executed in the previous time slot */
			gpio_idx = (curr_idx > 0) ? (curr_idx - 1) : (max_slots[config_common.scs] - 1);
			insert_rf_ctrl_gpio_actions(&time_table[gpio_idx],
										config_common.interfaces[i].rf_fem_gpio_ctrl_ptr,
										tmp_action.action.rf_ctrl_action.time_offset + tick_len,
										switch_txrx);

			DEBUG_RF_TA(i);
		}

		curr_idx += duration / tick_len;
	}

	curr_idx += duration / tick_len;
	*next_time_slot = curr_idx + 1;
	//PRINTF("next time slot = %d\r\n", *next_time_slot);
}
