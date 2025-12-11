// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#include "projdefs.h"
#include "rf_dev.h"
#include "rf_core.h"
#ifdef YUCCA_RF
#include "yuc_rfic_cmd.h"
#endif

#include "pmc.h"

#ifdef CONFIG_L1C_ENABLE
#include "l1ca_trace.h"
#endif

void vRFCoreTask( void * pvParameters )
{
    RficHandle_t xHandle = pvParameters;
    rf_sw_cmd_desc_t * pxRFSWCmdDesc = NULL;
    EventBits_t uxBits;
    uint32_t ulEventCore;
    int32_t iRet;

#ifdef CONFIG_L1C_ENABLE
    vTaskSetApplicationTaskTag(NULL, (void *)BSP_RF_TASK);
#endif

    while( 1 )
    {
        RF_STATS_SET_VALUE( xHandle->pxStats->core_task_state, 1 );
        uxBits = xEventGroupWaitBits( xHandle->xRFCoreTaskEventGrp,
                                      RF_CORE_TASK_EVENT_MASK,
                                      pdTRUE,  /* Clear all the events before returning */
                                      pdFALSE, /* Any one event should make this API return */
                                      portMAX_DELAY );

#ifdef CONFIG_L1C_ENABLE
        volatile uint32_t start_ts = PMC_CTR_READ(PMR_PMC0);
#endif

        RF_STATS_SET_VALUE( xHandle->pxStats->core_task_state, 2 );
        /* Not present in stats for la1224 */
        /* RF_STATS_SET_VALUE( xHandle->pxStats->rf_last_core_task_events, uxBits ); */

        if( uxBits & RF_LOCAL_CMD_EVENT )
        {
            RF_STATS_ADD( xHandle->pxStats->core_task_local_cmd_count );

            if( xQueueReceive( xHandle->xRFLocalQueue, &pxRFSWCmdDesc, 0 ) == pdPASS )
            {
                iRet = iHandleRFCmds( xHandle, pxRFSWCmdDesc );
                pxRFSWCmdDesc->result = iRet;
                pxRFSWCmdDesc->status = RF_SW_CMD_STATUS_DONE;

                if( iRet == RF_SW_CMD_RESULT_CMD_INVALID )
                {
                    RF_STATS_ADD( xHandle->pxStats->local_invalid_cmd_count );
                }

                if( iRet == RF_SW_CMD_RESULT_DESC_INVALID )
                {
                    RF_STATS_ADD( xHandle->pxStats->local_desc_errors );
                }

                if( xQueueSend( xHandle->xRFLocalQueueResp, &iRet, 0 ) == pdFAIL )
                {
                    RF_STATS_ADD( xHandle->pxStats->local_resp_send_failed );
                }

                RF_STATS_SET_VALUE( xHandle->pxStats->desc_latest_result[ pxRFSWCmdDesc->core_id ], ( uint32_t ) iRet );
            }
            else
            {
                RF_STATS_ADD( xHandle->pxStats->local_queue_recv_failed );
            }
        }

        if( uxBits & RF_CORE_REMOTE_EVENT_MASK )
        {
            RF_STATS_ADD( xHandle->pxStats->core_task_remote_cmd_count );
            ulEventCore = 0;

            while( uxBits && ( ulEventCore <= RF_HOST_CORE_ID ) )
            {
                if( uxBits & ( 1ul << ulEventCore ) )
                {
                    if( ulEventCore != RF_LOCAL_CORE )
                    {
                        if( xQueueReceive( xHandle->xRFRemoteQueue[ ulEventCore ], &pxRFSWCmdDesc, 0 ) == pdPASS )
                        {
                            iRet = iHandleRFCmds( xHandle, pxRFSWCmdDesc );
                            pxRFSWCmdDesc->result = iRet;
                            pxRFSWCmdDesc->status = RF_SW_CMD_STATUS_DONE;

                            if( iRet == RF_SW_CMD_RESULT_CMD_INVALID )
                            {
                                RF_STATS_ADD( xHandle->pxStats->remote_invalid_cmd_count[ ulEventCore ] );
                            }

                            if( iRet == RF_SW_CMD_RESULT_DESC_INVALID )
                            {
                                RF_STATS_ADD( xHandle->pxStats->remote_desc_errors );
                            }

                            RF_STATS_SET_VALUE( xHandle->pxStats->desc_latest_result[ pxRFSWCmdDesc->core_id ], ( uint32_t ) iRet );
                        }
                        else
                        {
                            RF_STATS_ADD( xHandle->pxStats->remote_queue_recv_failed[ ulEventCore ] );
                        }
                    }
                    else
                    {
                        /* This shouldn't happen */
                        RF_STATS_ADD( xHandle->pxStats->remote_event_failure );
                    }

                    uxBits &= ~( 1ul << ulEventCore );
                }

                ulEventCore++;
            }
        }
                
#ifdef CONFIG_L1C_ENABLE
        l1ca_trace_event(PERF_RF_TASK, 0, 0, 0, 0, PMC_CTR_READ(PMR_PMC0) - start_ts);
#endif
    }
}
