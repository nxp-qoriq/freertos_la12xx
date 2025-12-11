// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#include "rf_api.h"
#include "mpic.h"
#include "projdefs.h"
#include "rf_config.h"
#include "rf_dev.h"
#include "rf_local.h"
#include "rf_remote.h"
#include "rf_core.h"
#include "gpio.h"
#include "gpio_regs.h"
#include "rf_sw_cmd_types.h"
#include "rf_sw_cmds.h"
#include "types.h"
#ifdef YUCCA_RF
#include "yuc_rfic_cmd.h"
#include "yuc_rfic.h"
#endif
#ifdef ICEWINGS_RF
#include "icw_cmd.h"
#endif
#include <stdint.h>

extern int32_t gRXGainLUT[];
extern int32_t gTXGainLUT[];
extern size_t gRXGainLUTsize;
extern size_t gTXGainLUTsize;

volatile rf_sw_cmd_desc_t * xCheckHandleGetDesc( RficHandle_t xHandle,
                                                 struct Time * xAPIEntryTime )
{
    if( xHandle == NULL )
    {
        RF_LOGERRMSG( "TEST FAILED xHandle is NULL" );
        return NULL;
    }

    rf_start_latency_calc( xAPIEntryTime );
    volatile rf_sw_cmd_desc_t * pDesc = xGetRFSWCmdDesc( xHandle, 0 );

    if( pDesc == NULL )
    {
        RF_LOGERRMSG( "Descriptor not available" );
        return NULL;
    }

    pDesc->timeout = RF_SW_CMD_TIMEOUT;
    pDesc->priority = RF_SW_CMD_DEFAULT_PRIORITY;
    pDesc->flags = RF_CMD_FLAGS_SWCMD;
    return pDesc;
}

volatile rf_sw_cmd_desc_t * xGetRFSWCmdDesc( RficHandle_t xHandle,
                                             uint32_t ulTimeoutInUS )
{
    uint32_t ulCoreId = ulMpicCurrentCore();

    ( void ) ulTimeoutInUS;

    if( xSemaphoreTake( xHandle->xRFSWCmdDescSema[ ulCoreId ], 0 ) == pdFALSE )
    {
        RF_LOGDBG( "SemaphoreTake failed for Coreid:[ %d ]\n\r",ulCoreId );
        return NULL;
    }
    else
    {
        return &( xHandle->pxPrvRFMData->swcmd_descs[ ulCoreId ] );
    }
}

int32_t lRFCmdExecute( RficHandle_t xHandle,
                       volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc,
                       struct Time * pxAPIEntryTime )
{
    int32_t ret = 0;

    pxRFSWCmdDesc->status = RF_SW_CMD_STATUS_POSTED;

    if( ulMpicCurrentCore() == RF_LOCAL_CORE )
    {
        ret = xRFPostLocalSWCmd( xHandle, pxRFSWCmdDesc, pxAPIEntryTime );
    }
    else
    {
        ret = iRaiseRemoteRFEvent( xHandle, pxRFSWCmdDesc->ipi_event_id, RF_LOCAL_CORE );

        if( ret )
        {
            ret = RF_SW_CMD_RESULT_ERROR;
        }
        else
        {
            while( pxRFSWCmdDesc->status != RF_SW_CMD_STATUS_DONE )
            {
                if( iHasTimeElapsedMPICGB( pxAPIEntryTime, pxRFSWCmdDesc->timeout ) )
                {
                    RF_STATS_ADD( xHandle->pxStats->remote_cmd_timeout[ pxRFSWCmdDesc->core_id ][ pxRFSWCmdDesc->cmd ] );
                    ret = RF_SW_CMD_RESULT_TIMEOUT;
                    break;
                }
            }

            if( ret != RF_SW_CMD_RESULT_TIMEOUT )
            {
                ret = pxRFSWCmdDesc->result;
            }
        }
    }

    xSemaphoreGive( xHandle->xRFSWCmdDescSema[ pxRFSWCmdDesc->core_id ] );
    pxRFSWCmdDesc->status = RF_SW_CMD_STATUS_FREE;

    rf_end_latency_calc_noframework(xHandle, pxRFSWCmdDesc->core_id, pxRFSWCmdDesc->cmd, pxAPIEntryTime );

    return ret;
}

RficHandle_t rfic_api_init( eRficDevType devtype )
{
    RficHandle_t xHandle = xRFGetDeviceHandle();
    xHandle->eDevtype = devtype;

    /* Below commented code will be enabled shortly when all calibrations are working */
    /* if ( check_caldata(YUC_CAL_RXDC_STATE | YUC_CAL_PLL_STATE | YUC_CAL_RXBW_STATE | YUC_CAL_TXBW_STATE) ) */
    /* { */
    /*     RF_LOGDBGMSG("Calibrations done successfully"); */
    /* } */
    /* else */
    /* { */
    /*     RF_LOGERRMSG("Calibrations not done"); */
    /*     return eRFDeviceFailed;//What to be returned?? */
    /* } */

    return xHandle;
}

int32_t rfic_change_mode( RficHandle_t xHandle,
                       eRficFR1Mode eMode, eRficFR1DupMode duplexMode )
{
    /** TODO Change this to SWCMD */
    if (!iCheckValidFR1Mode(eMode))
    {
        RF_LOGERRMSG("Invalid mode");
        return RF_SW_CMD_RESULT_INVALID_MODE;
    }
    xHandle->eFR1Mode = eMode;
    xHandle->eFR1DupMode = duplexMode;
#ifdef YUCCA_RF
    
    pYucInfo->eFR1Mode = eMode;
    
    if( eMode & eFR1Mode2t2r0 )
        pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic1_addr;
    else if( eMode & eFR1Mode2t2r1 )
        pYucInfo->llcp_rfic_addr = pYucInfo->llcp_rfic2_addr;

    RF_LOGDBG( " LLCP Addr: 0x%0x \n\r", pYucInfo->llcp_rfic_addr );
#endif
    return pdPASS;
}

int32_t rfic_adjust_pll_freq( RficHandle_t xHandle,
                              uint32_t freq_khz , eRficFR1TXRX trxpath )
{
    int32_t xRet= RF_SW_CMD_RESULT_OK;
    sw_cmd_setget_trxpll_t * xCmdDataSet;
    
#ifdef YUCCA_RF
    if( !(freq_khz >= YUC_CAL_PLL_FREQ_MIN && freq_khz <= YUC_CAL_PLL_FREQ_MAX ) )
#endif
#ifdef ICEWINGS_RF
    if( !(freq_khz >= ICW_PLL_FREQ_MIN && freq_khz <= ICW_PLL_FREQ_MAX ) )
#endif
    {
        RF_LOGERRMSG("Frequency not in valid range");
        return RF_SW_CMD_RESULT_CMD_PARAMS_INVALID;
    }

    RF_CHECK_HANDLE_N_GET_CMD_DESC;

    xCmdDataSet = ( sw_cmd_setget_trxpll_t * ) ( &pDesc->data );
    pDesc->cmd = RF_SWCMD_SETTRXPLL;
    xCmdDataSet->freq_khz = freq_khz;
    xCmdDataSet->trxpath = trxpath;

    RF_LOGDBG( "\n\r freq_khz - %d\n\r", xCmdDataSet->freq_khz );

    RFCMDEXECUTE;
    return xRet;
}

uint32_t rfic_pll_status( RficHandle_t xHandle, sw_cmd_pll_status_t *xPllData )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    RF_CHECK_HANDLE_N_GET_CMD_DESC;
    
    pDesc->cmd = RF_SWCMD_PLLSTATUS;

    RFCMDEXECUTE;
    sw_cmd_pll_status_t *xPllRespData = ( sw_cmd_pll_status_t * ) ( pDesc->data );
    xPllData->yuc1_trxstatus = xPllRespData->yuc1_trxstatus;
    xPllData->yuc1_calstatus = xPllRespData->yuc1_calstatus;
    xPllData->yuc2_trxstatus = xPllRespData->yuc2_trxstatus;
    xPllData->yuc2_calstatus = xPllRespData->yuc2_calstatus;
    return xRet;
}

int32_t rfic_switch_tx( RficHandle_t xHandle )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    
    RF_CHECK_HANDLE_N_GET_CMD_DESC;
    pDesc->cmd = RF_SWCMD_SWTX;

    RFCMDEXECUTE;
    return xRet;
}

int32_t rfic_switch_rx( RficHandle_t xHandle )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    
    RF_CHECK_HANDLE_N_GET_CMD_DESC;
    pDesc->cmd = RF_SWCMD_SWRX;
    /* Set rxdc flag to 0 as the api is not called from calibration */
    pDesc->data[0] = 0;

    RFCMDEXECUTE;
    return xRet;
}

int32_t rfic_ctrl_relative_tx_gain( RficHandle_t xHandle,
                          int32_t ucGainIndB, int32_t *psEnforcedGain )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;

    RF_CHECK_HANDLE_N_GET_CMD_DESC;
    pDesc->cmd = RF_SWCMD_REL_TXGAIN;
    
    sw_cmd_ctrl_relative_gain_t * xSWCmdData = ( sw_cmd_ctrl_relative_gain_t * ) ( pDesc->data );
    xSWCmdData->ucGainIndB = ucGainIndB;

    RFCMDEXECUTE;
    *psEnforcedGain = xSWCmdData->psEnforcedGain;
    return xRet;
}

static int32_t rfic_ctrl_relative_RxSRx_gain( RficHandle_t xHandle, uint32_t uCmd,
                          int32_t ucGainIndB, int32_t *psEnforcedGain )
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    RF_CHECK_HANDLE_N_GET_CMD_DESC;
    pDesc->cmd = uCmd;
    
    sw_cmd_ctrl_relative_gain_t * xSWCmdData = ( sw_cmd_ctrl_relative_gain_t * ) ( pDesc->data );
    xSWCmdData->ucGainIndB = ucGainIndB;

    RFCMDEXECUTE;
    *psEnforcedGain = xSWCmdData->psEnforcedGain;
    return xRet;
}

int32_t rfic_ctrl_relative_rx_gain( RficHandle_t xHandle,
                          int32_t ucGainIndB, int32_t *psEnforcedGain )
{
    return rfic_ctrl_relative_RxSRx_gain(xHandle, RF_SWCMD_REL_RXGAIN, ucGainIndB, psEnforcedGain);
}

int32_t rfic_ctrl_relative_srx_gain( RficHandle_t xHandle,
                          int32_t ucGainIndB, int32_t *psEnforcedGain )
{
    return rfic_ctrl_relative_RxSRx_gain(xHandle, RF_SWCMD_REL_SRXGAIN, ucGainIndB, psEnforcedGain);
}

int32_t rfic_set_srx( RficHandle_t xHandle,
                             rf_srx_path_t path, bool enable )
{
#ifdef YUCCA_RF
    int32_t xRet = RF_SW_CMD_RESULT_OK;
    sw_cmd_setdpd_t * xCmdData;
    eRficFR1Mode mode = pYucInfo->eFR1Mode;

    if ( mode !=  path )
    {
        if ( ( mode == eFR1Mode2t2r0 ) || ( mode == eFR1Mode2t2r1 ) )
        {
            if ( !(mode & path) )
            {
                RF_LOGERRMSG("User input given srx path is invalid for current mode ");
                return RF_SW_CMD_RESULT_INVALID_DPDPATH;
            }
        }
        else if ( ( mode != eFR1Mode4t4r ) )
        {
            RF_LOGERRMSG("User input given srx path is invalid for current mode ");
            return RF_SW_CMD_RESULT_INVALID_DPDPATH;
        }
    }

    RF_CHECK_HANDLE_N_GET_CMD_DESC;
    pDesc->cmd = RF_SWCMD_SETDPD;
    xCmdData = ( sw_cmd_setdpd_t * ) ( &pDesc->data );
    xCmdData->enable = enable;
    xCmdData->dpd_path = path;
    RFCMDEXECUTE;
    RF_LOGDBG( " xRet: %d \n", xRet );
    return xRet;
#else
    UNUSED(xHandle);
    UNUSED(path);
    UNUSED(enable);
    return RF_SW_CMD_RESULT_NOT_IMPLEMENTED;
#endif
}

int32_t rfic_enable_trx(RficHandle_t xHandle)
{
    int32_t xRet = RF_SW_CMD_RESULT_OK;

    RF_CHECK_HANDLE_N_GET_CMD_DESC;

    pDesc->cmd = RF_SWCMD_SETPATH;

    RFCMDEXECUTE;
    return xRet;
}

int32_t rfic_get_rx_gain_idx( RficHandle_t xHandle )
{
    /* TODO Change this to swcmd */
    int32_t xRxGainCurIdx = -1;
#ifdef YUCCA_RF
    yucCurData_t *yuc1data = (yucCurData_t *)xHandle->rfic_priv;
    yucCurData_t *yuc2data = (yucCurData_t *)(xHandle->rfic_priv + sizeof(yucCurData_t));
    if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
    {
        xRxGainCurIdx = yuc1data->rxGainIdx;
    }
    else
    {
        xRxGainCurIdx = yuc2data->rxGainIdx;
    }
#elif ICEWINGS_RF
    icwCurData_t *icwdata = (icwCurData_t *)xHandle->rfic_priv;
    xRxGainCurIdx = icwdata->rxgain_idx;
#endif
    return xRxGainCurIdx;
}

int32_t rfic_get_tx_gain_idx( RficHandle_t xHandle )
{
    /* TODO Change this to swcmd */
    int32_t xTxGainCurIdx = -1;
#ifdef YUCCA_RF
    yucCurData_t *yuc1data = (yucCurData_t *)xHandle->rfic_priv;
    yucCurData_t *yuc2data = (yucCurData_t *)(xHandle->rfic_priv + sizeof(yucCurData_t));
    if ( xHandle->eFR1Mode & eFR1Mode2t2r0 )
    {
        xTxGainCurIdx = yuc1data->txGainIdx;
    }
    else
    {
        xTxGainCurIdx = yuc2data->txGainIdx;
    }
#elif ICEWINGS_RF
    icwCurData_t *icwdata = (icwCurData_t *)xHandle->rfic_priv;
    xTxGainCurIdx = icwdata->txgain_idx;
#endif
    return xTxGainCurIdx;
}

int32_t *rfic_get_rx_gain_tbl( RficHandle_t xHandle, uint32_t *pTblSize)
{
    UNUSED(xHandle);

    *pTblSize = gRXGainLUTsize;
    return gRXGainLUT;
}

int32_t *rfic_get_tx_gain_tbl( RficHandle_t xHandle, uint32_t *pTblSize)
{
    UNUSED(xHandle);

    *pTblSize = gTXGainLUTsize;
    return gTXGainLUT;
}
