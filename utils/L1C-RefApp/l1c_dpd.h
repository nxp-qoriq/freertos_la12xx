/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef _L1C_DPD_H
#define _L1C_DPD_H

#include "l1c_config.h"
#include "l1c_axiq.h"
#include "l1c_vspa_agent.h"

#define VSPA_FECA_PHYS_ADDR_START  0xE5000000
#define VSPA_FECA_TX_RX_OFFSET     0xC8000

#define GEUL_FECA_BASE_ADDR        0xE0000000

#define DPD_COEFF_BUFFER      (VSPA_FECA_PHYS_ADDR_START)
#define TX_QEC_COEFF_BUFFER   (DPD_COEFF_BUFFER + COEFF_BUFFER_SIZE)
#define RX_QEC_COEFF_BUFFER   (TX_QEC_COEFF_BUFFER + COEFF_BUFFER_SIZE)

#define TX_SIGNAL_BUFFER      (RX_QEC_COEFF_BUFFER + COEFF_BUFFER_SIZE)
#define DPD_SIGNAL_BUFFER     (TX_SIGNAL_BUFFER + VSPA_FECA_TX_RX_OFFSET)
#define SRX_SIGNAL_BUFFER     (DPD_SIGNAL_BUFFER + VSPA_FECA_TX_RX_OFFSET)

#define TRAINING_SEGMENTS_MULTIPLIER (2) // for DPD training we need 2x number of reference signal segments

typedef enum {
	DPD_CONTINOUS = 0,
	DPD_STOP = 1,
	DPD_SEQUENCE = 2,
	DPD_INIT = 3,
	DPD_END = 4,
	DPD_IDLE = 5,
	DPD_MAX
} dpd_modes;

typedef struct dpd_vars {
	uint32_t tx_address;
	uint32_t dpd_dump_address;
	uint32_t rx_address;
	uint32_t signal_segment_count;
	uint32_t training_segment_count;
	uint32_t interface_id;
	uint32_t srx_interface_id;
	uint8_t srx_trx_allow_steps;
	tdd_tbgen_seq_t *srx_trx_allow;
	uint32_t advance;
	uint32_t repeat;
	uint32_t segment_size;
	uint32_t dpd_sample_rate;
	uint32_t tx_dcs_sample_rate;
	uint32_t rx_dcs_sample_rate;
	uint32_t ifft_sample_rate;
	uint32_t fft_sample_rate;
	int32_t f_nco;
	dpd_modes dpd_running_mode;
	int32_t dpd_seq_remaining;
} dpd_vars_t;

void l1c_get_tbgen_dcs_values_for_GeulB0( dpd_vars_t* d);
int32_t l1c_get_vspa_comp_for_GeulB0( dpd_vars_t* d, uint32_t rx_tx);

void vL1CDPDInit();
void vL1CDemoStart(int limited_slot_no);
void vL1CDemoStop();
void vL1CDemoConfig(l1c_config_t *);
void vL1CDPDConfig(l1c_config_dpd_t *cfg);
void vL1CDPDConfigSRx(uint8_t srx);
void vL1CDPDConfigTx(uint8_t srx);
void vL1CDPDIfftRate(config_sample_rate_e rate);
void vL1CDPDFftRate(config_sample_rate_e rate);
void vL1CDPDRate(config_sample_rate_e rate);
void vL1CDPDDACRate(config_sample_rate_e rate);
void vL1CDPDADCRate(config_sample_rate_e rate);
void vL1CDPDConfigRepeat(uint32_t cnt);
void vL1CDPDConfigTrainingSeg(uint32_t cnt);
void vL1CDPDConfigSignalSeg(uint32_t cnt);
void vL1CDPDRun();
void vL1CDPDStart(uint32_t dpd_iter);
void vL1CDPDStop();
void vL1CDPDCfgDump();
void vL1CDPDDump();
void vL1CDPDGen(int64_t freq_mHz);
void vL1CUpdate_DPD_QEC(l1c_update_t *);
void vL1CUpdate_TDD_QEC(l1c_update_t *);
void vL1CDemoDebug(l1c_debug_t *);
void vL1CIRQ(l1c_irq_type_e );
void send_host_notification(uint32_t msg_id, uint32_t data, uint32_t data2, uint32_t data3);
#endif
