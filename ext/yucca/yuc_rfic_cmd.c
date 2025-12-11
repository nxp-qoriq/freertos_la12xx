// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2023 NXP
 */

#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "common.h"
#include "immap.h"
#include "portmacro.h"
#include "projdefs.h"
#include "rf_dev.h"
#include "types.h"
#include <rf_sw_cmds.h>
#include "yuc_rfic_common.h"
#include "yuc_rfic_cmd.h"
#include "yuc_rfic.h"
#include "yuc_rfic_types.h"
#include <patron_fem.h>
#include "gpio.h"
#include "gpio_regs.h"
#include <math.h>

#define SETBIT( val, pos )      ( val |= ( 1UL << pos ) )
#define CLEARBIT( val, pos )    ( val &= ( ~( 1UL << pos ) ) )

#define YUC_TXGAIN_STEP_ERR_POINT 23

int32_t gRXGainLUT[] = {
    /* 0 */ 0,
    /* 1 */ 21,
    /* 2 */ 42,
    /* 3 */ 58,
    /* 4 */ 77,
    /* 5 */ 98,
    /* 6 */ 114,
    /* 7 */ 135,
    /* 8 */ 152,
    /* 9 */ 171,
    /* 10 */ 192,
    /* 11 */ 212,
    /* 12 */ 231,
    /* 13 */ 252,
    /* 14 */ 270,
    /* 15 */ 289,
    /* 16 */ 310,
    /* 17 */ 329,
    /* 18 */ 349,
    /* 19 */ 369,
    /* 20 */ 383,
    /* 21 */ 403,
    /* 22 */ 423,
    /* 23 */ 442,
    /* 24 */ 462,
    /* 25 */ 482,
    /* 26 */ 501,
    /* 27 */ 521,
    /* 28 */ 541,
    /* 29 */ 561,
    /* 30 */ 580,
    /* 31 */ 600,
    /* 32 */ 621,
    /* 33 */ 639,
    /* 34 */ 654
};
size_t gRXGainLUTsize = sizeof(gRXGainLUT)/sizeof(gRXGainLUT[0]);

int32_t gTXGainLUT[] = {
    /* 0 */ 0,
    /* 1 */ 16,
    /* 2 */ 32,
    /* 3 */ 50,
    /* 4 */ 76,
    /* 5 */ 94,
    /* 6 */ 115,
    /* 7 */ 135,
    /* 8 */ 154,
    /* 9 */ 177,
    /* 10 */ 196,
    /* 11 */ 214,
    /* 12 */ 236,
    /* 13 */ 255,
    /* 14 */ 273,
    /* 15 */ 294,
    /* 16 */ 313,
    /* 17 */ 331,
    /* 18 */ 351,
    /* 19 */ 371,
    /* 20 */ 390,
    /* 21 */ 406,
    /* 22 */ 424,
    /* 23 */ 442,
    /* 24 */ 459,
    /* 25 */ 476,
    /* 26 */ 487,
    /* 27 */ 505,
    /* 28 */ 517,
    /* 29 */ 527,
    /* 30 */ 534,
    /* 31 */ 545
};

const rxgain_table_t rxGainTable[YUC_RX_GAIN_MAX_IDX + 1] =
    {
        { 0, 0 },
        { 0, 1 },
        { 0, 2 },
        { 0, 3 },
        { 1, 1 },
        { 1, 2 },
        { 1, 3 },
        { 2, 2 },
        { 2, 3 },
        { 3, 1 },
        { 3, 2 },
        { 3, 3 },
        { 4, 1 },
        { 4, 2 },
        { 4, 3 },
        { 5, 1 },
        { 5, 2 },
        { 5, 3 },
        { 6, 1 },
        { 6, 2 },
        { 6, 3 },
        { 7, 1 },
        { 7, 2 },
        { 7, 3 },
        { 7, 4 },
        { 7, 5 },
        { 7, 6 },
        { 7, 7 },
        { 7, 8 },
        { 7, 9 },
        { 7, 10 },
        { 7, 11 },
        { 7, 12 },
        { 7, 13 },
        { 7, 14 }
    };
size_t gTXGainLUTsize = sizeof(gTXGainLUT)/sizeof(gTXGainLUT[0]);
/* Forward declarations of Yucca firmware command apis of type iRFCmdApi */
int32_t iprvYucRficGetVersion( RficHandle_t xHandle,
                               rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetRegister( RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetRegister( RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetRegisterMasked( RficHandle_t xHandle,
                                      rf_sw_cmd_desc_t * pCmdDesc );
#if YUC_ALL_FWCMDS
int32_t iprvYucRficSetProperty( RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetProperty( RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc );
#endif
int32_t iprvYucRficGetSysStatus( RficHandle_t xHandle,
                                 rf_sw_cmd_desc_t * pCmdDesc );
#if YUC_ALL_FWCMDS
int32_t iprvYucRficMeasAuxAdc( RficHandle_t xHandle,
                               rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetMeasPa( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetMeasPa( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
#endif
int32_t iprvYucRficGetTemperature( RficHandle_t xHandle,
                                   rf_sw_cmd_desc_t * pCmdDesc );
#if YUC_ALL_FWCMDS
int32_t iprvYucRficCalResistor( RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficCalRegulator( RficHandle_t xHandle,
                                 rf_sw_cmd_desc_t * pCmdDesc );
#endif
int32_t iprvYucRficSetTxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetTxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetRxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetRxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetTrxPll( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetTrxPll( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetCalPll( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetCalPll( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetPath( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetPath( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetGain( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetGain( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetActive( RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetRfLoopBack( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetBbLoopBack( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
#if YUC_ALL_FWCMDS
int32_t iprvYucRficSetRSSI( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetRSSI( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
#endif
int32_t iprvYucRficSetRxDc( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficGetRxDc( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSetCalRxDc( RficHandle_t xHandle,
                               rf_sw_cmd_desc_t * pCmdDesc );

/** Forward declarations fo Yucca swcmd apis of type iRFCmdApi */
int32_t iprvYucRficSwSwTx( RficHandle_t xHandle,
                           rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwSwRx( RficHandle_t xHandle,
                           rf_sw_cmd_desc_t * pCmdDesc );
int32_t iYucRficFr1DpdSwCtrl( RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc );
int32_t iYucRficFr1FemLna2Bypass( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iYucRficFr1SetMode( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iYucRficFr1GetMode( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwSetTxBw( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwSetRxBw( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwSetTrxPll( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwSetPath( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwPllStatus( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwRelTxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwRelRxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwSetDPD( RficHandle_t xHandle,
                               rf_sw_cmd_desc_t * pCmdDesc );
int32_t iprvYucRficSwRelSRxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );

/**
 * Array of yucca firmware and sw commands
 * Index mapped to FW command id as per the comments
 * Please maintain index correcly.
 */
cmdHandler_t pYucFWCmdHandlers[ YUC_FW_CMD_MAX_COUNT ] =
{
    { NULL,                         0                             }, /** Nothing no command id with index 0 */
    { iprvYucRficGetVersion,        YUC_SM_ALL                    }, /** YUC_FWCMD_GETVERSION        (0x001) */
    { iprvYucRficSetRegister,       YUC_SM_ALL                    }, /** YUC_FWCMD_SETREGISTER       (0x002) */
    { iprvYucRficGetRegister,       YUC_SM_ALL                    }, /** YUC_FWCMD_GETREGISTER       (0x003) */
    { iprvYucRficSetRegisterMasked, YUC_SM_ALL                    }, /** YUC_FWCMD_SETREGISTERMASKED (0x004) */
    #if YUC_ALL_FWCMDS
    { iprvYucRficSetProperty,       YUC_SM_ALL                    }, /** YUC_FWCMD_SETPROPERTY       (0x005) */
    { iprvYucRficGetProperty,       YUC_SM_ALL                    }, /** YUC_FWCMD_GETPROPERTY       (0x006) */
    #else
    { NULL,                         0                             },
    { NULL,                         0                             },
    #endif
    { NULL,                         0                             },
    { iprvYucRficGetSysStatus,      YUC_SM_ALL                    }, /** YUC_FWCMD_GETSYSSTATUS      (0x008) */
    #if YUC_ALL_FWCMDS
    { iprvYucRficMeasAuxAdc,        YUC_SM_ALL                    }, /** YUC_FWCMD_MEASAUXADC        (0x009) */
    { iprvYucRficSetMeasPa,         YUC_SM_ALL                    }, /** YUC_FWCMD_SETMEASPA         (0x00A) */
    { iprvYucRficGetMeasPa,         YUC_SM_ALL                    }, /** YUC_FWCMD_GETMEASPA         (0x00B) */
    #else
    { NULL,                         0                             },
    { NULL,                         0                             },
    { NULL,                         0                             },
    #endif
    { iprvYucRficGetTemperature,    YUC_SM_ALL                    }, /** YUC_FWCMD_GETTEMPERATURE    (0x00C) */
    #if YUC_ALL_FWCMDS
    { iprvYucRficCalResistor,       YUC_SM_STANDBY                }, /** YUC_FWCMD_CALRESISTOR       (0x00D) */
    { iprvYucRficCalRegulator,      YUC_SM_STANDBY                }, /** YUC_FWCMD_CALREGULATOR      (0x00E) */
    #else
    { NULL,                         0                             },
    { NULL,                         0                             },
    #endif
    { iprvYucRficSetTxBw,           YUC_SM_ALL                    }, /** YUC_FWCMD_SETTXBW           (0x00F) */
    { iprvYucRficGetTxBw,           YUC_SM_ALL                    }, /** YUC_FWCMD_GETTXBW           (0x010) */
    { iprvYucRficSetRxBw,           YUC_SM_ALL                    }, /** YUC_FWCMD_SETRXBW           (0x011) */
    { iprvYucRficGetRxBw,           YUC_SM_ALL                    }, /** YUC_FWCMD_GETRXBW           (0x012) */
    { iprvYucRficSetTrxPll,         YUC_SM_STANDBY|YUC_SM_TRXPLL  }, /** YUC_FWCMD_SETTRXPLL         (0x013) */
    { iprvYucRficGetTrxPll,         YUC_SM_ALL                    }, /** YUC_FWCMD_GETTRXPLL         (0x014) */
    { iprvYucRficSetCalPll,         YUC_SM_CALPLL|YUC_SM_PREPARED }, /** YUC_FWCMD_SETCALPLL         (0x015) */
    { iprvYucRficGetCalPll,         YUC_SM_ALL                    }, /** YUC_FWCMD_GETCALPLL         (0x016) */
    { iprvYucRficSetPath,           YUC_SM_TRXPLL|YUC_SM_PREPARED }, /** YUC_FWCMD_SETPATH           (0x017) */
    { iprvYucRficGetPath,           YUC_SM_ALL                    }, /** YUC_FWCMD_GETPATH           (0x018) */
    { iprvYucRficSetGain,           YUC_SM_ALL                    }, /** YUC_FWCMD_SETGAIN           (0x019) */
    { iprvYucRficGetGain,           YUC_SM_ALL                    }, /** YUC_FWCMD_GETGAIN           (0x01A) */
    { iprvYucRficSetActive,         YUC_SM_ACTIVE                 }, /** YUC_FWCMD_SETACTIVE         (0x01B) */
    { iprvYucRficSetRfLoopBack,     YUC_SM_CALPLL|YUC_SM_RFLOOP   }, /** YUC_FWCMD_SETRFLOOPBACK     (0x01C) */
    { iprvYucRficSetBbLoopBack,     YUC_SM_STANDBY|YUC_SM_BBLOOP  }, /** YUC_FWCMD_SETBBLOOPBACK     (0x01D) */
    #if YUC_ALL_FWCMDS
    { iprvYucRficSetRSSI,           YUC_SM_ALL                    }, /** YUC_FWCMD_SETRSSI           (0x01E) */
    { iprvYucRficGetRSSI,           YUC_SM_ALL                    }, /** YUC_FWCMD_GETRSSI           (0x01F) */
    #else
    { NULL,                         0                             },
    { NULL,                         0                             },
    #endif
    { iprvYucRficSetRxDc,           YUC_SM_ALL                    }, /** YUC_FWCMD_SETRXDC           (0x020) */
    { iprvYucRficGetRxDc,           YUC_SM_ALL                    }, /** YUC_FWCMD_GETRXDC           (0x021) */
    { iprvYucRficSetCalRxDc,        YUC_SM_ALL                    }, /** YUC_FWCMD_SETCALRXDC        (0x022) */
};
cmdHandler_t pYucSWCmdHandlers[ RF_SW_CMD_MAX_COUNT ] =
{
    { iprvYucRficSwSwTx,            YUC_SM_ALL                    }, /** RF_SWCMD_SWTX          */
    { iprvYucRficSwSwRx,            YUC_SM_ALL                    }, /** RF_SWCMD_SWRX          */
    { iYucRficFr1FemLna2Bypass,     YUC_SM_ALL                    }, /** RF_SWCMD_SETLNABY PASS */
    { iYucRficFr1DpdSwCtrl,         YUC_SM_ALL                    }, /** RF_SWCMD_SETDPDSW      */
    { iYucRficFr1SetMode,           YUC_SM_ALL                    }, /** RF_SWCMD_SETMODE       */
    { iYucRficFr1GetMode,           YUC_SM_ALL                    }, /** RF_SWCMD_GETMODE       */
    { iprvYucRficSwSetTxBw,         YUC_SM_ALL                    }, /** RF_SWCMD_SETTXBW       */
    { iprvYucRficSwSetRxBw,         YUC_SM_ALL                    }, /** RF_SWCMD_SETRXBW       */
    { iprvYucRficSwSetTrxPll,       YUC_SM_STANDBY                }, /** RF_SWCMD_SETTRXPLL     */
    { iprvYucRficSwSetPath,         YUC_SM_TRXPLL                 }, /** RF_SWCMD_SETPATH       */
    { iprvYucRficSwPllStatus,       YUC_SM_ALL                    }, /** RF_SWCMD_PLLSTATUS     */
    { iprvYucRficSwRelTxGain,       YUC_SM_ALL                    },/**< RF_SWCMD_REL_TXGAIN */
    { iprvYucRficSwRelRxGain,       YUC_SM_ALL                    },/**< RF_SWCMD_REL_RXGAIN */
    { iprvYucRficSwSetDPD,          YUC_SM_ACTIVE                 }, /**< RF_SWCMD_SETDPD */
    { iprvYucRficSwRelSRxGain,      YUC_SM_ALL                    },/**< RF_SWCMD_REL_SRXGAIN */
};

char *cYucStateToStr( uint32_t state )
{
    switch ( state )
    {
        RET_CASE( YUC_SS_ACTIVE );
        RET_CASE( YUC_SS_BBLOOP );
        RET_CASE( YUC_SS_PREPARED );
        RET_CASE( YUC_SS_TRXPLLON );
        RET_CASE( YUC_SS_CALPLLON );
        RET_CASE( YUC_SS_RFLOOP );
        RET_CASE( YUC_SS_STANDBY );
    }
    return YUC_RTC_UNKNOWN;
}

static int32_t iprvYucRficGetSysState()
{
    int32_t xRet = RF_SW_CMD_RESULT_ERROR;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETSYSSTATUS_WDS ] = { 0 };
    sw_cmd_get_sys_status_t CmdData;

    ulCmdData = YUC_FWCMD_GETSYSSTATUS;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL, 0 );
    if( xRet )
    {
        RF_LOGERRMSG( "Get Sys State command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETSYSSTATUS_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Sys State response fail" );
        goto out;
    }
    YUC_FWCMD_GETSYSSTATUS_SYSSTATE_W2P( CmdData.sys_state, uRficWds[ 1 ] );

    return CmdData.sys_state;

out:
    return -xRet;
}

int32_t iprvSMTrxPll(RficHandle_t xHandle, bool enable)
{
    rf_sw_cmd_desc_t *pDesc = &(pYucInfo->pSMDesc);
    if (enable)
    {
        pDesc->cmd = RF_SWCMD_SETTRXPLL;
        sw_cmd_setget_trxpll_t *pll = (sw_cmd_setget_trxpll_t *)(&(pDesc->data));
        pll->freq_khz = pYucInfo->state_data.trxpllFreqKhz;
        return iprvYucRficSwSetTrxPll(xHandle, pDesc);
    }
    else
    {
        pDesc->cmd = YUC_FWCMD_SETTRXPLL;
        sw_cmd_setget_trxpll_t *pll = (sw_cmd_setget_trxpll_t *)(&(pDesc->data));
        pll->freq_khz = pYucInfo->state_data.trxpllFreqKhz;
        pll->mode = YUC_OFF;
        return iprvYucRficSetTrxPll(xHandle, pDesc);
    }
}

int32_t iprvSMCalPll(RficHandle_t xHandle, bool enable)
{
    rf_sw_cmd_desc_t *pDesc = &(pYucInfo->pSMDesc);
    pDesc->cmd = YUC_FWCMD_SETCALPLL;
    sw_cmd_setget_calpll_t *pll = (sw_cmd_setget_calpll_t *)(&(pDesc->data));
    pll->freq_khz = pYucInfo->state_data.calpllFreqKhz;
    pll->mode = enable ? YUC_ON : YUC_OFF;
    pll->cal_band = 0;
    return iprvYucRficSetCalPll(xHandle, pDesc);
}
int32_t iprvSMBbLoopBack(RficHandle_t xHandle, bool enable)
{
    rf_sw_cmd_desc_t *pDesc = &(pYucInfo->pSMDesc);
    pDesc->cmd = YUC_FWCMD_SETBBLOOPBACK;
    sw_cmd_set_bbloopback_t *bb = (sw_cmd_set_bbloopback_t *)(pDesc->data);
    bb->rx = enable ? pYucInfo->state_data.bbRx : YUC_RX_NONE;
    return iprvYucRficSetBbLoopBack(xHandle, pDesc);
}
int32_t iprvSMSetPath(RficHandle_t xHandle, bool enable)
{
    rf_sw_cmd_desc_t *pDesc = &(pYucInfo->pSMDesc);
    pDesc->cmd = YUC_FWCMD_SETPATH;
    sw_cmd_setget_path_t *path = (sw_cmd_setget_path_t *)(&(pDesc->data));
    path->path = enable ? pYucInfo->state_data.path : YUC_PATH_NONE;
    path->band = pYucInfo->state_data.pathBand;
    path->rssi_mode = pYucInfo->state_data.pathRssi;
    path->dpd = enable ? pYucInfo->state_data.pathDpdMode : YUC_DPD_NONE;
    path->rxbw = pYucInfo->state_data.pathRxbw;
    path->txbw = pYucInfo->state_data.pathTxbw;
    return iprvYucRficSetPath(xHandle, pDesc);
}
int32_t iprvSMActive(RficHandle_t xHandle, bool enable)
{
    rf_sw_cmd_desc_t *pDesc = &(pYucInfo->pSMDesc);
    pDesc->cmd = YUC_FWCMD_SETACTIVE;
    sw_cmd_setactive_t *act = (sw_cmd_setactive_t *)(&(pDesc->data));
    act->dpd_rx = pYucInfo->state_data.actDpdRx;
    act->mode = enable ? pYucInfo->state_data.actMode : YUC_ACT_OFF;
    return iprvYucRficSetActive(xHandle, pDesc);
}
int32_t iprvSMRFLoop(RficHandle_t xHandle, bool enable)
{
    rf_sw_cmd_desc_t *pDesc = &(pYucInfo->pSMDesc);
    pDesc->cmd = YUC_FWCMD_SETRFLOOPBACK;
    sw_cmd_set_rfloopback_t *rf = (sw_cmd_set_rfloopback_t *)(&(pDesc->data));
    rf->rx = enable ? pYucInfo->state_data.rfRx : YUC_RX_NONE;
    rf->band = pYucInfo->state_data.rfBand;
    rf->loop_mode = pYucInfo->state_data.rfLoopmode;
    return iprvYucRficSetRfLoopBack(xHandle, pDesc);
}


void iprvGetTargetState(rf_sw_cmd_desc_t *pDesc, uint32_t *uTgtSt, bool *bMode)
{
    sw_cmd_setget_trxpll_t * pll = (sw_cmd_setget_trxpll_t *)(pDesc->data);
    sw_cmd_set_bbloopback_t * bb = (sw_cmd_set_bbloopback_t *)(pDesc->data);
    sw_cmd_setget_calpll_t * cpll = ( sw_cmd_setget_calpll_t * ) ( pDesc->data );
    sw_cmd_setget_path_t * path = ( sw_cmd_setget_path_t * ) ( pDesc->data );
    sw_cmd_setactive_t * act = ( sw_cmd_setactive_t * ) ( pDesc->data );
    sw_cmd_set_rfloopback_t * rf = ( sw_cmd_set_rfloopback_t * ) ( pDesc->data );
    switch(pDesc->cmd)
    {
        case YUC_FWCMD_SETTRXPLL:
            if (pll->mode == YUC_ON)
            {
                *bMode = pdTRUE;
                *uTgtSt = YUC_SS_STANDBY;
            }
            else 
            {
                *bMode = pdFALSE;
                *uTgtSt = YUC_SS_TRXPLLON;
            }
            break;
        case YUC_FWCMD_SETCALPLL:
            if (cpll->mode == YUC_ON)
            {
                *bMode = pdTRUE;
                *uTgtSt = YUC_SS_PREPARED;
            }
            else 
            {
                *bMode = pdFALSE;
                *uTgtSt = YUC_SS_CALPLLON;
            }
            break;
        case YUC_FWCMD_SETBBLOOPBACK:
            if (bb->rx != YUC_RX_NONE)
            {
                *bMode = pdTRUE;
                *uTgtSt = YUC_SS_STANDBY;
            }
            else 
            {
                *bMode = pdFALSE;
                *uTgtSt = YUC_SS_BBLOOP;
            }
            break;
        case YUC_FWCMD_SETACTIVE:
            if (act->mode != YUC_ACT_OFF)
            {
                *bMode = pdTRUE;
                *uTgtSt = YUC_SS_PREPARED;
            }
            else 
            {
                *bMode = pdFALSE;
                *uTgtSt = YUC_SS_ACTIVE;
            }
            break;
        case YUC_FWCMD_SETPATH:
            if (path->path != YUC_PATH_NONE)
            {
                *bMode = pdTRUE;
                *uTgtSt = YUC_SS_TRXPLLON;
            }
            else 
            {
                *bMode = pdFALSE;
                *uTgtSt = YUC_SS_PREPARED;
            }
            break;
        case YUC_FWCMD_SETRFLOOPBACK:
            if (rf->rx != YUC_RX_NONE)
            {
                *bMode = pdTRUE;
                *uTgtSt = YUC_SS_CALPLLON;
            }
            else 
            {
                *bMode = pdFALSE;
                *uTgtSt = YUC_SS_RFLOOP;
            }
            break;
        case RF_SWCMD_SETTRXPLL:
            *bMode = pdTRUE;
            *uTgtSt = YUC_SS_STANDBY;
            break;
        case RF_SWCMD_SETPATH:
            *bMode = pdTRUE;
            *uTgtSt = YUC_SS_TRXPLLON;
            break;
        case RF_SWCMD_SETDPD:
            *bMode = pdTRUE;
            *uTgtSt = YUC_SS_ACTIVE;
            break;
        default:
            RF_LOGERRMSG("Yucca command canbe executed from any state. Not expected to be called here.");
            break;
    }
}


int32_t iprvHandleSM(RficHandle_t xHandle, rf_sw_cmd_desc_t *pDesc)
{
    uint32_t curSt = 0;
    uint32_t uTgtSt = 0;
    uint32_t st = 0;
    bool bMode = pdFALSE;
    bool fallback = false;
    bool fallingBack = false;
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    RF_LOGDBGMSG("Entered SM");

    curSt = iprvYucRficGetSysState();
    
    iprvGetTargetState(pDesc, &uTgtSt, &bMode);
    if ( bMode == pdFALSE && uTgtSt != curSt )
    {
        RF_LOGDBGMSG("Disable a state from other state is unnecessary");
        return xRet;
    }
    
    RF_LOGDBG("State [%s] - [%s]",cYucStateToStr(curSt), cYucStateToStr(uTgtSt));

    if ( curSt == uTgtSt )
    {
        if (pDesc->flags & RF_CMD_FLAGS_FWCMD)
            return pYucFWCmdHandlers[pDesc->cmd].func(xHandle, pDesc);
        if (pDesc->flags & RF_CMD_FLAGS_SWCMD)
        {
            if ( pDesc->cmd <= RF_SWCMD_END )
            {
                return pYucSWCmdHandlers[pDesc->cmd].func(xHandle, pDesc);
            }
            else
            {
                RF_LOGERR("Yuc RF: Invalid SW CMD:[%d] for same Target state\n", pDesc->cmd);
                return RF_SW_CMD_RESULT_ERROR;
            }
        }
    }
    else
    {
        st = curSt;
        while(1)
        {
            RF_LOGDBG("SM Loop [%s] - [%s], fallback[%d] fallingback[%d]",cYucStateToStr(st), cYucStateToStr(uTgtSt), fallback, fallingBack);
            u32 sysSt = iprvYucRficGetSysState();
            if (st != sysSt)
            {
                RF_LOGERR("Yucca States out of sync st[%s] - sysstate[%s]", cYucStateToStr(st), cYucStateToStr(sysSt));
            }
            if (st == uTgtSt)
            {
                /* No need to run the given command if we are in falling back mode */
                if (fallingBack == true)
                {
                    break;
                }

                if (pDesc->flags & RF_CMD_FLAGS_FWCMD)
                    pYucFWCmdHandlers[pDesc->cmd].func(xHandle, pDesc);

                if (pDesc->flags & RF_CMD_FLAGS_SWCMD)
                {
                   if ( pDesc->cmd <= RF_SWCMD_END )
                   {
                        pYucSWCmdHandlers[pDesc->cmd].func(xHandle, pDesc);
                   }
                   else
                   {
                        RF_LOGERR(" Yuc RF: Invalid SW CMD: %d", pDesc->cmd);
                        return RF_SW_CMD_RESULT_ERROR;
                   }
                }

                st = iprvYucRficGetSysState();

                switch(st)
                {
                    case YUC_SS_BBLOOP:
                    case YUC_SS_RFLOOP:
                    case YUC_SS_ACTIVE:
                        fallback = false;
                        break;
                }

                /* We have already checked above for falling back so no need to check again */
                if ( fallback )
                {

                    if ( st != curSt )
                    {
                        uTgtSt = curSt;
                        fallback = false;
                        fallingBack = true;
                    }
                    else 
                    {
                        break;
                    }
                }
                else 
                {
                    break;
                }
            }
            RF_LOGDBG("State after tgt check [%s] - [%s]",cYucStateToStr(st), cYucStateToStr(uTgtSt));
            switch (st)
            {
                case YUC_SS_STANDBY:
                    switch (uTgtSt)
                    {
                        case YUC_SS_BBLOOP:
                            iprvSMBbLoopBack(xHandle, true);
                            st = uTgtSt;
                            break;
                        case YUC_SS_TRXPLLON:
                        case YUC_SS_PREPARED:
                        case YUC_SS_CALPLLON:
                        case YUC_SS_RFLOOP:
                        case YUC_SS_ACTIVE:
                            xRet = iprvSMTrxPll(xHandle, true);
                            st = YUC_SS_TRXPLLON;
                            break;
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                case YUC_SS_BBLOOP:
                    /**
                     * no chance of fallback path for bbloop
                     * going to stanby and executing bbloop enable command sends sys state to bbloop
                     */
                    fallback = false;
                    switch (uTgtSt)
                    {
                        case YUC_SS_STANDBY:
                        case YUC_SS_TRXPLLON:
                        case YUC_SS_PREPARED:
                        case YUC_SS_CALPLLON:
                        case YUC_SS_RFLOOP:
                        case YUC_SS_ACTIVE:
                            xRet = iprvSMBbLoopBack(xHandle, false);
                            st = YUC_SS_STANDBY;
                            break;
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                case YUC_SS_TRXPLLON:
                    fallback = false;
                    switch (uTgtSt)
                    {
                        case YUC_SS_STANDBY:
                            fallback = true;
                        case YUC_SS_BBLOOP:
                            xRet = iprvSMTrxPll(xHandle, false);
                            st = YUC_SS_STANDBY;
                            break;
                        case YUC_SS_PREPARED:
                        case YUC_SS_CALPLLON:
                        case YUC_SS_RFLOOP:
                        case YUC_SS_ACTIVE:
                            xRet = iprvSMSetPath(xHandle, true);
                            st = YUC_SS_PREPARED;
                            fallback = false;
                            break;
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                case YUC_SS_PREPARED:
                    fallback = false;
                    switch (uTgtSt)
                    {
                        case YUC_SS_STANDBY:
                        case YUC_SS_TRXPLLON:
                            fallback = true;
                        case YUC_SS_BBLOOP:
                            xRet = iprvSMSetPath(xHandle,false);
                            st = YUC_SS_TRXPLLON;
                            break;
                        case YUC_SS_CALPLLON:
                        case YUC_SS_RFLOOP:
                            xRet = iprvSMCalPll(xHandle, true);
                            st = YUC_SS_CALPLLON;
                            break;
                        case YUC_SS_ACTIVE:
                            xRet = iprvSMActive(xHandle, true);
                            st = YUC_SS_ACTIVE;
                            break;
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                case YUC_SS_CALPLLON:
                    fallback = false;
                    switch (uTgtSt)
                    {
                        case YUC_SS_STANDBY:
                        case YUC_SS_TRXPLLON:
                        case YUC_SS_PREPARED:
                            fallback = true;
                        case YUC_SS_BBLOOP:
                        case YUC_SS_ACTIVE:
                            xRet = iprvSMCalPll(xHandle, false);
                            st = YUC_SS_PREPARED;
                            break;
                        case YUC_SS_RFLOOP:
                            xRet = iprvSMRFLoop(xHandle, true);
                            st = YUC_SS_RFLOOP;
                            break;
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                case YUC_SS_RFLOOP:
                    fallback = true;
                    switch (uTgtSt)
                    {
                        case YUC_SS_BBLOOP:
                        case YUC_SS_ACTIVE:
                            fallback = false;
                        case YUC_SS_STANDBY:
                        case YUC_SS_TRXPLLON:
                        case YUC_SS_PREPARED:
                        case YUC_SS_CALPLLON:
                            xRet = iprvSMRFLoop(xHandle, false);
                            st = YUC_SS_CALPLLON;
                            break;                            
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                case YUC_SS_ACTIVE:
                    fallback = true;
                    switch (uTgtSt)
                    {
                        case YUC_SS_BBLOOP:
                        case YUC_SS_CALPLLON:
                        case YUC_SS_RFLOOP:
                            fallback = false;
                        case YUC_SS_STANDBY:
                        case YUC_SS_TRXPLLON:
                        case YUC_SS_PREPARED:
                            xRet = iprvSMActive(xHandle, false);
                            st = YUC_SS_PREPARED;
                            break;                            
                        default:
                            RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                            return RF_SW_CMD_RESULT_INVALID_SM_STATE;
                    }
                    break;
                default:
                    RF_LOGERR("Unexpected cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                    return pdFAIL;
            }
            if (xRet)
            {
                RF_LOGERR("SMFAIL cur:%s st:%s tgt:%s",cYucStateToStr(curSt), cYucStateToStr(st), cYucStateToStr(uTgtSt));
                return xRet;
            }
        }
    }
    return RF_SW_CMD_RESULT_OK;
}

int32_t iYucHandleCmd( RficHandle_t xHandle,
                       rf_sw_cmd_desc_t * pCmdDesc )
{
    if (pCmdDesc->flags & RF_CMD_FLAGS_FWCMD)
    {
        if( pCmdDesc->cmd <= YUC_FWCMD_END )
        {
            if( !pYucFWCmdHandlers[ pCmdDesc->cmd ].func )
            {
                RF_LOGERR( "Invalid FW command ID : %04x", pCmdDesc->cmd );
                return RF_SW_CMD_RESULT_CMD_INVALID;
            }
            else
            {
                if (pYucFWCmdHandlers[ pCmdDesc->cmd ].state == YUC_SM_ALL)
                {
                    return pYucFWCmdHandlers[ pCmdDesc->cmd ].func( xHandle, pCmdDesc );
                }
                else
                {
                    return iprvHandleSM(xHandle, pCmdDesc);
                }
            }
        }
    }
    else if (pCmdDesc->flags & RF_CMD_FLAGS_SWCMD)
    {
        if( pCmdDesc->cmd <= RF_SWCMD_END )
        {
            if( !pYucSWCmdHandlers[ pCmdDesc->cmd ].func )
            {
                RF_LOGERR( "Invalid SW command ID : %04x", pCmdDesc->cmd );
                return RF_SW_CMD_RESULT_CMD_INVALID;
            }
            else
            {
                if (pYucSWCmdHandlers[ pCmdDesc->cmd ].state == YUC_SM_ALL)
                {
                    return pYucSWCmdHandlers[ pCmdDesc->cmd ].func( xHandle, pCmdDesc );
                }
                else
                {
                    return iprvHandleSM(xHandle, pCmdDesc);
                }
            }
        }
    }
    RF_LOGERR( "Invalid command ID : %04x not found in both FWCMDs and SWCMDs", pCmdDesc->cmd );
    return RF_SW_CMD_RESULT_ERROR;
}

int32_t iprvYucRficGetVersion( __attribute__((unused))RficHandle_t xHandle,
                               rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETVERS_WDS ] = { 0 };
    sw_cmd_get_vers_t * xCmdData = ( sw_cmd_get_vers_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETVERSION;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get version command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETVERS_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get version response fail" );
        goto out;
    }

    YUC_FWCMD_GETVERS_HWVERS_W2P( xCmdData->hw_ver, uRficWds[ 1 ] );
    YUC_FWCMD_GETVERS_MINVERS_W2P( xCmdData->min_ver, uRficWds[ 2 ] );
    YUC_FWCMD_GETVERS_MAJVERS_W2P( xCmdData->maj_ver, uRficWds[ 2 ] );
    YUC_FWCMD_GETVERS_TGTVERS_W2P( xCmdData->tgt_ver, uRficWds[ 2 ] );
    YUC_FWCMD_GETVERS_BLDNR_W2P( xCmdData->bld_nr, uRficWds[ 3 ] );
    YUC_FWCMD_GETVERS_TSTNR_W2P( xCmdData->test_nr, uRficWds[ 3 ] );
    YUC_FWCMD_GETVERS_DBGFLG_W2P( xCmdData->dbg_flg, uRficWds[ 3 ] );

out:
    return xRet;
}

int32_t iprvYucRficSetRegister( __attribute__((unused))RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_SET_REGISTER_WDS ] = { 0 };
    sw_cmd_setget_register_t * xCmdData = ( sw_cmd_setget_register_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETREGISTER;
    YUC_FWCMD_REGISTER_ADDR_P2W( xCmdData->addr, uRficWds[ 0 ] );
    YUC_FWCMD_REGISTER_VALUE1_P2W( xCmdData->value1, uRficWds[ 1 ] );
    YUC_FWCMD_REGISTER_VALUE2_P2W( xCmdData->value2, uRficWds[ 2 ] );
    YUC_FWCMD_REGISTER_VALUE3_P2W( xCmdData->value3, uRficWds[ 3 ] );
    YUC_FWCMD_REGISTER_VALUE4_P2W( xCmdData->value4, uRficWds[ 4 ] );
    YUC_FWCMD_REGISTER_VALUE5_P2W( xCmdData->value5, uRficWds[ 5 ] );
    YUC_FWCMD_REGISTER_VALUE6_P2W( xCmdData->value6, uRficWds[ 6 ] );
    YUC_FWCMD_REGISTER_VALUE7_P2W( xCmdData->value7, uRficWds[ 7 ] );
    YUC_FWCMD_PROPERTY_VALUE8_P2W( xCmdData->value8, uRficWds[ 8 ] );

    RF_LOGDBG( "Set Register addr  0x%x", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SET_REGISTER_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Register command fail" );
    }

    return xRet;
}

int32_t iprvYucRficGetRegister( __attribute__((unused))RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GET_REGISTER_WDS ] = { 0 };
    sw_cmd_setget_register_t * xCmdData = ( sw_cmd_setget_register_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETREGISTER;
    YUC_FWCMD_REGISTER_ADDR_P2W( xCmdData->addr, uRficWds[ 0 ] );
    YUC_FWCMD_REGISTER_LENGTH_P2W( xCmdData->length, uRficWds[ 1 ] );

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_GET_REGISTER_IN_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Register command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                           &uRficWds[ 0 ], YUC_FWCMD_GET_REGISTER_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Register response fail" );
        goto out;
    }

    YUC_FWCMD_REGISTER_RTC_W2P( xCmdData->rtc, uRficWds[ 0 ] );
    YUC_FWCMD_REGISTER_VALUE1_W2P( xCmdData->value1, uRficWds[ 1 ] );
    YUC_FWCMD_REGISTER_VALUE2_W2P( xCmdData->value2, uRficWds[ 2 ] );
    YUC_FWCMD_REGISTER_VALUE3_W2P( xCmdData->value3, uRficWds[ 3 ] );
    YUC_FWCMD_REGISTER_VALUE4_W2P( xCmdData->value4, uRficWds[ 4 ] );
    YUC_FWCMD_REGISTER_VALUE5_W2P( xCmdData->value5, uRficWds[ 5 ] );
    YUC_FWCMD_REGISTER_VALUE6_W2P( xCmdData->value6, uRficWds[ 6 ] );
    YUC_FWCMD_REGISTER_VALUE7_W2P( xCmdData->value7, uRficWds[ 7 ] );
    YUC_FWCMD_REGISTER_VALUE8_W2P( xCmdData->value8, uRficWds[ 8 ] );

out:
    return xRet;
}

int32_t iprvYucRficSetRegisterMasked( __attribute__((unused))RficHandle_t xHandle,
                                      rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_SET_REGISTER_MASK_WDS ] = { 0 };
    sw_cmd_setget_register_t * xCmdData = ( sw_cmd_setget_register_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETREGISTERMASKED;
    YUC_FWCMD_REGISTER_ADDR_P2W( xCmdData->addr, uRficWds[ 0 ] );
    YUC_FWCMD_REGISTER_VALUE1_P2W( xCmdData->value1, uRficWds[ 1 ] );
    YUC_FWCMD_REGISTER_MASK_P2W( xCmdData->mask, uRficWds[ 2 ] );

    RF_LOGDBG( "Set Register Masked addr  0x%x", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SET_REGISTER_MASK_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Register Masked command fail" );
    }

    return xRet;
}

#if YUC_ALL_FWCMDS
int32_t iprvYucRficSetProperty( __attribute__((unused))RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_SET_PROPERTY_WDS ] = { 0 };
    sw_cmd_setget_property_t * xCmdData = ( sw_cmd_setget_property_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETPROPERTY;
    YUC_FWCMD_PROPERTY_ID_P2W( xCmdData->property_id, uRficWds[ 0 ] );
    YUC_FWCMD_PROPERTY_VALUE1_P2W( xCmdData->value1, uRficWds[ 1 ] );
    YUC_FWCMD_PROPERTY_VALUE2_P2W( xCmdData->value2, uRficWds[ 2 ] );
    YUC_FWCMD_PROPERTY_VALUE3_P2W( xCmdData->value3, uRficWds[ 3 ] );
    YUC_FWCMD_PROPERTY_VALUE4_P2W( xCmdData->value4, uRficWds[ 4 ] );
    YUC_FWCMD_PROPERTY_VALUE5_P2W( xCmdData->value5, uRficWds[ 5 ] );
    YUC_FWCMD_PROPERTY_VALUE6_P2W( xCmdData->value6, uRficWds[ 6 ] );
    YUC_FWCMD_PROPERTY_VALUE7_P2W( xCmdData->value7, uRficWds[ 7 ] );
    YUC_FWCMD_PROPERTY_VALUE8_P2W( xCmdData->value8, uRficWds[ 8 ] );

    RF_LOGDBG( "Set Propertyid  %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SET_PROPERTY_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Property command fail" );
    }

    return xRet;
}

int32_t iprvYucRficGetProperty( __attribute__((unused))RficHandle_t xHandle,
                                rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GET_PROPERTY_WDS ] = { 0 };
    sw_cmd_setget_property_t * xCmdData = ( sw_cmd_setget_property_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETPROPERTY;
    YUC_FWCMD_PROPERTY_ID_P2W( xCmdData->property_id, uRficWds[ 0 ] );
    YUC_FWCMD_PROPERTY_VALUE1_P2W( xCmdData->length, uRficWds[ 1 ] );

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            2 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Property command fail" );
        goto out;
    }
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                           &uRficWds[ 0 ], YUC_FWCMD_GET_PROPERTY_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Property response fail" );
        goto out;
    }

    YUC_FWCMD_PROPERTY_RTC_W2P( xCmdData->rtc, uRficWds[ 0 ] );
    YUC_FWCMD_PROPERTY_VALUE1_W2P( xCmdData->value1, uRficWds[ 1 ] );
    YUC_FWCMD_PROPERTY_VALUE2_W2P( xCmdData->value2, uRficWds[ 2 ] );
    YUC_FWCMD_PROPERTY_VALUE3_W2P( xCmdData->value3, uRficWds[ 3 ] );
    YUC_FWCMD_PROPERTY_VALUE4_W2P( xCmdData->value4, uRficWds[ 4 ] );
    YUC_FWCMD_PROPERTY_VALUE5_W2P( xCmdData->value5, uRficWds[ 5 ] );
    YUC_FWCMD_PROPERTY_VALUE6_W2P( xCmdData->value6, uRficWds[ 6 ] );
    YUC_FWCMD_PROPERTY_VALUE7_W2P( xCmdData->value7, uRficWds[ 7 ] );
    YUC_FWCMD_PROPERTY_VALUE8_W2P( xCmdData->value8, uRficWds[ 8 ] );

out:
    return xRet;
}
#endif

int32_t iprvYucRficGetSysStatus( __attribute__((unused))RficHandle_t xHandle,
                                 rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETSYSSTATUS_WDS ] = { 0 };
    sw_cmd_get_sys_status_t * xCmdData = ( sw_cmd_get_sys_status_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETSYSSTATUS;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Sys Status command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                           &uRficWds[ 0 ], YUC_FWCMD_GETSYSSTATUS_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Sys Status response fail" );
        goto out;
    }

    YUC_FWCMD_GETSYSSTATUS_RTC_W2P( xCmdData->rtc, uRficWds[ 0 ] );
    YUC_FWCMD_GETSYSSTATUS_SYSSTATE_W2P( xCmdData->sys_state, uRficWds[ 1 ] );
    YUC_FWCMD_GETSYSSTATUS_TRXPLL_VCO_LO_W2P( xCmdData->trxpll_vco_det_lo, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_TRXPLL_VCO_HI_W2P( xCmdData->trxpll_vco_det_hi, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_TRXPLL_VTUNE_LO_W2P( xCmdData->trxpll_vtune_det_lo, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_TRXPLL_VTUNE_HI_W2P( xCmdData->trxpll_vtune_det_hi, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_TRXPLL_UNLOCK_W2P( xCmdData->trxpll_unlock, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_CALPLL_VTUNE_LO_W2P( xCmdData->calpll_vtune_det_lo, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_CALPLL_VTUNE_HI_W2P( xCmdData->calpll_vtune_det_hi, uRficWds[ 2 ] );
    YUC_FWCMD_GETSYSSTATUS_CALPLL_UNLOCK_W2P( xCmdData->calpll_unlock, uRficWds[ 2 ] );

out:
    return xRet;
}

#if YUC_ALL_FWCMDS
int32_t iprvYucRficMeasAuxAdc( __attribute__((unused))RficHandle_t xHandle,
                               __attribute__((unused))rf_sw_cmd_desc_t * pCmdDesc )
{
    return RF_SW_CMD_RESULT_NOT_IMPLEMENTED;
}

int32_t iprvYucRficSetMeasPa( __attribute__((unused))RficHandle_t xHandle,
                              __attribute__((unused))rf_sw_cmd_desc_t * pCmdDesc )
{
    return RF_SW_CMD_RESULT_NOT_IMPLEMENTED;
}

int32_t iprvYucRficGetMeasPa( __attribute__((unused))RficHandle_t xHandle,
                              __attribute__((unused))rf_sw_cmd_desc_t * pCmdDesc )
{
    return RF_SW_CMD_RESULT_NOT_IMPLEMENTED;
}
#endif
int32_t iprvYucRficGetTemperature( __attribute__((unused))RficHandle_t xHandle,
                                   rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GET_TEMPERATURE_WDS ] = { 0 };
    sw_cmd_get_temperature_t * xCmdData = ( sw_cmd_get_temperature_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETTEMPERATURE;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Temperature command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                           &uRficWds[ 0 ], YUC_FWCMD_GET_TEMPERATURE_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Temperature response fail" );
        goto out;
    }

    YUC_FWCMD_TEMPERATURE_RTC_W2P( xCmdData->rtc, uRficWds[ 0 ] );
    YUC_FWCMD_TEMPERATURE_W2P( xCmdData->temperature, uRficWds[ 1 ] );
    YUC_FWCMD_TEMPERATURE_VALUERAW_W2P( xCmdData->value_raw, uRficWds[ 2 ] );

out:
    return xRet;
}

#if YUC_ALL_FWCMDS
int32_t iprvYucRficCalResistor( __attribute__((unused))RficHandle_t xHandle,
                                __attribute__((unused))rf_sw_cmd_desc_t * pCmdDesc )
{
    return RF_SW_CMD_RESULT_NOT_IMPLEMENTED;
}

int32_t iprvYucRficCalRegulator( __attribute__((unused))RficHandle_t xHandle,
                                 __attribute__((unused))rf_sw_cmd_desc_t * pCmdDesc )
{
    return RF_SW_CMD_RESULT_NOT_IMPLEMENTED;
}
#endif

int32_t iprvYucRficSetBw( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc,
                            u32 ulCmdData)
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u16 uRficWds[ YUC_FWCMD_GET_TXRXBW_WDS ] = { 0 };
    sw_cmd_setget_txrxbw_t * xCmdData = ( sw_cmd_setget_txrxbw_t * ) ( pCmdDesc->data );

    if ( ulCmdData == YUC_FWCMD_SETTXBW )
        YUC_FWCMD_TXBW_BW_P2W( xCmdData->bw, uRficWds[ 0 ] );
    if ( ulCmdData == YUC_FWCMD_SETRXBW )
        YUC_FWCMD_RXBW_BW_P2W( xCmdData->bw, uRficWds[ 0 ] );
    YUC_FWCMD_TXRXBW_BWFTUNE_P2W( xCmdData->bw_ftune, uRficWds[ 1 ] );

    RF_LOGDBG( "Set Bw %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SET_TXRXBW_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Bw command fail" );
    }
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GET_TXRXBW_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Bw response fail" );
        goto out;
    }

    if ( ulCmdData == YUC_FWCMD_SETTXBW )
        YUC_FWCMD_TXBW_BW_W2P( xCmdData->bw, uRficWds[ 1 ] );
    if ( ulCmdData == YUC_FWCMD_SETTXBW )
        YUC_FWCMD_RXBW_BW_W2P( xCmdData->bw, uRficWds[ 1 ] );
    YUC_FWCMD_TXRXBW_BWFTUNE_W2P( xCmdData->bw_ftune, uRficWds[ 2 ] );

out:
    return xRet;
}

int32_t iprvYucRficSetRxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    return iprvYucRficSetBw(xHandle, pCmdDesc, YUC_FWCMD_SETRXBW);
}

int32_t iprvYucRficSetTxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    return iprvYucRficSetBw(xHandle, pCmdDesc, YUC_FWCMD_SETTXBW);
}

int32_t iprvYucRficGetBw( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc,
                            u32 ulCmdData)
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u16 uRficWds[ YUC_FWCMD_GET_TXRXBW_WDS ] = { 0 };
    sw_cmd_setget_txrxbw_t * xCmdDataResp = ( sw_cmd_setget_txrxbw_t * ) ( pCmdDesc->data );

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get BW command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GET_TXRXBW_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Bw response fail" );
        goto out;
    }

    if ( ulCmdData == YUC_FWCMD_GETTXBW )
        YUC_FWCMD_TXBW_BW_W2P( xCmdDataResp->bw, uRficWds[ 1 ] );
    if ( ulCmdData == YUC_FWCMD_GETRXBW )
        YUC_FWCMD_RXBW_BW_W2P( xCmdDataResp->bw, uRficWds[ 1 ] );
    YUC_FWCMD_TXRXBW_BWFTUNE_W2P( xCmdDataResp->bw_ftune, uRficWds[ 2 ] );

out:
    return xRet;
}

int32_t iprvYucRficGetRxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    return iprvYucRficGetBw(xHandle, pCmdDesc, YUC_FWCMD_GETRXBW);
}


int32_t iprvYucRficGetTxBw( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    return iprvYucRficGetBw(xHandle, pCmdDesc, YUC_FWCMD_GETTXBW);
}

int32_t iprvYucRficSetTrxPll( __attribute__((unused))RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData, uFreq_msw, uFreq_lsw;
    u16 uRficWds[ YUC_FWCMD_GETTRXPLL_WDS ] = { 0 };
    sw_cmd_setget_trxpll_t * xCmdData = ( sw_cmd_setget_trxpll_t * ) ( pCmdDesc->data );
    ulCmdData = YUC_FWCMD_SETTRXPLL;
    u32 uCmdLen = YUC_FWCMD_TRXPLL_WDS;
    u32 freq_khz = xCmdData->freq_khz * 2;
    uFreq_msw = ((freq_khz & 0xff0000) >> 16);
    uFreq_lsw = (freq_khz & 0xffff);
    if (pCmdDesc->cmd == RF_SWCMD_SETTRXPLL)
    {
        YUC_FWCMD_TRXPLL_CAL_CAP_P2W( xCmdData->cal_cap, uRficWds[ 2 ] );
        YUC_FWCMD_TRXPLL_CAL_CURRENT_P2W( xCmdData->cal_current, uRficWds[ 2 ] );
    }
    else
    {
        uCmdLen -= 1;
    }
    YUC_FWCMD_TRXPLL_MODE_P2W( xCmdData->mode, uRficWds[ 0 ] );
    YUC_FWCMD_TRXPLL_VCO_SEL_P2W( xCmdData->vco_sel, uRficWds[ 0 ] );
    YUC_FWCMD_TRXPLL_FREQ_MSW_P2W( uFreq_msw, uRficWds[ 0 ] );
    YUC_FWCMD_TRXPLL_FREQ_LSW_P2W( uFreq_lsw, uRficWds[ 1 ] );

    RF_LOGDBG( "Set TrxPll %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            uCmdLen);

    if( xRet )
    {
        RF_LOGERRMSG( "Set TrxPll command fail" );
        goto out;
    }
    else if ( xCmdData->mode != YUC_OFF )
    {
        pYucInfo->state_data.trxpllFreqKhz = xCmdData->freq_khz;
    }

    sw_cmd_setget_trxpll_t * xCmdDataResp = ( sw_cmd_setget_trxpll_t * ) ( pCmdDesc->data );
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETTRXPLL_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get TrxPll response fail" );
        goto out;
    }

    YUC_FWCMD_TRXPLL_MODE_W2P( xCmdDataResp->mode, uRficWds[ 1 ] );
    YUC_FWCMD_TRXPLL_VCO_SEL_W2P( xCmdDataResp->vco_sel, uRficWds[ 1 ] );
    YUC_FWCMD_TRXPLL_FREQ_MSW_W2P( uFreq_msw, uRficWds[ 1 ] );
    YUC_FWCMD_TRXPLL_FREQ_LSW_W2P( uFreq_lsw, uRficWds[ 2 ] );
    YUC_FWCMD_TRXPLL_CAL_CAP_W2P( xCmdDataResp->cal_cap, uRficWds[ 3 ] );
    YUC_FWCMD_TRXPLL_CAL_CURRENT_W2P( xCmdDataResp->cal_current, uRficWds[ 3 ] );
    xCmdDataResp->freq_khz = ( uFreq_msw << 16 );
    xCmdDataResp->freq_khz |= ( uFreq_lsw & 0xffff );
    xCmdDataResp->freq_khz = ( xCmdDataResp->freq_khz + 1 );
    RF_LOGDBG( "pll frequency: %d", xCmdDataResp->freq_khz );
    xCmdDataResp->freq_khz = ( xCmdDataResp->freq_khz >> 1 );
out:
    return xRet;
}

int32_t iprvYucRficGetTrxPll( __attribute__((unused))RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData, uFreq_msw, uFreq_lsw;
    u16 uRficWds[ YUC_FWCMD_GETTRXPLL_WDS ] = { 0 };
    sw_cmd_setget_trxpll_t * xCmdDataResp = ( sw_cmd_setget_trxpll_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETTRXPLL;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get TrxPll command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETTRXPLL_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get TrxPll response fail" );
        goto out;
    }

    YUC_FWCMD_TRXPLL_MODE_W2P( xCmdDataResp->mode, uRficWds[ 1 ] );
    YUC_FWCMD_TRXPLL_VCO_SEL_W2P( xCmdDataResp->vco_sel, uRficWds[ 1 ] );
    YUC_FWCMD_TRXPLL_FREQ_MSW_W2P( uFreq_msw, uRficWds[ 1 ] );
    YUC_FWCMD_TRXPLL_FREQ_LSW_W2P( uFreq_lsw, uRficWds[ 2 ] );
    YUC_FWCMD_TRXPLL_CAL_CAP_W2P( xCmdDataResp->cal_cap, uRficWds[ 3 ] );
    YUC_FWCMD_TRXPLL_CAL_CURRENT_W2P( xCmdDataResp->cal_current, uRficWds[ 3 ] );
    xCmdDataResp->freq_khz = ( uFreq_msw << 16 );
    xCmdDataResp->freq_khz |= ( uFreq_lsw & 0xffff );
    xCmdDataResp->freq_khz = ( xCmdDataResp->freq_khz >> 1 );

out:
    return xRet;
}

int32_t iprvYucRficSetCalPll( __attribute__((unused))RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_CALPLL_WDS ] = { 0 };
    sw_cmd_setget_calpll_t * xCmdData = ( sw_cmd_setget_calpll_t * ) ( pCmdDesc->data );
    u32 uFreq_msw, uFreq_lsw;
    u32 freq_khz = xCmdData->freq_khz * 2;
    uFreq_msw = ((freq_khz & 0xff0000) >> 16);
    uFreq_lsw = (freq_khz & 0xffff);

    ulCmdData = YUC_FWCMD_SETCALPLL;
    YUC_FWCMD_CALPLL_MODE_P2W( xCmdData->mode, uRficWds[ 0 ] );
    YUC_FWCMD_CALPLL_FREQ_MSW_P2W( uFreq_msw, uRficWds[ 0 ] );
    YUC_FWCMD_CALPLL_FREQ_LSW_P2W( uFreq_lsw, uRficWds[ 1 ] );
    YUC_FWCMD_CALPLL_CAL_BAND_P2W( xCmdData->cal_band, uRficWds[ 2 ] );

    RF_LOGDBG( "Set Calpll %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_CALPLL_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set CalPll command fail" );
    }
    else if ( xCmdData->mode != YUC_OFF )
    {
        pYucInfo->state_data.calpllFreqKhz = xCmdData->freq_khz;
    }

    return xRet;
}

int32_t iprvYucRficGetCalPll( __attribute__((unused))RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETCALPLL_WDS ] = { 0 };
    sw_cmd_setget_calpll_t * xCmdData = ( sw_cmd_setget_calpll_t * ) ( pCmdDesc->data );
    u32 freq_msw, freq_lsw;

    ulCmdData = YUC_FWCMD_GETCALPLL;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get CalPll command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                           &uRficWds[ 0 ], YUC_FWCMD_GETCALPLL_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get CalPll response fail" );
        goto out;
    }

    YUC_FWCMD_CALPLL_MODE_W2P( xCmdData->mode, uRficWds[ 1 ] );
    YUC_FWCMD_CALPLL_FREQ_MSW_W2P( freq_msw, uRficWds[ 1 ] );
    YUC_FWCMD_CALPLL_FREQ_LSW_W2P( freq_lsw, uRficWds[ 2 ] );
    YUC_FWCMD_CALPLL_CAL_BAND_W2P( xCmdData->cal_band, uRficWds[ 3 ] );
    xCmdData->freq_khz = ( freq_msw << 16 );
    xCmdData->freq_khz |= ( freq_lsw & 0xffff );
    xCmdData->freq_khz = ( xCmdData->freq_khz >> 1 );

out:
    return xRet;
}

/*In sysfs entries, we have to pass parameters as per enum structure */
int32_t iprvYucRficSetPath( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETPATH_WDS ] = { 0 };
    u32 uCmdLen = YUC_FWCMD_PATH_WDS;
    sw_cmd_setget_path_t * xCmdData = ( sw_cmd_setget_path_t * ) ( pCmdDesc->data );
    YUC_FWCMD_PATH_PATH_P2W( xCmdData->path, uRficWds[ 0 ] );
    ulCmdData = YUC_FWCMD_SETPATH;
    YUC_FWCMD_PATH_BAND_P2W(xCmdData->band, uRficWds[1]);
    YUC_FWCMD_PATH_RSSI_P2W(xCmdData->rssi_mode, uRficWds[1]);
    YUC_FWCMD_PATH_DPD_P2W(xCmdData->dpd, uRficWds[1]);
    YUC_FWCMD_PATH_RXBW_P2W(xCmdData->rxbw, uRficWds[1]);
    YUC_FWCMD_PATH_TXBW_P2W(xCmdData->txbw, uRficWds[1]);
    RF_LOGDBG( "Set path %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            uCmdLen );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Path command fail" );
    }
    else if (xCmdData->path != YUC_PATH_NONE)
    {
        pYucInfo->state_data.path = xCmdData->path;
        pYucInfo->state_data.pathBand = xCmdData->band;
        pYucInfo->state_data.pathRssi = xCmdData->rssi_mode;
        pYucInfo->state_data.pathDpdMode = xCmdData->dpd;
        pYucInfo->state_data.pathRxbw = xCmdData->rxbw;
        pYucInfo->state_data.pathTxbw = xCmdData->txbw;
    }
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETPATH_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Path response fail" );
        goto out;
    }

    YUC_FWCMD_PATH_PATH_W2P( xCmdData->path, uRficWds[ 1 ] );
    YUC_FWCMD_PATH_BAND_W2P( xCmdData->band, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_RSSI_W2P( xCmdData->rssi_mode, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_DPD_W2P( xCmdData->dpd, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_RXBW_W2P( xCmdData->rxbw, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_TXBW_W2P( xCmdData->txbw, uRficWds[ 2 ] );

out:

    return xRet;
}

int32_t iprvYucRficGetPath( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETPATH_WDS ] = { 0 };

    sw_cmd_setget_path_t * xCmdData = ( sw_cmd_setget_path_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETPATH;

    RF_LOGDBG( "Get path %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Path command fail" );
        goto out;
    }
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETPATH_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Path response fail" );
        goto out;
    }

    YUC_FWCMD_PATH_PATH_W2P( xCmdData->path, uRficWds[ 1 ] );
    YUC_FWCMD_PATH_BAND_W2P( xCmdData->band, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_RSSI_W2P( xCmdData->rssi_mode, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_DPD_W2P( xCmdData->dpd, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_RXBW_W2P( xCmdData->rxbw, uRficWds[ 2 ] );
    YUC_FWCMD_PATH_TXBW_W2P( xCmdData->txbw, uRficWds[ 2 ] );

out:
    return xRet;
}

static void iprvYucReadGainResp(RficHandle_t xHandle, u16 * uRficWds, sw_cmd_get_gain_t * xCmdDataResp)
{
    YUC_FWCMD_GAIN_MANUAL_W2P( xCmdDataResp->rx1_manual, uRficWds[ 1 ] );
    YUC_FWCMD_GETGAIN_HASHMODE_W2P( xCmdDataResp->rx1_hash_mode, uRficWds[ 1 ] );
    YUC_FWCMD_GETGAIN_CHANNEL_W2P( xCmdDataResp->channel, uRficWds[ 1 ] );

    xCmdDataResp->rx1_bbgain = 0;
    xCmdDataResp->rx1_rfgain = 0;
    xCmdDataResp->rx2_bbgain = 0;
    xCmdDataResp->rx2_rfgain = 0;
    xCmdDataResp->rx1_gain = 0;
    xCmdDataResp->rx2_gain = 0;

    if( xCmdDataResp->rx1_hash_mode == 0 )
    {
        YUC_FWCMD_GAIN_BBGAIN_W2P( xCmdDataResp->rx1_bbgain, uRficWds[ 1 ] );
        YUC_FWCMD_GAIN_RFGAIN_W2P( xCmdDataResp->rx1_rfgain, uRficWds[ 1 ] );
    }
    else
    {
        YUC_FWCMD_GAIN_RXGAIN_W2P( xCmdDataResp->rx1_gain, uRficWds[ 1 ] );
    }

    YUC_FWCMD_GAIN_MANUAL_W2P( xCmdDataResp->rx2_manual, uRficWds[ 2 ] );
    YUC_FWCMD_GETGAIN_HASHMODE_W2P( xCmdDataResp->rx2_hash_mode, uRficWds[ 2 ] );

    if( xCmdDataResp->rx2_hash_mode == 0 )
    {
        YUC_FWCMD_GAIN_BBGAIN_W2P( xCmdDataResp->rx2_bbgain, uRficWds[ 2 ] );
        YUC_FWCMD_GAIN_RFGAIN_W2P( xCmdDataResp->rx2_rfgain, uRficWds[ 2 ] );
    }
    else
    {
        YUC_FWCMD_GAIN_RXGAIN_W2P( xCmdDataResp->rx2_gain, uRficWds[ 2 ] );
    }

    YUC_FWCMD_GETGAIN_TXGAIN_W2P( xCmdDataResp->tx1_gain, uRficWds[ 3 ] );
    YUC_FWCMD_GAIN_TXGAIN_W2P( xCmdDataResp->tx2_gain, uRficWds[ 3 ] );
    YUC_FWCMD_GETGAIN_GAINRAW_W2P( xCmdDataResp->rx1_gain_raw, uRficWds[ 4 ] );
    YUC_FWCMD_GETGAIN_GAINRAW_W2P( xCmdDataResp->rx2_gain_raw, uRficWds[ 5 ] );
    YUC_FWCMD_GETGAIN_GAINRAW_W2P( xCmdDataResp->tx1_gain_raw, uRficWds[ 6 ] );
    YUC_FWCMD_GETGAIN_GAINRAW_W2P( xCmdDataResp->tx2_gain_raw, uRficWds[ 7 ] );

    if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
    {
        xCmdDataResp->tx_gain_idx = pYucInfo->yucData[FR1_IDX_YC1 - 1]->txGainIdx;
        xCmdDataResp->rx_gain_idx = pYucInfo->yucData[FR1_IDX_YC1 -1]->rxGainIdx;
    }
    else
    {
        xCmdDataResp->tx_gain_idx = pYucInfo->yucData[FR1_IDX_YC2 - 1]->txGainIdx;
        xCmdDataResp->rx_gain_idx = pYucInfo->yucData[FR1_IDX_YC2 -1]->rxGainIdx;
    }
}

int32_t iprvYucRficSetGain( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETGAIN_WDS ] = { 0 };
    u16 uPath = 0;
    sw_cmd_set_gain_t * xCmdData = ( sw_cmd_set_gain_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETGAIN;
    YUC_FWCMD_GAIN_MANUAL_P2W( xCmdData->manual, uRficWds[ 0 ] );
    YUC_FWCMD_GAIN_CHANNEL_P2W( xCmdData->channel, uRficWds[ 0 ] );
    YUC_FWCMD_GAIN_HASHMODE_P2W( xCmdData->hash_mode, uRficWds[ 0 ] );

    if( ( uPath = ( xCmdData->path & ( YUC_PATH_TX1 | YUC_PATH_TX2 ) ) ) )
    {
        YUC_FWCMD_GAIN_PATH_P2W( uPath, uRficWds[ 0 ] );
        YUC_FWCMD_GAIN_TXGAIN_P2W( xCmdData->tx_gain, uRficWds[ 0 ] );
    }

    if( ( uPath = ( xCmdData->path & ( YUC_PATH_RX1 | YUC_PATH_RX2 ) ) ) )
    {
        YUC_FWCMD_GAIN_PATH_P2W( uPath, uRficWds[ 0 ] );
        if (xCmdData->hash_mode)
        {
            YUC_FWCMD_GAIN_RXGAIN_P2W( xCmdData->rx_gain, uRficWds[ 0 ] );
        }
        else
        {
            YUC_FWCMD_GAIN_BBGAIN_P2W( xCmdData->bb_gain, uRficWds[ 0 ] );
            YUC_FWCMD_GAIN_RFGAIN_P2W( xCmdData->rf_gain, uRficWds[ 0 ] );
        }
    }

    RF_LOGDBG( "Set Gain %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_GAIN_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Gain command fail" );
    }
    sw_cmd_get_gain_t * xCmdDataResp = ( sw_cmd_get_gain_t * ) ( pCmdDesc->data );
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETGAIN_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Gain response fail" );
        goto out;
    }
    iprvYucReadGainResp(xHandle, uRficWds, xCmdDataResp);

out:
    return xRet;
}

int32_t iprvYucRficGetGain( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETGAIN_WDS ] = { 0 };
    sw_cmd_get_gain_t * xCmdDataResp = ( sw_cmd_get_gain_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETGAIN;

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, NULL,
                            0 );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Gain command fail" );
        goto out;
    }
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETGAIN_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Gain response fail" );
        goto out;
    }

    /* read command response */
    iprvYucReadGainResp(xHandle, uRficWds, xCmdDataResp);
out:
    return xRet;
}

int32_t iprvYucRficSetActive( __attribute__((unused))RficHandle_t xHandle,
                              rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_SETACTIVE_WDS_OUT ] = { 0 };
    sw_cmd_setactive_t * xCmdData = ( sw_cmd_setactive_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETACTIVE;
    YUC_FWCMD_SETACTIVE_MODE_P2W( xCmdData->mode, uRficWds[ 0 ] );
    YUC_FWCMD_SETACTIVE_DPD_RX_P2W( xCmdData->dpd_rx, uRficWds[ 0 ] );

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SETACTIVE_WDS_IN );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Active command fail" );
        goto out;
    }
    else if ( xCmdData->mode != YUC_ACT_OFF )
    {
        pYucInfo->state_data.actDpdRx = xCmdData->dpd_rx;
        pYucInfo->state_data.actMode = xCmdData->mode;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_SETACTIVE_WDS_OUT );

    if( xRet )
    {
        RF_LOGERRMSG( "Set Active response fail" );
        goto out;
    }

    YUC_FWCMD_SETACTIVE_RTC_W2P( xCmdData->rtc, uRficWds[ 0 ] );
    YUC_FWCMD_SETACTIVE_MODE_W2P( xCmdData->mode, uRficWds[ 1 ] );
    YUC_FWCMD_SETACTIVE_DPD_RX_W2P( xCmdData->dpd_rx, uRficWds[ 1 ] );

out:
    return xRet;
}

int32_t iprvYucRficSetRfLoopBack( __attribute__((unused))RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_RFLOOPBACK_WDS ] = { 0 };
    sw_cmd_set_rfloopback_t * xCmdData = ( sw_cmd_set_rfloopback_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETRFLOOPBACK;
    YUC_FWCMD_SETRFLOOPBACK_RECEIVER_P2W( xCmdData->rx, uRficWds[ 0 ] );
    YUC_FWCMD_SETRFLOOPBACK_BAND_P2W( xCmdData->band, uRficWds[ 0 ] );
    YUC_FWCMD_SETRFLOOPBACK_LOOPMODE_P2W( xCmdData->loop_mode, uRficWds[ 0 ] );

    RF_LOGDBG( "SetRFLoopBack %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_RFLOOPBACK_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "SetRfLoopBack command fail" );
    }
    else if ( xCmdData->rx != YUC_RX_NONE )
    {
        pYucInfo->state_data.rfRx = xCmdData->rx;
        pYucInfo->state_data.rfBand = xCmdData->band;
        pYucInfo->state_data.rfLoopmode = xCmdData->loop_mode;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_RFLOOPBACK_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get RF Loopback response fail" );
        goto out;
    }

    YUC_FWCMD_SETRFLOOPBACK_RTC_W2P( xCmdData->rtc, uRficWds[ 0 ] );

out:
    return xRet;
}

int32_t iprvYucRficSetBbLoopBack( __attribute__((unused))RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_BBLOOPBACK_WDS ] = { 0 };
    sw_cmd_set_bbloopback_t * xCmdData = ( sw_cmd_set_bbloopback_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETBBLOOPBACK;
    YUC_FWCMD_SETBBLOOPBACK_RECEIVER_P2W( xCmdData->rx, uRficWds[ 0 ] );

    RF_LOGDBG( "SetBbLoopBack %d", uRficWds[ 0 ] );
    YUC_FWCMD_SETBBLOOPBACK_TXBW_P2W( xCmdData->txbw, uRficWds[ 0 ] );
    YUC_FWCMD_SETBBLOOPBACK_RXBW_P2W( xCmdData->rxbw, uRficWds[ 0 ] );

    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_BBLOOPBACK_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "SetBbLoopBack command fail" );
    }
    else if ( xCmdData->rx != YUC_RX_NONE )
    {
        pYucInfo->state_data.bbRx = xCmdData->rx;
    }

    return xRet;
}

#if YUC_ALL_FWCMDS
int32_t iprvYucRficSetRSSI( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETRSSI_WDS ] = { 0 };
    sw_cmd_setget_rssi_t * xCmdData = ( sw_cmd_setget_rssi_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETRSSI;

    YUC_FWCMD_RSSI_RECEIVER_P2W( xCmdData->receiver, uRficWds[ 0  ] );
    YUC_FWCMD_RSSI_MODE_P2W( xCmdData->mode, uRficWds[ 0 ] );
    YUC_FWCMD_RSSI_RATE_P2W( xCmdData->rate, uRficWds [ 0 ] );	
    YUC_FWCMD_RSSI_THRESHOLD_P2W( xCmdData->threshold, uRficWds [ 0 ] ); 	
    YUC_FWCMD_RSSI_REF_G0_0_P2W( xCmdData->ref_g0_0, uRficWds[ 1 ] );	
    YUC_FWCMD_RSSI_REF_G0_1_P2W( xCmdData->ref_g0_1, uRficWds[ 1 ] );	
    YUC_FWCMD_RSSI_REF_G0_2_P2W( xCmdData->ref_g0_2, uRficWds[ 1 ] );	
    YUC_FWCMD_RSSI_REF_G1_0_P2W( xCmdData->ref_g1_0, uRficWds[ 2 ] );	
    YUC_FWCMD_RSSI_REF_G1_1_P2W( xCmdData->ref_g1_1, uRficWds[ 2 ] );	
    YUC_FWCMD_RSSI_REF_G1_2_P2W( xCmdData->ref_g1_2, uRficWds[ 2 ] );	
    YUC_FWCMD_RSSI_REF_GOTHER_0_P2W( xCmdData->ref_gother_0, uRficWds[ 3 ] );
    YUC_FWCMD_RSSI_REF_GOTHER_1_P2W( xCmdData->ref_gother_1, uRficWds[ 3 ] );
    YUC_FWCMD_RSSI_REF_GOTHER_1_P2W( xCmdData->ref_gother_2, uRficWds[ 3 ] );

    RF_LOGDBG( "Set RSSI %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SETRSSI_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Set RSSI command fail" );
    }

    return xRet;
}

int32_t iprvYucRficGetRSSI( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_GETRSSI_WDS ] = { 0 };
    sw_cmd_setget_rssi_t * xCmdData = ( sw_cmd_setget_rssi_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETRSSI;

    YUC_FWCMD_RSSI_RECEIVER_P2W( xCmdData->receiver, uRficWds[ 0  ] );

    RF_LOGDBG( "Set RSSI %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            1 );

    if( xRet )
    {
        RF_LOGERRMSG( "Set RSSI command fail" );
    }
    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_GETRSSI_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get Path response fail" );
        goto out;
    }

    YUC_FWCMD_RSSI_MODE_W2P( xCmdData->mode, uRficWds[ 1 ] );
    YUC_FWCMD_RSSI_RATE_W2P( xCmdData->rate, uRficWds [ 1 ] );	
    YUC_FWCMD_RSSI_THRESHOLD_W2P( xCmdData->threshold, uRficWds [ 1 ] ); 	
    YUC_FWCMD_RSSI_REF_G0_0_W2P( xCmdData->ref_g0_0, uRficWds[ 2 ] );	
    YUC_FWCMD_RSSI_REF_G0_1_W2P( xCmdData->ref_g0_1, uRficWds[ 2 ] );	
    YUC_FWCMD_RSSI_REF_G0_2_W2P( xCmdData->ref_g0_2, uRficWds[ 2 ] );	
    YUC_FWCMD_RSSI_REF_G1_0_W2P( xCmdData->ref_g1_0, uRficWds[ 3 ] );	
    YUC_FWCMD_RSSI_REF_G1_1_W2P( xCmdData->ref_g1_1, uRficWds[ 3 ] );	
    YUC_FWCMD_RSSI_REF_G1_2_W2P( xCmdData->ref_g1_2, uRficWds[ 3 ] );	
    YUC_FWCMD_RSSI_REF_GOTHER_0_W2P( xCmdData->ref_gother_0, uRficWds[ 4 ] );
    YUC_FWCMD_RSSI_REF_GOTHER_1_W2P( xCmdData->ref_gother_1, uRficWds[ 4 ] );
    YUC_FWCMD_RSSI_REF_GOTHER_1_W2P( xCmdData->ref_gother_2, uRficWds[ 4 ] );

out:
    return xRet;
}
#endif

int32_t iprvYucRficSetRxDcCaldata(uint32_t freq_idx, u32 rx)
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData, uIdx;
    volatile yuc_rxdc_gain_cal_t *rxdc;
    u16 uRficWdsRxI[ YUC_FWCMD_RXDC_WDS ] = { 0 };
    u16 uRficWdsRxQ[ YUC_FWCMD_RXDC_WDS ] = { 0 };
    u16 uRficWdsDPDI[ YUC_FWCMD_RXDC_WDS ] = { 0 };
    u16 uRficWdsDPDQ[ YUC_FWCMD_RXDC_WDS ] = { 0 };
    ulCmdData = YUC_FWCMD_SETRXDC;
    rxdc = &(pYucInfo->cal_data->freq_rxdc[freq_idx].rx_rxdc[rx-1]);
    YUC_FWCMD_RXDC_RECEIVER_P2W( rx, uRficWdsRxI[ 0 ] );
    YUC_FWCMD_RXDC_RECEIVER_P2W( rx, uRficWdsRxQ[ 0 ] );
    YUC_FWCMD_RXDC_RECEIVER_P2W( rx, uRficWdsDPDI[ 0 ] );
    YUC_FWCMD_RXDC_RECEIVER_P2W( rx, uRficWdsDPDQ[ 0 ] );
    YUC_FWCMD_RXDC_CHANNEL_P2W( YUC_CH_RX, uRficWdsRxI[ 0 ] );
    YUC_FWCMD_RXDC_IQSELECT_P2W( YUC_I, uRficWdsRxI[ 0 ] );
    YUC_FWCMD_RXDC_CHANNEL_P2W( YUC_CH_RX, uRficWdsRxQ[ 0 ] );
    YUC_FWCMD_RXDC_IQSELECT_P2W( YUC_Q, uRficWdsRxQ[ 0 ] );
    YUC_FWCMD_RXDC_CHANNEL_P2W( YUC_CH_TX, uRficWdsDPDI[ 0 ] );
    YUC_FWCMD_RXDC_IQSELECT_P2W( YUC_I, uRficWdsDPDI[ 0 ] );
    YUC_FWCMD_RXDC_CHANNEL_P2W( YUC_CH_TX, uRficWdsDPDQ[ 0 ] );
    YUC_FWCMD_RXDC_IQSELECT_P2W( YUC_Q, uRficWdsDPDQ[ 0 ] );
    for( uIdx = 0; uIdx < YUC_RX_RF_GAIN_LEVELS; uIdx++)
    {
        YUC_FWCMD_RXDC_FINE_P2W( rxdc->gain_rxdc[uIdx].i_fine, uRficWdsRxI[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_COARSE_P2W( rxdc->gain_rxdc[uIdx].i_coarse, uRficWdsRxI[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_FINE_P2W( rxdc->gain_rxdc[uIdx].q_fine, uRficWdsRxQ[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_COARSE_P2W( rxdc->gain_rxdc[uIdx].q_coarse, uRficWdsRxQ[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_FINE_P2W( rxdc->gain_dpd_rxdc[uIdx].i_fine, uRficWdsDPDI[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_COARSE_P2W( rxdc->gain_dpd_rxdc[uIdx].i_coarse, uRficWdsDPDI[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_FINE_P2W( rxdc->gain_dpd_rxdc[uIdx].q_fine, uRficWdsDPDQ[ uIdx + 1 ] );
        YUC_FWCMD_RXDC_COARSE_P2W( rxdc->gain_dpd_rxdc[uIdx].q_coarse, uRficWdsDPDQ[ uIdx + 1 ] );
    }
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWdsRxI,
                           YUC_FWCMD_RXDC_WDS );
    if( xRet )
        goto out;
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWdsRxQ,
                           YUC_FWCMD_RXDC_WDS );
    if( xRet )
        goto out;
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWdsDPDI,
                           YUC_FWCMD_RXDC_WDS );
    if( xRet )
        goto out;
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWdsDPDQ,
                           YUC_FWCMD_RXDC_WDS );
    if( xRet )
        goto out;
out:
    if( xRet )
    {
        RF_LOGERRMSG( "SetRxDc command fail" );
    }
    return xRet;
}

int32_t iprvYucRficSetRxDc( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_RXDC_WDS ] = { 0 };
    sw_cmd_setget_rxdc_t * xCmdData = ( sw_cmd_setget_rxdc_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETRXDC;
    YUC_FWCMD_RXDC_RECEIVER_P2W( xCmdData->rx, uRficWds[ 0 ] );
    YUC_FWCMD_RXDC_CHANNEL_P2W( xCmdData->channel, uRficWds[ 0 ] );
    YUC_FWCMD_RXDC_IQSELECT_P2W( xCmdData->iq_select, uRficWds[ 0 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine0, uRficWds[ 1 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse0, uRficWds[ 1 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine1, uRficWds[ 2 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse1, uRficWds[ 2 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine2, uRficWds[ 3 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse2, uRficWds[ 3 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine3, uRficWds[ 4 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse3, uRficWds[ 4 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine4, uRficWds[ 5 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse4, uRficWds[ 5 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine5, uRficWds[ 6 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse5, uRficWds[ 6 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine6, uRficWds[ 7 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse6, uRficWds[ 7 ] );
    YUC_FWCMD_RXDC_FINE_P2W( xCmdData->fine7, uRficWds[ 8 ] );
    YUC_FWCMD_RXDC_COARSE_P2W( xCmdData->coarse7, uRficWds[ 8 ] );

    RF_LOGDBG( "SetRxDc %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_RXDC_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "SetRxDc command fail" );
    }

    return xRet;
}

void xRxDcSignConvert(int32_t *i)
{
    /* If 6th bin any value convert it into 2's comp form of that number */
    if (*i & 0x00000020)
        *i = (*i | 0xffffffe0);
}

int32_t iprvYucRficGetRxDc( __attribute__((unused))RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_RXDC_WDS ] = { 0 };

    sw_cmd_setget_rxdc_t * xCmdData = ( sw_cmd_setget_rxdc_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_GETRXDC;
    YUC_FWCMD_RXDC_RECEIVER_P2W( xCmdData->rx, uRficWds[ 0 ] );
    YUC_FWCMD_RXDC_CHANNEL_P2W( xCmdData->channel, uRficWds[ 0 ] );
    YUC_FWCMD_RXDC_IQSELECT_P2W( xCmdData->iq_select, uRficWds[ 0 ] );

    RF_LOGDBG( "Get RxDc %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_GETRXDC_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get RxDc command fail" );
        goto out;
    }

    /* read command response */
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_RESP_ADDR,
                            &uRficWds[ 0 ], YUC_FWCMD_RXDC_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "Get RxDc response fail" );
        goto out;
    }

    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine0, uRficWds[ 1 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse0, uRficWds[ 1 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine1, uRficWds[ 2 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse1, uRficWds[ 2 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine2, uRficWds[ 3 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse2, uRficWds[ 3 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine3, uRficWds[ 4 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse3, uRficWds[ 4 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine4, uRficWds[ 5 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse4, uRficWds[ 5 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine5, uRficWds[ 6 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse5, uRficWds[ 6 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine6, uRficWds[ 7 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse6, uRficWds[ 7 ] );
    YUC_FWCMD_RXDC_FINE_W2P( xCmdData->fine7, uRficWds[ 8 ] );
    YUC_FWCMD_RXDC_COARSE_W2P( xCmdData->coarse7, uRficWds[ 8 ] );
    xRxDcSignConvert(&xCmdData->fine0);
    xRxDcSignConvert(&xCmdData->fine1);
    xRxDcSignConvert(&xCmdData->fine2);
    xRxDcSignConvert(&xCmdData->fine3);
    xRxDcSignConvert(&xCmdData->fine4);
    xRxDcSignConvert(&xCmdData->fine5);
    xRxDcSignConvert(&xCmdData->fine6);
    xRxDcSignConvert(&xCmdData->fine7);
    xRxDcSignConvert(&xCmdData->coarse0);
    xRxDcSignConvert(&xCmdData->coarse1);
    xRxDcSignConvert(&xCmdData->coarse2);
    xRxDcSignConvert(&xCmdData->coarse3);
    xRxDcSignConvert(&xCmdData->coarse4);
    xRxDcSignConvert(&xCmdData->coarse5);
    xRxDcSignConvert(&xCmdData->coarse6);
    xRxDcSignConvert(&xCmdData->coarse7);

out:
    return xRet;
}

int32_t iprvYucRficSetCalRxDc( __attribute__((unused)) RficHandle_t xHandle,
                               rf_sw_cmd_desc_t * pCmdDesc )
{
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    u32 ulCmdData;
    u16 uRficWds[ YUC_FWCMD_SETCALRXDC_WDS ] = { 0 };
    sw_cmd_setcal_rxdc_t * xCmdData = ( sw_cmd_setcal_rxdc_t * ) ( pCmdDesc->data );

    ulCmdData = YUC_FWCMD_SETCALRXDC;
    YUC_FWCMD_SETCALRXDC_RECEIVER_P2W( xCmdData->rx, uRficWds[ 0 ] );
    YUC_FWCMD_SETCALRXDC_MODE_P2W( xCmdData->mode, uRficWds[ 0 ] );
    YUC_FWCMD_SETCALRXDC_IFINE_P2W( xCmdData->ifine, uRficWds[ 1 ] );
    YUC_FWCMD_SETCALRXDC_ICOARSE_P2W( xCmdData->icoarse, uRficWds[ 1 ] );
    YUC_FWCMD_SETCALRXDC_QFINE_P2W( xCmdData->qfine, uRficWds[ 2 ] );
    YUC_FWCMD_SETCALRXDC_QCOARSE_P2W( xCmdData->qcoarse, uRficWds[ 2 ] );

    RF_LOGDBG( "SetCalRxDc %d", uRficWds[ 0 ] );
    xRet = xYucRficCmdProc( ulCmdData, YUC_RFIC_CMD_ADDR, uRficWds,
                            YUC_FWCMD_SETCALRXDC_WDS );

    if( xRet )
    {
        RF_LOGERRMSG( "SetCalRxDc command fail" );
    }

    return xRet;
}


/**
 * SW commands implementation starts here.
 * Below function is just and prototype will updated in future.
 */

int32_t iYucRficSwitchRxtx( RficHandle_t xHandle,
                          bool bIsRx , bool bIsRxDcCalib)
{
    int32_t iRet = RF_SW_CMD_RESULT_OK;

    if( ulMpicCurrentCore() != RF_LOCAL_CORE )
    {
        RF_LOGERRMSG("Should run from RF_LOCAL_CORE");
        return RF_SW_CMD_RESULT_INVALID_CORE;
    }
    xFemSwitchRxTX(xHandle, bIsRx, bIsRxDcCalib);
    return iRet;
}
int32_t iprvYucRficSwSwTx( RficHandle_t xHandle,
                           __attribute__((unused))rf_sw_cmd_desc_t * pCmdDesc )
{
    return iYucRficSwitchRxtx(xHandle, FALSE, FALSE);
}
int32_t iprvYucRficSwSwRx( RficHandle_t xHandle,
                           rf_sw_cmd_desc_t * pCmdDesc )
{
    uint32_t *rxdc = (uint32_t *)(pCmdDesc->data);
    return iYucRficSwitchRxtx(xHandle, TRUE, *rxdc);
}

int32_t iYucRficFr1FemLna2Bypass( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    int32_t iRet = RF_SW_CMD_RESULT_OK;

    if( ulMpicCurrentCore() != RF_LOCAL_CORE )
    {
        return RF_SW_CMD_RESULT_INVALID_CORE;
    }

    sw_cmd_setget_lnabypass_t * xCmdData = ( sw_cmd_setget_lnabypass_t * ) ( pCmdDesc->data );
    yuc_on_off_t onoff;
    if ( xCmdData->enable )
        onoff = YUC_ON;
    else
        onoff = YUC_OFF;

    xFemLna2Bypass(xHandle, onoff);

    return iRet;
}

int32_t iYucRficFr1DpdSwCtrl( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    if( ulMpicCurrentCore() != RF_LOCAL_CORE )
    {
        return RF_SW_CMD_RESULT_INVALID_CORE;
    }

    sw_cmd_setget_dpdsw_t * xCmdData = ( sw_cmd_setget_dpdsw_t * ) ( pCmdDesc->data );
    yuc_on_off_t onoff;
    if ( xCmdData->enable )
        onoff = YUC_ON;
    else
        onoff = YUC_OFF;

    xFemDpdSwCtrl(xHandle, onoff);
    return RF_SW_CMD_RESULT_OK;
}

int32_t iCheckValidFR1Mode( uint32_t fr1_mode )
{
    switch(fr1_mode)
    {
        case eFR1Mode1t1r0 :
        case eFR1Mode1t1r1 :
        case eFR1Mode1t1r2 :
        case eFR1Mode1t1r3 :
        case eFR1Mode2t2r0 :
        case eFR1Mode2t2r1 :
        case eFR1Mode4t4r :
            return pdTRUE;
        default :
            return pdFALSE;
    }
}

int32_t iYucRficFr1SetMode( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    sw_cmd_setget_mode_t * xCmdData = ( sw_cmd_setget_mode_t * ) ( pCmdDesc->data );
    eRficFR1Mode eMode = xCmdData->mode;
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    if (!iCheckValidFR1Mode(eMode))
    {
        RF_LOGERRMSG("Invalid mode");
        return RF_SW_CMD_RESULT_INVALID_MODE;
    }
    xHandle->eFR1Mode = eMode;
#ifdef YUCCA_RF
    pYucInfo->eFR1Mode = eMode;
    
    if( eMode & eFR1Mode2t2r0 )
        pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic1_addr;
    else if( eMode & eFR1Mode2t2r1 )
        pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic2_addr;

    RF_LOGDBG( " LLCP Addr: 0x%0x \n\r", pYucInfo->llcp_rfic_addr );
#endif
    return xRet;
}

int32_t iYucRficFr1GetMode( RficHandle_t xHandle,
                            rf_sw_cmd_desc_t * pCmdDesc )
{
    sw_cmd_setget_mode_t * xCmdData = ( sw_cmd_setget_mode_t * ) ( pCmdDesc->data );

    xCmdData->mode = xHandle->eFR1Mode;
    /* xCmdData->type = xHandle->eFR1Type; */
    /* xCmdData->idx  = xHandle->uFR1Idx; */
    return RF_SW_CMD_RESULT_OK;
}

int32_t check_caldata( uint32_t state )
{
    RF_LOGDBG("Cal data - Magic[0x%x] State[0x%x] Ver[0x%x] Recalb[0x%x]", pYucInfo->cal_data->magic,
            pYucInfo->cal_data->state, pYucInfo->cal_data->ver, pYucInfo->cal_data->recalib);
    if( ( pYucInfo->cal_data->magic == RFCAL_MAGIC_NUM ) 
            && ( pYucInfo->cal_data->state & state ) )
        return RF_SW_CMD_RESULT_OK;
    else
        return RF_SW_CMD_RESULT_INVALID_CALDATA;
}

int32_t iprvYucRficSwSetTxBw( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    pCmdDesc->cmd = YUC_FWCMD_SETTXBW;
    sw_cmd_setget_txrxbw_t *bwdata = ( sw_cmd_setget_txrxbw_t * ) pCmdDesc->data;
    if( check_caldata( YUC_CAL_TXBW_STATE ) == RF_SW_CMD_RESULT_OK)
    {
        bwdata->bw_ftune = pYucInfo->cal_data->tx_bw[bwdata->bw];
    }
    else 
    {
        RF_LOGDBGMSG("TxBW calibration data not available, So using FWCMD");
    }
    return iprvYucRficSetTxBw( xHandle, pCmdDesc );
}

int32_t iprvYucRficSwSetRxBw( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    pCmdDesc->cmd = YUC_FWCMD_SETRXBW;
    sw_cmd_setget_txrxbw_t *bwdata = ( sw_cmd_setget_txrxbw_t * ) pCmdDesc->data;
    if( check_caldata( YUC_CAL_RXBW_STATE ) == RF_SW_CMD_RESULT_OK)
    {
        bwdata->bw_ftune = pYucInfo->cal_data->rx_bw[bwdata->bw];
    }
    else
    {
        RF_LOGDBGMSG("RxBW calibration data not available, So using FWCMD");
    }
    return iprvYucRficSetRxBw( xHandle, pCmdDesc );
}

int32_t iprvYucRficSwSetTrxPll( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc )
{
    u32 freq_idx, rx;
    BaseType_t xRet = RF_SW_CMD_RESULT_OK;
    sw_cmd_setget_trxpll_t *plldata = ( sw_cmd_setget_trxpll_t * ) pCmdDesc->data;
    freq_idx = GET_CAL_PLL_FREQ_IDX(plldata->freq_khz);
    plldata->mode = YUC_ON;
    if( (check_caldata( YUC_CAL_PLL_STATE ) == RF_SW_CMD_RESULT_OK) && (freq_idx == plldata->freq_khz))
    {
        plldata->vco_sel = pYucInfo->cal_data->pll[freq_idx].vco_sel;
        plldata->cal_cap = pYucInfo->cal_data->pll[freq_idx].cal_cap;
        plldata->cal_current = pYucInfo->cal_data->pll[freq_idx].cal_current;
    }
    else
    {
        /* If calib data is not available we should use FW cmd */
        plldata->vco_sel = 7;
        pCmdDesc->cmd = YUC_FWCMD_SETTRXPLL;
        RF_LOGDBGMSG("PLL calibration data not available so using FWCMD");
    }
    xRet = iprvYucRficSetTrxPll( xHandle, pCmdDesc );
    if( xRet )
        goto out;

    freq_idx = GET_CAL_RXDC_FREQ_IDX(plldata->freq_khz);
    if( check_caldata( YUC_CAL_RXDC_STATE ) == RF_SW_CMD_RESULT_OK)
    {
        for( rx = YUC_RX_1; rx <= YUC_RX_2; rx++ )
        {
            xRet = iprvYucRficSetRxDcCaldata(freq_idx, rx);
            if( xRet )
                goto out;
        }
    }
    else
    {
        RF_LOGDBGMSG("RxDc calibration data not available might be calling from RXDC calibration");
    }
out:
    return xRet;
}

int32_t iprvYucRficSwSetPath( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    uint32_t path = 0, act = 0, dpd_mode = 0, dpd_rx = 0;
    eRficFR1Mode mode = xHandle->eFR1Mode;

    act = YUC_ACT_TDD;
    if ( ( mode & eFR1Mode1t1r0 ) || ( mode & eFR1Mode1t1r2 ) )
    {
        path = YUC_PATH_RX1_TX1;
        dpd_rx = YUC_RX_1;
    }
    if ( ( mode & eFR1Mode1t1r1 ) || ( mode & eFR1Mode1t1r3 ) )
    {
        path |= YUC_PATH_RX2_TX2;
        dpd_rx |= YUC_RX_2;
    }
    RF_LOGDBG("Yucca path: %d ", path);
    
    /* Make sure all other parameters intact while setting TRX eg. BW */
    xRet = iprvYucRficGetPath(xHandle, pDesc);
    if (xRet)
    {
        RF_LOGERR(" xRet : %d", xRet);
        return xRet;
    }

    sw_cmd_setget_path_t *xCmdData = (sw_cmd_setget_path_t *)(pDesc->data);
    xCmdData->path = path;
    xCmdData->band = YUC_BAND_MB;
    /* Using same parameter as the values are same */
    xCmdData->rssi_mode = dpd_rx;
    xCmdData->dpd = YUC_OFF;
    dpd_mode = xCmdData->dpd;

    xRet = iprvYucRficSetPath(xHandle, pDesc);
    if (xRet)
    {
        RF_LOGERR(" xRet : %d", xRet);
        return xRet;
    }

    sw_cmd_setactive_t* xCmdData1 = (sw_cmd_setactive_t*)(pDesc->data);

    xCmdData1->mode = act;
    xCmdData1->dpd_rx = dpd_mode != YUC_DPD_NONE ? dpd_rx : YUC_RX_NONE;

    xRet = iprvYucRficSetActive(xHandle, pDesc);
    if (xRet)
    {
        RF_LOGERR(" xRet : %d", xRet);
    }
    return xRet;
}


int32_t iprvYucRficSwRelTxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc )
{
    int32_t xRet = 0;
    int8_t xTxGainCurIdx = 0, xTxGainIdx = 0;
    int16_t xTxCurGain = 0;
    uint16_t xTxGain = 0;
    rf_sw_cmd_desc_t pFWDesc = {0};
    sw_cmd_ctrl_relative_gain_t * xSWCmdData = ( sw_cmd_ctrl_relative_gain_t * ) ( pDesc->data );

    if( !( abs( xSWCmdData->ucGainIndB * 0.1 ) <= ( YUC_TX_GAIN_MAX_IDX << 1 ) ) )
    {
        RF_LOGERRMSG("User input Gain is not in valid range");
        return RF_SW_CMD_RESULT_INVALID_GAIN;
    }
    
    int16_t xDeltaGain = (int16_t)( xSWCmdData->ucGainIndB );
    RF_LOGDBG("Requested gain change in (*10) scale in dB %d",xDeltaGain);
    
    sw_cmd_set_gain_t * xFWCmdData = ( sw_cmd_set_gain_t * ) ( pFWDesc.data );
    pFWDesc.cmd = YUC_FWCMD_SETGAIN;

    if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
    {
        xTxGainCurIdx = pYucInfo->yucData[FR1_IDX_YC1 - 1]->txGainIdx;
    }
    else
    {
        xTxGainCurIdx = pYucInfo->yucData[FR1_IDX_YC2 - 1]->txGainIdx;
    }

    if ( xHandle->eFR1Mode & ( eFR1Mode1t1r0 | eFR1Mode1t1r2 ) )
    {
        xFWCmdData->path = YUC_PATH_TX1;
    }

    if( xHandle->eFR1Mode & (eFR1Mode1t1r1 | eFR1Mode1t1r3 ) )
    {
        xFWCmdData->path |= YUC_PATH_TX2;
    }

    xTxCurGain = gTXGainLUT[xTxGainCurIdx]; 
    xTxGain = xTxCurGain + xDeltaGain; 
    xTxGainIdx = (int8_t) round (( xTxGain >> 1 ) * 0.1 );

    if ( !( ( xTxGainIdx ) >= YUC_TX_GAIN_MIN_IDX ) )
    {
        RF_LOGERRMSG("Gain cannot be set beyond minimum gain");
        return RF_SW_CMD_RESULT_GAIN_MINOUT;
    }
    if ( !( ( xTxGainIdx ) <= YUC_TX_GAIN_MAX_IDX ) || ( xTxGain > gTXGainLUT[YUC_TX_GAIN_MAX_IDX] ) )
    {
        RF_LOGERRMSG("Gain cannot exceed maximum gain");
        return RF_SW_CMD_RESULT_GAIN_MAXOUT;
    }

    int8_t xstart = ( xTxGainIdx - 1 >= 0 ? xTxGainIdx - 1 : 0 ) , xend = ( xTxGainIdx + 1 > YUC_TX_GAIN_MAX_IDX ? YUC_TX_GAIN_MAX_IDX : xTxGainIdx + 1 );
    if ( xTxGainIdx >= YUC_TXGAIN_STEP_ERR_POINT )
    {
        xstart = YUC_TXGAIN_STEP_ERR_POINT;
        xend = YUC_TX_GAIN_MAX_IDX;
    }

    int16_t xmingainerr = INT16_MAX;
    int8_t xi = 0, xerr = 0;
    for ( xi = xstart ; xi <= xend ; xi++ )
    {
        xerr = abs(gTXGainLUT[xi] - xTxGain);
        if ( xerr < xmingainerr )
        {
            xmingainerr = xerr;
            xTxGainIdx = xi;
        }
        /* gain values in lut are always increasing so when ever we found gain err is increasing we can */
        /*     come out of the loop */
        else
        {
            break;
        }
    }

    RF_LOGDBG(" YUC Gain: xTxGainCurIdx : %d, xDeltaGain : %d, xTxGainIdx : %d", xTxGainCurIdx, xDeltaGain, xTxGainIdx );

    xFWCmdData->tx_gain = (uint32_t) xTxGainIdx;
    xFWCmdData->manual = 0;
    xFWCmdData->hash_mode = 0;
    xFWCmdData->channel = YUC_CH_TX;

    xRet = iprvYucRficSetGain(xHandle, &pFWDesc);
    if (!xRet)
    {
        if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
        {
            pYucInfo->yucData[FR1_IDX_YC1 - 1]->txGainIdx = xTxGainIdx;
            xSWCmdData->psEnforcedGain = ( gTXGainLUT[xTxGainIdx] - gTXGainLUT[xTxGainCurIdx] );
        }
        else
        {
            pYucInfo->yucData[FR1_IDX_YC2 - 1]->txGainIdx = xTxGainIdx;
            xSWCmdData->psEnforcedGain = ( gTXGainLUT[xTxGainIdx] - gTXGainLUT[xTxGainCurIdx] ) ;
        }
    }
    return xRet;
}

static int32_t iprvYucRficSwRelRxSRxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc, uint32_t uChannel)
{
    int32_t xRet = 0;
    int8_t xRxGainIdx = 0, xRxGainCurIdx = 0;
    uint16_t xRxGain = 0;
    int16_t xRxCurGain = 0;
    rf_sw_cmd_desc_t pFWDesc = {0};
    sw_cmd_ctrl_relative_gain_t * xSWCmdData = ( sw_cmd_ctrl_relative_gain_t * ) ( pDesc->data );
    
    if( !( abs( xSWCmdData->ucGainIndB * 0.1) <= ( YUC_RX_GAIN_MAX_IDX << 1 ) ) )
    {
        RF_LOGERRMSG("User input Gain is not in valid range");
        return RF_SW_CMD_RESULT_INVALID_GAIN;
    }


    int16_t xDeltaGain = (int16_t)( xSWCmdData->ucGainIndB );
    RF_LOGDBG("Requested gain change in (*10) scale in dB %d",xDeltaGain);

    sw_cmd_set_gain_t * xFWCmdData = ( sw_cmd_set_gain_t * ) ( pFWDesc.data );
    pFWDesc.cmd = YUC_FWCMD_SETGAIN;

    if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
    {
        xRxGainCurIdx = pYucInfo->yucData[FR1_IDX_YC1 - 1]->rxGainIdx;
    }
    else
    {
        xRxGainCurIdx = pYucInfo->yucData[FR1_IDX_YC2 - 1]->rxGainIdx;
    }

    if ( xHandle->eFR1Mode & ( eFR1Mode1t1r0 | eFR1Mode1t1r2 ) )
    {
        xFWCmdData->path = YUC_PATH_RX1;
    }

    if( xHandle->eFR1Mode & (eFR1Mode1t1r1 | eFR1Mode1t1r3) )
    {
        xFWCmdData->path |= YUC_PATH_RX2;
    }

    xRxCurGain = gRXGainLUT[xRxGainCurIdx];
    xRxGain = xRxCurGain + xDeltaGain;
    xRxGainIdx = (int8_t) round (( xRxGain >> 1 ) * 0.1 );
    
    if ( !( ( xRxGainIdx) >= YUC_RX_GAIN_MIN_IDX ) )
    {
        RF_LOGERRMSG("Gain cannot be set beyond minimum gain");
        return RF_SW_CMD_RESULT_GAIN_MINOUT;
    }
    if ( !( ( xRxGainIdx ) <= YUC_RX_GAIN_MAX_IDX ) || ( xRxGain > gRXGainLUT[YUC_RX_GAIN_MAX_IDX] ) )
    {
        RF_LOGERRMSG("Gain cannot exceed maximum gain");
        return RF_SW_CMD_RESULT_GAIN_MAXOUT;
    }

    int8_t xstart = ( xRxGainIdx - 1 >= 0 ? xRxGainIdx - 1 : 0 ), xend = ( xRxGainIdx + 1 > YUC_RX_GAIN_MAX_IDX ? YUC_RX_GAIN_MAX_IDX : xRxGainIdx + 1 );
    int16_t xmingainerr = INT16_MAX;
    int8_t xi = 0, xerr = 0;

    for ( xi = xstart ; xi <= xend ; xi++ )
    {
        xerr = abs(gRXGainLUT[xi] - xRxGain);
        if ( xerr < xmingainerr )
        {
            xmingainerr = xerr;
            xRxGainIdx = xi;
        }
        else
        {
            break;
        }
    }
        
    RF_LOGDBG(" YUC Gain: xRxGainCurIdx : %d, xDeltaGain : %d, xRxGainIdx : %d", xRxGainCurIdx, xDeltaGain, xRxGainIdx );

    xFWCmdData->manual = 1;
    xFWCmdData->hash_mode = 0;
    xFWCmdData->channel = uChannel;
    xFWCmdData->bb_gain = (uint32_t) rxGainTable[xRxGainIdx].bbgain_idx;
    xFWCmdData->rf_gain = (uint32_t) rxGainTable[xRxGainIdx].rfgain_idx;
    RF_LOGDBG(" rxGainTable[xRxGainIdx].bbgain_idx : %d, rxGainTable[xRxGainIdx].rfgain_idx : %d", rxGainTable[xRxGainIdx].bbgain_idx, rxGainTable[xRxGainIdx].rfgain_idx );

    xRet = iprvYucRficSetGain(xHandle, &pFWDesc);
    
    if (!xRet)
    {
        if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
        {
            pYucInfo->yucData[FR1_IDX_YC1 - 1]->rxGainIdx = xRxGainIdx;
            xSWCmdData->psEnforcedGain = ( gRXGainLUT[xRxGainIdx] - gRXGainLUT[xRxGainCurIdx] );
        }
        else
        {
            pYucInfo->yucData[FR1_IDX_YC2 - 1]->rxGainIdx = xRxGainIdx;
            xSWCmdData->psEnforcedGain = ( gRXGainLUT[xRxGainIdx] - gRXGainLUT[xRxGainCurIdx] );
        }
    }

    return xRet;
}
int32_t iprvYucRficSwRelRxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc )
{
    return iprvYucRficSwRelRxSRxGain(xHandle, pDesc, YUC_CH_RX);
}
int32_t iprvYucRficSwRelSRxGain( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc )
{
    return iprvYucRficSwRelRxSRxGain(xHandle, pDesc, YUC_CH_TX);
}

int32_t iprvYucRficSwPllStatus( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    sw_cmd_pll_status_t * xPllData = NULL;
    /* To retrieve data from the FWCMD */
    sw_cmd_get_sys_status_t * xCmdData = ( sw_cmd_get_sys_status_t * ) ( pDesc->data );
    eRficFR1Mode mode = xHandle->eFR1Mode;
    uint32_t yuc1TrxPllStatus = RF_PLL_NOTENABLED;
    uint32_t yuc1CalPllStatus = RF_PLL_NOTENABLED;
    uint32_t yuc2TrxPllStatus = RF_PLL_NOTENABLED;
    uint32_t yuc2CalPllStatus = RF_PLL_NOTENABLED;
    if( mode & eFR1Mode2t2r0 )
    {
        xRet = iprvYucRficGetSysStatus(xHandle, pDesc);
        if(xRet)
            goto out;
        yuc1TrxPllStatus = xCmdData->trxpll_unlock;
        yuc1CalPllStatus = xCmdData->calpll_unlock;
        RF_LOGDBG(" YUC1 Status: TRXPLL - %d, CALPLL - %d", yuc1TrxPllStatus, yuc1CalPllStatus);
    }
    if( mode & eFR1Mode2t2r1 )
    {
        xRet = iprvYucRficGetSysStatus(xHandle, pDesc);
        if(xRet)
            goto out;
        yuc2TrxPllStatus = xCmdData->trxpll_unlock;
        yuc2CalPllStatus = xCmdData->calpll_unlock;
        RF_LOGDBG(" YUC2 Status: TRXPLL - %d, CALPLL - %d", yuc2TrxPllStatus, yuc2CalPllStatus);
    }
    /* Overwriting the same structure to send back the data from the SWCMD */
    xPllData = ( sw_cmd_pll_status_t * ) ( pDesc->data );
    xPllData->yuc1_trxstatus = yuc1TrxPllStatus;
    xPllData->yuc1_calstatus = yuc1CalPllStatus;
    xPllData->yuc2_trxstatus = yuc2TrxPllStatus;
    xPllData->yuc2_calstatus = yuc2CalPllStatus;

out:
    return xRet;
}

int32_t iprvYucRficSwSetDPD( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pDesc )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    uint32_t dpd_rx = 0;
    rf_sw_cmd_desc_t pFWDesc = {0};

    sw_cmd_setdpd_t * xCmdData = ( sw_cmd_setdpd_t * ) ( pDesc->data );
    if ( xCmdData->enable )
    {
        if ( ( xCmdData->dpd_path & eFR1Mode1t1r0 ) || ( xCmdData->dpd_path & eFR1Mode1t1r2 ) )
        {
            dpd_rx = YUC_RX_1;
        }
        if ( ( xCmdData->dpd_path & eFR1Mode1t1r1 ) || ( xCmdData->dpd_path & eFR1Mode1t1r3 ) )
        {
            dpd_rx |= YUC_RX_2;
        }
    }

    RF_LOGDBG("Yucca dpd_rx: %d ", dpd_rx);
    sw_cmd_setactive_t * xCmdData1 = ( sw_cmd_setactive_t * ) ( pFWDesc.data );

    xCmdData1->mode = YUC_ACT_TDD;
    xCmdData1->dpd_rx = dpd_rx;

    xRet = iprvYucRficSetActive(xHandle, &pFWDesc);
    if (xRet)
    {
        RF_LOGDBG(" xRet : %d", xRet);
    }

    return xRet;
}
