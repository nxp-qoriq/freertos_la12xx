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
#include "ppu_intrinsics.h"
#include "l1c_defs.h"
#include "l1c_dma.h"
#include "qdma.h"
#include "l1c_vspa_agent.h"
#include "l1c_vspa_proc.h"
#include "l1c_rf_ctrl.h"
#include "l1c_axiq.h"
#include "l1c_fwk_tasks.h"
#include "l1c_time_agent.h"
#include "l1c_time_proc.h"
#include "l1c_dpd.h"
#include "l1c_ipi.h"
#include "gul_bsp_init.h"
#include "l1c_debug.h"
#include "l1c_bench.h"

extern volatile uint32_t brd_ver;
extern void init_l1c_refapp_host_if();

uint32_t max_slots[SCS_MAX] = {
	[SCS_kHz15]  = MAX_SLOTS_30kHz / 2,
	[SCS_kHz30]  = MAX_SLOTS_30kHz,
	[SCS_kHz60]  = MAX_SLOTS_60kHz,
	[SCS_kHz120] = MAX_SLOTS_120kHz,
	[SCS_kHz240] = MAX_SLOTS_240kHz,
};

/* maximum buffer (freq-domain samples) size for supported number of slots/scs */
#define MAX_TRX_BUFFER_SIZE (((config_common.scs < SCS_kHz120) ? OFDM_SYM_SIZE : OFDM_SYM_SIZE_8K) * MAX_SYMBOLS * (max_slots[config_common.scs]))

/* global ul-DL common config used for all computes */
tdd_ul_dl_config_common_t config_common __attribute__ ((section (".smem")));

uint32_t vspa_phys_addr_start __attribute__ ((section (".smem")));
uint64_t start_airtime __attribute__ ((section (".smem")));
uint64_t air_slots __attribute__ ((section (".smem")));
uint64_t no_irqs[4] __attribute__ ((section (".smem")));

bool app_started __attribute__ ((section (".smem")));

void init_l1c_refapp_buffers()
{
	uint32_t i, j;
	// fix for the TX buffers reuse to fit available memory, TX_RX_OFFSET is half the normal size, Rx still has dedicated buffers per if
	uint32_t coeff_space_offset = ((config_common.scs == SCS_kHz120)) ? TX_RX_OFFSET * 3 : TX_RX_OFFSET * 2;

	mod_mem_region_t *scratch_buf = bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
	vspa_phys_addr_start = scratch_buf->addr_v;

	for (i = 0; i < MAX_USED_INTERFACES; i++)
	{
		for (j = 0; j < MAX_BUFFERS_PER_INTERFACE; j++)
		{
			config_common.interfaces[i].tx_buffers[j] = vspa_phys_addr_start + MAX_TRX_BUFFER_SIZE * ((i << 1) + j);
			config_common.interfaces[i].rx_buffers[j] = vspa_phys_addr_start + TX_RX_OFFSET + MAX_TRX_BUFFER_SIZE * ((i << 1) + j);
		}

		config_common.interfaces[i].tx_qec_buffer = vspa_phys_addr_start + coeff_space_offset + COEFF_BUFFER_SIZE * (i << 2);
		config_common.interfaces[i].rx_qec_buffer = config_common.interfaces[i].tx_qec_buffer + COEFF_BUFFER_SIZE;
		config_common.interfaces[i].cfr_buffer    = config_common.interfaces[i].rx_qec_buffer + COEFF_BUFFER_SIZE;
		config_common.interfaces[i].dpd_buffer    = config_common.interfaces[i].cfr_buffer + COEFF_BUFFER_SIZE;
	}

	/* forced by available memory to reuse Tx buffers between interfaces 1 and 2 */
	if (config_common.scs == SCS_kHz120) {
		config_common.interfaces[1].tx_buffers[0] = config_common.interfaces[0].tx_buffers[0];
		config_common.interfaces[1].tx_buffers[1] = config_common.interfaces[0].tx_buffers[1];
	}
}

void scs_debug_dump(tdd_scs_t *scs)
{
	if (!scs)
		return;

	switch (*scs)
	{
		case SCS_kHz15:
			PRINTF("15 kHz \r\n");
			break;
		case SCS_kHz30:
			PRINTF("30 kHz \r\n");
			break;
		case SCS_kHz60:
			PRINTF("60 kHz \r\n");
			break;
		case SCS_kHz120:
			PRINTF("120 kHz \r\n");
			break;
		case SCS_kHz240:
			PRINTF("240 kHz \r\n");
			break;
		default:
			PRINTF("Invalid SCS value \r\n");
	}
}

void pattern_debug_dump(tdd_ul_dl_pattern_t *p)
{
	uint8_t i;

	if (!p)
		return;

	if ((!p->dl_slots) && (!p->ul_slots))
		return;

	/* show how the slot pattern looks like */
	for (i = 0; i < p->dl_slots; i++)
		PRINTF("D");
	if (p->dl_syms || p->ul_syms)
		PRINTF("S");
	for (i = 0; i < p->ul_slots; i++)
		PRINTF("U");

	if (!p->dl_syms && !p->ul_syms)
		return;

	/* show the flexible/special slot symbols */
	PRINTF(", S: ");
	for (i = 0; i < p->dl_syms; i++)
		PRINTF("d");
	for (i = 0; i < (MAX_SYMBOLS - p->dl_syms - p->ul_syms); i++)
		PRINTF("g");
	for (i = 0; i < p->ul_syms; i++)
		PRINTF("u");

	if (config_common.tdd_ul_dl_gap)
		PRINTF("\t(UL time advance %d ns)", config_common.tdd_ul_dl_gap);
}

bool_t check_config(tdd_ul_dl_config_common_t *cfg)
{
	tdd_ul_dl_pattern_t *p = &cfg->pattern;
	tdd_ul_dl_pattern_t *p2 = &cfg->pattern2;
	uint8_t total_slots = 0;
	uint8_t i;
	bool_t active_interfaces = pdFALSE;

	/* compute number of total slots */
	total_slots = p->dl_slots + p->ul_slots;
	total_slots += p2->dl_slots + p2->ul_slots;
	total_slots += (p->dl_syms || p->ul_syms) ? 1 : 0;
	total_slots += (p2->dl_syms || p2->ul_syms) ? 1 : 0;

	if (total_slots == 0) {
		PRINTF("No pattern(s) were given.\r\n");
		return 0;
	}

	if (total_slots > max_slots[cfg->scs]) {
		PRINTF("Given pattern(s) exceed the 20ms window.\r\n");
		return 0;
	}

	if (max_slots[cfg->scs] % total_slots != 0) {
		PRINTF("Given pattern(s) cannot be repeated in a 20ms window.\r\n");
		return 0;
	}

	for (i = 0;  i < MAX_USED_INTERFACES; i++)
        active_interfaces |= cfg->interfaces[i].in_use;

	if (!active_interfaces) {
		PRINTF("No active interfaces.\r\n");
		return 0;
	}

	return 1;
}

void vL1CTasksDestroyAll()
{
	uint8_t i;

	/* destroy them all */
	for (i = L1C_TIME_PROC_TASK; i < L1C_TASK_MAX_ID; i++)
	{
		if (tasks_map.tasks[i].core_id != crt_core_id)
			continue;

		if (tasks_map.tasks[i].task_handle != NULL)
			vTaskDelete(tasks_map.tasks[i].task_handle);

		if (tasks_map.tasks[i].tick == TICK_ENABLE)
			vSemaphoreDelete(tasks_map.tasks[i].tick_sem);

		memset(&tasks_map.tasks[i], 0, sizeof(tasks_map.tasks[i]));
	}
}

void l1c_tdd_stop()
{
	l1c_vspa_proc_stop(1);
	l1c_time_add_stop_action();

	/* wait for tasks to consume ticks until end of patterns */
	vTaskDelay(100);

	/* destroy all alive tasks */
	vL1CTasksDestroyAll();

	/* free-up other resources */
	destroy_all();
	destroy_deferred_actions();

	//PRINTF("l1c_tdd_stop() done on core %d !!!!!!!!!\r\n", crt_core_id);
}

void l1c_tdd_start()
{
	l1c_qdma_init();

	/* Create procedures tasks */
	l1c_create_task(VSPA_AGENT_CORE, L1C_VSPA_SLOTS_TASK, "VspaSlots",     TICK_ENABLE,  L1C_HIGH_PRIO, DEFAULT_STACK_SIZE, l1c_vspa_proc_slots);
	l1c_create_task(TIME_AGENT_CORE, L1C_TIME_PROC_TASK,  "TimeSlots",     TICK_ENABLE,  L1C_HIGH_PRIO, DEFAULT_STACK_SIZE, l1c_time_proc_slots);

	/* configure TBGEN to issue periodical interrupts (ticks) for core where tasks require tick */
	/* do this after creation of procedures */
	for (uint8_t i = 1; i < L1C_TASK_MAX_ID; i++)
	{
		if (!tasks_map.tasks[i].task_handle)
			continue;

		if (tasks_map.tasks[i].core_id != crt_core_id)
			continue;

		if (tasks_map.tasks[i].tick == TICK_ENABLE)
			l1c_setup_periodical_tick(TBGEN_1, start_airtime, tick_interval[config_common.scs], crt_core_id);
	}
}

void vL1CDemoStart(int limited_slot_no)
{
	config_common.limited_slot_no = limited_slot_no;

	e200_trace(E200_TRACE_MSG_TDD_START, E200_TRACE_PARAM_TRACK);

#if defined(GEUL_LA1238RDB)
	if (brd_ver >= 1)
	{
		PRINTF("\r\nRunning on LA1238-RDB Rev.B or better\r\n");
		config_common.tdd_control_for_rf_card = 1;
	}
#elif defined(GEUL_LA1224)
	if (brd_ver >= GEUL_HOST_REVC_VAL)
	{
		PRINTF("\r\nRunning on LA1224-RDB Rev.C or better\r\n");
		config_common.tdd_control_for_rf_card = 0;
	}
#endif

	if (!check_config(&config_common))
		return;

	/* keep this for now.. */
	if (app_started)
	{
		PRINTF("\r\nL1C RefApp already started. Restart Geul or use 'l1c stop' command.\r\n");
		return;
	}
	else
		app_started = 1;

	/* update global/running config variables */
	for (uint8_t i = 0; i < MAX_USED_INTERFACES; i++)
	{
		if (!config_common.interfaces[i].in_use)
			continue;

		if (! config_common.tdd_control_for_rf_card)
			config_common.interfaces[i].rf_fem_ctrl_ptr =
				l1c_get_rf_fem_controls(config_common.interfaces[i].interface_id);
		else
			config_common.interfaces[i].rf_fem_tdd_ctrl_ptr =
				l1c_get_rf_tdd_fem_controls(config_common.interfaces[i].interface_id);

#if defined(GEUL_LA1224) || (defined(MW_GPIO_TEST) && defined(GEUL_LA1238RDB))
		config_common.interfaces[i].rf_fem_gpio_ctrl_ptr =
			l1c_get_rf_gpio_fem_controls(config_common.interfaces[i].interface_id);
#endif

		config_common.interfaces[i].axiq_ctrl =
			l1c_get_axiq_ctrl(config_common.interfaces[i].interface_id);
		config_common.interfaces[i].lp_wp_ctrl =
			l1c_get_timer_lp_ctrl(config_common.interfaces[i].interface_id);
	}

	tbgen_offset_calc();

	/* detect the presence of the PPS synchronization signal */
	PRINTF("Waiting for PPS sync... ");
	config_common.pps_available = wait_pps_sync(TBGEN_1);

	if (config_common.pps_available) {
		PRINTF("PPS detected\r\n");
		/* align RefApp operation to the PPS synchronization signal */
		start_airtime = ullTbgenGet10MSCounter(TBGEN_1);
	} else {
		PRINTF("no PPS available\r\n");
		/* read current TBGEN Master counter to use as a reference for future time events */
		start_airtime = ullTbgenGetMasterCounter(TBGEN_1);
	}

#ifndef NO_RF
	if (config_common.rf_fem_ctrl)
	{
		for (int i = 0; i < MAX_USED_INTERFACES; i++)
		{
			uint8_t dcs, intf;

			if (!config_common.interfaces[i].in_use)
				continue;

			dcs = config_common.interfaces[i].interface_id >> 1;
			intf = config_common.interfaces[i].interface_id & 1;
			PRINTF("Initializing RF FEM control signals for interface %d (%s_%d)\r\n",
					i, (dcs == 2) ? "HS_DCS" : (dcs == 0 ? "LS_DCS0" : "LS_DCS1"), intf);

			if (!config_common.tdd_control_for_rf_card)
			{
				if (config_common.scs <= SCS_kHz60)
				{
					/* initial GPIO state - set polarity */
					l1c_rf_ctrl_sig_setup(config_common.interfaces[i].rf_fem_ctrl_ptr,
								start_airtime + TBGEN_1S + 5 * TBGEN1_25_US);

					l1c_rf_ctrl_sig_transition(config_common.interfaces[i].rf_fem_ctrl_ptr,
									start_airtime + TBGEN_1S + 7 * TBGEN1_25_US, 1 /* tx->rx */);
				} else {
					PRINTF("Non-TDD timer control for RF card for SCS > 60KHz is not supported\r\n");
				}
			}
		}

		/* setup PMUX to associate GPIOs with TBGEN */
		l1c_gpio_pmux_setup();

		PRINTF("RF FEM signals controlled by RefApp - toggling activated !\r\n");
	} else {
#endif
		PRINTF("NO_RF define is active ! No config/toggle for RF FEM signals\r\n");
#ifndef NO_RF
	}
#endif

	/* next tick will be aligned to start_airtime */
	start_airtime += 3 * TBGEN_1S - 1 * tick_interval[config_common.scs];

	/* apply pps offset */
	if (config_common.pps_available)
		start_airtime += (config_common.pps_offset * TBGEN_4NS) / 4;

	/* register IPI agent commands */
	l1c_bind_ipi_cmd(L1C_IPI_TDD_START, cores_to_mask(2, VSPA_AGENT_CORE, TIME_AGENT_CORE), &l1c_tdd_start);
	l1c_bind_ipi_cmd(L1C_IPI_TDD_STOP, cores_to_mask(2, VSPA_AGENT_CORE, TIME_AGENT_CORE), &l1c_tdd_stop);

	l1c_send_ipi_cmd(L1C_IPI_TDD_START);
}

void vL1CDemoStop()
{
	e200_trace(E200_TRACE_MSG_TDD_STOP, E200_TRACE_PARAM_TRACK);

	l1c_send_ipi_cmd(L1C_IPI_TDD_STOP);

	/* wait for tasks to consume ticks until end of patterns */
	vTaskDelay(100);

	/* stop the tick/slot interrupt */
	iTbgenDisableTimer(TBGEN_1, RX_ALIGNMENT, TIMER_INSTANCE_0);
	iTbgenDisableTimer(TBGEN_1, RX_ALIGNMENT, TIMER_INSTANCE_1);
	iTbgenDisableTimer(TBGEN_1, RX_ALIGNMENT, TIMER_INSTANCE_2);
	iTbgenDisableTimer(TBGEN_1, RX_ALIGNMENT, TIMER_INSTANCE_3);

	for (uint8_t i = 0; i < TDD_MAX_INSTANCE; i++)
	{
		if (!trx_enable_steps[i])
			continue;

		iTbgenDisableTimer(TBGEN_2, TDD, TIMER_INSTANCE_0 + i);
	}

	/* destroy all alive tasks */
	vL1CTasksDestroyAll();

	PRINTF("air_slots = 0x%lx%lx\r\n", (u32)(air_slots >> 32), (u32)air_slots);
	PRINTF("vspa msgs = 0x%lx%lx\r\n", (u32)(vspa_agent_msgs >> 32), (u32)vspa_agent_msgs);
	PRINTF("no_ticks[0]   = 0x%lx%lx\r\n", (u32)(no_irqs[0] >> 32), (u32)no_irqs[0]);
	PRINTF("no_ticks[1]   = 0x%lx%lx\r\n", (u32)(no_irqs[1] >> 32), (u32)no_irqs[1]);
	PRINTF("no_ticks[2]   = 0x%lx%lx\r\n", (u32)(no_irqs[2] >> 32), (u32)no_irqs[2]);
	PRINTF("no_ticks[3]   = 0x%lx%lx\r\n", (u32)(no_irqs[3] >> 32), (u32)no_irqs[3]);

	air_slots = 0;
	vspa_agent_msgs = 0;
	app_started = 0;
	memset(no_irqs, 0, sizeof(no_irqs));

	dump_core_msg_count();
	dump_vspa_debug_stats();
}

#define PRINT_INTF_ENTRY(intf_no, intf_desc, intf, num_bufs) \
	do { \
		int buf_idx; \
		uint64_t tmp1, tmp2, tmp3, tmp4; \
		tmp1 = (intf).tx_qec_buffer - vspa_phys_addr_start + HOST_VIRT_ADDR_START; \
		tmp2 = (intf).rx_qec_buffer - vspa_phys_addr_start + HOST_VIRT_ADDR_START; \
		tmp3 = (intf).cfr_buffer - vspa_phys_addr_start + HOST_VIRT_ADDR_START; \
		tmp4 = (intf).dpd_buffer - vspa_phys_addr_start + HOST_VIRT_ADDR_START; \
		PRINTF("| interface%d   | %-11s | qec | 0x%x | 0x%x | %#x%8x | %#x%8x |\r\n", \
				(intf_no + 1), \
				(intf_desc), \
				(intf).tx_qec_buffer, \
				(intf).rx_qec_buffer, \
				(uint32_t)(tmp1 >> 32), (uint32_t)tmp1, \
				(uint32_t)(tmp2 >> 32), (uint32_t)tmp2); \
		PRINTF("|              |             | cfr | 0x%x |    n/a     | %#x%8x |      n/a     |\r\n", \
				(intf).cfr_buffer, \
				(uint32_t)(tmp3 >> 32), (uint32_t)tmp3); \
		PRINTF("|              |             | dpd | 0x%x |    n/a     | %#x%8x |      n/a     |\r\n", \
				(intf).dpd_buffer, \
				(uint32_t)(tmp4 >> 32), (uint32_t)tmp4); \
		for (buf_idx = 0; buf_idx < num_bufs; buf_idx++) { \
			tmp1 = (intf).tx_buffers[buf_idx] - vspa_phys_addr_start + HOST_VIRT_ADDR_START; \
			tmp2 = (intf).rx_buffers[buf_idx] - vspa_phys_addr_start + HOST_VIRT_ADDR_START; \
			PRINTF("|              |             |  %d  | 0x%x | 0x%x | %#x%8x | %#x%8x |\r\n", \
					buf_idx, \
					(intf).tx_buffers[buf_idx], \
					(intf).rx_buffers[buf_idx], \
					(uint32_t)(tmp1 >> 32), (uint32_t)tmp1, \
					(uint32_t)(tmp2 >> 32), (uint32_t)tmp2); \
		} \
	} while(0)

void config_dump ()
{
	uint8_t i;

	e200_trace(E200_TRACE_MSG_TDD_CONFIG_DUMP, E200_TRACE_PARAM_TRACK);

	PRINTF("\r\n");
	PRINTF(" __________________________________________________________________________________________ \r\n" \
			"|              |             |                         Tx/Rx Buffers                       |\r\n" \
			"|              |             |                VSPA           |            Host             |\r\n" \
			"|              |    Name     |-----+------------+------------+--------------+--------------|\r\n" \
			"|              |             | No. |     Tx     |     Rx     |      Tx      |      Rx      |\r\n" \
			"|--------------|-------------|-----+------------+------------+--------------+--------------|\r\n");

	for (i = 0; i < MAX_USED_INTERFACES; i++)
	{
		char intf_name[10];
		uint8_t dcs, intf;

		intf = config_common.interfaces[i].interface_id & 0x1;
		dcs = config_common.interfaces[i].interface_id >> 1;

		if (!config_common.interfaces[i].in_use)
			sprintf(intf_name, "%s", "Disabled");
		else {
			sprintf(intf_name, "%s:%d", (dcs == 2) ? "HS-DCS" : ((dcs == 0) ? "LS-DCS0" : "LS-DCS1"), intf);
		}

		PRINT_INTF_ENTRY(i, intf_name, config_common.interfaces[i], MAX_BUFFERS_PER_INTERFACE);
		PRINTF("|______________|_____________|_____|____________|____________|______________|______________|\r\n");
	}

	PRINTF("\r\nTx/Rx buffers sizes = %#x (%d bytes)", MAX_TRX_BUFFER_SIZE, MAX_TRX_BUFFER_SIZE);
	PRINTF("\r\nQEC/CFR coeffs buffers sizes = %#x (%d bytes)", COEFF_BUFFER_SIZE, COEFF_BUFFER_SIZE);
	PRINTF("\r\nRX always listen: %s", config_common.rx_always_listen ? "Yes" : "No");
	PRINTF("\r\nRF FEM Control: %s", config_common.rf_fem_ctrl ? "Yes" : "No");
	PRINTF("\r\nPPS offset: %d ns", config_common.pps_offset);
	PRINTF("\r\nUL-DL gap: %d ns\r\n", config_common.tdd_ul_dl_gap);
	PRINTF("\r\nSCS: ");
	scs_debug_dump(&config_common.scs);
	PRINTF("Slot count: %d\r\n", max_slots[config_common.scs]);
	PRINTF("\r\npattern:  ");
	pattern_debug_dump(&config_common.pattern);
	PRINTF("\r\npattern2: ");
	pattern_debug_dump(&config_common.pattern2);
	PRINTF("\r\n");

	l1c_vspa_slot_config_dump_all();
	dump_axiq_sequences();
}

void vL1CDemoConfig(l1c_config_t *cfg)
{
	mixer_config_msg_t vspa_mixer_config_msg;
	uint8_t intf_idx;
	uint8_t core_id;

	switch (cfg->type)
	{
		case L1C_CONFIG_PATTERN:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else
			{
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_PATT, E200_TRACE_PARAM_TRACK);
				config_common.pattern = cfg->data.cfg_pattern.p;
			}
			break;
		case L1C_CONFIG_PATTERN2:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else
			{
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_PATT, E200_TRACE_PARAM_TRACK);
				config_common.pattern2 = cfg->data.cfg_pattern.p;
			}
			break;
		case L1C_CONFIG_RX_ALWAYS_ON:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else
			{
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_RXON, E200_TRACE_PARAM_TRACK);
				config_common.rx_always_listen = cfg->data.rx_always_listen;
			}
			break;
		case L1C_CONFIG_RF_CTRL:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else
			{
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_RFCTL, E200_TRACE_PARAM_TRACK);
				config_common.rf_fem_ctrl = cfg->data.rf_fem_ctrl;
			}
			break;
		case L1C_CONFIG_TDD_UL_DL_GAP:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else
			{
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_GAP, E200_TRACE_PARAM_TRACK);
				config_common.tdd_ul_dl_gap = cfg->data.tdd_ul_dl_gap;
			}
			break;
		case L1C_CONFIG_TDD_PPS_OFFSET:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else
			{
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_PPS, E200_TRACE_PARAM_TRACK);
				config_common.pps_offset = cfg->data.pps_offset;
			}
			break;
		case L1C_CONFIG_INTERFACE:
		case L1C_CONFIG_INTERFACE2:
			if (app_started)
			{
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
				break;
			}

			e200_trace(E200_TRACE_MSG_TDD_CONFIG_IF, E200_TRACE_PARAM_TRACK);

			intf_idx = (cfg->type  == L1C_CONFIG_INTERFACE) ? 0 : 1;

			if (cfg->data.cfg_interface.dcs >= DCS_NUM_MAX) {
				config_common.interfaces[intf_idx].in_use = 0;
				PRINTF("Disabled interface %d\r\n", intf_idx + 1);
			}
			else {
				config_common.interfaces[intf_idx].interface_id =
						cfg->data.cfg_interface.interface + (cfg->data.cfg_interface.dcs << 1);
				config_common.interfaces[intf_idx].in_use = 1;
			}

			break;
		case L1C_CONFIG_DUMP:
			config_dump();
			break;
		case L1C_CONFIG_SCS:
			if (app_started)
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
			else {
				e200_trace(E200_TRACE_MSG_TDD_CONFIG_SCS, E200_TRACE_PARAM_TRACK);
				config_common.scs = cfg->data.scs;
				/* re-init buffer sizes and reset vspa slot configs */
				init_l1c_refapp_buffers();
				reset_vspa_slots();
			}
			break;
		case L1C_CONFIG_MIXER:
			if (l1c_vspa_configured_fr1()) {
				PRINTF("Mixer config available only for FR2 Rx paths, select interface and SCS first!\r\n");
			} else {
				core_id = l1c_vspa_core_mapping(cfg->data.mixer.intf_idx, VSPA_RX_CORE, 0); // first VSPA Rx core runs the mixer
				vspa_mixer_config_msg.freq_mili_hz = swap_uint32(cfg->data.mixer.freq);
				l1c_vspa_agent_enqueue_msg(core_id, L1C_MSG_A2V_MIXER_CONFIG, &vspa_mixer_config_msg, sizeof(mixer_config_msg_t));
			}
			break;
		default:
			break;
	}
	vPortFree(cfg);
}

void vL1CUpdate_TDD_QEC(l1c_update_t *cfg)
{
	coeff_update_msg_t vspa_coeff_update_msg;
	bool_t found = pdFALSE;
	uint8_t i = 0, rx_or_tx = VSPA_TX_CORE, k = 0;
	char *helper_str = NULL;
	uint32_t helper_addr = 0;

	switch (cfg->type)
	{
		case L1C_UPDATE_DPD:
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_DPD);
			rx_or_tx = VSPA_TX_CORE;
			break;
		case L1C_UPDATE_TX_QEC:
			rx_or_tx = VSPA_TX_CORE;
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_TX_QEC);
			break;
		case L1C_UPDATE_RX_QEC:
			rx_or_tx = VSPA_RX_CORE;
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_RX_QEC);
			break;
		case L1C_UPDATE_CFR:
			rx_or_tx = VSPA_TX_CORE;
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_CFR);
			break;
		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
			break;
	}

	/* interfaces have their own dpd/cfr/qec buffers when running TDD operation,
	 * therefore a reverse look-up of core-id -> buffer is required
	 */

	for (i = 0; i < MAX_USED_INTERFACES; i++) {
		if (!config_common.interfaces[i].in_use)
			continue;

		for (k = 0; k < VSPA_TX_RX_CORE_NUM; k++)
			if (cfg->core_id == l1c_vspa_core_mapping(i, rx_or_tx, k)) {
				found = pdTRUE;
				goto end_lookup;
			}
	}

end_lookup:
	if (!found) {
		PRINTF("VSPA core %d not in use!\r\n", cfg->core_id);
		vPortFree(cfg);
		return;
	}

#if 0
	PRINTF("cfg->core_id = %d\r\n", cfg->core_id);
	PRINTF("found i=%d, j=%d, k=%d\r\n", i, j, k);
#endif

	switch (cfg->type)
	{
		case L1C_UPDATE_TX_QEC:
			helper_str = "TX-QEC";
			helper_addr = config_common.interfaces[i].tx_qec_buffer;
			break;
		case L1C_UPDATE_RX_QEC:
			helper_str = "RX-QEC";
			helper_addr = config_common.interfaces[i].rx_qec_buffer;
			break;
		case L1C_UPDATE_CFR:
			helper_str = "CFR";
			helper_addr = config_common.interfaces[i].cfr_buffer;
			break;
		case L1C_UPDATE_DPD:
			helper_str = "DPD";
			helper_addr = config_common.interfaces[i].dpd_buffer;
			break;
		default:
			PRINTF("Parameter not supported!\r\n");
			vPortFree(cfg);
			return;
	}

	PRINTF("Updating %s coeffs for core %d (intf %d, buffer=%#x)\r\n",
		   helper_str,
		   (uint8_t) l1c_vspa_core_mapping(i, rx_or_tx, k),
		   i,
		   helper_addr);

	vspa_coeff_update_msg.memory_address = swap_uint32(helper_addr);
	l1c_vspa_agent_enqueue_msg(cfg->core_id, L1C_MSG_A2V_COEFF_UPDATE, &vspa_coeff_update_msg, sizeof(coeff_update_msg_t));

	vPortFree(cfg);
}

void init_l1c_agents()
{
//	e200_trace_enable(E200_TRACE_MSG_VSPA);
	e200_trace_enable(E200_TRACE_MSG_RADIO_FRAME);

	l1c_create_task(VSPA_AGENT_CORE, L1C_VSPA_AGENT_TASK, "VspaAgent",     TICK_DISABLE, L1C_HIGH_PRIO, DEFAULT_STACK_SIZE, l1c_vspa_agent_main);
	l1c_create_task(TIME_AGENT_CORE, L1C_TIME_AGENT_TASK, "TimeAgent",     TICK_DISABLE, L1C_LOW_PRIO, DEFAULT_STACK_SIZE, l1c_time_agent_main);
	l1c_create_task(TIME_AGENT_CORE, L1C_TIME_AGENT2_TASK,"TimeAgent2",    TICK_ENABLE,  L1C_MED_PRIO, DEFAULT_STACK_SIZE, l1c_time_agent_deferred_main);
}

void init_l1c_refapp()
{
	tdd_ul_dl_config_common_t cfg_default = {
#if 0
		.interfaces[0] = {
			.in_use = 1,
			.interface_id = 0,
		},
		.pattern = { 7, 6, 2, 4 },
#endif
		.tdd_ul_dl_gap = UL_DL_GAP_DEFAULT_VALUE,
		.pps_offset = PPS_OFFSET_DEFAULT_VALUE,
		.rf_fem_ctrl = true,
		.scs = SCS_kHz30,
	};

	/* let only core-0 do some init */
	if (crt_core_id == 0)
	{
		memcpy(&config_common, &cfg_default, sizeof(tdd_ul_dl_config_common_t));

		init_l1c_refapp_buffers();
	}

	e200_trace_init();
	init_l1c_agents();
	init_l1c_ipi();
	vL1CDPDInit();
}
