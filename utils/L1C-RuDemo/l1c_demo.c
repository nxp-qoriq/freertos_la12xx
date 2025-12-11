/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#include "FreeRTOS.h"
#include "semphr.h"

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

#ifdef LA12XX_DRIVER_PCI_LAT_FP
#include <pcie_ep_lat_fpga.h>
#endif

#include "l1c_vspa_agent.h"
#include "l1c_vspa_proc.h"
#include "l1c_rf_ctrl.h"
#include "l1c_axiq.h"
#include "l1c_fwk_tasks.h"
#include "l1c_time_agent.h"
#include "l1c_time_proc.h"
#include "l1c_dpd.h"
#include "l1c_ipi.h"
#include "geul_avi.h"
#include "gul_bsp_init.h"
#include "l1c_debug.h"
#include "l1c_bench.h"

extern volatile uint32_t brd_ver;
extern void init_l1c_refapp_host_if();

static AviHandle_t *pxAviHandle;
static VspaCore_t eVspaCore = VSPA_CORE_7;

static int lphy_mode;
static bool tdd_mode = false;

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
uint64_t axiq_start_time __attribute__ ((section (".smem")));

bool app_started __attribute__ ((section (".smem")));
modem_mode_e modem_mode __attribute__ ((section (".smem")));


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
	//l1c_create_task(VSPA_AGENT_CORE, L1C_VSPA_SLOTS_TASK, "VspaSlots",     TICK_ENABLE,  7, DEFAULT_STACK_SIZE, l1c_vspa_proc_slots);
	l1c_create_task(TIME_AGENT_CORE, L1C_TIME_PROC_TASK,  "TimeSlots",     TICK_ENABLE,  7, DEFAULT_STACK_SIZE, l1c_time_proc_slots);

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

uint64_t vL1CDemoStart(int limited_slot_no)
{
#ifdef LA12XX_DRIVER_PCI_LAT_FP
	uint64_t tx_en_time, rx_en_time;
	uint64_t tx_adrv_delay, rx_adrv_delay;
#endif
	uint64_t rf_start_time;

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
		if (config_common.scs > SCS_kHz60) {
			config_common.tdd_control_for_rf_card = 1;
		}
	}
#endif

	if (!check_config(&config_common))
		return 0;

	/* keep this for now.. */
	if (app_started)
	{
		PRINTF("\r\nL1C RefApp already started. Restart Geul or use 'l1c stop' command.\r\n");
		return 0;
	}
	else
		app_started = 1;

	/* update global/running config variables */
	for (uint8_t i = 0; i < MAX_USED_INTERFACES; i++)
	{
		if (!config_common.interfaces[i].in_use)
			continue;

		config_common.interfaces[i].rf_fem_ctrl_ptr =
			l1c_get_rf_fem_controls(config_common.interfaces[i].interface_id);
		config_common.interfaces[i].rf_fem_tdd_ctrl_ptr =
			l1c_get_rf_tdd_fem_controls(config_common.interfaces[i].interface_id);

#if defined(GEUL_LA1224) || (defined(MW_GPIO_TEST) && defined(GEUL_LA1238RDB))
		config_common.interfaces[i].rf_fem_gpio_ctrl_ptr =
			l1c_get_rf_gpio_fem_controls(config_common.interfaces[i].interface_id);
#endif

		if (lphy_mode)
			config_common.interfaces[i].axiq_ctrl = NULL;
		else
			config_common.interfaces[i].axiq_ctrl =
				l1c_get_axiq_ctrl(config_common.interfaces[i].interface_id);
		config_common.interfaces[i].lp_ctrl =
			l1c_get_lp_ctrl(config_common.interfaces[i].interface_id);
	}

	for (uint8_t i = 0; i < MAX_OBS_IF; i++)
	{
		if (!config_common.obs_if[i].in_use)
			continue;

		if (lphy_mode)
			config_common.obs_if[i].axiq_ctrl = NULL;
		else
			config_common.obs_if[i].axiq_ctrl = l1c_get_axiq_ctrl(config_common.obs_if[i].interface_id);

		config_common.interfaces[i].lp_ctrl = NULL;
	}

	/* compute TBGEN offsets */
	tbgen_offset_calc();
	//tbgen_offset = get_tbgen2_mc_offset_diff(ullTbgenGetMasterCounter(TBGEN_1) +  TBGEN_1S);

	/* detect the presence of the PPS synchronization signal */
	PRINTF("Waiting for PPS sync... ");
	config_common.pps_available = wait_pps_sync(TBGEN_1);

	if (config_common.pps_available) {
		uint64_t tbgen1_pps, tbgen2_pps;

		/* align RefApp operation to the PPS synchronization signal */

		tbgen1_pps = ullTbgenGet10MSCounter(TBGEN_1);
		tbgen2_pps = ullTbgenGet10MSCounter(TBGEN_2);

		if (config_common.scs >= SCS_kHz120)
			start_airtime = tbgen2_pps;
		else
			start_airtime = tbgen1_pps;
		
		if (tbgen1_pps != tbgen2_pps) {
			PRINTF("TBGEN1 and TBGEN2 not in sync\r\n");
			PRINTF("TBGEN1: PPS at: 0x%lx%08lx\r\n", PRINT_64_HI(tbgen1_pps), PRINT_64_LO(tbgen1_pps));
			PRINTF("TBGEN2: PPS at: 0x%lx%08lx\r\n", PRINT_64_HI(tbgen2_pps), PRINT_64_LO(tbgen2_pps));
		} else {
			PRINTF("PPS detected at: 0x%lx%08lx\r\n", PRINT_64_HI(start_airtime), PRINT_64_LO(start_airtime));
		}
	} else {
		PRINTF("PPS not available\r\n");

		/* read current TBGEN Master counter to use as a reference for future time events */
		if (config_common.scs >= SCS_kHz120)
			start_airtime = ullTbgenGetMasterCounter(TBGEN_2);
		else
			start_airtime = ullTbgenGetMasterCounter(TBGEN_1);
	}

#ifndef NO_RF
	if (config_common.rf_fem_ctrl)
	{
		if (!config_common.tdd_control_for_rf_card) {
			if (config_common.scs > SCS_kHz60) {
				PRINTF("Non-TDD timer control for RF card for SCS > 60KHz is not supported\r\n");
				return 0;
			}
		}

		for (int i = 0; i < MAX_USED_INTERFACES; i++)
		{
			uint8_t dcs, intf;

			if (!config_common.interfaces[i].in_use)
				continue;

			dcs = config_common.interfaces[i].interface_id >> 1;
			intf = config_common.interfaces[i].interface_id & 1;
			PRINTF("Initializing RF FEM control signals for interface %d (%s_%d)\r\n",
					i, (dcs == 2) ? "HS_DCS" : (dcs == 0 ? "LS_DCS0" : "LS_DCS1"), intf);

			/* initial GPIO state - set polarity */
			l1c_rf_ctrl_sig_setup(config_common.interfaces[i].rf_fem_ctrl_ptr,
						start_airtime + TBGEN_1S + 5 * TBGEN_25_US);

			l1c_rf_ctrl_sig_transition(config_common.interfaces[i].rf_fem_ctrl_ptr,
						start_airtime + TBGEN_1S + 7 * TBGEN_25_US, 1 /* tx->rx */);
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

	/* apply pps offset */
	if (config_common.pps_available)
		start_airtime += (config_common.pps_offset * TBGEN_4NS) / 4;

	/* Export start airtime to host */
	rf_start_time = start_airtime + TBGEN_1S;
	axiq_start_time = rf_start_time;

	/* next tick will be aligned to start_airtime */
	start_airtime += TBGEN_1S - 1 * tick_interval[config_common.scs];

	/* Set both of the trigger outputs*/
	if (l1c_trig_out_setup(TRIG_OUT_1, rf_start_time))
		PRINTF("Failed to setup RF_START TRIG_OUT1\n\r");
	else
		PRINTF("RF_START TRIG_OUT1 init for for 0x%lx%08lx\r\n",
			   PRINT_64_HI(rf_start_time), PRINT_64_LO(rf_start_time));

	if (l1c_trig_out_setup(TRIG_OUT_2, rf_start_time))
		PRINTF("Failed to setup RF_START TRIG_OUT2\n\r");
	else
		PRINTF("RF_START TRIG_OUT2 init for for 0x%lx%08lx\r\n",
			   PRINT_64_HI(rf_start_time), PRINT_64_LO(rf_start_time));

#ifdef LA12XX_DRIVER_PCI_LAT_FP
	tx_adrv_delay = TBGEN_1_US;			//Assumed delay value
	tx_en_time = rf_start_time - tx_adrv_delay;

	if (l1c_trig_out_setup(TX_ENABLE, tx_en_time))
		PRINTF("Failed to setup TX_ENABLE trigger\n\r");
	else
		PRINTF("TX_ENABLE trigger init for 0x%lx%08lx\r\n", PRINT_64_HI(tx_en_time), PRINT_64_LO(tx_en_time));

	rx_adrv_delay = TBGEN_1_US;			//Assumed delay value
	rx_en_time = rf_start_time;

	if (tdd_mode) {
		PRINTF("\r\nAdvanced RX_ENABLE trig_out by UL-DL gap: %d tbgen clocks\r\n", config_common.tdd_ul_dl_gap);
		rx_en_time = rx_en_time - config_common.tdd_ul_dl_gap;
	}

	rx_en_time = rx_en_time + rx_adrv_delay;

	if (l1c_trig_out_setup(RX_ENABLE, rx_en_time))
		PRINTF("Failed to setup RX_ENABLE trigger\n\r");
	else
		PRINTF("RX_ENABLE trigger init for 0x%lx%08lx\r\n", PRINT_64_HI(rx_en_time), PRINT_64_LO(rx_en_time));
#endif

	/* register IPI agent commands */
	l1c_bind_ipi_cmd(L1C_IPI_TDD_START, cores_to_mask(2, VSPA_AGENT_CORE, TIME_AGENT_CORE), &l1c_tdd_start);
	l1c_bind_ipi_cmd(L1C_IPI_TDD_STOP, cores_to_mask(2, VSPA_AGENT_CORE, TIME_AGENT_CORE), &l1c_tdd_stop);

	l1c_send_ipi_cmd(L1C_IPI_TDD_START);

	return rf_start_time;
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

	PRINTF("\r\n\n");
	for (i = 0; i < MAX_OBS_IF; i++) {
		char intf_name[10];
		uint8_t dcs, intf;
		uint64_t samples = 0, period = 0;

		intf = config_common.obs_if[i].interface_id & 0x1;
		dcs = config_common.obs_if[i].interface_id >> 1;

		if (!config_common.obs_if[i].in_use)
			sprintf(intf_name, "%s", "Disabled");
		else {
			sprintf(intf_name, "%s:%d", (dcs == 2) ? "HS-DCS" : ((dcs == 0) ? "LS-DCS0" : "LS-DCS1"), intf);
			samples = config_common.obs_if[i].samples;
			period = config_common.obs_if[i].period;
		}

		PRINTF("Observation Interface[%d]: %s pulse_samples: 0x%lx%08lx period_samples: 0x%lx%08lx\r\n", i, intf_name, 
		       PRINT_64_HI(samples), PRINT_64_LO(samples), PRINT_64_HI(period), PRINT_64_LO(period));
	}
	
	PRINTF(" __________________________________________________________________________________________ \r\n");
		

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
		case L1C_CONFIG_INTERFACE1:
		case L1C_CONFIG_INTERFACE2:
		case L1C_CONFIG_INTERFACE3:
			if (app_started)
			{
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
				break;
			}

			e200_trace(E200_TRACE_MSG_TDD_CONFIG_IF, E200_TRACE_PARAM_TRACK);

			if (cfg->type == L1C_CONFIG_INTERFACE) {
				intf_idx = 0;
			} else if (cfg->type == L1C_CONFIG_INTERFACE1) {
				intf_idx = 1;
			} else if (cfg->type == L1C_CONFIG_INTERFACE2) {
				intf_idx = 2;
			} else if (cfg->type == L1C_CONFIG_INTERFACE3) {
				intf_idx = 3;
			} else {
				PRINTF("Interface not found for cfg->type: %d\n\r", cfg->type);
				break;
			}

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

		case L1C_CONFIG_OBS_IF:
		case L1C_CONFIG_OBS_IF1:
			if (app_started) {
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
				break;
			}

			if (cfg->type == L1C_CONFIG_OBS_IF) {
				intf_idx = 0;
			} else if (cfg->type == L1C_CONFIG_OBS_IF1) {
				intf_idx = 1;
			} else {
				PRINTF("Interface not found for cfg->type: %d\n\r", cfg->type);
				break;
			}

			if (cfg->data.cfg_obs_if.dcs != DCS_HS) {
				PRINTF("LSDCS cannot be used as observation interface in pulse mode\r\n");
				break;
			}

			if (cfg->data.cfg_obs_if.dcs >= DCS_NUM_MAX) {
				config_common.obs_if[intf_idx].in_use = 0;
				PRINTF("Disabled observation interface %d\r\n", intf_idx + 1);
			} else {
				config_common.obs_if[intf_idx].interface_id =
						cfg->data.cfg_obs_if.interface + (cfg->data.cfg_obs_if.dcs << 1);
				config_common.obs_if[intf_idx].samples = cfg->data.cfg_obs_if.samples;
				config_common.obs_if[intf_idx].period = cfg->data.cfg_obs_if.period;
				config_common.obs_if[intf_idx].in_use = 1;
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



static bool_t iGeulVspaCore( uint32_t ulIrq, VspaCore_t *peVspaCore )
{
	ulIrq = ulIrq - INTERNAL_IRQ_OFFSET;
        switch( ulIrq )
        {
                case 66 :
                case 67 :
                        *peVspaCore = VSPA_CORE_0;
                        break;

                case 69 :
                case 70 :
                        *peVspaCore = VSPA_CORE_1;
                        break;

                case 72 :
                case 73 :
                        *peVspaCore = VSPA_CORE_2;
                        break;

                case 75 :
                case 76 :
                        *peVspaCore = VSPA_CORE_3;
                        break;

                case 78 :
                case 79 :
                        *peVspaCore = VSPA_CORE_4;
                        break;

                case 81 :
                case 82 :
                        *peVspaCore = VSPA_CORE_5;
                        break;

                case 84 :
                case 85 :
                        *peVspaCore = VSPA_CORE_6;
                        break;

                case 87 :
                case 88 :
                        *peVspaCore = VSPA_CORE_7;
                        break;

                default :
                        log_isr( "%s : Invalid interrupt number :%u\n\r", __func__, ulIrq );
                        return false;
        }
        return true;
}

#ifdef LA12XX_DRIVER_PCI_LAT_FP

int enable_fpga_ls_ant(uint32_t ls_tx_ant_mask, uint32_t ls_rx_ant_mask)
{
	xSetDma_t enable_dma = { 0 };
	uint32_t ant_id = 0;

	while (ls_tx_ant_mask || ls_rx_ant_mask) {
		int tx = ls_tx_ant_mask & 1;
		int rx = ls_rx_ant_mask & 1;

		switch (ant_id) {
		case 0:
				if (tx) {
					enable_dma.selectChannel |= SEL_TX_DMA_CH0;
					enable_dma.setChannel |= EN_TX_DMA_CH0;
				}
				if (rx) {
					enable_dma.selectChannel |= SEL_RX_DMA_CH0;
					enable_dma.setChannel |= EN_RX_DMA_CH0;
				}
				break;

		case 1:
				if (tx) {
					enable_dma.selectChannel |= SEL_TX_DMA_CH1;
					enable_dma.setChannel |= EN_TX_DMA_CH1;
				}
				if (rx) {
					enable_dma.selectChannel |= SEL_RX_DMA_CH1;
					enable_dma.setChannel |= EN_RX_DMA_CH1;
				}
				break;

		case 2:
				if (tx) {
					enable_dma.selectChannel |= SEL_TX_DMA_CH2;
					enable_dma.setChannel |= EN_TX_DMA_CH2;
				}
				if (rx) {
					enable_dma.selectChannel |= SEL_RX_DMA_CH2;
					enable_dma.setChannel |= EN_RX_DMA_CH2;
				}
				break;

		case 3:
				if (tx) {
					enable_dma.selectChannel |= SEL_TX_DMA_CH3;
					enable_dma.setChannel |= EN_TX_DMA_CH3;
				}
				if (rx) {
					enable_dma.selectChannel |= SEL_RX_DMA_CH3;
					enable_dma.setChannel |= EN_RX_DMA_CH3;
				}
				break;

		default:
				log_err("Can't enable fpga channel_id: %u\n\r", ant_id);
				return -1;
		}

		ls_tx_ant_mask = ls_tx_ant_mask >> 1;
		ls_rx_ant_mask = ls_rx_ant_mask >> 1;

		ant_id++;
		continue;
	}

	PcieEpSetDma(&enable_dma);
	return 0;
}

void config_fpga()
{
	xConfigChannels_t channel_config;
	xChConfig_t*channelCfg;

	channel_config.channels = (CNFG_RX_CHANNEL_0 | CNFG_TX_CHANNEL_0 |
							   CNFG_RX_CHANNEL_1 | CNFG_TX_CHANNEL_1 |
							   CNFG_RX_CHANNEL_2 | CNFG_TX_CHANNEL_2 |
							   CNFG_RX_CHANNEL_3 | CNFG_TX_CHANNEL_3 );

	//---------------------CH 0 ------------------------------------------------
	channelCfg = &channel_config.channelCfg[CHANNEL_0];

	channelCfg->tx_config.channelTotalSizeAddr = 0xE2000168;
	channelCfg->rx_config.channelTotalSizeAddr = 0xE2800178;

	channelCfg->tx_config.channelAddr = 0xE210a000;
	channelCfg->tx_config.channelSize = 0xa000;

	channelCfg->rx_config.channelAddr = 0xE2900000;
	channelCfg->rx_config.channelSize = 0xa000;

	//-------------------CH1 ----------------------------------------------------
	channelCfg = &channel_config.channelCfg[CHANNEL_1];

	channelCfg->tx_config.channelTotalSizeAddr = 0xE2400168;
	channelCfg->rx_config.channelTotalSizeAddr = 0xE2C00178;

	channelCfg->tx_config.channelAddr = 0xE250a000;
	channelCfg->tx_config.channelSize = 0xa000;

	channelCfg->rx_config.channelAddr = 0xE2D00000;
	channelCfg->rx_config.channelSize = 0xa000;

	//-------------------CH2-------------------------------------------------------
	channelCfg = &channel_config.channelCfg[CHANNEL_2];

	channelCfg->tx_config.channelTotalSizeAddr = 0xE2800168;
	channelCfg->rx_config.channelTotalSizeAddr = 0xE2000178;

	channelCfg->tx_config.channelAddr = 0xE290a000;
	channelCfg->tx_config.channelSize = 0xa000;

	channelCfg->rx_config.channelAddr = 0xE2100000;
	channelCfg->rx_config.channelSize = 0xa000;

	//-------------------CH3-------------------------------------------------------
	channelCfg = &channel_config.channelCfg[CHANNEL_3];

	channelCfg->tx_config.channelTotalSizeAddr = 0xE2C00168;
	channelCfg->rx_config.channelTotalSizeAddr = 0xE2400178;

	channelCfg->tx_config.channelAddr = 0xE2D0a000;
	channelCfg->tx_config.channelSize = 0xa000;

	channelCfg->rx_config.channelAddr = 0xE2500000;
	channelCfg->rx_config.channelSize = 0xa000;

	PcieEpConfigDmaBuf(&channel_config);
}

#endif //LA12XX_DRIVER_PCI_LAT_FP


static int set_pattern(uint32_t pattern, tdd_ul_dl_pattern_t *patt)
{
	uint32_t dl_slots = pattern & 0xF;
	uint32_t dl_syms = (pattern >> 8) & 0xF;
	uint32_t ul_slots = (pattern >> 12) & 0xF;
	uint32_t ul_syms = (pattern >> 20) & 0xF;

	log_info("pattern -> dl_slots: %d dl_syms: %d ul_slots: %d ul_syms: %d\r\n",
			dl_slots, dl_syms, ul_slots, ul_syms);

	if (dl_syms >= MAX_SYMBOLS || ul_syms >= MAX_SYMBOLS) {
		log_err("Invalid pattern\n\r");
		return -1;
	}

	patt->dl_slots = dl_slots;
	patt->dl_syms = dl_syms;
	patt->ul_slots = ul_slots;
	patt->ul_syms = ul_syms;

	return 0;
}

static bool_t bVSPA0GroupBInterrupt( uint32_t ulIrq, void *pvDevData )
{
	VspaRegs_t *pVspaRegs;
	uint32_t status;
	AviMboxData_t recv_mbox;

	UNUSED(pvDevData);

	if(false == iGeulVspaCore(ulIrq, &eVspaCore))
	{
		log_isr( "%s : Invalid interrupt generated\n\r", __func__ );
		return false;
	}

	pVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR(eVspaCore);
	status = IN_32( &pVspaRegs->ulVspaStatus);

	// GroupB interrupts have both E200 and VCPU mapped to it
	// If E200 MBOX clear the E200 status
	if (status & E200_MBOX1_STATUS)	{
		log_dbg( "%s: VSPA : [%d] E200_MBOX1_STATUS, intr = %u\n\r", __func__, eVspaCore, ulIrq );
		OUT_32( &pVspaRegs->ulVspaStatus, E200_MBOX1_STATUS );
		return true;
	}

	if (0 == (status & VSPA_MBOX1_STATUS)) {
		log_err( "ERR: Invalid VSPA mbox status: 0x%x eVspaCore: %d %d\n\r", 
				status, eVspaCore, ulIrq);
		return false;
	}

	if (AVI_SUCCESS != exGeulAviHostHandleMboxIrq(pxAviHandle, eVspaCore, VSPA_MBOX_1, &recv_mbox)) {
		log_err("exGeulAviHostRecvMboxFromVspa failed\r\n");
		return -1;
	}

	log_dbg("MBOX_ISR recvd mbox from VSPA[%d]:  Msb = 0x%x Lsb = 0x%x status 0x%x\n\r",
		   eVspaCore, recv_mbox.ulMsb, recv_mbox.ulLsb, status);

	return true;
}


int process_vspa_l1c_config_81(AviMboxData_t recv_mbox)
{
	uint32_t pattern0, pattern1;

	pattern0 = recv_mbox.ulLsb & 0xFFFFFF;
	if (pattern0)
		set_pattern(pattern0, &config_common.pattern);


	/* Check for second pattern */
	pattern1 = recv_mbox.ulMsb & 0xFFFFFF;
	if (pattern1)
		set_pattern(pattern1, &config_common.pattern2);

	config_common.tdd_ul_dl_gap = 100 * (recv_mbox.ulLsb >> 24);

	return 0;
}

int process_vspa_l1c_config_80(AviMboxData_t recv_mbox)
{
	uint32_t tx_ls_ant_mask, rx_ls_ant_mask, mu;
	uint64_t rf_start_time, curr_time;
	AviMboxData_t send_mbox;
	uint32_t tx_mode, rx_mode;

	lphy_mode = recv_mbox.ulLsb >> 31;
	if (!lphy_mode) {
		log_info("L1C config from vspa is ignored for non-lphy case. Not sending ACK\r\n");
		return 0;
	}

	rx_ls_ant_mask = recv_mbox.ulMsb & 0xF;
	if (!rx_ls_ant_mask) {
		log_err("None of LS antennas are enabled\n\r");
		return -1;
	}

	tx_ls_ant_mask = recv_mbox.ulLsb & 0xF;
	if (!tx_ls_ant_mask) {
		log_err("None of LS antennas are enabled\n\r");
		return -1;
	}

	if (tdd_mode) {
		rx_mode = (recv_mbox.ulMsb >> 6) & 0x1;
		tx_mode = (recv_mbox.ulLsb >> 6) & 0x1;

		if (rx_mode != tx_mode) {
		}
	}

	mu = (recv_mbox.ulMsb >> 28) & 0x7;
	switch (mu) {
		case 0:
			config_common.scs = SCS_kHz15;
			break;
		case 1:
			config_common.scs = SCS_kHz30;
			break;
		case 2:
			config_common.scs = SCS_kHz60;
			break;
		case 3:
			config_common.scs = SCS_kHz120;
			break;
		case 4:
			config_common.scs = SCS_kHz240;
			break;
		default:
			log_err("Invalid Numerology: %u\n\r", mu);
			break;
	}

	/* Sending ACK to VSPA */
	send_mbox.ulMsb = 0x80 << 24;
	send_mbox.ulLsb = 0x00;

	/* Send a mailbox ack to vspa 7*/
	if (AVI_SUCCESS != exGeulAviHostSendSlowMboxToVspa(pxAviHandle, eVspaCore, VSPA_MBOX_1, send_mbox)) {
		log_err("Sending ACK vspa mbox failed\n\r");
		return -1;
	}

#ifdef LA12XX_DRIVER_PCI_LAT_FP
	config_fpga();
#endif

	rf_start_time = vL1CDemoStart(0);
	if (!rf_start_time) {
		log_err("vL1CDemoStart failed\n\r");
		return -1;
	}

	curr_time = ullTbgenGetMasterCounter(TBGEN_1);

	PRINTF("%s: rf_start_time: 0x%lx%08lx, curr_time: 0x%lx%08lx\n\r",
	       modem_mode == CPE? "CPE":"RU",
		   PRINT_64_HI(rf_start_time), PRINT_64_LO(rf_start_time),
		   PRINT_64_HI(curr_time), PRINT_64_LO(curr_time));

#ifdef LA12XX_DRIVER_PCI_LAT_FP
	while (ullTbgenGetMasterCounter(TBGEN_1) < rf_start_time - (TBGEN_50NS * 72));
	enable_fpga_ls_ant(tx_ls_ant_mask, rx_ls_ant_mask);
#endif

	return 0;
}


int wait_for_l1c_config()
{
	uint32_t msg_id;
	int ret;
	bool config_complete = false;
	QueueHandle_t currHandle = NULL;
	AviMboxData_t recv_mbox;

	log_dbg("Waiting for L1C config from VSPA... \n\r");
	currHandle =  pxAviHandle->xVspaIntrNo[ eVspaCore ][ VSPA_MBOX_1 ].VspaToCm4QMbox;
	if( pdPASS == xQueueReceive( currHandle, (void *)&recv_mbox, portMAX_DELAY))
	{
		log_dbg( "\nRcvd VSPA MBOX[%d] msb_lsb 0x%x 0x%x\n\r",
				eVspaMboxIndex, recv_mbox.ulMsb, recv_mbox.ulLsb);
	}

	/*Process received mailbox */
	log_dbg("Processing L1C config from VSPA\r\n");

	msg_id = recv_mbox.ulMsb >> 24;

	if (0x80 == msg_id) {
		ret = process_vspa_l1c_config_80(recv_mbox);
		config_complete = true;
	} else if (0x81 == msg_id) {
		ret = process_vspa_l1c_config_81(recv_mbox);
		tdd_mode = true;
	} else {
		log_err("Invalid mbox type from vspa, msg_id: %u\n\r", msg_id);
		memset(&recv_mbox , 0, sizeof(recv_mbox));
		return -1;
	}

	if (ret) {
		log_err("process_vspa_l1c_config failed\n\r");
		return -1;
	}

	if (config_complete) {
		log_info("L1C CONFIG COMPLETE from VSPA\r\n");
		return 1;
	} else {
		return 0;
	}
}

#ifdef LA12XX_DRIVER_PCI_LAT_FP
l1c_trig_out_signal_t * l1c_get_trig_out_controls(int trig_num);

int pull_down_trig_out_gpios()
{
	for (int trig_num = 0; trig_num < NUM_TRIG_OUT; trig_num++) {
		l1c_trig_out_signal_t *trig_out = l1c_get_trig_out_controls(trig_num);

		if (trig_out->timer_type == TDD) {
			TimerInstance_t timer_instance = trig_out->timer_instance & TDD_INSTANCE_MASK;;
			uint8_t ucTxRx = (timer_instance & TDD_TX_RX_MASK) >> 4;

			if (iConfPMuxModeTbgenTdd(timer_instance, ucTxRx/2)) {
				log_err("Reset of trigger number: %d failed\n\r", trig_num);
				return -1;
			}
		} else {
			iConfPMuxModeTbgen(trig_out->tbgen, trig_out->timer_type, trig_out->timer_instance);
		}
	}

	return 0;
}

#endif // LA12XX_DRIVER_PCI_LAT_FP


void init_l1c_refapp()
{
	tdd_ul_dl_config_common_t cfg_default = {
		.interfaces[0] = {
			.in_use = 1,
			.interface_id = 0,
		},
		.pattern = { 7, 6, 2, 4 },

		.tdd_ul_dl_gap = UL_DL_GAP_DEFAULT_VALUE,
		.pps_offset = PPS_OFFSET_DEFAULT_VALUE,
		.rf_fem_ctrl = true,
		.scs = SCS_kHz30,
	};

	/* let only core-0 do some init */
	if (crt_core_id == 0) {
#ifdef LA12XX_DRIVER_PCI_LAT_FP
		pull_down_trig_out_gpios();
#endif

		pxAviHandle = pxGeulAviInit(xTaskGetCurrentTaskHandle());

		if (!pxAviHandle) {
			log_err("pxGeulAviInit failed\r\n");
			return;
		}


		/* Mailbox from all VSPA get handled on first core */
		for (VspaCore_t core = VSPA_CORE_0; core < VSPA_CORE_MAX; core++) {
			if (AVI_SUCCESS != exGeulRegisterVspaInterrupt(pxAviHandle, core,
					VSPA_MBOX_1, bVSPA0GroupBInterrupt, pxAviHandle, VSPA_MBOX_RW, true)) {
				log_err("exGeulRegisterVspaInterrupt failed for vspa: %d\n\r", core);
			}
		}

		memcpy(&config_common, &cfg_default, sizeof(tdd_ul_dl_config_common_t));
		init_l1c_refapp_buffers();
	}

	e200_trace_init();
	init_l1c_agents();
	init_l1c_ipi();
	//vL1CDPDInit();
}
