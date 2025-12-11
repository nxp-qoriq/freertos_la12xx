/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "tbgen_new.h"
#include "la12xx_tbgen.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "l1c_defs.h"
#include "l1c_axiq.h"
#include "l1c_rf_ctrl.h"
#include "l1c_time_proc.h"
// #define LS10465GRU  // Specific for S. board

TimerParams_t ctrl_sig_params;

//#define IN_USE  1
#define TRIG_OUT_ONESHOT 1

l1c_trig_out_signal_t * l1c_get_trig_out_controls(int trig_num);


int l1c_trig_out_setup(int trig_num, uint64_t start_time)
{
	l1c_trig_out_signal_t *trig_out;
	TimerInstance_t timer_instance;
	
	trig_out = l1c_get_trig_out_controls(trig_num);
	if (!trig_out) {
		PRINTF("trig_num: %d not found\n\r", trig_num);
		return -1;
	}

	if (trig_out->tbgen == TBGEN_2)
		start_time = start_time + tbgen_offset();

	timer_instance = trig_out->timer_instance & TDD_INSTANCE_MASK;

	if (trig_out->timer_type == TDD) {
		uint8_t ucTxRx = (trig_out->timer_instance & TDD_TX_RX_MASK) >> 4;
		vuint32 * timer = l1c_get_tdd_ctrl(trig_out->tbgen, timer_instance);
		tdd_tbgen_seq_t steps[2];

		if (iConfPMuxModeTbgenTdd(timer_instance, ucTxRx/2)) {
			PRINTF("PMUX config failed for out trigger number: %d\n\r", trig_num);
			return -1;
		}

#ifdef TRIG_OUT_ONESHOT
		steps[0].duration = 0x0;
		steps[0].tx_rx_allowed = ucTxRx;
		l1c_tdd_timer_program(timer, start_time, &steps[0], 1, 1, TDD_MODE_00);
#else
		steps[0].duration = 16;
		steps[0].tx_rx_allowed = ucTxRx;

		steps[1].duration = 245759992;
		steps[1].tx_rx_allowed = TDD_MODE_00;
		l1c_tdd_timer_program(timer, start_time, &steps[0], 2, 1, TDD_MODE_00);
#endif
	} else {
		iConfPMuxModeTbgen(trig_out->tbgen, trig_out->timer_type, trig_out->timer_instance);

		memset(&ctrl_sig_params, 0, sizeof(ctrl_sig_params));

#ifdef TRIG_OUT_ONESHOT
		ctrl_sig_params.eTrigMode = TM_ONE_SHOT;
		ctrl_sig_params.eSm = STROBE_MODE_TOGGLE;
#else
		// Lite-On Reference Implementaion Observation
		// PULSE mode trigger is not observed for TRIG_OUT1
		ctrl_sig_params.eTrigMode = TM_REPETITIVE;
		ctrl_sig_params.eSm = STROBE_MODE_PULSE;
		ctrl_sig_params.uInterval = 245760000;
		ctrl_sig_params.ePw = 16;
#endif

		ctrl_sig_params.ePolarity = STROBE_POL_RISING;
		ctrl_sig_params.uOffset = start_time;

		iTbgenProgramTimer(trig_out->tbgen, trig_out->timer_type, timer_instance, &ctrl_sig_params);
		iTbgenEnableTimer(trig_out->tbgen, trig_out->timer_type, timer_instance);
	}

	return 0;
}


void l1c_rf_ctrl_sig_setup(rf_ctrl_tbgen_signal_t *controls, u64 transition_time)
{
	uint8_t i;

	memset(&ctrl_sig_params, 0, sizeof(ctrl_sig_params));

	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++)
	{
		if (unlikely(!controls || !controls[i].desc))
			break;

		/* we are doing non-TDD operations here */
		if (controls[i].timer_type == TDD)
			continue;

#ifdef L1C_REFAPP_DEBUG
		PRINTF("rf_fem_control[%d]: %s set to %d\r\n", i, controls[i].desc, controls[i].polarity);
#endif
		ctrl_sig_params.eTrigMode = TM_ONE_SHOT;
		ctrl_sig_params.eSm = STROBE_MODE_TOGGLE;
		ctrl_sig_params.ePolarity = controls[i].polarity;
		ctrl_sig_params.uOffset = transition_time;

		iTbgenProgramTimer(controls[i].tbgen, controls[i].timer_type, controls[i].timer_instance, &ctrl_sig_params);
		iTbgenEnableTimer(controls[i].tbgen, controls[i].timer_type, controls[i].timer_instance);
	}
}

void l1c_rf_ctrl_sig_transition(rf_ctrl_tbgen_signal_t *controls, u64 transition_time, bool_t tx_to_rx)
{
	u32 i;

	/* check if rf_fem ctrl list is empty and exit early */
	if (!controls[0].in_use)
		return;

	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++)
	{
		/* no more signals to control */
		if (!controls[i].in_use)
			break;

		/* we are doing non-TDD operations here */
		if (controls[i].timer_type == TDD)
			continue;

		/* avoid glitches in TBGEN signals when programming transitions */
		if ((tx_to_rx && !controls[i].tx_on) || (!tx_to_rx && controls[i].tx_on))
			continue;

		controls[i].tx_on = !tx_to_rx;

		iTbgenReloadTimerAndPolarity(controls[i].tbgen,
									 controls[i].timer_type,
									 controls[i].timer_instance,
									 ((tx_to_rx) ? controls[i].polarity : !controls[i].polarity),
									 transition_time + ((tx_to_rx) ? controls[i].tx_rx_delta : controls[i].rx_tx_delta));
	}

	return;
}
