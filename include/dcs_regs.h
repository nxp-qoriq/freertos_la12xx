// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#ifndef _SRC_DCS_REGS_H_
#define _SRC_DCS_REGS_H_

#include "types.h"
#include "config.h"
#include "dcs_plat_config.h"


/******************************************************************************
	Memory Map(4K) of DCS is split into following sub-sections : 
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	1) AMB general purpose registers.
	2) Calibration Engine programming and control registers.
	3) Conversion Pair ADC/DAC registers.
******************************************************************************/


#define BIT(nr)					(1UL << (nr))
#define BIT_MASK(nr, mask)		(mask << (nr))
#define REG_RW			0



/******************************************************************************
	DCS Offsets 
******************************************************************************/
/* Arch Def v0.9, Ch 16.2 - Footer Note :
Following CCSR targets allow only 32-bit register READ/WRITE accesses
	DCS Macros: LS_DCS_MACRO1, LS_DCS_MACRO2, HS_DCS_MACRO
*/

/* -= Top Level Configuration Registers [ 0x0000 - 0x007F ] =- */
/* IP_ID */
#define IP_ID_OFFSET				0x0	/* IP_ID should be 0x1C04092D.*/

/* IP_RW_TEST_REG */
#define IP_RW_TEST_REG_OFFSET		0x10

/* CLK_DIVIDER_SEL */
/* Moved to dcs_plat_config.h */
/* Bit Fields - Start */
#define ANALOG_OUT_ENABLE			0
#define ANALOG_OUT_ENABLE_MASK		0b1
#define DIV_SEL_SD1					1
#define DIV_SEL_SD2					4
#define DIV_SEL_SD_MASK				0b111
#define CK_EN_SD1					7
#define CK_EN_SD2					8
#define CK_EN_SD_MASK				0b1
/* Bit Fields - End */


/* CLK_DIVIDER_RESET */
/* Moved to dcs_plat_config.h */
/* Bit Fields - Start */
#define DIV_RST						0
#define DIV_RST_1					1
#define DIV_RST_2					2
#define DIV_RST_MASK				0b1
#define SYNC_A						3
#define SYNC_B						4
#define SYNC_MASK					0b1
/* Bit Fields - End */


/* IQ_PAIR_CLK_SEL_1 */
/* Moved to dcs_plat_config.h */
/* Bit Fields - Start */
#define DIV_SEL_ADC_1				0
#define DIV_SEL_DAC_1				2
#define DIV_SEL_ADC_2				4
#define DIV_SEL_DAC_2				6
#define DIV_SEL_MASK				0b11
/* Bit Fields - End */


/* CAL_ENGINE_CTRL */
/* Moved to dcs_plat_config.h */


/* LOW_POWER_CFG */ 
/* Moved to dcs_plat_config.h */


/* GLOBAL_SYNC_CTRL */
#define GLOBAL_SYNC_CTRL_OFFSET		0x70
/* Bit Fields - Start */
#define ENABLE_ALL_ADCS				0
#define ENABLE_ALL_DACS				1
#define RESET_ALL_FIFOS				2
#define FIFOS_MASK					0b1
/* Bit Fields - End */



/* -= Calibration Engine ALU Programming Registers [ 0x0080 - 0x00FF ] =- */
/* BG_CAL_ALU_ACC0 */
#define BG_CAL_ALU_ACC0_OFFSET		0xC0

/* BG_CAL_ALU_ACC1 */
#define BG_CAL_ALU_ACC1_OFFSET		0xC4

/* BG_CAL_ALU_ACC2 */
#define BG_CAL_ALU_ACC2_OFFSET		0xC8

/* BG_CAL_ALU_ACC3 */
#define BG_CAL_ALU_ACC3_OFFSET		0xCC

/* BG_CAL_ALU_CONST1 */
#define BG_CAL_ALU_CONST1_OFFSET	0xD0

/* BG_CAL_ALU_CONST2 */
#define BG_CAL_ALU_CONST2_OFFSET	0xD4



/* -= General Calibration Configuration Registers [ 0x0100 - 0x01FF ] =- */
/* Shall be populated once implemented */

/* -= APB Test/Debug Register [ 0x0200 - 0x0203 ] =- */
/* APB_RESERVED */
#define APB_RESERVED_OFFSET			0x200
/* Bit Fields - Start */
#define CK_SEL						6
#define CK_SEL_MASK					0b1
/* Bit Fields - End */



/* -= Unused address space [ 0x0204 - 0x03FF ] =- */



/* -= Calibration Engine Control Unit (Pico16) Programming Registers [ 0x0400 - 0x05FF ] =- */
/* Shall be populated once implemented */


/* -= Unused address space [ 0x0600 - 0x07FF ] =- */


/* -= Conversion Pair 1 I/Q ADC / DAC Registers [ 0x0800 - 0x08FF ] =- */
/* ADC_ENABLE */
#define ADC_ENABLE_PAIR_1			0x800
/* Bit Fields - Start */
#define START_ADC_IQ				0
#define ENABLE_ADC_I				1
#define ENABLE_ADC_Q				2
#define ENABLE_ADC_MASK				0b1
/* Bit Fields - End */

/* ADC_IQPAIR_MODE_CONTROL */
#define ADC_IQPAIR_MODE_CONTROL_1	0x804
/* Bit Fields - Start */
#define CK_PHASE_I					0
#define CK_PHASE_Q					1
#define CH_SEL_I					2
#define CH_SEL_Q					3
#define RXCLK_FORCE_ON				4
#define TXCLK_FORCE_ON				5
#define ADC_IQPAIR_MODE_CONTROL_MASK	0b1
/* Bit Fields - End */

/* ADC_CONFIGURATION_I */
#define ADC_CONFIGURATION_I_1		0x808
/* Bit Fields - Start */
#define STG1_REDMODE_I				0
#define STG1_BINMODE_I				1
#define STG1_XXXMODE_I_MASK			0b1
#define STG1_EOC_SEL_I				11
#define STG1_EOC_SEL_I_MASK			0b11
#define STG2_EOC_SEL_I				13
#define STG2_EOC_SEL_I_MASK			0b111
/* Bit Fields - End */


/* ADC_AUTOCAL_START_I */
#define ADC_AUTOCAL_START_I1		0x80C
/* Bit Fields - Start */
#define AUTOCAL_ADC_I				0
#define AUTOCAL_ADC_I_MASK			0b1
/* Bit Fields - End */


/* ADC_RESIDUAL_AMPLIFIER_CTRL_I */
#define ADC_RESIDUAL_AMPLIFIER_CTRL_I1		0x81C
/* Bit Fields - Start */
#define RA_GMODE_I					5
#define RA_GMODE_I_MASK				0b1
/* Bit Fields - End */


/* ADC_CAL_STATUS_I */
#define ADC_CAL_STATUS_I1			0x820
/* Bit Fields - Start */
#define COMP1_CAL_DONE_I			0
#define COMP2_CAL_DONE_I			1
#define RA_OFFSET_CAL_DONE_I		2
#define FORE_GAIN_CAL_DONE_I		3
#define CAL_DONE_MASK				0b1111
/* Bit Fields - End */

/* ADC_CONFIGURATION_Q */
#define ADC_CONFIGURATION_Q_1		0x824
/* Bit Fields - Start */
#define STG1_REDMODE_Q				0
#define STG1_BINMODE_Q				1
#define STG1_XXXMODE_Q_MASK			0b1
#define STG1_EOC_SEL_Q				11
#define STG1_EOC_SEL_Q_MASK			0b11
#define STG2_EOC_SEL_Q				13
#define STG2_EOC_SEL_Q_MASK			0b111
/* Bit Fields - End */

/* ADC_AUTOCAL_START_Q */
#define ADC_AUTOCAL_START_Q1		0x828
/* Bit Fields - Start */
#define AUTOCAL_ADC_Q				0
#define AUTOCAL_ADC_Q_MASK			0b1
/* Bit Fields - End */

/* ADC_RESIDUAL_AMPLIFIER_CTRL_Q */
#define ADC_RESIDUAL_AMPLIFIER_CTRL_Q1		0x838
/* Bit Fields - Start */
#define RA_GMODE_Q					5
#define RA_GMODE_Q_MASK				0b1
/* Bit Fields - End */


/* ADC_CAL_STATUS_Q */
#define ADC_CAL_STATUS_Q1			0x83C
/* Bit Fields - Start */
#define COMP1_CAL_DONE_Q			0
#define COMP2_CAL_DONE_Q			1
#define RA_OFFSET_CAL_DONE_Q		2
#define FORE_GAIN_CAL_DONE_Q		3
#define CAL_DONE_MASK				0b1111
/* Bit Fields - End */


/* DAC_CTRL */
#define DAC_CTRL_1					0x844
/* Bit Fields - Start */
#define DAC_ENABLE					0
#define DAC_ENABLE_MASK				0b1
/* Bit Fields - End */

/* DAC_I_CONFIG */
#define DAC_I_CONFIG_1				0x848
/* Bit Fields - Start */
#define LFSR_DAC_PROG_I				9
#define LFSR_DAC_PROG_I_MASK		0b111111
/* Bit Fields - End */

/* DAC_Q_CONFIG */
#define DAC_Q_CONFIG_1				0x850
/* Bit Fields - Start */
#define LFSR_DAC_PROG_Q				9
#define LFSR_DAC_PROG_Q_MASK		0b111111
/* Bit Fields - End */

/* ----------==========----------==========----------==========----------==========---------- */


/* -= Conversion Pair 2 I/Q ADC / DAC Registers [ 0x0900 - 0x09FF ] =- */
/* ADC_ENABLE */
#define ADC_ENABLE_PAIR_2			0x900
/* Bit Fields - Start */
#define START_ADC_IQ				0
#define ENABLE_ADC_I				1
#define ENABLE_ADC_Q				2
#define ENABLE_ADC_MASK				0b1
/* Bit Fields - End */

/* ADC_IQPAIR_MODE_CONTROL */
#define ADC_IQPAIR_MODE_CONTROL_2	0x904
/* Bit Fields - Start */
#define CK_PHASE_I					0
#define CK_PHASE_Q					1
#define CH_SEL_I					2
#define CH_SEL_Q					3
#define RXCLK_FORCE_ON				4
#define TXCLK_FORCE_ON				5
#define ADC_IQPAIR_MODE_CONTROL_MASK	0b1
/* Bit Fields - End */

/* ADC_CONFIGURATION_I */
#define ADC_CONFIGURATION_I_2		0x908
/* Bit Fields - Start */
#define STG1_REDMODE_I				0
#define STG1_BINMODE_I				1
#define STG1_XXXMODE_I_MASK			0b1
#define STG1_EOC_SEL_I				11
#define STG1_EOC_SEL_I_MASK			0b11
#define STG2_EOC_SEL_I				13
#define STG2_EOC_SEL_I_MASK			0b111
/* Bit Fields - End */



/* ADC_AUTOCAL_START_I */
#define ADC_AUTOCAL_START_I2		0x90C
/* Bit Fields - Start */
#define AUTOCAL_ADC_I				0
#define AUTOCAL_ADC_I_MASK			0b1
/* Bit Fields - End */

/* ADC_RESIDUAL_AMPLIFIER_CTRL_I */
#define ADC_RESIDUAL_AMPLIFIER_CTRL_I2		0x91C
/* Bit Fields - Start */
#define RA_GMODE_I					5
#define RA_GMODE_I_MASK				0b1
/* Bit Fields - End */


/* ADC_CAL_STATUS_I */
#define ADC_CAL_STATUS_I2			0x920
/* Bit Fields - Start */
#define COMP1_CAL_DONE_I			0
#define COMP2_CAL_DONE_I			1
#define RA_OFFSET_CAL_DONE_I		2
#define FORE_GAIN_CAL_DONE_I		3
#define CAL_DONE_MASK				0b1111
/* Bit Fields - End */

/* ADC_CONFIGURATION_Q */
#define ADC_CONFIGURATION_Q_2		0x924
/* Bit Fields - Start */
#define STG1_REDMODE_Q				0
#define STG1_BINMODE_Q				1
#define STG1_XXXMODE_Q_MASK			0b1
#define STG1_EOC_SEL_Q				11
#define STG1_EOC_SEL_Q_MASK			0b11
#define STG2_EOC_SEL_Q				13
#define STG2_EOC_SEL_Q_MASK			0b111
/* Bit Fields - End */



/* ADC_AUTOCAL_START_Q */
#define ADC_AUTOCAL_START_Q2		0x928
/* Bit Fields - Start */
#define AUTOCAL_ADC_Q				0
#define AUTOCAL_ADC_Q_MASK			0b1
/* Bit Fields - End */

/* ADC_RESIDUAL_AMPLIFIER_CTRL_Q */
#define ADC_RESIDUAL_AMPLIFIER_CTRL_Q2		0x938
/* Bit Fields - Start */
#define RA_GMODE_Q					5
#define RA_GMODE_Q_MASK				0b1
/* Bit Fields - End */


/* ADC_CAL_STATUS_Q */
#define ADC_CAL_STATUS_Q2			0x93C
/* Bit Fields - Start */
#define COMP1_CAL_DONE_Q			0
#define COMP2_CAL_DONE_Q			1
#define RA_OFFSET_CAL_DONE_Q		2
#define FORE_GAIN_CAL_DONE_Q		3
#define CAL_DONE_MASK				0b1111
/* Bit Fields - End */

/* DAC_CTRL */
#define DAC_CTRL_2					0x944
/* Bit Fields - Start */
#define DAC_ENABLE					0
#define DAC_ENABLE_MASK				0b1
/* Bit Fields - End */

/* DAC_I_CONFIG */
#define DAC_I_CONFIG_2				0x948
/* Bit Fields - Start */
#define LFSR_DAC_PROG_I				9
#define LFSR_DAC_PROG_I_MASK		0b111111
/* Bit Fields - End */

/* DAC_Q_CONFIG */
#define DAC_Q_CONFIG_2				0x950
/* Bit Fields - Start */
#define LFSR_DAC_PROG_Q				9
#define LFSR_DAC_PROG_Q_MASK		0b111111
/* Bit Fields - End */





#endif /* _SRC_DCS_REGS_H_ */
