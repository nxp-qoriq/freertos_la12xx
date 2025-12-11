// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#ifndef _SRC_DCS_HS_REGS_H_
#define _SRC_DCS_HS_REGS_H_

#include <dcs_regs.h>

/* CLK_CTRL Register (DCS_HS) */
#define DCS_HS_CLK_CTRL_OFFSET		0x08

/* Bit Fields - Start */
#define SOC_DIG_CLK_DIV_SEL		2
#define SOC_DIG_CLK_DIV_SEL_MASK	0b111
#define SOC_DIG_CLK_ENABLE		3
#define SOC_DIG_CLK_ENABLE_MASK		0b1
#define CK_CORE_DIV_SEL			4
#define CK_CORE_DIV_SEL_MASK		0b1
#define CK_EN_D1			5
#define CK_EN_D1_MASK			0b1
#define CK_EN_D2			6
#define CK_EN_D2_MASK			0b1
#define IP_ENABLE			7
#define IP_ENABLE_MASK			0b1
/* Bit Fields - End */

/* DAC_ENABLE_CTRL Register (DCS_HS) */
#define DAC_ENABLE_CTRL_OFFSET		0x1C

/* Bit Fields - Start */
#define ENABLE_TXI1_DAC			0
#define ENABLE_TXI1_DAC_MASK		0b1
#define ENABLE_TXQ1_DAC			1
#define ENABLE_TXQ1_DAC_MASK		0b1
#define ENABLE_TXI2_DAC			2
#define ENABLE_TXI2_DAC_MASK		0b1
#define ENABLE_TXQ2_DAC			3
#define ENABLE_TXQ2_DAC_MASK		0b1
#define ENABLE_16G_DAC			4
#define ENABLE_16G_DAC_MASK		0b1
/* Bit Fields - End */

/* DAC_READY_STAT Register (DCS HS) */
#define DAC_READY_STAT_OFFSET		0x20

/* Bit Fields - Start */
#define TXI1_DAC_READY			0
#define TXI1_DAC_READY_MASK		0b1
#define TXQ1_DAC_READY			1
#define TXQ1_DAC_READY_MASK		0b1
#define TXI2_DAC_READY			2
#define TXI2_DAC_READY_MASK		0b1
#define TXQ2_DAC_READY			3
#define TXQ2_DAC_READY_MASK		0b1
#define DAC_16G_READY			4
#define DAC_16G_READY_MASK		0b1
/* Bit Fields - End */

/* ADC_ENABLE_CTRL Register (DCS_HS) */
#define ADC_ENABLE_CTRL_OFFSET		0x14

/* Bit Fields - Start */
#define ENABLE_RXI1_ADC			0
#define ENABLE_RXI1_ADC_MASK		0b1
#define ENABLE_RXQ1_ADC			1
#define ENABLE_RXQ1_ADC_MASK		0b1
#define ENABLE_RXI2_ADC			2
#define ENABLE_RXI2_ADC_MASK		0b1
#define ENABLE_RXQ2_ADC			3
#define ENABLE_RXQ2_ADC_MASK		0b1
#define ENABLE_16G_ADC			4
#define ENABLE_16G_ADC_MASK		0b1
/* Bit Fields - End */

/* ADC_CAL_COMPLETE_STAT Register (DCS_HS) */
#define ADC_CAL_COMPLETE_STAT_OFFSET	0x18

/* Bit Fields - Start */
#define RXI1_ADC_CAL_COMPLETE		0
#define RXI1_ADC_CAL_COMPLETE_MASK	0b1
#define RXQ1_ADC_CAL_COMPLETE		1
#define RXQ1_ADC_CAL_COMPLETE_MASK	0b1
#define RXI2_ADC_CAL_COMPLETE		2
#define RXI2_ADC_CAL_COMPLETE_MASK	0b1
#define RXQ2_ADC_CAL_COMPLETE		3
#define RXQ2_ADC_CAL_COMPLETE_MASK	0b1
#define ADC_16G_CAL_COMPLETE		4
#define ADC_16G_CAL_COMPLETE_MASK	0b1
/* Bit Fields - End */

/* ADC_DAC_REGISTER_CONTROL Register (DCS_HS)*/
#define ADC_DAC_REGISTER_CTRL_OFFSET	0x0c

/* Bit Fields - Start */
#define IQA_BUS_REQUEST			0
#define IQA_BUS_REQUEST_MASK		0b1
#define ADC_16G_4G_REGS_SEL		1
#define ADC_16G_4G_REGS_SEL_MASK	0b1
/* Bit Fields - End */

/* IREF_CTRL Register (DCS HS)*/
#define IREF_CTRL_OFFSET		0xfc0

/* Bit Fields - Start */
#define EXTRNL_REFRENCE			0
#define EXTRNL_REFRENCE_MASK		0b1
#define INTRNL_REFRENCE			1
#define INTRNL_REFRENCE_MASK		0b1
#define IREF_CTRL_RESERVED		2
#define IREF_CTRL_RESERVED_MASK		0b11111111111111
/* Bit Fields - End */

#endif
