/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2022 NXP */

#ifndef _L1C_TIME_UTILS_H
#define _L1C_TIME_UTILS_H
#include "l1c_time_agent.h"

typedef struct time_actions_node_s
{
	struct time_actions_node_s *next;
	time_action_t action;
} time_actions_node_t;

time_actions_node_t* new_node(time_action_t time_action_data);
void remove_executed_from_list(time_actions_node_t **node);
void add_to_list(time_actions_node_t **node, time_action_t *to_add);
void add_to_list_sorted(time_actions_node_t **head_ref, time_actions_node_t *new_node);
void remove_all_list(time_actions_node_t **node);

void insert_rf_time_actions(uint32_t *next_time_slot);
void insert_rf_ctrl_gpio_actions(time_actions_node_t **node, rf_ctrl_gpio_signal_t *controls, u64 transition_time, bool_t tx_to_rx);
#endif
