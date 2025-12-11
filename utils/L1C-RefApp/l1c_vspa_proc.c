/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

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
#include "ppc.h"
#include "qdma.h"
#include "ppu_intrinsics.h"
#include "l1c_vspa_agent.h"
#include "l1c_dma.h"
#include "l1c_fwk_tasks.h"
#include "l1c_debug.h"
#include "l1c_vspa_proc.h"

extern void vL1CUpdate_DPD_QEC(l1c_update_t *);

#define DBG_DUMP_QDMA 0x0010
#define DBG_DUMP_CMDS 0x0020

#ifdef DBG_DUMP_QDMA
extern uint32_t vspa_phys_addr_start __attribute__ ((section (".smem")));
#endif

/* issue QDMA transfer, no CB to be called on transfer end */
int qdma_memcpy_nocb(BaseType_t dst, BaseType_t src, uint32_t size, uint8_t ch_id);

/* slot information and slot count */
tdd_ctrl_t vspa_config_slots __attribute__ ((section (".smem")));

#define FR2_HRAM_BUFF_BASE		0xE5000000
#define FR2_HRAM_BUFF_SYM_SIZE		(8192 * 4)
#define FR2_HRAM_BUFF_SLOT_SIZE		(14 * FR2_HRAM_BUFF_SYM_SIZE)
#define FR2_HRAM_BUFF_TX_SLOT_CNT	3
#define FR2_HRAM_BUFF_RX_SLOT_CNT	3
/* HRAM buffers: [ Tx if1   ][ Tx if2   ][ Rx if1   ][ Rx if2   ] */
/*               [ S0 S1 S2 ][ S0 S1 S2 ][ S0 S1 S2 ][ S0 S1 S2 ] */
#define FR2_HRAM_TX_BUFF(intf) (FR2_HRAM_BUFF_BASE + (intf) * FR2_HRAM_BUFF_TX_SLOT_CNT * FR2_HRAM_BUFF_SLOT_SIZE)
#define FR2_HRAM_RX_BUFF(intf) (FR2_HRAM_BUFF_BASE + (2 + (intf)) * FR2_HRAM_BUFF_RX_SLOT_CNT * FR2_HRAM_BUFF_SLOT_SIZE)
#define FR2_HRAM_TX_SLOT(intf, idx) (FR2_HRAM_TX_BUFF(intf) + idx * FR2_HRAM_BUFF_SLOT_SIZE)
#define FR2_HRAM_RX_SLOT(intf, idx) (FR2_HRAM_RX_BUFF(intf) + idx * FR2_HRAM_BUFF_SLOT_SIZE)

/* mapping vspa cores per tx/rx role and interface - FR1 FD TDD */
uint8_t vspa_core_mapping_fr1[MAX_USED_INTERFACES][2 /*Tx, Rx*/][VSPA_TX_RX_CORE_NUM] __attribute__ ((section (".shared.data"))) = {
	[0][VSPA_TX_CORE] = { 4, 0 }, /* Interface 1, TX */
	[0][VSPA_RX_CORE] = { 2, 6 }, /* Interface 1, RX */
	[1][VSPA_TX_CORE] = { 5, 1 }, /* Interface 2, TX */
	[1][VSPA_RX_CORE] = { 3, 7 }, /* Interface 2, RX */
};

/* mapping vspa cores per tx/rx role and interface FR2 FD TDD */
uint8_t vspa_core_mapping_fr2[MAX_USED_INTERFACES][2 /*Tx, Rx*/][VSPA_TX_RX_CORE_NUM] __attribute__ ((section (".shared.data"))) = {
	[0][VSPA_TX_CORE] = { 0, 6 }, /* Interface 1, TX */
	[0][VSPA_RX_CORE] = { 4, 6 }, /* Interface 1, RX */
	[1][VSPA_TX_CORE] = { 1, 7 }, /* Interface 2, TX */
	[1][VSPA_RX_CORE] = { 5, 7 }, /* Interface 2, RX */
};

static bool_t vspa_stop_sending_msgs __attribute__ ((section (".smem")));
static uint8_t buff_idx __attribute__ ((section (".smem")));

bool l1c_vspa_configured_fr1()
{
	/* all interfaces perform FR1 TDD or FR2 TDD, no mixing suppported */
	if ((config_common.interfaces[0].in_use && config_common.interfaces[0].interface_id < 4) ||
		(config_common.interfaces[1].in_use && config_common.interfaces[1].interface_id < 4))
		return true;

	return false;
}

int qdma_memcpy_nocb(BaseType_t dst, BaseType_t src, uint32_t size, uint8_t ch_id)
{
	DescriptorFormat_t qcd;

	memset(&qcd, 0, sizeof(qcd));

	/* Assemble a QDMA Ultra-Short Format job without the interrupt on completion */
	qcd.LowAddrBase  = (BaseType_t)swap_uint32((uint32_t)src);
	qcd.dLowAddrBase = (BaseType_t)swap_uint32((uint32_t)dst);
	qcd.DataLen = swap_uint32(size);
	qcd.Cfg1 = swap_uint32(0x30000000); // QDMA_DF_SHORT_FMT
	qcd.Cfg2 = swap_uint32(0x0b00b200); // RDTTYPE, WRTTYPE, EOL

	/* Issue the QDMA job */
	return iDmaIssueCore(&qcd, ch_id, crt_core_id);
}

uint8_t l1c_vspa_core_mapping(uint8_t intf, uint8_t tx_rx, uint8_t idx)
{
	uint8_t (*vspa_core_mapping)[MAX_USED_INTERFACES][2 /*Tx, Rx*/][VSPA_TX_RX_CORE_NUM];
	bool fr1;

	fr1 = l1c_vspa_configured_fr1();
	vspa_core_mapping = (fr1) ? &vspa_core_mapping_fr1 : &vspa_core_mapping_fr2;

	return (*vspa_core_mapping)[intf][tx_rx][idx];
}

void l1c_slot_config_dump(tdd_slot_config_t *s)
{
	PRINTF("tx_offset=0x%08x, rx_offset=0x%08x, dl_syms=%2d, ul_syms=%2d, long_cp:%d\r\n",
			s->tx_buff_addr,
			s->rx_buff_addr,
			s->tx_syms,
			s->rx_syms,
			s->long_cp);
}

void l1c_vspa_slot_config_dump_all()
{
	uint8_t i, j, k;

	PRINTF("Configured VSPA core mapping for %s operation:\r\n", l1c_vspa_configured_fr1() ? "FR1" : "FR2");
	for (i = 0; i < MAX_USED_INTERFACES; i++)
	{
		PRINTF("[ interface%d ][ Tx ][ ", i + 1);
		for(k = 0; k < VSPA_TX_RX_CORE_NUM; k++)
			PRINTF(" %d", l1c_vspa_core_mapping(i, 0, k));
		PRINTF(" ]\r\n[ interface%d ][ Rx ][ ", i + 1);
		for(k = 0; k < VSPA_TX_RX_CORE_NUM; k++)
			PRINTF(" %d", l1c_vspa_core_mapping(i, 1, k));
		PRINTF(" ]\r\n");
	}

	for (j = 0; j < vspa_config_slots.count; j++)
	{
		PRINTF("SLOTS[%2d]: ", j);
		l1c_slot_config_dump(&vspa_config_slots.slots[j]);
	}
}

void dump_vspa_debug_stats()
{
	PRINTF("Last buffer index: %d\r\n", buff_idx);

	debug_vspa_dma_stat();
}

void reset_vspa_slots()
{
	vspa_config_slots.count = 0;
}

void add_slot_config_pattern(tdd_ctrl_t *cfg_slots, tdd_ul_dl_pattern_t *p, tdd_scs_t scs, uint32_t tx_offset, uint32_t rx_offset)
{
	uint8_t idx = 0, start_idx = 0;
	uint8_t i;
	uint32_t ofdm_sym_size = (scs < SCS_kHz120) ? OFDM_SYM_SIZE : OFDM_SYM_SIZE_8K;

	start_idx = cfg_slots->count;
	idx = start_idx;

	cfg_slots->count += p->dl_slots + p->ul_slots;

	for (i = 0; i < p->dl_slots; i++) {
		cfg_slots->slots[idx].tx_buff_addr = tx_offset + ofdm_sym_size * i * MAX_SYMBOLS;
		cfg_slots->slots[idx].tx_syms = MAX_SYMBOLS;
		cfg_slots->slots[idx].long_cp = (idx % scs) ? 0 : 1;
		idx++;
	}

	if (p->dl_syms || p->ul_syms) {
		cfg_slots->slots[idx].tx_buff_addr = tx_offset + ofdm_sym_size * i * MAX_SYMBOLS;
		cfg_slots->slots[idx].rx_buff_addr = rx_offset;
		cfg_slots->slots[idx].tx_syms = p->dl_syms;
		cfg_slots->slots[idx].rx_syms = p->ul_syms;
		cfg_slots->slots[idx].long_cp = (idx % scs) ? 0 : 1;
		cfg_slots->count++;
		idx++;
	}

	/* fill UL slots */
	for (i = 0; i < p->ul_slots; i++) {
		cfg_slots->slots[idx].rx_buff_addr = rx_offset + ofdm_sym_size * (p->ul_syms + i * MAX_SYMBOLS);
		cfg_slots->slots[idx].rx_syms = MAX_SYMBOLS;
		cfg_slots->slots[idx].long_cp = (idx % scs) ? 0 : 1;
		idx++;
	}
}

void vspa_proc_add_patterns(tdd_ul_dl_config_common_t *config)
{
	uint8_t i;
	uint32_t start_tx_offset = 0;
	uint32_t start_rx_offset = 0;
	tdd_ul_dl_pattern_t *p = NULL, *p2 = NULL;
	uint8_t total_slots = 0, k = 0;
	uint32_t ofdm_sym_size = (config->scs < SCS_kHz120) ? OFDM_SYM_SIZE : OFDM_SYM_SIZE_8K;

	/* setting up pattern structure */
	memset(&vspa_config_slots, 0, sizeof(tdd_ctrl_t));
	p = &config->pattern;
	p2 = &config->pattern2;

	/* compute number of total slots */
	total_slots = p->dl_slots + p->ul_slots;
	total_slots += p2->dl_slots + p2->ul_slots;
	total_slots += (p->dl_syms || p->ul_syms) ? 1 : 0;
	total_slots += (p2->dl_syms || p2->ul_syms) ? 1 : 0;

	/* figure out how many times the pattern needs to be repeaded within 20ms window */
	k = max_slots[config->scs] / total_slots;
	PRINTF("Given pattern(s) will be repeated %d times to fill %d slots\r\n", k, max_slots[config->scs]);

	for (i = 0; i < k; i++)
	{
		add_slot_config_pattern(&vspa_config_slots,
								p,
								config->scs,
								start_tx_offset,
								start_rx_offset);

		start_tx_offset += (p->dl_syms + p->dl_slots * MAX_SYMBOLS) * ofdm_sym_size;
		start_rx_offset += (p->ul_syms + p->ul_slots * MAX_SYMBOLS) * ofdm_sym_size;

		/* add pattern2; if it's empty, the APIs will do nothing */
		add_slot_config_pattern(&vspa_config_slots,
								p2,
								config->scs,
								start_tx_offset,
								start_rx_offset);

		start_tx_offset += (p2->dl_syms + p2->dl_slots * MAX_SYMBOLS) * ofdm_sym_size;
		start_rx_offset += (p2->ul_syms + p2->ul_slots * MAX_SYMBOLS) * ofdm_sym_size;
	}

	/* if listening always... update the slots */
	if (config->rx_always_listen)
	{
		for (i = 0; i < MAX_SLOTS; i++)
		{
			vspa_config_slots.slots[i].rx_syms = MAX_SYMBOLS;
			vspa_config_slots.slots[i].rx_buff_addr = ofdm_sym_size * i * MAX_SYMBOLS;
		}
	}
}

void will_send_to_core(int core_no, int send_to_cores[4])
{
	int j;

	/* if the core is already in the table, nothing to do */
	for (j = 0; j < 4; j++)
		if (send_to_cores[j] == core_no) {
			return;
		}

	/* else insert the core in the table */
	for (j = 0; j < 4; j++)
		if (send_to_cores[j] == -1) {
			send_to_cores[j] = core_no;
			return;
		}
}

void l1c_vspa_proc_slots(void *pvParameters)
{
	task_id_t task_id = *(task_id_t *)pvParameters;
	tdd_slot_config_t *pslot, *pslot2;
	tdd_slot_config_t slot_tmp;
	coeff_update_msg_t vspa_coeff_update_msg;
	uint8_t (*vspa_core_mapping)[MAX_USED_INTERFACES][2 /*Tx, Rx*/][VSPA_TX_RX_CORE_NUM];
	uint8_t slot_idx = 0, slot_n_minus_2_idx = 0, hram_tx_idx = 0, hram_rx_idx = 0, dma_hram_rx_idx = 0;
	BaseType_t dst, src;
	uint32_t trans_size;
	uint8_t i;
	bool fr1;
	int32_t slot_abs_cnt = 0;
#ifdef DBG_DUMP_QDMA
	uint64_t ddr_addr;
#endif

	fr1 = l1c_vspa_configured_fr1();
	ovly_options_t ovl = (fr1) ? RT_OVLY_FR1_TDD : RT_OVLY_FR2_TDD;
	vspa_core_mapping = (fr1) ? &vspa_core_mapping_fr1 : &vspa_core_mapping_fr2;

	buff_idx = 0;
	vspa_stop_sending_msgs = 0;

	for (i=0; i < L1C_VSPA_NUM_CORES; i++) {
		l1c_vspa_set_runtime_overlay(i, ovl);
		vTaskDelay(10);
	}

	vspa_proc_add_patterns(&config_common);
	vspa_stop_sending_msgs = 0;

	/* TX/RX QEC, CFR, DPD coeff updates */
	for (i = 0; i < MAX_USED_INTERFACES; i++)
	{
		uint8_t core;

		if (!config_common.interfaces[i].in_use)
			continue;

		/* send TX QEC coeff update message to last Tx core */
		core = (*vspa_core_mapping)[i][VSPA_TX_CORE][1];
		vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_TX_QEC);
		vspa_coeff_update_msg.memory_address = swap_uint32(config_common.interfaces[i].tx_qec_buffer);
		l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_COEFF_UPDATE, &vspa_coeff_update_msg, sizeof(coeff_update_msg_t));
#ifdef DBG_DUMP_CMDS
if (e200_print_mask_get(DBG_DUMP_CMDS)) {
		PRINTF("VSPA TX QEC coeff update msg sent to core %d with coeffs @ %#x\r\n",
			core, swap_uint32(vspa_coeff_update_msg.memory_address));
}
#endif

		/* send RX QEC coeff update message to first Rx core */
		core = (*vspa_core_mapping)[i][VSPA_RX_CORE][0];
		vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_RX_QEC);
		vspa_coeff_update_msg.memory_address = swap_uint32(config_common.interfaces[i].rx_qec_buffer);
		l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_COEFF_UPDATE, &vspa_coeff_update_msg, sizeof(coeff_update_msg_t));
#ifdef DBG_DUMP_CMDS
if (e200_print_mask_get(DBG_DUMP_CMDS)) {
		PRINTF("VSPA RX QEC coeff update msg sent to core %d with coeffs @ %#x\r\n",
			core, swap_uint32(vspa_coeff_update_msg.memory_address));
}
#endif

		if (fr1) {
			/* send CFR coeff update message to first Tx core */
			core = (*vspa_core_mapping)[i][VSPA_TX_CORE][0];
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_CFR);
			vspa_coeff_update_msg.memory_address = swap_uint32(config_common.interfaces[i].cfr_buffer);
			l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_COEFF_UPDATE, &vspa_coeff_update_msg, sizeof(coeff_update_msg_t));
#ifdef DBG_DUMP_CMDS
if (e200_print_mask_get(DBG_DUMP_CMDS)) {
			PRINTF("VSPA CFR coeff update msg sent to core %d with coeffs @ %#x\r\n",
				core, swap_uint32(vspa_coeff_update_msg.memory_address));
}
#endif

			/* send DPD coeff update message to last Tx core*/
			core = (*vspa_core_mapping)[i][VSPA_TX_CORE][1];
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_DPD);
			vspa_coeff_update_msg.memory_address = swap_uint32(config_common.interfaces[i].dpd_buffer);
			l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_COEFF_UPDATE, &vspa_coeff_update_msg, sizeof(coeff_update_msg_t));
#ifdef DBG_DUMP_CMDS
if (e200_print_mask_get(DBG_DUMP_CMDS)) {
			PRINTF("VSPA DPD coeff update msg sent to core %d with coeffs @ %#x\r\n",
				core, swap_uint32(vspa_coeff_update_msg.memory_address));
}
#endif
		}
	}

	/*

	FR2 - QDMA TX slots for each interface to HRAM, following this time diagram:

	  dma S0
	    tick -1            tick 0             tick 1             tick 2             tick 3
	     cmd S0             cmd S1             cmd S2             cmd S3             cmd S4
	      dma S1             dma S2             dma S3             dma S4             dma S5
	    <preparation>     [ slot S0         ][ slot S1         ][ slot S2         ] ...
	                      [ Tx_allowed                                                  ]

	FR2 - QDMA RX slots for each interface to HRAM, following this time diagram:

	                      [ Rx_allowed                                                                     ]
	                      [ slot SN         ][ slot SN+1       ][ slot SN+2       ][ slot SN+3       ] ...
	    tick N-1           tick N             tick N+1           tick N+2           tick N+3
	                                                              dma SN             dma SN+1

	*/

	if (!fr1) {
		for (i = 0; i < MAX_USED_INTERFACES; i++)
		{
			if (!config_common.interfaces[i].in_use)
				continue;

			dst = (BaseType_t)FR2_HRAM_TX_SLOT(i, hram_tx_idx);
			src = config_common.interfaces[i].tx_buffers[buff_idx]; // first, no offset to add
			trans_size = vspa_config_slots.slots[0].tx_syms * FR2_HRAM_BUFF_SYM_SIZE;
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {
			ddr_addr = src - vspa_phys_addr_start + HOST_VIRT_ADDR_START;
			PRINTF("QDMA[%u] for FR2 TX from DDR %#x%8x to HRAM %#x, size %#x\r\n", 2 * (slot_idx & 1) + i, (uint32_t)(ddr_addr >> 32), (uint32_t)ddr_addr, dst, trans_size);
}
#endif
			if(qdma_memcpy_nocb(dst, src, trans_size, 2 * (slot_idx & 1) + i) > 0) {
#ifndef DBG_DUMP_QDMA
				PRINTF("QDMA tx issue at slot %d\r\n", slot_abs_cnt);
#endif
			}
		}
	}

	/* wait for tick, in a loop */
	for( ;; )
	{
		wait_for_tick(task_id);

		/* check if end of radio window was reached */
		if (slot_idx == 0)
		{
			e200_trace(E200_TRACE_MSG_RADIO_FRAME, slot_abs_cnt);

			/* check for stop action */
			if (vspa_stop_sending_msgs)
				continue;
		}

		/* check if imposed number of slots has been met */
		if (config_common.limited_slot_no > 0 && slot_abs_cnt >= config_common.limited_slot_no) {
			if (slot_abs_cnt == config_common.limited_slot_no) {
				PRINTF("Requested slot count %d reached, stopped sending slot control messages to VSPA\r\n", config_common.limited_slot_no);
				l1c_vspa_proc_stop(1);
			}

			/* for FR2 the RX QDMA has a two slot shift, accomodate the processing of those here */
			if (!fr1 && slot_abs_cnt < config_common.limited_slot_no + 2) {

				// compute slot index required for rx QDMAs
				slot_n_minus_2_idx = (slot_idx + max_slots[config_common.scs] - 2) % max_slots[config_common.scs];
				pslot2 = &vspa_config_slots.slots[slot_n_minus_2_idx];

				for (i = 0; i < MAX_USED_INTERFACES; i++) {

					if (!config_common.interfaces[i].in_use)
						continue;

					if (pslot2->rx_syms) {
						src = (BaseType_t)FR2_HRAM_RX_SLOT(i, dma_hram_rx_idx);
						dst = config_common.interfaces[i].rx_buffers[buff_idx] + pslot2->rx_buff_addr;
						trans_size = pslot2->rx_syms * FR2_HRAM_BUFF_SYM_SIZE;
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {

						ddr_addr = dst - vspa_phys_addr_start + HOST_VIRT_ADDR_START;
						PRINTF("QDMA[%u] for FR2-RX to DDR %#x%8x from HRAM %#x, size %#x\r\n", 4 + 2 * (1 - (slot_idx & 1)) + i, (uint32_t)(ddr_addr >> 32), (uint32_t)ddr_addr, src, trans_size);
}
#endif
						if (pslot2->rx_syms < 7)
							vUdelay(20); // wait for HRAM data to be copied before issuing the QDMA transfer
						if (qdma_memcpy_nocb(dst, src, trans_size, 4 + 2 * (1 - (slot_idx & 1)) + i) > 0) {
#ifndef DBG_DUMP_QDMA
							PRINTF("QDMA rx issue at slot %d\r\n", slot_abs_cnt);
#endif
						}
					}
				}

				/* update slot index here, as the normal incrementation does not occur after the end of imposed slot count */
				slot_idx = (slot_idx + 1) % max_slots[config_common.scs];
				/* same for hram_rx_idx */
				hram_rx_idx = (hram_rx_idx + 1) % FR2_HRAM_BUFF_RX_SLOT_CNT;
				if (pslot2->rx_syms)
					dma_hram_rx_idx = (dma_hram_rx_idx + 1) % FR2_HRAM_BUFF_RX_SLOT_CNT;
			}

			/* no further processing */
			slot_abs_cnt++;
			continue;
		} else {
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {
			PRINTF("\r\n############ Tick for slot %d############ \r\n", slot_abs_cnt);
}
#endif
		}

		// increment absolute slot counter
		slot_abs_cnt++;

		// FR2 Rx - perform QDMA to DDR two slots after the one it was received in the HRAM location
		if (!fr1 && slot_abs_cnt > 2) {
			// compute slot index required for rx QDMAs, valid after the first two slots
			slot_n_minus_2_idx = (slot_idx + max_slots[config_common.scs] - 2) % max_slots[config_common.scs];
			pslot2 = &vspa_config_slots.slots[slot_n_minus_2_idx];
			for (i = 0; i < MAX_USED_INTERFACES; i++) {

				if (!config_common.interfaces[i].in_use)
					continue;

				if (pslot2->rx_syms) {
					src = (BaseType_t)FR2_HRAM_RX_SLOT(i, dma_hram_rx_idx);
					dst = config_common.interfaces[i].rx_buffers[buff_idx] + pslot2->rx_buff_addr;
					trans_size = pslot2->rx_syms * FR2_HRAM_BUFF_SYM_SIZE;
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {
					ddr_addr = dst - vspa_phys_addr_start + HOST_VIRT_ADDR_START;
					PRINTF("QDMA[%u] for FR2 RX to DDR %#x%8x from HRAM %#x, size %#x\r\n", 4 + 2 * (1 - (slot_idx & 1)) + i, (uint32_t)(ddr_addr >> 32), (uint32_t)ddr_addr, src, trans_size);
}
#endif
					if (pslot2->rx_syms < 7)
						vUdelay(20); // wait for HRAM data to be copied before issuing the QDMA transfer
					if (qdma_memcpy_nocb(dst, src, trans_size, 4 + 2 * (1 - (slot_idx & 1)) + i) > 0) {
#ifndef DBG_DUMP_QDMA
						PRINTF("QDMA Rx issue at slot %d\r\n", slot_abs_cnt);
#endif
					}
				}
			}
			if (pslot2->rx_syms)
				dma_hram_rx_idx = (dma_hram_rx_idx + 1) % FR2_HRAM_BUFF_RX_SLOT_CNT;
		}

		/* use as a base the slot template with precomputed address offsets */
		pslot = &vspa_config_slots.slots[slot_idx];

		for (i = 0; i < MAX_USED_INTERFACES; i++)
		{
			int send_to_cores[4] = {-1, -1, -1, -1};
			int j;

			if (!config_common.interfaces[i].in_use)
				continue;

			slot_tmp = *pslot;
			slot_tmp.interface = config_common.interfaces[i].interface_id;

			if (pslot->tx_syms)
			{
				if (fr1) {
					slot_tmp.tx_buff_addr += config_common.interfaces[i].tx_buffers[buff_idx];
				} else {
					slot_tmp.tx_buff_addr = FR2_HRAM_TX_SLOT(i, hram_tx_idx);
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {
					PRINTF("slot %u Tx HRAM ADDR %#x\r\n", slot_idx, slot_tmp.tx_buff_addr);
}
#endif
				}
				slot_tmp.tx_buff_addr = swap_uint32(slot_tmp.tx_buff_addr);
			} else {
				slot_tmp.tx_buff_addr = 0;
			}

			if (pslot->rx_syms)
			{
				if (fr1) {
					slot_tmp.rx_buff_addr += config_common.interfaces[i].rx_buffers[buff_idx];
				} else {
					slot_tmp.rx_buff_addr = FR2_HRAM_RX_SLOT(i, hram_rx_idx);
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {
					PRINTF("slot %u Rx HRAM ADDR %#x\r\n", slot_idx, slot_tmp.rx_buff_addr);
}
#endif
				}
				slot_tmp.rx_buff_addr = swap_uint32(slot_tmp.rx_buff_addr);
			} else {
				slot_tmp.rx_buff_addr = 0;
			}

			if (pslot->tx_syms)
			{
				will_send_to_core((*vspa_core_mapping)[i][0][0], send_to_cores);
				will_send_to_core((*vspa_core_mapping)[i][0][1], send_to_cores);
			}

			if (pslot->rx_syms)
			{
				will_send_to_core((*vspa_core_mapping)[i][1][0], send_to_cores);
				will_send_to_core((*vspa_core_mapping)[i][1][1], send_to_cores);
			}

			for (j = 0; j < 4; j++) {
				if (send_to_cores[j] != -1) {
					l1c_vspa_agent_enqueue_msg(send_to_cores[j],
								   L1C_MSG_A2V_SLOT_CONFIG,
								   &slot_tmp,
								   sizeof(tdd_slot_config_t));
#ifdef DBG_DUMP_CMDS
if (e200_print_mask_get(DBG_DUMP_CMDS)) {
					PRINTF("VSPA ctrl msg sent to core %d with %d tx syms @ %#x , %d rx syms @ %#x\r\n",
						send_to_cores[j], pslot->tx_syms, swap_uint32(slot_tmp.tx_buff_addr), pslot->rx_syms, swap_uint32(slot_tmp.rx_buff_addr));
}
#endif
				}
			}
		}

		/* update slot index */
		slot_idx = (slot_idx + 1) % max_slots[config_common.scs];

		if (!fr1 && pslot->rx_syms)
			hram_rx_idx = (hram_rx_idx + 1) % FR2_HRAM_BUFF_RX_SLOT_CNT;

		/* check if end of radio window was reached */
		if (slot_idx == 0)
		{
			/* check for stop action */
			if (vspa_stop_sending_msgs)
				continue;

			/* alternate buffers */
			buff_idx++;
			buff_idx %= MAX_BUFFERS_PER_INTERFACE;
		}


		/* FR2 - use QDMA to transfer next slot Tx data into HRAM */
		if (!fr1) {
			if (!(config_common.limited_slot_no > 0 && config_common.limited_slot_no == slot_abs_cnt)) {
				/* use as a base the next slot template with precomputed address offsets */
				pslot = &vspa_config_slots.slots[slot_idx];

				if (pslot->tx_syms) {
					/* will DMA next slot */
					hram_tx_idx = (hram_tx_idx + 1) % FR2_HRAM_BUFF_TX_SLOT_CNT;

					for (i = 0; i < MAX_USED_INTERFACES; i++)
					{
						if (!config_common.interfaces[i].in_use)
							continue;

						dst = (BaseType_t)FR2_HRAM_TX_SLOT(i, hram_tx_idx);
						src = config_common.interfaces[i].tx_buffers[buff_idx] + pslot->tx_buff_addr;
						trans_size = pslot->tx_syms * FR2_HRAM_BUFF_SYM_SIZE;
#ifdef DBG_DUMP_QDMA
if (e200_print_mask_get(DBG_DUMP_QDMA)) {
						ddr_addr = src - vspa_phys_addr_start + HOST_VIRT_ADDR_START;
						PRINTF("QDMA[%u] for FR2 TX from DDR %#x%8x to HRAM %#x, size %#x\r\n", 2 * (slot_idx & 1) + i, (uint32_t)(ddr_addr >> 32), (uint32_t)ddr_addr, dst, trans_size);
}
#endif
						if(qdma_memcpy_nocb(dst, src, trans_size, 2 * (slot_idx & 1) + i) > 0) {
#ifndef DBG_DUMP_QDMA
							PRINTF("QDMA Tx issue at slot %d\r\n", slot_abs_cnt);
#endif
						}
					}
				}
			}
		}
	}
}

void l1c_vspa_proc_stop(bool_t param)
{
	vspa_stop_sending_msgs = param;
}
