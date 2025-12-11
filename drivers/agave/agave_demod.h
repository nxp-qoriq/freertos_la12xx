// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#ifndef __AGAVE_DEMOD_H
#define __AGAVE_DEMOD_H

/*
 * Macro Definition
 */
#define AGV_DEMOD_READ_OPR      ( 1 << 7 )

#define AGV_DEMOD_REG22         ( 0x16 )
#define AGV_DEMOD_RESET         ( 0xF8 )

#define AGV_DEMOD_REG16         ( 0x10 )
#define AGV_DEMOD_ATTN_MASK     ( ~( 0xF8 ))
#define AGV_DEMOD_ATTN_SHIFT    ( 3 )
#define AGV_DEMOD_ATTN_MIN      ( 0 )
#define AGV_DEMOD_ATTN_MAX      ( 31 )

#define AGV_DEMOD_REG21         ( 0x15 )
#define AGV_DEMOD_GAIN_MASK     ( ~( 0x70 ))
#define AGV_DEMOD_GAIN_SHIFT    ( 4 )
#define AGV_DEMOD_GAIN_MIN      ( 0 )
#define AGV_DEMOD_GAIN_MAX      ( 7 )

/* LO matching register - For 3.5G */
#define AGV_DEMOD_REG18         ( 0x12 )
#define AGV_DEMOD_REG19         ( 0x13 )
#define AGV_DEMOD_LO_MATCH1     ( 0x40 )
#define AGV_DEMOD_LO_MATCH2     ( 0x80 )

/*
 * Function Declaration
 */
int32_t prvAgvDemodReadReg( AgvDevice_t *pxAgvDev, AgvRx_t eRxId, uint8_t addr,
                            uint8_t *data );
int32_t prvAgvDemodWriteReg( AgvDevice_t *pxAgvDev, AgvRx_t eRxId, uint8_t addr,
                             uint8_t data );
BaseType_t xAgvDemodGainCtrl( AgvDevice_t *pxAgvDev, AgvRx_t eRxId,
                              int32_t iAttn, int32_t iGain );
int32_t AgvDemodInit( AgvDevice_t *pxAgvDev );
#endif //__AGAVE_DEMOD_H
