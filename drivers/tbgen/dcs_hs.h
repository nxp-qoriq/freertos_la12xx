// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2022 NXP
 */

#ifndef _SRC_DCS_HS_H_
#define _SRC_DCS_HS_H

#include <dcs.h>
#include <dcs_hs_regs.h>
#include <ppc.h>

/* Start - Error Codes */
#define DCS_ERR_ENABLE			1
#define DCS_ERR_DISABLE			2
#define DCS_ERR_FW_LOAD			3
#define DCS_ID_MISMATCH			4
#define DCS_WAIT_TIMEOUT		5

/*End - Error Codes*/

#define HS_ENABLE_RXI1_ADC		(0x01<<0)
#define HS_ENABLE_RXQ1_ADC		(0x01<<1)
#define HS_ENABLE_RXI2_ADC		(0x01<<2)
#define HS_ENABLE_RXQ2_ADC		(0x01<<3)
#define HS_ENABLE_16G_ADC		(0x01<<4)

#define HS_ENABLE_RXI1_DAC		(0x01<<0)
#define HS_ENABLE_RXQ1_DAC		(0x01<<1)
#define HS_ENABLE_RXI2_DAC		(0x01<<2)
#define HS_ENABLE_RXQ2_DAC		(0x01<<3)
#define HS_ENABLE_16G_DAC		(0x01<<4)

/* RISCV_MAILBOX_REG0_RESP Register */
#define RISCV_MAILBOX_REG0_RESP_OFFSET		0x48
#define INIT_CAL_DONE				(0x01 << 4) /* 4th Bit Set (As per FW) */
#define INIT_CAL_COUNT_EXCEED			0x10

/* RISCV_MAILBOX_REG3_RESP Register */
#define RISCV_MAILBOX_REG3_RESP_OFFSET		0x54 /* Mailbox Register for RISC-V communication (32 bit) */

/* Request internal ADC/DAC Bus */
#define RELSE_INTRNL_ADC_DAC_BUS_VAL		0x00
#define RQST_INTRNL_ADC_DAC_BUS_VAL		0x01

/* Enabling reference */
#define IREF_ENABLE_REF_VAL			0x07

/* RISCV_MAILBOX_REG0 Register */
#define RISCV_MAILBOX_REG0_OFFSET		0x38
#define SET_ADC_CALIBRATION_MODE		0x135

/* RISCV_MAILBOX_REG1 Register (DCS_HS) */
#define RISCV_MAILBOX_REG1_OFFSET		0x3C

#define ENABLE_EXTRA_DCS_CONFIG			0
/*
 * To configure the Clock of DCS HS 
 * pvDevHandle :-
 * DcsParams_t :-
 *
 */
int vDcsHSClockConfig(DcsBlock_t eDcsBlock, DcsParams_t * pxDcsParam);
int vSetDcsHSCal(DcsBlock_t eDcsBlock, DcsParams_t * pxDcsParam);
int vReInitiateCal(DcsBlock_t eDcsBlock);
int vEnable4GAdcBlock(DcsBlock_t eDcsBlock, DcsParams_t * pxDcsParam);
int vEnable4GDacBlock(DcsBlock_t eDcsBlock, DcsParams_t * pxDcsParam);

#endif
