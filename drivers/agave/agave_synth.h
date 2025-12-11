// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __AGAVE_SYNTH_H
#define __AGAVE_SYNTH_H
#include "FreeRTOS.h"

/*
 * Macro Definition
 */
#define AGV_SYNTH_READ_OPR	( 1 << 7 )
#define AGV_SYNTH_ADDR_MASK	0x00FF0000
#define AGV_SYNTH_DATA_MASK	0x0000FFFF

#define AGV_SYNTH_REG0		0x0
#define AGV_SYNTH_POWERDOWN	( 1 << 0 )
#define AGV_SYNTH_RESET		( 1 << 1 )
#define AGV_SYNTH_FCAL		( 1 << 3 )

#define CH_DIV_SEG_REG			0x23
#define BUF_CH_DIV_REG			0x24
#define INT_N_DIV_REG			0x26
#define NUM_MSB_N_DIV_FRAC_REG	0x2C
#define NUM_LSB_N_DIV_FRAC_REG	0x2D
#define MUX_OUT_A_REG			0x2F
#define BUF_VCO_OUT_REG			0x1F


#define MASK_INT_N_DIV			0xE001


#define CHDIV_SEG1		2
#define CHDIV_SEG2_EN		7
#define CHDIV_SEG2		9
#define RFIC_OSC_FREQ		122880
/*
 * Function Declaration
 */
int32_t AgvSynthInit( AgvDevice_t *pxAgvDev );
int32_t prvAgvSynthReadReg( AgvDevice_t *pxAgvDev, uint8_t addr,
                            uint16_t *data );
int32_t prvAgvSynthWriteReg( AgvDevice_t *pxAgvDev, uint8_t addr,
                             uint16_t data );
void AgvAdjustPllFreq(AgvDevice_t *pxAgvDev, int32_t freq_khz );
#endif //__AGAVE_SYNTH_H
