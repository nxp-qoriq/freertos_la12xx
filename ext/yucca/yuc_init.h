// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __YUC_INIT_H
#define __YUC_INIT_H

#include <yuc_rfic_common.h>

#define FR1_IDX_YC1                        1
#define FR1_IDX_YC2                        2

typedef struct yucStateData {
    u32 bbRx;
    u32 trxpllFreqKhz;
    u32 calpllFreqKhz;
    u32 path;
    u32 pathBand;
    u32 pathRssi;
    u32 pathDpdMode;
    u32 pathRxbw;
    u32 pathTxbw;
    u32 actMode;
    u32 actDpdRx;
    u32 rfRx;
    u32 rfBand;
    u32 rfLoopmode;
} yucStateData_t;

typedef struct yucCurData {
    uint8_t txGainIdx;
    uint8_t rxGainIdx;
} yucCurData_t;

typedef struct YucRfInfo
{
    u32 llcp_rfic_addr; /* active Yucca addr */
    u32 llcp_rfic1_addr;
    u32 llcp_rfic2_addr;
    eRficFR1Mode eFR1Mode;
    volatile yuc_cal_data_t *cal_data;
    yuc_sys_state_t state;
    yuc_state_events_t event;
    yucStateData_t state_data; /* Need another for yucca2? */
    yucCurData_t *yucData[2];
    rf_sw_cmd_desc_t pSMDesc;   /**< This is temporary buffer to be used by SM for calling several
                                     FW or SW commands to avoid huge stack size. */
} YucRfInfo_t;

YucRfInfo_t * pYucInfo;

/*
 * Function Declaration
 */
int32_t iYucInit( RficHandle_t pxRFDevice);
int32_t iYucDeInit( void );
#endif
