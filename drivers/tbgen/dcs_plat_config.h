// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#ifndef _DCS_PLAT_CONFIG_H_
#define _DCS_PLAT_CONFIG_H_


/* Project specific defines go here */
#define WDL		1
#define GEUL	2


/* Select project to compile for (from options above) */
#define DCS_PLATFORM	GEUL		/* Valid Values - GEUL , WDL */


/******************************************************************************
	DCS PLL configuration Registers
******************************************************************************/




/******************************************************************************
	DCS Blocks - 16 KB each
******************************************************************************/




/* Platform Specific IP Offsets */
#if ( DCS_PLATFORM == GEUL )						/* GEUL Defines */
#define DCS_LS1_BASE			( CCSR_BASE_ADDR + 0x01280000UL )
#define DCS_LS2_BASE			( CCSR_BASE_ADDR + 0x01284000UL )
#define DCS_HS_BASE				( CCSR_BASE_ADDR + 0x01290000UL )
#define DCS_CLK_GEN_BASE		( CCSR_BASE_ADDR + 0x01380000UL )

/* Registers offset */
#define SCFG_CONFIG_CTRL0_OFFSET		0x0
#define LS_DCS_C2C_EN     (0x01 << 31)

#define SCFG_CONFIG_CTRL1_OFFSET		0x4
#define LS_DCS_C2C_CLK_EN (0x01 <<0)

#define SCFG_CONFIG_CTRL5_OFFSET		0x24
#define LS_DCS_SYNC_BYPASS_EN (1 << 25)

/*DCS PLL REG Offset*/
#define DCS_PLLRSTCTL_OFFSET			( 0x400 )

/* DCS PLL locking Check*/
#define DCS_PLL_STATUS_CHK			( 0x01 << 23 )

#define HS_CLK_CTRL_OFFSET				( 0x8 )
#define CLK_DIVIDER_SEL_OFFSET			( 0x20 )
#define CLK_DIVIDER_RESET_OFFSET		( 0x30 )
#define IQ_PAIR_CLK_SEL_1_OFFSET		( 0x40 )
#define CAL_ENGINE_CTRL_OFFSET			( 0x50 )
#define LOW_POWER_CFG_OFFSET			( 0x60 )


/* Offset of PORSR1 in DCFG block */
#define DCFG_PORSR1_OFFSET				0
#define DCFG_PORSR1_DCS_PLL_START_BIT	8
#define DCFG_PORSR1_DCS_PLL_MASK		0b1111
#define DCFG_PORCR1_OFFSET              (0x80000)

/* DCS PLL */
#define CCSR_DCGU_BASE_ADDR				( CCSR_BASE_ADDR + 0x01380000UL )
#define DCS_PLL_CONFIG_ADDR				( CCSR_DCGU_BASE_ADDR + PLLCR0_OFFSET )
#define DCS_PLL_RESET_ADDR				( CCSR_DCGU_BASE_ADDR + RSTCTL_OFFSET )

/* Clk Gen PLL Control / Status Register 0  */
#define PLLCR0_OFFSET					0x404UL
/* Bit Fields - Start */
#define CLK_SEL_SIDE					9
#define CLK_SEL_SIDE_MASK				0b11
#define CLK_SEL_TOP						14
#define CLK_SEL_TOP_MASK				0b11
#define CLK_SEL_TOP_SET_983			    0b11
#define CLK_SEL_TOP_SET_1966			0b10
#define CLKOUT_SIDE_EN					8
#define CLKOUT_SIDE_EN_MASK				0b1
#define REFCLK_SEL						16
#define REFCLK_SEL_MASK				0b11111
/* Bit Fields - End */

/* ClkGen Reset Control Register */
#define RSTCTL_OFFSET					0x0UL
/* Bit Fields - Start */
#define STP_REQ							26
#define STP_REQ_MASK					0b1
#define RST_DONE						30
#define RST_DONE_MASK					0b1
#define RST_REQ							31
#define RST_REQ_MASK					0b1
/* Bit Fields - End */



#elif ( DCS_PLATFORM == WDL )						/* WDL Defines */
#define DCS_LS1_BASE			( CCSR_BASE_ADDR + 0x01040000UL )
#define DCS_LS2_BASE			( NULL )			/* Not valid for WDL */
#define DCS_HS_BASE				( CCSR_BASE_ADDR + 0x01050000UL )


#define CLK_DIVIDER_SEL_OFFSET			( 0x8 )
#define CLK_DIVIDER_RESET_OFFSET		( 0xC )
#define IQ_PAIR_CLK_SEL_1_OFFSET		( 0x10 )
#define CAL_ENGINE_CTRL_OFFSET			( 0x14 )
#define LOW_POWER_CFG_OFFSET			( 0x18 )


/* Offset of PORSR2 in DCFG block */
#define DCFG_PORSR2_OFFSET				0
#define DCFG_PORSR2_DCS_PLL_START_BIT	8
#define DCFG_PORSR2_DCS_PLL_MASK		0b11

/* DCS PLL */
#define CCSR_DCGU_BASE_ADDR				( CCSR_BASE_ADDR + 0x01380000UL )
#define DCS_PLL_CONFIG_ADDR				( CCSR_DCGU_BASE_ADDR )
#define DCS_PLL_RESET_ADDR				( CCSR_DCGU_BASE_ADDR )

/* Bit Fields - Start */
#define CLK_SEL_SIDE					11
#define CLK_SEL_SIDE_MASK				0b11
#define CLK_SEL_TOP					14
#define CLK_SEL_TOP_MASK				0b11
#define CLKOUT_SIDE_EN					26
#define CLKOUT_SIDE_EN_MASK				0b1
#define REFCLK_SEL					20
#define REFCLK_SEL_MASK				0b11111

#define STP_REQ							27	//or 2 or xx ??	/* TODO : Need to check if this is correct bit to stop dcs pll in SeaEagle */
#define STP_REQ_MASK					0b1
#define RST_DONE						3
#define RST_DONE_MASK					0b1
#define RST_REQ							31
#define RST_REQ_MASK					0b1
/* Bit Fields - End */


#endif


/******************************************************************************
* STRUCT DCS PLL and DCS HS & LS Lookup
******************************************************************************/
typedef enum DcsPllClk {
#if ( DCS_PLATFORM == GEUL )
	DCS_PLL_CLK_0,	/* HS: 3932.16 MHz	LS: 983.04 MHz*/
	DCS_PLL_CLK_1,	/* HS: 3932.16 MHz	LS: 491.52 MHz*/
	DCS_PLL_CLK_2,	/* HS: 1966.08 MHz	LS: 983.04 MHz*/
	DCS_PLL_CLK_3,	/* HS: 1966.08 MHz	LS: 491.52 MHz*/
	DCS_PLL_CLK_4,	/* Not Valid */
	DCS_PLL_CLK_5,	/* Not Valid */
    DCS_PLL_CLK_6,  /* Geul_B0: HS:983.04 MHz  LS: 491.52 MHz  */
    DCS_PLL_CLK_7,  /* Geul_B0: HS:983.04 MHz  LS: OFF         */
	DCS_PLL_CLK_8,	/* HS: 3520 MHz LS: Off */
	DCS_PLL_CLK_9,	/* Not Valid */
    DCS_PLL_CLK_10, /* Geul_B0: HS:1966.08 MHz LS: 491.52 MHz  */
    DCS_PLL_CLK_11, /* Geul_B0: HS:1966.08 MHz LS: OFF         */
	DCS_PLL_CLK_12,	/* Not Valid */
    DCS_PLL_CLK_13, /* Geul_B0: Not Valid */
    DCS_PLL_CLK_14, /* Geul_B0: HS:OFF         LS: 491.52 MHz  */
    DCS_PLL_CLK_15, /* Geul_B0: HS:OFF         LS: OFF         */
#elif ( DCS_PLATFORM == WDL )
	DCS_PLL_CLK_0,	/* 640 MHz*/
	DCS_PLL_CLK_1,	/* 983.04 MHz*/
	DCS_PLL_CLK_2,	/* 635.40 MHz*/
	DCS_PLL_CLK_3,	/* 847.20 MHz*/
#endif

} DcsPllClk_t;

typedef struct DcsHsLsLookup {
	DcsPllClk_t eDCSPllVal;
#if ( DCS_PLATFORM == GEUL )
	double fDcsClkHs;
#endif
	double fDcsClkLs;
} DcsHsLsLookup_t;


#endif /* _DCS_PLAT_CONFIG_H_ */
