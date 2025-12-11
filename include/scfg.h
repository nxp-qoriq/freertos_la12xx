// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef __SCFG_H__
#define __SCFG_H__

struct scfg_regs {
	uint32_t conf_ctrl0;
	uint32_t conf_ctrl1;
	uint32_t conf_ctrl2;
	uint32_t conf_ctrl3;
	uint32_t rstout_pulwid_xcvr1;
	uint32_t rstout_pulwid_xcvr2;
	uint32_t rstout_pulwid_xcvr3;
	uint32_t rstout_pulwid_xcvr4;
	uint32_t conf_ctrl4;
	uint32_t conf_ctrl5;
	uint32_t conf_ctrl6;
}__attribute__((packed));

typedef enum TbgenDivider {
	SCFG_TBGEN_REF_CLK_DIVIDER_1 = 0,
	SCFG_TBGEN_REF_CLK_DIVIDER_2 = 1,
	SCFG_TBGEN_REF_CLK_DIVIDER_4 = 2,
	SCFG_TBGEN_REF_CLK_DIVIDER_8 = 3,
	SCFG_TBGEN_REF_CLK_DIVIDER_16 = 4,
	SCFG_INVALID_TBGEN_REF_CLK_DIVIDER = 5,
} TbgenDivider_t;

/*CONFIG_CTRL1*/
#define TBGEN2_REF_CLK_SEL_IPG_CLK_2	(1 << 21)
#define TBGEN2_REF_CLKSEL_DCS_CLK1	(1 << 26)
#define TBGEN2_CD_EN			(1 << 13)
#define TBGEN2_REF_CLK_RAT		(  17 )
#define TBGEN1_CD_EN			( 1 << 2 )
#define TBGEN1_GEN_DEVICE_CLK_RAT	( 1 << 3 )
#define TBGEN1_REF_CLK_RAT		( 6 )
#define TBGEN1_REF_CLK_SEL		( 1 << 24 )

#define LLCP_CLK_EN			( 1 << 26 )

/*CONFIG_CTRL5*/
#define RX_CLK_SYNC_EN	(1 << 20)
#define TX_CLK_SYNC_EN	(1 << 19)

/*CONFIG_CTRL6*/
#define HS_DCS_PCLK_DIS		(1 << 14)
#define HS_DCS_AXIQ_CLK_DIS	(1 << 6)
#ifdef GEUL_BOOT_MODE_XSPI
/* Required for Tbgen clockig
 * As this is done in Yami code by HSDSC part.
 * So in flexspi boot, there is no host side.
 */
#define HSLSDCSCLKMASK			( 0xFFFF8003 )
/*Enable IP & clk_d1, clk_d2*/
#define HSDCS_CLK_CTRL			( 0xeb )
#endif
#endif
