/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef _L1C_TIME_AGENT_H
#define _L1C_TIME_AGENT_H

#include "l1c_defs.h"
#include "l1c_axiq.h"
#include "l1c_rf_ctrl.h"
#include "l1c_fwk_tasks.h"
#include "gpio.h"

/* GPIO Time Actions */
#define MAX_GPIO_TIME_ACTIONS                3

#define TIME_AGENT_NUM_Q_ENTRIES        10

#define CAL_SPIN_10_US    (1*6144) // cycles
#define CAL_SPIN_50_US    (5*6144) // cycles
#define CAL_SPIN_100_US  (10*6144) // cycles
#define CAL_SPIN_250_US  (25*6144) // cycles
#define CAL_SPIN_500_US  (50*6144) // cycles
#define CAL_SPIN_1000_US (100*6144) // cycles

typedef struct {
	GpioModule_t gpio;
	uint8_t  pin;
} gpio_output_t;

typedef struct {
	int64_t time_offset;
	uint32_t value;
	gpio_output_t output;
} gpio_time_action_t;

/* TBGen non-TDD Time Actions */
typedef struct {
	rf_ctrl_tbgen_signal_t *rf_gpio;
	uint8_t switch_txrx;
} rf_trx_output_t;

typedef struct {
	int64_t time_offset;
	rf_trx_output_t output;
} rf_trx_time_action_t;

/* TBGen TDD Time Actions */
typedef struct {
	int64_t time_offset;
	vuint32 *timer;
	vuint32 *timer_lp_wp;
	tdd_tbgen_seq_t *steps;
	uint32_t steps_count;
} tbgen_tdd_output_t;

typedef struct {
	int64_t time_offset;
	tbgen_tdd_output_t output;
	uint8_t update;
	uint8_t repetitive;
	uint8_t idle_txrx_mode;
} tbgen_tdd_time_action_t;

typedef struct {
	core_id_t core_id;
	task_id_t task_id;
	const char *task_name;
	tick_type_t tick_en;
	UBaseType_t task_prio;
	TaskFunction_t task_code;
} task_details_t;

typedef struct {
	int64_t time_offset;
	uint8_t valid;
	uint8_t executed;
	task_details_t *task_details;
} task_time_action_t;

typedef struct {
	int64_t time_offset;
} generic_time_action_t;

typedef enum {
	TIME_ACTION_INVALID = 0,
	TIME_ACTION_GPIO,
	TIME_ACTION_TDD,
	TIME_ACTION_RF,
	TIME_ACTION_TASK,
	TIME_ACTION_CNT_MAX
} time_action_type_t;

typedef struct {
	time_action_type_t type;
	union {
		generic_time_action_t   generic_action;
		gpio_time_action_t      gpio_action;
		tbgen_tdd_time_action_t tdd_action;
		rf_trx_time_action_t    rf_ctrl_action;
		task_time_action_t      task_action;
	} action;
	uint64_t when; /* 0 means NOW! */
	uint8_t executed;
} time_action_t;

typedef struct {
	time_action_t msg;
} time_agent_msg_t;

extern uint8_t time_slots;

void l1c_setup_periodical_tick(uint8_t eTbgenInstance, uint64_t uStartOffset, uint64_t uDuration, uint8_t e200_dst_core);
void l1c_stop_periodical_tick(uint8_t eTbgenInstance, uint8_t e200_dst_core);
void l1c_time_agent_main(void *);
void l1c_time_agent_deferred_main(void *);
bool_t wait_pps_sync(uint8_t eTbgenInstance);
void cal_spin_loop(int cycles_mul_of_5);
void tbgen_wait_master_counter(uint8_t tbgen_no, uint64_t mc);
void l1c_time_agent_enqueue_msg(time_action_t *t);
void destroy_deferred_actions();
#endif
