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
#include "l1c_vspa_agent.h"
#include "l1c_vspa_proc.h"
#include "l1c_rf_ctrl.h"
#include "l1c_axiq.h"
#include "l1c_fwk_tasks.h"
#include "l1c_time_agent.h"
#include "l1c_time_proc.h"
#include "l1c_dma.h"
#include "l1c_debug.h"
#include "pmc.h"

e200_trace_data_t e200_traces[E200_NUM_CORES][E200_TRACE_SIZE] __attribute__ ((section (".smem")));
uint32_t e200_trace_index[E200_NUM_CORES] __attribute__ ((section (".smem")));
#define E200_TRACES_MAX_CNT 40
uint32_t e200_trace_en[E200_TRACES_MAX_CNT] = {0};
static uint32_t e200_print_mask = 0;

void e200_print_mask_set(uint32_t mask)
{
	e200_print_mask = mask;
}

uint32_t e200_print_mask_get(uint32_t mask)
{
	return e200_print_mask & mask;
}

uint64_t get_send_receive_latency (uint8_t core, uint32_t msg_type)
{
	uint32_t i;
	uint64_t t1 = 0, t2 = 0;

	for (i = 0; i < E200_TRACE_SIZE; i++)
		if ((e200_traces[core][i].msg == msg_type) && (e200_traces[core][i].param == E200_TRACE_PARAM_END))
			if (e200_traces[core][i].cnt > t2)
				t2 = e200_traces[core][i].cnt;

	if (t2 == 0)
		return 0;

	for (i = 0; i < E200_TRACE_SIZE; i++)
		if ((e200_traces[core][i].msg == msg_type) && (e200_traces[core][i].param == E200_TRACE_PARAM_BEGIN))
			if (e200_traces[core][i].cnt > t1 && e200_traces[core][i].cnt < t2)
				t1 = e200_traces[core][i].cnt;

	if (t1 == 0)
		return 0;

	return t2 - t1;
}

void debug_e200_stats()
{
	uint64_t t = 0;

	for (uint8_t core_id = 0; core_id < E200_NUM_CORES; core_id++)
	{
		t = get_send_receive_latency(core_id, E200_TRACE_MSG_VSPA);
		PRINTF("Core %d VSPA messages send-receive latency: ", core_id);
		if (t == 0)
			PRINTF("No messages sent to VSPA\r\n");
		else
			PRINTF("0x%x%x (TBGEN2 cycles)\r\n", (uint32_t)(t >> 32), (uint32_t)(t));
	}

	PRINTF("\r\n");

	for (uint8_t core_id = 0; core_id < E200_NUM_CORES; core_id++)
	{
		t = get_send_receive_latency(core_id, E200_TRACE_MSG_IPI_DEMO);
		PRINTF("Core %d IPI TDD messages send-receive latency: ", core_id);
		if (t == 0)
			PRINTF("No messages sent through IPI ");
		else
			PRINTF("0x%x%x (TBGEN2 cycles) ", (uint32_t)(t >> 32), (uint32_t)(t));

		if (core_id == 0)
			PRINTF("(IPI send core)\r\n");
		else
			PRINTF("\r\n");
	}

	PRINTF("\r\n");

	for (uint8_t core_id = 0; core_id < E200_NUM_CORES; core_id++)
	{
		t = get_send_receive_latency(core_id, E200_TRACE_MSG_IPI_HOST_CLI);
		PRINTF("Core %d IPI HOST CLI messages send-receive latency: ", core_id);
		if (t == 0)
			PRINTF("No messages sent through IPI ");
		else
			PRINTF("0x%x%x (TBGEN2 cycles) ", (uint32_t)(t >> 32), (uint32_t)(t));

		if (core_id == 0)
			PRINTF("(IPI send core)\r\n");
		else
			PRINTF("\r\n");
	}

	PRINTF("\r\n");
}

bool e200_trace_enabled(uint32_t msg)
{
	int i;

	for (i = 0; i < E200_TRACES_MAX_CNT; i++)
		if (e200_trace_en[i] == msg)
			return true;

	return false;
}

static inline void e200_trace_swap(uint32_t msg_a, uint32_t msg_b)
{
	int i;

	for (i = 0; i < E200_TRACES_MAX_CNT; i++)
		if (e200_trace_en[i] == msg_a) {
			e200_trace_en[i] = msg_b;

			return;
		}
}

void e200_trace_init()
{
	int i;

	for (i = 0; i < E200_TRACES_MAX_CNT; i++)
		e200_trace_en[i] = 0;
}

void e200_trace_enable(uint32_t msg)
{
	e200_trace_swap(0, msg);
}

void e200_trace_disable(uint32_t msg)
{
	e200_trace_swap(msg, 0);
}

void e200_trace_fn(uint32_t msg, uint32_t param, bool enabled)
{
	if (!enabled)
		return;

	e200_traces[crt_core_id][e200_trace_index[crt_core_id]].cnt = ullTbgenGetMasterCounter(TBGEN_2);
	e200_traces[crt_core_id][e200_trace_index[crt_core_id]].msg = msg;
	e200_traces[crt_core_id][e200_trace_index[crt_core_id]].param = param;
	e200_trace_index[crt_core_id]++;

	if (e200_trace_index[crt_core_id] >= E200_TRACE_SIZE) {
		e200_trace_index[crt_core_id] = 0;
	}
}

void e200_trace_all_fn(uint32_t msg, uint32_t param, bool enabled)
{
	uint64_t cnt = ullTbgenGetMasterCounter(TBGEN_2);

	if (!enabled)
		return;

	for (uint8_t core_id = 0; core_id < E200_NUM_CORES; core_id++)
	{
		e200_traces[core_id][e200_trace_index[core_id]].cnt = cnt;
		e200_traces[core_id][e200_trace_index[core_id]].msg = msg;
		e200_traces[core_id][e200_trace_index[core_id]].param = param;
		e200_trace_index[core_id]++;

		if (e200_trace_index[core_id] >= E200_TRACE_SIZE) {
			e200_trace_index[core_id] = 0;
		}
	}
}

void debug_e200_trace()
{
	uint16_t i, none;
	uint8_t core_idx;
	uint32_t delta = 0, us;
	uint64_t last = 0;

	e200_trace(E200_TRACE_MSG_DEBUG_E200, E200_TRACE_PARAM_TRACK);

	for (core_idx = 0; core_idx < E200_NUM_CORES; core_idx++)
	{
		none = 1;
		last = 0;
		delta = 0;
		for (i = 0; i < E200_TRACE_SIZE; i++)
		{
			if (e200_traces[core_idx][i].cnt > 0  || e200_traces[core_idx][i].msg > 0) {
				if (last > 0)
					delta = (uint32_t)(e200_traces[core_idx][i].cnt - last);
				last = e200_traces[core_idx][i].cnt;
				// clock freq 245.76 MHz = 1024*24/100
				us = delta / 1024;
				us *= 100;
				us /= 24;
				PRINTF("%d,0x%08x%08x,0x%08x,0x%08x, dt = %u (%u us)\r\n",
						core_idx,
						(uint32_t)((e200_traces[core_idx][i].cnt) >> 32),
						(uint32_t)(e200_traces[core_idx][i].cnt),
						e200_traces[core_idx][i].msg,
						e200_traces[core_idx][i].param,
						delta, us);
				none = 0;
			}
		}
		if (none)
			PRINTF("%d, no traces\r\n", core_idx);
	}
}

inline void l1c_vspa_trace_msg_send(uint8_t core, trace_msg_t *msg)
{
	l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_TRACE, msg, sizeof(trace_msg_t));
}

tdd_trace_data_t trace[TDD_TRACE_SIZE] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

void debug_vspa_dma_stat()
{
	PRINTF("VSPA DMA stats abort:\r\n");

	for (uint8_t i = 0; i < L1C_VSPA_NUM_CORES; i++)
		PRINTF("core[%d]: %#x\r\n", i, vspa_get_dmareg_stat_abort(i, 0xFFFFFFFF));
}

void debug_vspa_trace()
{
	uint16_t i, none;
	trace_msg_t msg;
	uint8_t core_idx;
	uint32_t delta = 0, us;
	uint64_t last = 0, cnt;

	e200_trace(E200_TRACE_MSG_DEBUG_VSPA, E200_TRACE_PARAM_TRACK);

	for (core_idx = 0; core_idx < L1C_VSPA_NUM_CORES; core_idx++)
	{
		memset(&trace, 0, sizeof(trace));

		msg.e200_trace_buff = (void *)swap_uint32((uint32_t)trace);
		l1c_vspa_trace_msg_send(core_idx, &msg);

		/* wait for DMA to finish */
		vTaskDelay(1000);

		none = 1;
		last = 0;
		delta = 0;
		for (i = 0; i < TDD_TRACE_SIZE; i++)
		{
			if (trace[i].cnt > 0  || trace[i].msg > 0) {
				cnt = swap_uint32((uint32_t)trace[i].cnt);
				cnt = cnt << 32;
				cnt |= swap_uint32((uint32_t)(trace[i].cnt >> 32));
				if (last > 0)
					delta = (uint32_t)(cnt - last);
				last = cnt;
				us = delta / 256; // 614.4 MHz = 256*24/10MHz
				us *= 10;
				us /= 24;
				PRINTF("%d,0x%08x%08x,0x%08x,0x%08x, dt = %u (%u us)\r\n",
						core_idx,
						swap_uint32((uint32_t)trace[i].cnt),
						swap_uint32((uint32_t)(trace[i].cnt >> 32)),
						swap_uint32(trace[i].msg),
						swap_uint32(trace[i].param),
						delta, us);
				none = 0;
			}
		}
		if (none)
			PRINTF("%d, no traces\r\n", core_idx);
	}
}

uint64_t __attribute__ ((section (".hif"))) __attribute__ ((aligned (64))) err_reg = 0;

void debug_error_register()
{
	err_reg_msg_t msg;
	uint8_t core_idx;
	uint32_t *p = (uint32_t *)&err_reg;

	for (core_idx = 0; core_idx < L1C_VSPA_NUM_CORES; core_idx++)
	{
		msg.e200_err_reg_buff = (void *)swap_uint32((uint32_t)(&err_reg));
		l1c_vspa_agent_enqueue_msg(core_idx, L1C_MSG_A2V_ERR_REG, &msg, sizeof(err_reg_msg_t));

		/* wait for DMA to finish */
		vTaskDelay(100);

		PRINTF("VSPA %d SW error flags: 0x%08x%08x\r\n",
			core_idx, swap_uint32(p[1]), swap_uint32(p[0]));
	}
}

void debug_vspa(l1c_debug_t *cfg)
{
	switch (cfg->data.component)
	{
		case L1C_DEBUG_TRACE:
			debug_vspa_trace();
			break;
		case L1C_DEBUG_ERROR_REGISTER:
			debug_error_register();
			break;
		default:
			PRINTF("Invalid component! Use 'l1c help' command to see all l1c commands\r\n");
			break;
	}
}

void debug_e200(l1c_debug_t *cfg)
{
	switch (cfg->data.component)
	{
		case L1C_DEBUG_TRACE:
			debug_e200_trace();
			break;
		case L1C_DEBUG_STATS:
			debug_e200_stats();
			break;
		default:
			PRINTF("Invalid component! Use 'l1c help' command to see all l1c commands\r\n");
			break;
	}
}

void debug_e200_print(l1c_debug_t *cfg)
{
	e200_print_mask_set(cfg->data.mask);
}

void vL1CDemoDebug(l1c_debug_t *cfg)
{
	switch (cfg->type)
	{
		case L1C_DEBUG_VSPA:
			debug_vspa(cfg);
			break;
		case L1C_DEBUG_E200:
			debug_e200(cfg);
			break;
		case L1C_DEBUG_PRINT:
			debug_e200_print(cfg);
			break;
		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
			break;
	}
	vPortFree(cfg);
}
