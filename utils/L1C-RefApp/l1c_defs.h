/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef __L1C_DEFS_H
#define __L1C_DEFS_H

#include "l1c_config.h"
#include "l1c_rf_ctrl.h"

/* TBGen Master counter defines */
#define TBGEN1_1_US         TBGEN1_REF_CLK
#define TBGEN1_25_US        (TBGEN1_REF_CLK * 25)
#define TBGEN1_50_US        (TBGEN1_25_US * 2)

#define TBGEN2_1_US         TBGEN2_REF_CLK
#define TBGEN2_25_US        (TBGEN2_REF_CLK * 25)
#define TBGEN2_50_US        (TBGEN2_25_US * 2)
#define TBGEN1_TBGEN2_DELTA (85)

/* TBgen Master counter aprox time defines */
#define TBGEN_4NS          (1)
#define TBGEN_50NS         (12)
#define TBGEN_100NS        (24)
#define TBGEN_150NS        (37)
#define TBGEN_500NS        (123) /* 1us = 245.xx */

#define TBGEN_500US        (0x1e000)

#define TBGEN_5MS          (0x12c000)
#define TBGEN_10MS         (0x258000)
#define TBGEN_1S           (0xea60000) /*@245MHz*/

/* number of slots impacts the size of buffers and duration */
#define MAX_SLOTS_30kHz    (40)
#define MAX_SLOTS_60kHz    (2*MAX_SLOTS_30kHz)
#define MAX_SLOTS_120kHz   (2*MAX_SLOTS_60kHz)
#define MAX_SLOTS_240kHz   (1*MAX_SLOTS_120kHz) // limited due to memory concerns

/* VSPA statically alocates the slot configs */
#define MAX_SLOTS          (MAX_SLOTS_240kHz)

/* max symbols in a slot */
#define MAX_SYMBOLS        (14)

/* max simultaneous interfaces */
#define MAX_USED_INTERFACES		2 /* 2T2R */
#define MAX_BUFFERS_PER_INTERFACE	2

#define OFDM_SYM_SIZE  (4096 * 4)
#define OFDM_SYM_SIZE_8K (4096 * 2 * 4)

/* L1C application tasks priorities */
#define L1C_LOW_PRIO  2
#define L1C_MED_PRIO  (L1C_LOW_PRIO + 2)
#define L1C_HIGH_PRIO (L1C_MED_PRIO + 1)

/* verify that priorities are in range */
#if (configMAX_PRIORITIES <= L1C_HIGH_PRIO)
#error configMAX_PRIORITIES value does not support configured L1C priority settings
#endif

typedef enum {
	DCS_LS0 = 0,
	DCS_LS1 = 1,
	DCS_HS  = 2,
	DCS_NUM_MAX
} dcs_block_e;

/* sub-carrier spacing */
typedef enum {
	SCS_kHz15 = 0,  /* not supported */
	SCS_kHz30 = 1,
	SCS_kHz60 = 2,
	SCS_kHz120 = 4,
	SCS_kHz240 = 8, /* not supported*/
	SCS_MAX
} tdd_scs_t;

typedef struct {
	uint32_t dl_slots;
	uint32_t dl_syms;   /* 0..13 */
	uint32_t ul_slots;
	uint32_t ul_syms;   /* 0..13 */
} tdd_ul_dl_pattern_t;

typedef struct {
	rf_ctrl_tbgen_signal_t *rf_fem_ctrl_ptr;
	rf_ctrl_gpio_signal_t *rf_fem_gpio_ctrl_ptr;
	rf_ctrl_tbgen_signal_t *rf_fem_tdd_ctrl_ptr;
	vuint32 * axiq_ctrl;
	vuint32 * lp_wp_ctrl;
	uint32_t tx_qec_buffer;
	uint32_t rx_qec_buffer;
	uint32_t cfr_buffer;
	uint32_t dpd_buffer;
	uint32_t tx_buffers[MAX_BUFFERS_PER_INTERFACE];
	uint32_t rx_buffers[MAX_BUFFERS_PER_INTERFACE];
	bool_t in_use;
	uint8_t interface_id;  /* 0 .. 3 (LS) 4 .. 5 (HS) */
	int64_t freq;
} l1c_interface_t;

typedef struct {
	tdd_scs_t scs;
	tdd_ul_dl_pattern_t pattern;
	tdd_ul_dl_pattern_t pattern2;
	int pps_offset; /* nS */
	bool_t pps_available;
	bool_t rx_always_listen;
	bool_t rf_fem_ctrl; /* runtime disable/enable RF FEM control */
	l1c_interface_t interfaces[MAX_USED_INTERFACES];
	uint8_t tdd_ul_dl_gap;    /* nS */
	bool_t tdd_control_for_rf_card; /* use TDD Switching Timer regs for RF FEM control */
	int limited_slot_no;
} tdd_ul_dl_config_common_t;

#ifndef UNUSED
#define UNUSED(_x) (void)(_x)
#endif

typedef enum {
	L1C_CONFIG_INVALID,
	L1C_CONFIG_PATTERN,
	L1C_CONFIG_PATTERN2,
	L1C_CONFIG_RX_ALWAYS_ON,
	L1C_CONFIG_RF_CTRL,
	L1C_CONFIG_DUMP,
	L1C_CONFIG_TDD_UL_DL_GAP,
	L1C_CONFIG_INTERFACE,
	L1C_CONFIG_INTERFACE2,
	L1C_CONFIG_TDD_PPS_OFFSET,
	L1C_CONFIG_SCS,
	L1C_CONFIG_MIXER,
} l1c_config_type_e;

typedef enum {
	L1C_DPD_INVALID,
	L1C_DPD_CONFIG,
	L1C_DPD_CONFIG_TX,
	L1C_DPD_CONFIG_SRX,
	L1C_DPD_RUN,
	L1C_DPD_START,
	L1C_DPD_STOP,
	L1C_DPD_GEN,
	L1C_DPD_DUMP,
} l1c_dpd_type_e;

typedef struct {
	tdd_ul_dl_pattern_t p;
} l1c_config_pattern_t;

typedef struct {
	uint8_t dcs;
	uint8_t interface;
} l1c_config_interface_t;

typedef struct {
	uint8_t dcs;
	uint8_t interface;
	uint32_t signal_segment_cnt;
	uint32_t training_segment_cnt;
	uint32_t repeat;
	uint32_t dpd_sample_rate;
	uint32_t ifft_sample_rate;
	uint32_t fft_sample_rate;
} l1c_config_dpd_t;

typedef struct {
	uint8_t intf_idx; /* 0 or 1 */
	int32_t freq;
} l1c_config_mixer_t;

typedef struct {
	l1c_config_type_e type;
	union {
		l1c_config_pattern_t cfg_pattern;
		l1c_config_interface_t cfg_interface;
		bool_t rx_always_listen;
		bool_t rf_fem_ctrl;
		uint8_t tdd_ul_dl_gap;
		int pps_offset;
		tdd_scs_t scs;
		l1c_config_mixer_t mixer;
	} data;
} l1c_config_t;

typedef enum {
	L1C_UPDATE_INVALID,
	L1C_UPDATE_DPD,
	L1C_UPDATE_TX_QEC,
	L1C_UPDATE_RX_QEC,
	L1C_UPDATE_CFR,
} l1c_update_type_e;

typedef struct {
	l1c_update_type_e type;
	uint8_t core_id;
} l1c_update_t;

typedef enum {
	L1C_DEBUG_INVALID,
	L1C_DEBUG_VSPA,
	L1C_DEBUG_E200,
	L1C_DEBUG_PRINT,
} l1c_debug_type_e;

typedef enum {
	L1C_DEBUG_INVALID_COM,
	L1C_DEBUG_TRACE,
	L1C_DEBUG_STATS,
	L1C_DEBUG_ERROR_REGISTER,
} l1c_debug_component_e;

typedef enum {
	L1C_IRQ_UNKW = -1,
	L1C_IRQ_ENABLE = 0,
	L1C_IRQ_DUMP = 1,
} l1c_irq_type_e;

typedef struct {
	l1c_debug_type_e type;
	union {
		l1c_debug_component_e component;
		uint32_t mask;
	} data;
} l1c_debug_t;

extern uint64_t start_airtime;
extern uint64_t air_slots;
extern uint64_t no_ticks;
extern uint64_t no_irqs[4];
extern uint32_t tick_interval[SCS_MAX];
extern uint32_t max_slots[SCS_MAX];

extern tdd_ul_dl_config_common_t config_common;

extern bool app_started;

#endif /* __L1C_DEFS_H */
