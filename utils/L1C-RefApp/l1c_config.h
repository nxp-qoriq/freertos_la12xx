/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef __L1C_CONFIG_H
#define __L1C_CONFIG_H

/* define NO_RF to prevent RefApp from controlling RF FEM signals */
//#undef NO_RF
#define NO_RF

/* undefine L1C_REFAPP_DEBUG to hide debug messages */
#if defined (DIORA_RF)
#undef L1C_REFAPP_DEBUG
#else
#define L1C_REFAPP_DEBUG
#endif
#undef L1C_REFAPP_DEBUG_ADVANCED



/* Agent & procedure must run on the same assigned e200 cores */
#define TIME_AGENT_CORE        L1_CORE_2
#define VSPA_AGENT_CORE        L1_CORE_0
/* DPD logic is similar to a VSPA procedure; only core 0 is currently supported for DPD */
#define DPD_CORE               VSPA_AGENT_CORE

/* Use DDR scratch buffer for VSPA TX & RX samples */
#ifdef GEUL_LA12XXGDE
#define HOST_VIRT_ADDR_START    0xDBE00000
#else
#define HOST_VIRT_ADDR_START    0x2360000000
#endif

#ifndef CONST_1MB
#define CONST_1MB 0x100000
#endif

#define TX_RX_OFFSET_FR1	(0x30 * CONST_1MB)
//#define TX_RX_OFFSET_FR2	(0x118 * CONST_1MB) // too large, not supported by BSP
#define TX_RX_OFFSET_FR2	(0x8c * CONST_1MB)
#define TX_RX_OFFSET ((config_common.scs < SCS_kHz60) ? TX_RX_OFFSET_FR1 : TX_RX_OFFSET_FR2)

#define COEFF_BUFFER_SIZE	0x1000 /* 4K */

/* PCI BAR addresses for FECA memories */
#if defined(GEUL_LA1238RDB) || defined(GEUL_LA1238CPE)
#define HOST_PCIE_BAR2_ADDR     0x9868000000
#elif defined(GEUL_LA1224)
#define HOST_PCIE_BAR2_ADDR     0xA068000000
#endif

/* Default value for configurable ul-dl gap and pps offset */
#define UL_DL_GAP_DEFAULT_VALUE   16
#define PPS_OFFSET_DEFAULT_VALUE  0

/* specific optimizations for RefApp can be configured here */
/* #pragma GCC optimize ("O3") */

/* experiment controlling devel radio card directly with GPIOs, without TBGEN */
/*
#define MW_GPIO_TEST
#define MW_REVA
*/

// temporary workaround for git trees divergency
extern void send_host_notification(uint32_t msg_id, uint32_t data, uint32_t data2, uint32_t data3);
#endif
