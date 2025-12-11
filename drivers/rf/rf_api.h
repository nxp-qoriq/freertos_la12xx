// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __RF_API_H__
#define __RF_API_H__

/**
 * @file        rf_api.h
 * @brief       This file contains the RF-related APIs
 * @addtogroup  RF_API
 * @{
 */
#include "Time.h"
#include "rf_dev.h"
#include "types.h"
#include <rf_sw_cmds.h>

volatile rf_sw_cmd_desc_t *xCheckHandleGetDesc(RficHandle_t xHandle, struct Time *xAPIEntryTime);

#define RF_CHECK_HANDLE_N_GET_CMD_DESC                                \
    struct Time xAPIEntryTime;                                         \
    volatile rf_sw_cmd_desc_t *pDesc = xCheckHandleGetDesc(xHandle, &xAPIEntryTime); \
    if ( pDesc == NULL ) \
        return RF_SW_CMD_RESULT_DESC_INVALID;

#define RFCMDEXECUTE \
    xRet = lRFCmdExecute( xHandle, pDesc, &xAPIEntryTime );

/** APIs with doxygen style comments can be used by L1C */

/**
 * @brief Initialize FR1 APIs.
 *
 * @param devtype ::eRficDevType type input parameter to select one of the below modes.
 *          eFR1_4T_4R_L -> Select FR1 chip of lower geul
 *          eFR1_4T_4R_R -> Select FR1 chip of upper geul
 *          eFR1_8T8R -> Select FR1 chips of both geuls
 *          eFR2_2T2R -> Select FR2 chip
 *
 * @return RficHandle_t type pointer which must be used for all following API invocations.
 *         NULL if FR1 is not in operating mode or any errors while initializing FR1 chips.
 */
RficHandle_t rfic_api_init( eRficDevType devtype );

/**
 * @brief Select set of transceivers to be activated for operation
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 * @param eMode ::eRficFR1Mode type input parameter to select one of the transceiver modes.
 *          eFR1Mode1t1r0 -> Select FR1 chip 1 with TX1 and RX1 paths
 *          eFR1Mode1t1r1 -> Select FR1 chip 1 with TX2 and RX2 paths
 *          eFR1Mode1t1r2 -> Select FR1 chip 2 with TX1 and RX1 paths
 *          eFR1Mode1t1r3 -> Select FR1 chip 2 with TX2 and RX2 paths
 *          eFR1Mode2t2r0 -> Select FR1 chip 1 with TX1, RX1, TX2, and RX2 paths
 *          eFR1Mode2t2r1 -> Select FR1 chip 2 with TX1, RX1, TX2, and RX2 paths
 *          eFR1Mode4t4r  -> Select both FR1 chips with TX1, RX1, TX2 and RX2 paths
 * @param duplexMode :: eRficFR1DupMode type input parameter which enables duplex mode TDD or FDD
 *
 * @return pdFAIL when failure, pdPASS when success
 */
int32_t rfic_change_mode( RficHandle_t xHandle,
                       eRficFR1Mode eMode, eRficFR1DupMode duplexMode );

/**
 * @brief Set frequency of FR1 chips which are selected using ::rfic_change_mode.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 * @param freq_khz Frequency in kHz to set.
 * @param trxpath :: eRficFR1TXRX type input parameter used to set for RX or TX.
 *
 * @return pdFAIL when failure, pdPASS when success
 */
int32_t rfic_adjust_pll_freq( RficHandle_t xHandle,
                              uint32_t freq_khz , eRficFR1TXRX trxpath );

/**
 * @brief Enable antenna in TX mode for all transceivers selected using ::rfic_change_mode.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 *
 * @return pdFAIL when failure, pdPASS when success
 */
int32_t rfic_switch_tx( RficHandle_t xHandle );

/**
 * @brief Enable antenna in RX mode for all transceivers selected using ::rfic_change_mode.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 *
 * @return pdFAIL when failure, pdPASS when success
 */
int32_t rfic_switch_rx( RficHandle_t xHandle );

/**
 * @brief Enable TX and RX paths of selected TRX using ::rfic_change_mode.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 *
 * @return pdFAIL when failure, pdPASS when success
 */
int32_t rfic_enable_trx(RficHandle_t xHandle);

/**
 * @brief Check PllStatus of FR1 chips which are selected using ::rfic_change_mode.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 *
 * @param xPllData sw_cmd_pll_status_t type pointer to hold both FR1 chips PllStatus info
 *
 * @return pdFAIL when failure, pdPASS when success
 */
uint32_t rfic_pll_status( RficHandle_t xHandle, sw_cmd_pll_status_t *xPllData );

/**
 * @brief Change TX gain by certain dB using ::rfic_ctrl_relative_tx_gain.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 * @param ucGainIndB Gain change required in multiple of 0.1 dB.
 * @param psEnforcedGain: Pointer which return the applied gain change(unit = 0.1dB)

 * @return error_code when failure, 0 when success.
 */
int32_t rfic_ctrl_relative_tx_gain( RficHandle_t xHandle,
                          int32_t ucGainIndB ,int32_t *psEnforcedGain);

/**
 * @brief Change Rx gain by certain dB using ::rfic_ctrl_relative_rx_gain.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 * @param ucGainIndB Gain change required in multiple of 0.1 dB.
 * @param psEnforcedGain: Pointer which return the applied gain change(unit = 0.1dB)
 *
 * @return error_code when failure, 0 when success
 */
int32_t rfic_ctrl_relative_rx_gain( RficHandle_t xHandle,
                          int32_t ucGainIndB ,int32_t *psEnforcedGain);

/**
 * @brief Change SRx gain by certain dB using ::rfic_ctrl_relative_srx_gain.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 * @param ucGainIndB Gain change required in multiple of 0.1 dB.
 * @param psEnforcedGain: Pointer which return the applied gain change(unit = 0.1dB)
 *
 * @return error_code when failure, 0 when success
 */
int32_t rfic_ctrl_relative_srx_gain( RficHandle_t xHandle,
                          int32_t ucGainIndB ,int32_t *psEnforcedGain);

/**
 * @brief Enable SRX Observation Path.
 *
 * @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
 * @param path rf_srx_path_t type, RX path [1|2|4|8|3|12|15] for SRX Observation.
 * @param enable bool type, 0 for off and 1 for on srx.
 *
 * @return error_code when failure, 0 when success
 */
int32_t rfic_set_srx( RficHandle_t xHandle,
                              rf_srx_path_t path, bool enable );

/**
* @brief API to get current RX gain index of RX gain table
*
* @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
*
* @return Positive number in case of success, -1 incase of any error
*/
int32_t rfic_get_rx_gain_idx( RficHandle_t xHandle );

/**
* @brief API to get current TX gain index of RX gain table
*
* @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
*
* @return Positive number in case of success, -1 incase of any error
*/
int32_t rfic_get_tx_gain_idx( RficHandle_t xHandle );

/**
* @brief API to get RX gain table
*
* @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
* @param[out] pTblSize Size of the returned array of integers
*
* @return Pointer to RX gain table in case of success, NULL in case of error
*/
int32_t *rfic_get_rx_gain_tbl( RficHandle_t xHandle, uint32_t *pTblSize);

/**
* @brief API to get TX gain table
*
* @param xHandle RficHandle_t type pointer obtained by ::rfic_api_init
* @param[out] pTblSize Size of the returned array of integers
*
* @return Pointer to TX gain table in case of success, NULL in case of error
*/
int32_t *rfic_get_tx_gain_tbl( RficHandle_t xHandle, uint32_t *pTblSize);

int32_t lRFCmdExecute( RficHandle_t xHandle,
                       volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc,
                       struct Time * pxAPIEntryTime );

volatile rf_sw_cmd_desc_t * xGetRFSWCmdDesc( RficHandle_t xHandle,
                                             uint32_t ulTimeoutInUS );

static inline void rf_start_latency_calc( struct Time * pxAPIEntryTime )
{
    vGetCurrentTimeMPICGB( pxAPIEntryTime );
}

static inline void rf_end_latency_calc_noframework( RficHandle_t xHandle,
                                                    uint32_t core_id,
                                                    uint32_t cmd,
                                                    struct Time * pxAPIEntryTime )
{
    xHandle->pxStats->cmd_latencies[ core_id ][ cmd ] =
        ulGetElapsedTimeMPICGB( pxAPIEntryTime );

    if( xHandle->pxStats->cmd_latencies[ core_id ][ cmd ] >
        xHandle->pxStats->max_cmd_latencies[ core_id ][ cmd ] )
    {
        xHandle->pxStats->max_cmd_latencies[ core_id ][ cmd ] =
            xHandle->pxStats->cmd_latencies[ core_id ][ cmd ];
    }
}

static inline void rf_end_latency_calc( RficHandle_t xHandle,
                                        volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc,
                                        struct Time * pxAPIEntryTime )
{
    xHandle->pxStats->cmd_latencies[ pxRFSWCmdDesc->core_id ][ pxRFSWCmdDesc->cmd ] =
        ulGetElapsedTimeMPICGB( pxAPIEntryTime );

    if( xHandle->pxStats->cmd_latencies[ pxRFSWCmdDesc->core_id ][ pxRFSWCmdDesc->cmd ] >
        xHandle->pxStats->max_cmd_latencies[ pxRFSWCmdDesc->core_id ][ pxRFSWCmdDesc->cmd ] )
    {
        xHandle->pxStats->max_cmd_latencies[ pxRFSWCmdDesc->core_id ][ pxRFSWCmdDesc->cmd ] =
            xHandle->pxStats->cmd_latencies[ pxRFSWCmdDesc->core_id ][ pxRFSWCmdDesc->cmd ];
    }
}

/** @} */

#endif /* ifndef __RF_API_H__ */
