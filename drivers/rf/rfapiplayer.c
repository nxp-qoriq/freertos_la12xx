// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include "Time.h"
#include "rfapiplayer.h"
#include "rf_api.h"
#include "debug_console.h"

void rfgroup_api_player(apiplayer_api_t *api)
{
#ifndef ENABLE_RF
    api->api_status = API_GROUP_NOTIMPLEMENTED;
    return;
#endif
    RficHandle_t xHandle = rfic_api_init(api->params[0]);
    struct Time pxAPIEntryTime;
    sw_cmd_pll_status_t xPllData;
    /* int32_t result; */
    /* uint32_t retlen = 0; */
    /* uint32_t ret[8] = {0}; */
    vGetCurrentTimeMPICGB( &pxAPIEntryTime );
    switch (api->id) {
        case RF_API_CHANGE_MODE:
            api->result = rfic_change_mode(xHandle, api->params[0], api->params[1]);
            break;
        case RF_API_ADJUST_PLL_FREQ:
            api->result = rfic_adjust_pll_freq(xHandle, api->params[0], api->params[1]);
            break;
        case RF_API_SWITCH_TX:
            api->result = rfic_switch_tx(xHandle);
            break;
        case RF_API_SWITCH_RX:
            api->result = rfic_switch_rx(xHandle);
            break;
        case RF_API_ENABLE_TRX:
            api->result = rfic_enable_trx(xHandle);
            break;
        case RF_API_PLL_STATUS:
            api->result = rfic_pll_status(xHandle, &xPllData);
            api->retlen = 4;
            api->ret[0] = xPllData.yuc1_trxstatus;
            api->ret[1] = xPllData.yuc1_calstatus;
            api->ret[2] = xPllData.yuc2_trxstatus;
            api->ret[3] = xPllData.yuc2_calstatus;
            break;
        case RF_API_CTRL_RELATIVE_TX_GAIN:
            api->result = rfic_ctrl_relative_tx_gain(xHandle, api->params[0], &api->ret[0]);
            api->retlen = 1;
            break;
        case RF_API_CTRL_RELATIVE_RX_GAIN:
            api->result = rfic_ctrl_relative_rx_gain(xHandle, api->params[0], &api->ret[0]);
            api->retlen = 1;
            break;
        case RF_API_CTRL_RELATIVE_SRX_GAIN:
            api->result = rfic_ctrl_relative_srx_gain(xHandle, api->params[0], &api->ret[0]);
            api->retlen = 1;
            break;
        case RF_API_SET_SRX:
            api->result = rfic_set_srx(xHandle, api->params[0], api->params[1]);
            break;
        case RF_API_GET_RX_GAIN_IDX:
            api->result = rfic_get_rx_gain_idx(xHandle);
            break;
        case RF_API_GET_TX_GAIN_IDX:
            api->result = rfic_get_tx_gain_idx(xHandle);
            break;
        case RF_API_GET_TX_GAIN_TBL:
            api->result = (uint32_t)rfic_get_tx_gain_tbl(xHandle, (uint32_t *)&(api->ret[0]));
            api->retlen = 1;
            break;
        case RF_API_GET_RX_GAIN_TBL:
            api->result = (uint32_t)rfic_get_rx_gain_tbl(xHandle, (uint32_t *)&(api->ret[0]));
            api->retlen = 1;
            break;
        default:
            api->state = API_ID_UNKNOWN;
            break;
    }
    api->latency = ulGetElapsedTimeMPICGB( &pxAPIEntryTime );
}
