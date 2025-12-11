/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

#ifndef _L1C_TIME_PROC_H
#define _L1C_TIME_PROC_H

#include "l1c_axiq.h"
#include "l1c_rf_ctrl.h"
#include "gpio.h"

int64_t tbgen_offset();
void tbgen_offset_calc();
extern tdd_tbgen_seq_t trx_allow[MAX_TDD_SEQUENCE_STEPS];
extern uint8_t trx_allow_steps;
extern tdd_tbgen_seq_t trx_enable[TDD_MAX_INSTANCE][MAX_TDD_SEQUENCE_STEPS];
extern int trx_advance[TDD_MAX_INSTANCE];
extern uint8_t trx_enable_steps[TDD_MAX_INSTANCE];

void dump_axiq_sequences();
void l1c_time_proc_main(void *pvParameters);
void l1c_time_add_stop_action();
void l1c_time_proc_slots(void *pvParameters);
void destroy_all();
int64_t get_tbgen2_mc_offset_diff(uint64_t tbgen1_mc);
void l1c_rf_tdd_signals_compute(rf_ctrl_tbgen_signal_t *tdd_signals);
#endif
