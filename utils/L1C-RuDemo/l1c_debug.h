/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

#ifndef _L1C_DEBUG_H
#define _L1C_DEBUG_H

#define E200_TRACE_SIZE  128
#define E200_NUM_CORES   4

enum E200_TRACE_msg_type {
	E200_TRACE_MSG_TDD_START       =  0x101,
	E200_TRACE_MSG_TDD_STOP        =  0x102,
	E200_TRACE_MSG_DPD_START       =  0x201,
	E200_TRACE_MSG_DPD_STOP        =  0x202,
	E200_TRACE_MSG_DPD_RUN         =  0x202,
	E200_TRACE_MSG_BENCH_E200      =  0x301,
	E200_TRACE_MSG_BENCH_VSPA      =  0x302,
	E200_TRACE_MSG_BENCH_QDMA      =  0x303,
	E200_TRACE_MSG_DEBUG_E200      =  0x401,
	E200_TRACE_MSG_DEBUG_VSPA      =  0x402,
	E200_TRACE_MSG_UPDATE_DPD      =  0x501,
	E200_TRACE_MSG_UPDATE_TXQEC    =  0x502,
	E200_TRACE_MSG_UPDATE_RXQEC    =  0x503,
	E200_TRACE_MSG_UPDATE_CFR      =  0x504,
	E200_TRACE_MSG_TDD_CONFIG_PATT =  0x601,
	E200_TRACE_MSG_TDD_CONFIG_IF   =  0x602,
	E200_TRACE_MSG_TDD_CONFIG_SCS  =  0x603,
	E200_TRACE_MSG_TDD_CONFIG_RXON =  0x604,
	E200_TRACE_MSG_TDD_CONFIG_PPS  =  0x605,
	E200_TRACE_MSG_TDD_CONFIG_GAP  =  0x606,
	E200_TRACE_MSG_TDD_CONFIG_DUMP =  0x607,
	E200_TRACE_MSG_DPD_CONFIG      =  0x608,
	E200_TRACE_MSG_DPD_CONFIG_DUMP =  0x609,
	E200_TRACE_MSG_VERSION         =  0x701,
	E200_TRACE_MSG_IRQ_ENABLE      =  0x801,
	E200_TRACE_MSG_IRQ_DUMP        =  0x802,
	E200_TRACE_MSG_VSPA            =  0x901,
	E200_TRACE_MSG_RADIO_FRAME     =  0x902,
	E200_TRACE_MSG_IPI_HOST_CLI    =  0x1001,
	E200_TRACE_MSG_IPI_DEMO        =  0x1002,
	E200_TRACE_MSG_TDD_CONFIG_RFCTL=  0x1003,
};

enum E200_TRACE_param_type {
	E200_TRACE_PARAM_TRACK      = 0x0,
	E200_TRACE_PARAM_BEGIN      = 0x1,
	E200_TRACE_PARAM_END        = 0x2,
};

typedef struct e200_trace_data_s {
	uint64_t cnt;
	uint32_t msg;
	uint32_t param;
} e200_trace_data_t;

#define e200_trace(msg, param) e200_trace_fn(msg, param, e200_trace_enabled(msg))
#define e200_trace_all(msg, param) e200_trace_all_fn(msg, param, e200_trace_enabled(msg))

void e200_print_mask_set(uint32_t mask);
uint32_t e200_print_mask_get(uint32_t mask);
void debug_vspa_dma_stat();
bool e200_trace_enabled(uint32_t msg);
void e200_trace_enable(uint32_t msg);
void e200_trace_init();
void e200_trace_fn(uint32_t msg, uint32_t param, bool enabled);
void e200_trace_all_fn(uint32_t msg, uint32_t param, bool enabled);

#endif
