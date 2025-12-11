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
#include "l1c_time_agent.h"
#include "l1c_time_proc.h"
#include "l1c_time_utils.h"
#include "l1c_fwk_tasks.h"
#include "l1c_axiq.h"

uint32_t ofdm_sym_time[SCS_MAX] __attribute__ ((section (".shared.data"))) = {
	[SCS_kHz15]  = 17536, /* 15KHz:  (4096 + 288)*4  */
	[SCS_kHz30]  = 8768,  /* 30KHz:  (4096 + 288)*2  */
	[SCS_kHz60]  = 4384,  /* 60KHz:  (4096 + 288)    */
	[SCS_kHz120] = 2192,  /* 120KHz: (4096 + 288)/2  */
	[SCS_kHz240] = 1096,  /* 240KHz: (4096 + 288)/4  */
};

uint32_t ofdm_long_sym_time[SCS_MAX] __attribute__ ((section (".shared.data"))) = {
	[SCS_kHz15]  = 17664, /* 15KHz:  (4096 + 320)*4  */
	[SCS_kHz30]  = 8896,  /* 30KHz:  (4096 + 352)*2  */
	[SCS_kHz60]  = 4512,  /* 60KHz:  (4096 + 416)    */
	[SCS_kHz120] = 2320,  /* 120KHz: (4096 + 544)/2  */
	[SCS_kHz240] = 1224,  /* 240KHz: (4096 + 800)/4  */
};

uint32_t tick_interval[SCS_MAX] __attribute__ ((section (".shared.data"))) = {
	[SCS_kHz15]  = TBGEN_500US * 2,  /* 0x3c000 */
	[SCS_kHz30]  = TBGEN_500US,      /* 0x1e000 */
	[SCS_kHz60]  = TBGEN_500US / 2,  /* 0xf000  */
	[SCS_kHz120] = TBGEN_500US / 4,  /* 0x7800  */
	[SCS_kHz240] = TBGEN_500US / 8,  /* 0x3C00  */
};

tdd_tbgen_seq_t trx_allow[MAX_TDD_SEQUENCE_STEPS] __attribute__ ((section (".smem")));
uint8_t trx_allow_steps __attribute__ ((section (".smem")));

tdd_tbgen_seq_t trx_enable[TDD_MAX_INSTANCE][MAX_TDD_SEQUENCE_STEPS];
tdd_tbgen_seq_t trx_tmp[MAX_TDD_SEQUENCE_STEPS];
int trx_advance[TDD_MAX_INSTANCE];
uint8_t trx_enable_steps[TDD_MAX_INSTANCE];


extern QueueHandle_t time_agent_queue;
uint8_t time_slots;

time_actions_node_t **time_table;

static int64_t tbgen1_to_tbgen2_offset __attribute__ ((section (".smem")));
extern uint64_t axiq_start_time;
int64_t tbgen_offset()
{
	return tbgen1_to_tbgen2_offset;
}

void time_map_dump_all();

void tbgen_offset_calc()
{
	vuint32 *tbgen_tstamp_trig_reg = (vuint32 *)(SCFG_BASE_ADDR + TBGEN_TSTAMP_TRIGGER_OFFSET);
	uint64_t tbgen1_tsgp2, tbgen2_tsgp2;
	vuint32 *tsgp2_hi, *tsgp2_lo;

	/* Enable SFCG based tbgen trigger timestamp2 */
	vConfigTsMcu(TBGEN_1, 2, 1);
	vConfigTsMcu(TBGEN_2, 2, 1);

	/* Trigger the SCFG based timestamping on both tbgen */
	out_le32(tbgen_tstamp_trig_reg, 0x3);

	/* Disable SCFG based tbgen trigger timestamp2 */
	vConfigTsMcu(TBGEN_1, 2, 0);
	vConfigTsMcu(TBGEN_2, 2, 0);

	tsgp2_hi = (vuint32 *)(TBGEN_BASE(TBGEN_1) + TBGEN_TSGP2HI_OFFSET);
	tsgp2_lo = (vuint32 *)(TBGEN_BASE(TBGEN_1) + TBGEN_TSGP2LO_OFFSET);

	tbgen1_tsgp2 = ((u64)TBGEN_READ_REGISTER(tsgp2_hi) << 32) | TBGEN_READ_REGISTER(tsgp2_lo);

	tsgp2_hi = (vuint32 *)(TBGEN_BASE(TBGEN_2) + TBGEN_TSGP2HI_OFFSET);
	tsgp2_lo = (vuint32 *)(TBGEN_BASE(TBGEN_2) + TBGEN_TSGP2LO_OFFSET);

	tbgen2_tsgp2 = ((u64)TBGEN_READ_REGISTER(tsgp2_hi) << 32) | TBGEN_READ_REGISTER(tsgp2_lo);

	if (tbgen1_tsgp2 != tbgen2_tsgp2) {
		tbgen1_to_tbgen2_offset = (int64_t) ((int64_t)tbgen1_tsgp2 - (int64_t)tbgen2_tsgp2);
		
		PRINTF("TBGEN1 @ %#lx%08lx, TBGEN2 @ %#lx%08lx, delta is %#lx%08lx\r\n",
			(uint32_t)(tbgen1_tsgp2 >> 32), (uint32_t)tbgen1_tsgp2,
			(uint32_t)(tbgen2_tsgp2 >> 32), (uint32_t)tbgen2_tsgp2,
			(uint32_t)(tbgen1_to_tbgen2_offset >> 32), (uint32_t)tbgen1_to_tbgen2_offset);
	} else {
		tbgen1_to_tbgen2_offset = 0;
		PRINTF("Both tbgen are in sync\n\r");
	}
}

void add_tdd_transition(rf_ctrl_tbgen_signal_t *tdd_signals, tdd_tbgen_seq_t *axiq_seq, uint8_t step)
{
	uint8_t i;
	bool is_tx = 0;

	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++)
	{
		/* check for empty entries */
		if (unlikely(!tdd_signals || !tdd_signals[i].desc))
			break;

		/* check for non-TDD entries */
		if (tdd_signals[i].timer_type != TDD)
			continue;

		uint8_t idx = tdd_signals[i].timer_instance & TDD_INSTANCE_MASK;
		uint8_t trx = (tdd_signals[i].timer_instance & TDD_TX_RX_MASK) >> 4; /* TDD_MODE_xx*/

		/* figure out whether we should transmit or receive and set the polarities */
		is_tx = !!(axiq_seq->tx_rx_allowed & TDD_MODE_10);

		if (is_tx) {
			if (tdd_signals[i].polarity == STROBE_POL_FALLING)
				trx = TDD_MODE_00;
		} else {
			if (tdd_signals[i].polarity == STROBE_POL_RISING)
				trx = TDD_MODE_00;
		}

		if (!step)
			trx_advance[idx] = (is_tx ? tdd_signals[i].rx_tx_delta : tdd_signals[i].tx_rx_delta);

		if (trx_enable[idx][step].duration) {
			/* A timer instance can only control two signals (tx_enable, rx_enable).
			   If a step for the timer instance is already programmed for a given
			   duration, then merge tx_rx_allowed
			*/
			trx_enable[idx][step].tx_rx_allowed |= trx;
		} else {
			trx_enable[idx][step].tx_rx_allowed = trx;
			trx_enable[idx][step].duration = axiq_seq->duration;
		}
	}
}

void l1c_rf_tdd_signals_compute(rf_ctrl_tbgen_signal_t *tdd_signals)
{
	uint8_t i, j, k;

	memset(trx_enable, 0, sizeof(trx_enable));
	memset(trx_enable_steps, 0, sizeof(trx_enable_steps));

	/* use the pre-computed AXIQ transitions to build the RF card signal transitions */
	for (i = 0; i < trx_allow_steps; i++)
		add_tdd_transition(tdd_signals, &trx_allow[i], i);

	/* Init number of steps for each TDD timer used for RF_ctrl equal to trx_allow_steps */
	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++) {
		uint8_t idx;

		/* check for empty entries */
		if (unlikely(!tdd_signals || !tdd_signals[i].desc))
			continue;

		/* check for non-TDD entries */
		if (tdd_signals[i].timer_type != TDD)
			continue;

		idx = tdd_signals[i].timer_instance & TDD_INSTANCE_MASK;
		trx_enable_steps[idx] = trx_allow_steps;
	}

	/* apply time advances and optimize number of entries that will be written to hw */
	for (i = 0; i < TDD_MAX_INSTANCE; i++)
	{
		if (!trx_enable_steps[i])
			continue;

		/* colapse consecutive sequences of the same type (tdd_mode_xx) */
		k = 0;
		j = 0;
		memset(trx_tmp, 0, sizeof(trx_tmp));

		trx_tmp[k].tx_rx_allowed = trx_enable[i][j].tx_rx_allowed;
		trx_tmp[k].duration = trx_enable[i][j].duration;

		for (j = 1; j < trx_enable_steps[i]; j++)
		{
			if (trx_tmp[k].tx_rx_allowed == trx_enable[i][j].tx_rx_allowed)
			{
				trx_tmp[k].duration += trx_enable[i][j].duration;
			}
			else
			{
				k++;
				trx_tmp[k].tx_rx_allowed = trx_enable[i][j].tx_rx_allowed;
				trx_tmp[k].duration = trx_enable[i][j].duration;
			}
		}

		/* apply advance */
		trx_tmp[0].duration -= trx_advance[i];
		trx_tmp[k].duration += trx_advance[i];

		memcpy(&trx_enable[i], &trx_tmp, sizeof(trx_tmp));
		trx_enable_steps[i] = k+1;
	}
}

void l1c_axiq_trx_allow_compute(tdd_ul_dl_pattern_t *p,
								tdd_scs_t scs,
								uint32_t *last_sym,
								bool_t always_listen,
								uint32_t tdd_ul_dl_gap_ns,
								tdd_tbgen_seq_t *t,
								uint8_t *steps)
{
	uint8_t tdd_idx = *steps;
	uint32_t duration_dl = 0;
	uint32_t duration_ul = 0;
	uint32_t duration_idle = 0;
	uint32_t duration_ul_dl_gap = 0;
	uint32_t sym_ctr = 0;
	uint32_t sym_idx = *last_sym;
	uint32_t long_sym_step = (scs * MAX_SYMBOLS);
	uint32_t pattern_g_syms = 0;
	uint32_t pattern_dl_syms = (p->dl_slots * MAX_SYMBOLS) + p->dl_syms;
	uint32_t pattern_ul_syms = (p->ul_slots * MAX_SYMBOLS) + p->ul_syms;

	if (pattern_dl_syms && pattern_ul_syms)
		pattern_g_syms = MAX_SYMBOLS - p->dl_syms - p->ul_syms;

	for (sym_ctr = 0; sym_ctr < pattern_dl_syms; sym_ctr++)
	{
		duration_dl += (sym_idx % long_sym_step) ? ofdm_sym_time[scs] : ofdm_long_sym_time[scs];
		sym_idx++;
	}

	for (sym_ctr = 0; sym_ctr < pattern_g_syms; sym_ctr++)
	{
		duration_idle += (sym_idx % long_sym_step)? ofdm_sym_time[scs] : ofdm_long_sym_time[scs];
		sym_idx++;
	}

	for (sym_ctr = 0; sym_ctr < pattern_ul_syms; sym_ctr++)
	{
		duration_ul += (sym_idx % long_sym_step)? ofdm_sym_time[scs] : ofdm_long_sym_time[scs];
		sym_idx++;
	}

	/* program sequences for DLs, G (idle), ULs - durations and modes
	 *
	 *                           ______________
	 *  RX_ALLOW   _____________|              |______ _ _ __
	 *             ________                       _______ _ _ _
	 *  TX_ALLOW           |_____________________|
	 *
	 *                 DL      G     UL         G   DL ...
	 *  MODE           10      00    01         00  10
	 *                TX=1          TX=0        |
	 *                RX=0          RX=1    ul-dl-gap
	 */

	if (duration_dl) {
		if (CPE == modem_mode)
			t[tdd_idx].tx_rx_allowed = TDD_MODE_01; /* rx_allow */
		else
			t[tdd_idx].tx_rx_allowed = TDD_MODE_10 | (always_listen ? TDD_MODE_01 : 0); /* tx_allow */
			
		t[tdd_idx++].duration = duration_dl;
	}

	/* substract the ul-dl-gap for TDD pattern */
	if (duration_dl && duration_ul) {
		duration_ul_dl_gap = (tdd_ul_dl_gap_ns * TBGEN_4NS) / 4;
		if (duration_idle > duration_ul_dl_gap)
			duration_idle -= duration_ul_dl_gap;
	}

	if (duration_idle) {
		t[tdd_idx].tx_rx_allowed = TDD_MODE_00 | (always_listen ? TDD_MODE_01 : 0);
		t[tdd_idx++].duration = duration_idle;
	}

	if (duration_ul) {
		if (CPE == modem_mode)
			t[tdd_idx].tx_rx_allowed = TDD_MODE_10 | (always_listen ? TDD_MODE_01 : 0); /* tx_allow */
		else
			t[tdd_idx].tx_rx_allowed = TDD_MODE_01; /* rx_allow */
		t[tdd_idx++].duration = duration_ul;
	}

	if (duration_ul_dl_gap && duration_idle) {
		t[tdd_idx].tx_rx_allowed = TDD_MODE_00 | (always_listen ? TDD_MODE_01 : 0);
		t[tdd_idx++].duration = duration_ul_dl_gap;
	}

	*last_sym = *last_sym + pattern_dl_syms + pattern_ul_syms + pattern_g_syms;
	*steps = tdd_idx;
	//PRINTF("total_syms: %u, tdd_idx: %d\n\r", *last_sym, tdd_idx);
}

void time_map_init()
{
	time_action_t tmp_action;
	uint32_t i, k;
	uint32_t total_syms = 0;
	uint32_t long_sym_step = (config_common.scs * MAX_SYMBOLS);

	time_slots = config_common.pattern.dl_slots;
	time_slots += config_common.pattern.ul_slots;
	if (config_common.pattern.dl_syms || config_common.pattern.ul_syms)
		time_slots += 1;

	time_slots += config_common.pattern2.dl_slots;
	time_slots += config_common.pattern2.ul_slots;
	if (config_common.pattern2.dl_syms || config_common.pattern2.ul_syms)
		time_slots += 1;

	/* figure out how many times the pattern needs to be repeaded within 20ms window */
	k = max_slots[config_common.scs] / time_slots;
	time_slots = max_slots[config_common.scs];

	time_table = (time_actions_node_t **)pvPortMalloc(time_slots * sizeof(time_actions_node_t *));
	for (i = 0; i < time_slots; i++) {
		time_table[i] = NULL;
	}

	memset(trx_allow, 0, sizeof(trx_allow));
	trx_allow_steps = 0;

	/* compute AXIQ timings */
	for (total_syms = 0;;)
	{
		l1c_axiq_trx_allow_compute(&config_common.pattern,
							   config_common.scs,
							   &total_syms,
							   config_common.rx_always_listen,
							   config_common.tdd_ul_dl_gap,
							   trx_allow,
							   &trx_allow_steps);

		l1c_axiq_trx_allow_compute(&config_common.pattern2,
							   config_common.scs,
							   &total_syms,
							   config_common.rx_always_listen,
							   config_common.tdd_ul_dl_gap,
							   trx_allow,
							   &trx_allow_steps);

		if (total_syms % long_sym_step == 0)
			break;
	}

	memset(&tmp_action, 0, sizeof(tmp_action));
	tmp_action.type = TIME_ACTION_TDD;
	tmp_action.executed = 0;
	tmp_action.action.tdd_action.update = 0;
	tmp_action.action.tdd_action.repetitive = 1;
	tmp_action.action.tdd_action.idle_txrx_mode = TDD_MODE_00;
	tmp_action.action.tdd_action.output.steps = trx_allow;
	tmp_action.action.tdd_action.output.steps_count = trx_allow_steps;
	tmp_action.action.tdd_action.time_offset = 0;

	for (i = 0; i < MAX_USED_INTERFACES; i++)
	{
		if (!config_common.interfaces[i].in_use)
			continue;

		tmp_action.action.tdd_action.output.timer = config_common.interfaces[i].axiq_ctrl;
		tmp_action.action.tdd_action.output.lp_timer = config_common.interfaces[i].lp_ctrl;
		add_to_list(&time_table[0], &tmp_action);

		/* compute RF card TDD signals, if any, based on AXIQ steps */
		l1c_rf_tdd_signals_compute(config_common.interfaces[i].rf_fem_tdd_ctrl_ptr);
	}

#ifndef NO_RF
	for (i = 0; i < MAX_OBS_IF; i++)
	{
		tdd_tbgen_seq_t steps[2];
		uint32_t clk_ratio;

		if (!config_common.obs_if[i].in_use)
			continue;

		clk_ratio = 8;	// hsdcs@19660800, tbgen@245760000

		/* Hardcoded 8192 hsadc samples once every 10s */
		steps[0].duration = config_common.obs_if[i].samples/clk_ratio;
		steps[0].tx_rx_allowed = TDD_MODE_01;

		steps[1].duration = config_common.obs_if[i].period/clk_ratio - steps[0].duration;
		steps[1].tx_rx_allowed = TDD_MODE_00;
		
		l1c_tdd_timer_program(config_common.obs_if[i].axiq_ctrl, axiq_start_time, &steps[0], 2, 1, 0);
	}
	
	if (!config_common.tdd_control_for_rf_card)
	{
		for (uint32_t idx = 0, i = 0; i < k; i++)
		{
			insert_rf_time_actions(&idx);
		}
	}

	/* if there are any TBGEN2 TDD timers involved in controlling RF card, add actions */
	for (i = 0; i < TDD_MAX_INSTANCE; i++)
	{
		if (!trx_enable_steps[i])
			continue;

		tmp_action.action.tdd_action.output.steps = trx_enable[i];
		tmp_action.action.tdd_action.output.steps_count = trx_enable_steps[i];
		tmp_action.action.tdd_action.idle_txrx_mode = TDD_MODE_11;
		tmp_action.action.tdd_action.output.timer = l1c_get_tdd_ctrl(TBGEN_2, i);
		tmp_action.action.generic_action.time_offset = -tbgen_offset() + trx_advance[i];
		add_to_list(&time_table[0], &tmp_action);
	}
#else
	(void) k;
#endif

	time_map_dump_all();

#if 1
	PRINTF("AXIQ sequences:\r\n");
	l1c_tdd_trx_dump(trx_allow, trx_allow_steps);

	for (uint8_t kk = 4; kk < TDD_MAX_INSTANCE; kk++)
	{
		PRINTF("trx_enable[%d] (trx_advance = %#x, steps = %d):\r\n", kk, trx_advance[kk], trx_enable_steps[kk]);
		l1c_tdd_trx_dump(trx_enable[kk], trx_enable_steps[kk]);
	}
#endif
}

void time_map_list_dump(time_actions_node_t *head)
{
	uint8_t i = 0;

	while (head)
	{
#ifdef L1C_REFAPP_DEBUG_ADVANCED
		PRINTF("\t\t[%d], type=%d, time_offset=0x%08x%08x, executed=%d, when_to_exec=0x%08x%08x\r\n",
				i++,
				head->action.type,
				(uint32_t)(head->action.action.generic_action.time_offset >> 32),
				(uint32_t)(head->action.action.generic_action.time_offset),
				head->action.executed,
				(uint32_t)(head->action.when >> 32),
				(uint32_t)(head->action.when)
			);
#else
		PRINTF("\t\t[%d], type=%d, time_offset=0x%08x%08x\r\n",
				i++,
				head->action.type,
				(uint32_t)(head->action.action.generic_action.time_offset >> 32),
				(uint32_t)(head->action.action.generic_action.time_offset)
			);
#endif
		head = head->next;
	}
}

void time_map_dump_all()
{
	uint8_t i;

	for (i = 0; i < time_slots; i++)
	{
		if (!time_table[i])
			continue;

		PRINTF("SLOT[%d]\r\n", i);
		time_map_list_dump(time_table[i]);
	}
}

void l1c_time_proc_slots(void *pvParameters)
{
	task_id_t task_id = *(task_id_t *)pvParameters;
	int64_t time_offset = 0L;
	time_actions_node_t *taction;
	time_action_t tmp_action;
	uint8_t idx = 0;

	time_map_init();

	/* wait for tick, in a loop */
	for( ;; )
	{
		wait_for_tick(task_id);

		/* if no valid pattern was configured, skip */
		if (!time_slots)
			continue;

		air_slots++;

		if (!time_offset)
			time_offset = start_airtime + air_slots * tick_interval[config_common.scs];
		else
			time_offset += tick_interval[config_common.scs];

		taction = time_table[idx];
		while (taction)
		{
			tmp_action = taction->action;

			if (tmp_action.executed) {
				taction = taction->next;
				continue;
			}

			tmp_action.action.generic_action.time_offset += (int64_t) time_offset;

			l1c_time_agent_enqueue_msg(&tmp_action);

			/* TDD programming only occurs once */
			if (taction->action.type == TIME_ACTION_TDD)
				taction->action.executed = 1;

			taction = taction->next;
		}

		idx++;
		idx %= time_slots;
	}
}

void dump_axiq_sequences()
{
	l1c_tdd_trx_dump(trx_allow, trx_allow_steps);
}

void l1c_time_add_stop_action()
{
	time_action_t tmp_action;

	/* new sequence to be added for gratiously stopping axiq */
	trx_allow[trx_allow_steps].tx_rx_allowed = TDD_MODE_00;
	trx_allow[trx_allow_steps].duration = TBGEN_1S; //max_slots[config_common.scs] * tick_interval[config_common.scs];

	memset(&tmp_action, 0, sizeof(tmp_action));
	tmp_action.type = TIME_ACTION_TDD;
	tmp_action.executed = 0;
	tmp_action.action.tdd_action.update = 1;
	tmp_action.action.tdd_action.output.steps = trx_allow;
	tmp_action.action.tdd_action.output.steps_count = trx_allow_steps;

	remove_all_list(&time_table[0]);

	for (uint8_t i = 0; i < MAX_USED_INTERFACES; i++)
	{
		if (!config_common.interfaces[i].in_use)
			continue;

		tmp_action.action.tdd_action.output.timer = config_common.interfaces[i].axiq_ctrl;
		tmp_action.action.tdd_action.output.lp_timer = config_common.interfaces[i].lp_ctrl;
		add_to_list(&time_table[0], &tmp_action);
	}

	tmp_action.type = TIME_ACTION_TDD;
	tmp_action.action.tdd_action.update = 0;
	tmp_action.action.tdd_action.output.steps_count = 0;

	remove_all_list(&time_table[1]);
	remove_all_list(&time_table[2]);

	for (uint8_t i = 0; i < MAX_USED_INTERFACES; i++)
	{
		if (!config_common.interfaces[i].in_use)
			continue;

		tmp_action.action.tdd_action.output.timer = config_common.interfaces[i].axiq_ctrl;
		tmp_action.action.tdd_action.output.lp_timer = config_common.interfaces[i].lp_ctrl;
		add_to_list(&time_table[2], &tmp_action);
	}

#ifndef NO_RF
	/* if there were any TDD actions related to TBGEN2 RF control, stop TDD timers */
	tmp_action.type = TIME_ACTION_TDD;
	tmp_action.action.tdd_action.update = 0;
	tmp_action.action.tdd_action.output.steps_count = 0;
	tmp_action.executed = 0;
	for (uint8_t i = 0; i < TDD_MAX_INSTANCE; i++)
	{
		if (!trx_enable_steps[i])
			continue;

		tmp_action.action.tdd_action.output.timer = l1c_get_tdd_ctrl(TBGEN_2, i);
		add_to_list(&time_table[1], &tmp_action);
	}

#endif
}

void destroy_all()
{
	uint8_t i;

	for (i = 0; i < max_slots[config_common.scs]; i++)
		remove_all_list(&time_table[i]);

	vPortFree(time_table);
	time_table = NULL;
}
