// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

/* #include "FreeRTOS.h" */
/* #include "queue.h" */
/* #include "event_groups.h" */
/* #include "rf_config.h" */
#include "rf_dev.h"
#include "rf_core.h"
#include "rf_common.h"
/* #include "types.h" */
/* #include "rf_hif.h" */
#include "rf_remote_isr.h"
/* #include "gul_host_if.h" */
/* #include "debug_console.h" */
/* #include "mpic.h" */

static uint32_t ulE200Cores[E200_CORE_COUNT] = { 0 };

int32_t iRemoteISRSetup( RficHandle_t xHandle )
{
	uint32_t i;
	int32_t iRet;
	uint32_t msi_index = 0;
	volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc;

	for( i = 0; i < E200_CORE_COUNT; i++ )
	{
		if( i != RF_LOCAL_CORE )
		{
			pxRFSWCmdDesc = &( xHandle->pxPrvRFMData->swcmd_descs[ i ] );
			pxRFSWCmdDesc->ipi_event_id = MSI_RF_1 + msi_index;
			ulE200Cores[pxRFSWCmdDesc->ipi_event_id - MSI_RF_1] = i;
			msi_index += 1;
		}
	}
	msi_index = 0;

	for( i = 0; i < RF_TOTAL_USERS; i++ )
	{
		if( i != RF_LOCAL_CORE )
		{
			iRet = lRegisterIrq( MSIB_INTR_START + MSI_RF_1 + msi_index, bRFRemoteISR, ( void * )xHandle );

			if( iRet != 1 )
			{
				RF_LOGERRMSG( "Remote RF IRQ register error" );
				return iRet;
			}

			bMpicEnable( DEVICE_SHARE_MESSAGE_B, MSI_RF_1 + msi_index );
			msi_index++;
		}
	}

	return 0;
}

bool_t bRFRemoteISR( uint32_t ulIrqNo, void * pvDevData )
{
    uint32_t ulMSIOffset = 0x10;
    uint32_t ulMSINumber = ulIrqNo - MSIB_INTR_START;
    RficHandle_t xHandle = pvDevData;
    volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint32_t ulCoreId;
	uint32_t ulEventId;

	if( ulMSINumber == HOST_MSI_RF )
	{
		ulCoreId = RF_HOST_CORE_ID;
		pxRFSWCmdDesc = &( xHandle->pxRFMData->host_swcmd );
	}
	else
	{
		ulCoreId = ulE200Cores[ulMSINumber - MSI_RF_1];
		pxRFSWCmdDesc = &( xHandle->pxPrvRFMData->swcmd_descs[ ulCoreId ] );
	}
	ulEventId = 1 << ulCoreId;

    RF_STATS_ADD( xHandle->pxStats->remote_isr_hit_count[ ulCoreId ] );

    if( xQueueSendFromISR( xHandle->xRFRemoteQueue[ ulCoreId ], &pxRFSWCmdDesc, NULL ) == pdPASS )
    {
        if( xEventGroupSetBitsFromISR( xHandle->xRFCoreTaskEventGrp, ulEventId, &xHigherPriorityTaskWoken ) == pdPASS )
        {
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
        else
        {
            RF_LOGERR( "Cmd set event bits failed for coreid : %d",ulCoreId );
            RF_STATS_ADD( xHandle->pxStats->remote_isr_event_send_failed[ ulCoreId ] );
        }
    }
    else
    {
        RF_LOGERR( "Cmd enqueue failed for coreid : %d", ulCoreId );
        RF_STATS_ADD( xHandle->pxStats->remote_isr_enqueue_failed[ ulCoreId ] );
    }

    /* receive msg from interrupt, read will also clear interrupt */
    mpic_in32( MPIC_REGS_MSIRB0 + (ulMSINumber * ulMSIOffset) );

    return true;
}

