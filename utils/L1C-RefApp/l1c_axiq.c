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
#include "l1c_rf_ctrl.h"
#include "l1c_axiq.h"

vuint32 * l1c_get_axiq_ctrl(uint8_t interface)
{
	vuint32 * puTimerCtrl;
	uint8_t ucTbgenNo, dcs;

	dcs = interface >> 1;
	if (dcs < DCS_HS)
	{
		ucTbgenNo = TBGEN_1;
	}
	else
	{
		ucTbgenNo = TBGEN_2;
		interface &= 0x1;
	}

	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * (interface) ));

	return puTimerCtrl;
}

vuint32 * l1c_get_tdd_ctrl(uint8_t tbgen_no, uint8_t instance)
{
	vuint32 * puTimerCtrl;

	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( tbgen_no ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * (instance) ));

	return puTimerCtrl;
}

void l1c_tdd_timer_program(vuint32 * timer,
					  uint64_t uStartOffset,
					  tdd_tbgen_seq_t *steps,
					  uint8_t steps_count,
					  uint8_t repetitive,
					  uint8_t idle_txrx_mode)
{
	vuint32 * puTddxOffsetHi = ( vuint32 * ) ((u8 *)timer + 0x4);
	vuint32 * puTddxOffsetLo = ( vuint32 * ) ((u8 *)timer + 0x8);
	vuint32 * puTddxMode     = ( vuint32 * ) ((u8 *)timer + 0xc);
	vuint32 * puTddxDuration = ( vuint32 * ) ((u8 *)timer + 0x10);
	uint32_t uTddMode = TDD_MODE_00;
	uint32_t uTimeStampHi, uTimeStampLo;
	uint8_t i;

	if (!steps_count) {
		TBGEN_WRITE_REGISTER(timer, (TBGEN_READ_REGISTER(timer) & CTRL_TMR_DISABLE_MASK));
		return;
	}
	/* populate durations and modes */
	for (i = 0; i < steps_count; i++)
	{
		TBGEN_WRITE_REGISTER((vuint32 *)((u8 *)puTddxDuration + i*4), steps[i].duration);
		uTddMode |= (steps[i].tx_rx_allowed << (i * 2));
	}

	TBGEN_WRITE_REGISTER(puTddxMode, uTddMode);

	/* program the offset where the sequence starts */
	uTimeStampHi = (uint32_t) ((uStartOffset & HI_WORD_MASK) >> HI_WORD_SHIFT_BITS);
	uTimeStampLo = (uint32_t) (uStartOffset & LO_WORD_MASK);

	TBGEN_WRITE_REGISTER( puTddxOffsetHi, uTimeStampHi );
	TBGEN_WRITE_REGISTER( puTddxOffsetLo, uTimeStampLo );

	/* Enable timer, generate continous sequences */
	TBGEN_WRITE_REGISTER(timer,
						(TDD_CTRL_TMREN_MASK << TDD_CTRL_TMREN_BIT) |
						((repetitive & TDD_CTRL_CONTSEQ_MASK) << TDD_CTRL_CONTSEQ_BIT) |
						((idle_txrx_mode & TDD_CTRL_TXRXEN_MASK) << TDD_CTRL_TXRXEN_BIT) |
						((steps_count - 1) << TDD_CTRL_BUFLENGTH_BIT));
}

void l1c_tdd_timer_update_steps(vuint32 * timer,
					  tdd_tbgen_seq_t *steps,
					  uint8_t last_step,
					  uint8_t updated_steps_count)
{
	if (!timer)
		return;

	uint32_t uTddMode = TDD_MODE_00;
	uint8_t i;
	vuint32 * puTddxMode     = ( vuint32 * ) ((u8 *)timer + 0xc);
	vuint32 * puTddxDuration = ( vuint32 * ) ((u8 *)timer + 0x10);
	uTddMode = TBGEN_READ_REGISTER(puTddxMode);

	/* populate durations and modes */
	for (i = last_step-1; i < updated_steps_count; i++)
	{
		TBGEN_WRITE_REGISTER((vuint32 *)((u8 *)puTddxDuration + i*4), steps[i].duration);
		uTddMode |= (steps[i].tx_rx_allowed << (i * 2));
	}

	TBGEN_WRITE_REGISTER(puTddxMode, uTddMode);

	TBGEN_WRITE_REGISTER(timer,
						(TDD_CTRL_TMREN_MASK << TDD_CTRL_TMREN_BIT) |
						(TDD_CTRL_CONTSEQ_MASK << TDD_CTRL_CONTSEQ_BIT) |
						((updated_steps_count - 1) << TDD_CTRL_BUFLENGTH_BIT));
}

void l1c_tdd_trx_dump(tdd_tbgen_seq_t *t, uint8_t steps)
{
	uint8_t i;

	for (i = 0; i < steps; i++)
		PRINTF("sequence[%d]: mode=%#x, duration=%#x\r\n", i, t[i].tx_rx_allowed, t[i].duration);

	PRINTF("\r\n");
}

vuint32 * l1c_get_timer_lp_ctrl(uint8_t interface)
{
	vuint32 * puTimerCtrl;
	uint8_t ucTbgenNo, dcs;

	dcs = interface >> 1;
	if (dcs < DCS_HS)
	{
		return NULL;		// Not using low-power mode for LSDCS
	}
	else
	{
		ucTbgenNo = TBGEN_2;
		interface &= 0x1;
		puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * (interface + 2) ));
	}

	return puTimerCtrl;
}

void l1c_timer_lp_wp_program(vuint32 * timer,
							uint64_t uStartOffset,
							tdd_tbgen_seq_t *steps,
							uint8_t steps_count,
							uint8_t repetitive)
{
	vuint32 * puTddxOffsetHi = ( vuint32 * ) ((u8 *)timer + 0x4);
	vuint32 * puTddxOffsetLo = ( vuint32 * ) ((u8 *)timer + 0x8);
	vuint32 * puTddxMode     = ( vuint32 * ) ((u8 *)timer + 0xc);
	vuint32 * puTddxDuration = ( vuint32 * ) ((u8 *)timer + 0x10);
	uint32_t uTimeStampHi, uTimeStampLo;
	uint32_t uTddMode = TDD_MODE_00;
	uint8_t i;

	if (!timer)
		return;

	if (!steps_count) {
		TBGEN_WRITE_REGISTER(timer, (TBGEN_READ_REGISTER(timer) & CTRL_TMR_DISABLE_MASK));
		return;
	}

	/* populate durations and modes */
	for (i = 0; i < steps_count; i++)
	{
		TBGEN_WRITE_REGISTER((vuint32 *)((u8 *)puTddxDuration + i*4), steps[i].duration);

		if (steps[i].tx_rx_allowed == TDD_MODE_10)
			uTddMode = uTddMode | (TDD_MODE_01 << (i * 2));
	}


	TBGEN_WRITE_REGISTER(puTddxMode, uTddMode);

	/* program the offset where the sequence starts */
	uTimeStampHi = (uint32_t) ((uStartOffset & HI_WORD_MASK) >> HI_WORD_SHIFT_BITS);
	uTimeStampLo = (uint32_t) (uStartOffset & LO_WORD_MASK);

	TBGEN_WRITE_REGISTER( puTddxOffsetHi, uTimeStampHi );
	TBGEN_WRITE_REGISTER( puTddxOffsetLo, uTimeStampLo );

	/* Enable timer, generate continous sequences */
	TBGEN_WRITE_REGISTER(timer,
			     (TDD_CTRL_TMREN_MASK << TDD_CTRL_TMREN_BIT) |
			     ((repetitive & TDD_CTRL_CONTSEQ_MASK) << TDD_CTRL_CONTSEQ_BIT) |
			     ((TDD_MODE_00 & TDD_CTRL_TXRXEN_MASK) << TDD_CTRL_TXRXEN_BIT) |
			     ((steps_count - 1) << TDD_CTRL_BUFLENGTH_BIT));
}


void l1c_timer_lp_wp_update_steps(vuint32 * timer,
								  tdd_tbgen_seq_t *steps,
								  uint8_t last_step,
								  uint8_t updated_steps_count)
{
	vuint32 * puTddxMode     = ( vuint32 * ) ((u8 *)timer + 0xc);
	vuint32 * puTddxDuration = ( vuint32 * ) ((u8 *)timer + 0x10);
	uint32_t uTddMode = TDD_MODE_00;
	uint8_t i;

	if (!timer)
		return;

	uTddMode = TBGEN_READ_REGISTER(puTddxMode);
	/* populate durations and modes */
	for (i = last_step-1; i < updated_steps_count; i++)
	{
		TBGEN_WRITE_REGISTER((vuint32 *)((u8 *)puTddxDuration + i*4), steps[i].duration);

		if (steps[i].tx_rx_allowed == TDD_MODE_10)
			uTddMode = uTddMode | (TDD_MODE_01 << (i * 2));
	}

	TBGEN_WRITE_REGISTER(puTddxMode, uTddMode);

	TBGEN_WRITE_REGISTER(timer,
						(TDD_CTRL_TMREN_MASK << TDD_CTRL_TMREN_BIT) |
						(TDD_CTRL_CONTSEQ_MASK << TDD_CTRL_CONTSEQ_BIT) |
						((updated_steps_count - 1) << TDD_CTRL_BUFLENGTH_BIT));
}
