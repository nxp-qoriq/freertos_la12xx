/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef _L1C_VSPA_AGENT_H
#define _L1C_VSPA_AGENT_H

#include "l1c_defs.h"

#define L1C_VSPA_NUM_CORES	(8)
#define L1C_VSPA_RING_DEPTH	(2)

// Internal interrupt from VSPA core (0..7), group A..C (0..2)
#define IRQ_INTERNAL_VSPA_OFFSET 66 // see RefMan 17.1.8 Internal interrupt sources
#define IRQ_VSPA_GROUP_A 0
#define IRQ_VSPA_GROUP_B 1
#define IRQ_VSPA_GROUP_C 2
#define IRQ_INTERNAL_VSPA(core, group) (IRQ_INTERNAL_VSPA_OFFSET + 3 * (core) + group)

#define L1C_VSPA_MSG_SIZE	(128)
#define L1C_VSPA_PAYLOAD_SIZE	(L1C_VSPA_MSG_SIZE - 4)

#define VSPA_VERSION_DESCRIPTION_LEN	120
#define TDD_TRACE_SIZE					256

/*
 * msg_hdr (4 bytes)
 * - msgId[4]   = Msg Type/ID
 * - seqId[8]   = Msg Counting Sequence (for interface sanity and debug)
 * - tStamp[20] = Msg TimeStamp (from HW counter - for trace/debug)
 *
 * payload (124 bytes)
 * - custom for msgId
 */
typedef struct {
    uint32_t msg_hdr;
    uint32_t payload[ L1C_VSPA_PAYLOAD_SIZE / 4 ];
} l1c_vspa_msg_t __attribute__ ((aligned (64)));

typedef struct {
	uint8_t ring_pos;
	uint8_t seq_id;
	uint32_t msg_count;
} l1c_vspa_ring_ctrl_t;

#define MSG_HDR_BUILD(msgId, seqId, tStamp) \
    ((msgId & 0x0000000F)<<28) | ((seqId & 0x000000FF) << 20) | (tStamp & 0x000FFFFF)

#define SEQ_ID_BIT_WIDTH            (8)
#define SEQ_ID_BIT_MASK             ((1 << (SEQ_ID_BIT_WIDTH)) - 1) //0xFF
#define SEQ_ID_INCR(x)              ((x + 1) & (SEQ_ID_BIT_MASK))

#define MSG_HDR_GET_MSGID(hdr)      ((hdr) >> 28)
#define MSG_HDR_GET_SEQID(hdr)      (((hdr) & 0x0FF00000) >> 20)
#define MSG_HDR_GET_TSTAMP(hdr)     ((hdr) & 0x000FFFFF)

typedef enum {
	L1C_MSG_A2V_SLOT_CONFIG = 0,
	L1C_MSG_A2V_OVERLAY = 1,
	L1C_MSG_A2V_DMA_BENCH = 2,
	L1C_MSG_A2V_VERSION = 3,
	L1C_MSG_A2V_TRACE = 4,
	L1C_MSG_A2V_SIGNAL_CONFIG = 5,
	L1C_MSG_A2V_COEFF_UPDATE = 6,
	L1C_MSG_A2V_ERR_REG = 7,
	L1C_MSG_A2V_MIXER_CONFIG = 8,
	L1C_MSG_A2V_SIG_GEN_CONFIG = 9,
	L1C_MSG_A2V_CNT /* Total number of A2V messages */
} l1c_msg_types;

/* L1C_MSG_A2V_SLOT_CONFIG payload */
typedef struct {
	uint32_t tx_buff_addr; /* AXI space tx input buffer address */
	uint32_t rx_buff_addr; /* AXI space rx dump buffer address */
	uint8_t tx_syms;       /* TX number of symbols in current slot */
	uint8_t rx_syms;       /* RX number of symbols in current slot */
	uint8_t long_cp;       /* bool 0/1 : long CP (not) required on first symbol of the slot */
	uint8_t interface;     /* DCS interface */
} tdd_slot_config_t;

/* Runtime overlay API */
typedef enum {
	RT_OVLY_FR1_TDD,
	RT_OVLY_BENCH,
	RT_OVLY_DPDH,
	RT_OVLY_FR2_TDD,
	RT_OVLY_CNT,
} ovly_options_t;

/* L1C_MSG_A2V_OVERLAY payload */
typedef struct ovly_msg_t {
	ovly_options_t ovly_selector;
} ovly_msg_t;

/* VSPA benchmark API */
typedef struct {
	uint32_t dma_axi_addr;      /* Starting address of AXI buffer used by the test */
	uint32_t axi_byte_count;    /* The number of bytes that get transferred */
	uint32_t transfer_mode;     /* To select the desired direction of the VSPA DMA transfer */
	uint32_t repeats;           /* Number of iterations for current DMA transfer */
	uint32_t meas_buff_addr;    /* AXI address where VSPA returns the measurement */
} vspa_dma_bench_msg_t;

typedef struct {
	char description[VSPA_VERSION_DESCRIPTION_LEN]; /* describe the build */
	uint16_t major;        /* increment for major changes, interface evolution */
	uint16_t middle;       /* increment for each release */
	uint16_t minor;        /* increment for each build */
	uint16_t tier;         /* VSPA application tier, always non-zero, used to check for transfer readiness on e200 */
} vspa_version_t __attribute__ ((aligned (64)));

typedef struct {
	vspa_version_t *e200_vspa_version_buff;
} version_msg_t;

typedef struct tdd_trace_data_s {
	int64_t cnt;
	uint32_t msg;
	uint32_t param;
} tdd_trace_data_t;

typedef struct trace_msg_t {
	void *e200_trace_buff;
} trace_msg_t;

typedef struct err_reg_msg_t {
	void *e200_err_reg_buff;
} err_reg_msg_t;

#define BASE_SAMPLE_RATE_HZ 122880000

typedef enum {
	SAMPLE_RATE_122  = 0,  /*  122.88 Msps */
	SAMPLE_RATE_245  = 1,  /*  245.76 Msps */
	SAMPLE_RATE_491  = 2,  /*  491.52 Msps */
	SAMPLE_RATE_983  = 3,  /*  983.04 Msps */
	SAMPLE_RATE_1966 = 4,  /* 1966.08 Msps */
	SAMPLE_RATE_3932 = 5,  /* 1966.08 Msps */
} config_sample_rate_e;

static inline uint32_t sample_rate_enum2Hz(config_sample_rate_e efs)
{
	uint32_t f_s;

	f_s = BASE_SAMPLE_RATE_HZ << efs;

	return f_s;
}

/* supported clock req <-> sample rates conversions so far */
static inline config_sample_rate_e sample_rate_kHz2enum(uint32_t freq)
{
	switch(freq) {
		case TBGEN_491_52_REF_CLK_KHZ: return SAMPLE_RATE_491; 
		case TBGEN_245_76_REF_CLK_KHZ: return SAMPLE_RATE_245;
		case TBGEN_122_88_REF_CLK_KHZ: return SAMPLE_RATE_122;
		default: return 0;
	}
	return 0;
}

static inline config_sample_rate_e sample_rate_str2enum(const char *c)
{
	if (!c)
		return 0;

	switch(c[0]) {
		case '1':
			return (c[1] == '9') ? SAMPLE_RATE_1966 : SAMPLE_RATE_122; 
		case '2': return SAMPLE_RATE_245;
		case '3': return SAMPLE_RATE_3932;
		case '4': return SAMPLE_RATE_491;
		case '9': return SAMPLE_RATE_983;
	}

	return 0;
}

static inline char * sample_rate_enum2str(config_sample_rate_e e)
{
	switch(e) {
		case SAMPLE_RATE_3932: return "3932.16 Msps";
		case SAMPLE_RATE_1966: return "1966.08 Msps";
		case SAMPLE_RATE_983: return  " 983.04 Msps";
		case SAMPLE_RATE_491: return  " 491.52 Msps";
		case SAMPLE_RATE_245: return  " 245.76 Msps";
		case SAMPLE_RATE_122: return  " 122.88 Msps";
	}

	return " ######     ";
}

/* Payload of SIGNAL_CONFIG message */
typedef struct {
	uint32_t fetch_address;          /* External memory address to fetch samples */
	uint32_t dump_address;           /* External memory address to dump samples */
	uint32_t signal_segment_count;   /* Segment to transmit for the current signal */
	uint32_t training_segment_count; /* Segments to use for DPD training for the current signal */
	uint32_t axiq_path;              /* AXIQ path for transmission/reception */
	uint32_t repeat;                 /* Number of repeats for the current signal */
	uint32_t segment_size;           /* Segment size in complex half-fixed samples */
	uint32_t dpd_sample_rate;    /* DPD runs at 245.76 MHz or 491.52 MHz */
	uint32_t tx_dcs_sample_rate; /* ADC sample rate */
	uint32_t rx_dcs_sample_rate; /* DAC sample rate */
	uint32_t ifft_sample_rate;    /* ifft data sample rate */
	uint32_t fft_sample_rate;    /* fft data sample rate */
} signal_config_t;

/* Cofficients type enum */
typedef enum {
	COEFF_TYPE_DPD,
	COEFF_TYPE_TX_QEC,
	COEFF_TYPE_RX_QEC,
	COEFF_TYPE_CFR,
	COEFF_TYPE_CNT
} coeff_type_t;

/* Payload of COEFF_UPDATE message */
typedef struct {
	uint32_t memory_address; /* Memory address at which the coefficients are located */
	coeff_type_t coeff_type; /* Type of coefficients (e.g. DPD, QEC) */
} coeff_update_msg_t;

typedef struct {
	int32_t freq_mili_hz;
} mixer_config_msg_t;

typedef struct {
	int32_t f_nco;
	config_sample_rate_e f_s;
} sig_gen_config_msg_t;

typedef struct {
	tdd_slot_config_t slots[MAX_SLOTS];
	uint8_t count;
} tdd_ctrl_t;

#define L1C_IF_A2V_HOST_FLAGS_MSG_RCVD_LSB 		16

typedef struct {
	l1c_vspa_msg_t vspa_msg;
	uint8_t vspa_core;
} vspa_envelope_t;

#define VSPA_AGENT_NUM_Q_ENTRIES  			8

extern uint64_t vspa_agent_msgs;

void l1c_vspa_agent_enqueue_msg(uint8_t vspa_core, l1c_msg_types msg_type, void *payload, size_t payload_size);
void l1c_vspa_agent_main(void *pvParameters);
void l1c_vspa_set_runtime_overlay(uint8_t vspa_core, ovly_options_t ovly);
int l1c_vspa_get_runtime_overlay(uint8_t vspa_core);
void dump_core_msg_count();
char *ovly_option_name(ovly_options_t ovly);

#endif
