/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef _L1C_AXIQ_H
#define _L1C_AXIQ_H

typedef struct
{
    uint32_t duration;
    uint32_t tx_rx_allowed;
} tdd_tbgen_seq_t;

void l1c_tdd_timer_program(vuint32 *timer, uint64_t uStartOffset, tdd_tbgen_seq_t *steps, uint8_t steps_count, uint8_t repetitive, uint8_t idle_txrx_mode);
void l1c_tdd_timer_update_steps(vuint32 * timer, tdd_tbgen_seq_t *steps, uint8_t last_step, uint8_t updated_steps_count);
vuint32 *l1c_get_axiq_ctrl(uint8_t interface);
vuint32 *l1c_get_tdd_ctrl(uint8_t tbgen_no, uint8_t instance);
void l1c_tdd_trx_dump(tdd_tbgen_seq_t *t, uint8_t steps);

vuint32 * l1c_get_timer_lp_ctrl(uint8_t interface);
void l1c_timer_lp_wp_program(vuint32 * timer,
							 uint64_t uStartOffset,
							 tdd_tbgen_seq_t *steps,
							 uint8_t steps_count,
							 uint8_t repetitive);
void l1c_timer_lp_wp_update_steps(vuint32 * timer,
								  tdd_tbgen_seq_t *steps,
								  uint8_t last_step,
								  uint8_t updated_steps_count);
#endif
