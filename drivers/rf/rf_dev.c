// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2023 NXP
 */

#include "rf_remote_isr.h"
#include "rf_common.h"
#include "rf_dev.h"
#include "rf_core.h"
#include "pmux.h"
#include "gpio.h"
#include <stdint.h>

#ifdef APIPLAYER_ENABLED
#include "apiplayer.h"
#endif

#if YUCCA_RF
#include "yuc_init.h"
#include "yuc_rfic_cmd.h"
#elif ICEWINGS_RF
#include "icw_cmd.h"
#include "icewings_init.h"
#endif

static RficHandle_t pxRFDevice = NULL;
RficHandle_t xRFGetDeviceHandle( void )
{

    if( ( pxRFDevice->eState[ RF_LOCAL_CORE ] == eRFDeviceInitialized )
#if YUCCA_RF
            && ( CHK_HIF_HOST_RDY( pGulModPriv->pHif, HIF_HOST_READY_RFIC ) )
            && ( CHK_HIF_HOST_RDY( pGulModPriv->pHif, HIF_HOST_READY_RFIC2) )
#endif
    )
            
    {
        return pxRFDevice;
    }
    else
    {
        RF_LOGDBG( " %s() eState:%x,%x HIF:%x \n\r", __func__,  
                pxRFDevice->eState[ RF_LOCAL_CORE ], eRFDeviceInitialized, 
                pGulModPriv->pHif->host_ready );
        return NULL;
    }
}

void xRFDevicePreInit( void )
{
    if( sizeof( RFDevice_t ) > RF_DEVICE_SIZE )
    {
        RF_LOGERRMSG("RF device size error");
        while ( 1 );
    }
    pxRFDevice = ( RFDevice_t * ) ( pGulModPriv->pHif->rf_hif.rf_prv_mdata.rfdev );
    memset( ( void * )pxRFDevice, 0, sizeof( RFDevice_t ) );
    pxRFDevice->pxStats = &( ( pGulModPriv->pHif->rf_hif ).stats.ep_stats );
    pxRFDevice->pxRFMData = &( pGulModPriv->pHif->rf_hif.rf_mdata );
    pxRFDevice->pxPrvRFMData = &( pGulModPriv->pHif->rf_hif.rf_prv_mdata );
    pxRFDevice->rfic_priv = (pGulModPriv->pHif->rf_hif.rf_prv_mdata.rficdev);
    pxRFDevice->eFR1Mode = eFR1Mode1t1r0;
}

int32_t iHandleRFCmds( RficHandle_t xHandle,
                       rf_sw_cmd_desc_t * pxRFSWCmdDesc )
{
    /**
     * TODO call rfic respective command handlers from here based on information
     * or do it some other generic way. presently calling yucca cmd handler.
     */
#ifdef APIPLAYER_ENABLED
    uint32_t *scraddr = NULL;
    if (pxRFSWCmdDesc->cmd == RF_SW_CMD_APIPLAYER)
    {
        scraddr = (uint32_t *)(pxRFSWCmdDesc->data);
        xStartAPIPlayer(*scraddr);
        return RF_SW_CMD_RESULT_OK;
    }
#else
    if (pxRFSWCmdDesc->cmd == RF_SW_CMD_APIPLAYER)
    {
        return RF_SW_CMD_RESULT_APIPLAYER_DISABLED;
    }
#endif

#ifdef ICEWINGS_RF
    return iIcwHandleCmd( xHandle, pxRFSWCmdDesc );
#endif
#ifdef YUCCA_RF
    return iYucHandleCmd( xHandle, pxRFSWCmdDesc );
#else
	return -1;
#endif
}


RficHandle_t xRFDeviceInit( void )
{
	uint32_t ulCoreId = ulMpicCurrentCore();
	uint32_t i;
	volatile rf_sw_cmd_desc_t *pxHostDesc;

	pxRFDevice = ( RFDevice_t * ) ( pGulModPriv->pHif->rf_hif.rf_prv_mdata.rfdev );

	pxRFDevice->pxPrvRFMData->swcmd_descs[ ulCoreId ].core_id = ulCoreId;
	pxRFDevice->xRFSWCmdDescSema[ ulCoreId ] = xSemaphoreCreateBinary();

	if( pxRFDevice->xRFSWCmdDescSema[ ulCoreId ] == NULL )
	{
		RF_DEV_INIT_DEATH_LOOP( ulCoreId );
	}
	else
	{
		xSemaphoreGive( pxRFDevice->xRFSWCmdDescSema[ ulCoreId ] );
		pxRFDevice->eState[ ulCoreId ] = eRFCmdDescSemaphoreCreated;
		RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );
	}

	/* Wait for the local core to complete the RF setup */
	if (ulCoreId != RF_LOCAL_CORE)
	{
		while( pxRFDevice->eState[ RF_LOCAL_CORE ] != eRFDeviceInitialized )
		{
			sync();
		}

		pxRFDevice->eState[ ulCoreId ] = eRFDeviceInitialized;
		RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );

		return pxRFDevice;
	}

	pxHostDesc = &( pxRFDevice->pxRFMData->host_swcmd );
	pxHostDesc->core_id = RF_HOST_CORE_ID;

	pxRFDevice->xRFLocalQueue = xQueueCreate( RF_LOCAL_QUEUE_LENGTH, sizeof( uint32_t * ) );

	if( pxRFDevice->xRFLocalQueue == NULL )
	{
		RF_DEV_INIT_DEATH_LOOP( ulCoreId );
	}
	else
	{
		pxRFDevice->eState[ ulCoreId ] = eRFLocalQueueCreated;
		RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );
	}

	for( i = 0; i < RF_TOTAL_USERS; i++ )
	{
		if( i != RF_LOCAL_CORE )
		{
			pxRFDevice->xRFRemoteQueue[ i ] = xQueueCreate( RF_REMOTE_QUEUE_LENGTH, sizeof( uint32_t * ) );

			if( pxRFDevice->xRFRemoteQueue[ i ] == NULL )
			{
				RF_DEV_INIT_DEATH_LOOP( ulCoreId );
			}
		}
	}

	pxRFDevice->eState[ ulCoreId ] = eRFRemoteQueueCreated;
	RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );

	pxRFDevice->xRFLocalQueueResp = xQueueCreate( RF_LOCAL_RESP_QUEUE_LENGTH, sizeof( uint32_t * ) );

	if( pxRFDevice->xRFLocalQueueResp == NULL )
	{
		RF_DEV_INIT_DEATH_LOOP( ulCoreId );
	}
	else
	{
		pxRFDevice->eState[ ulCoreId ] = eRFLocalRespQueueCreated;
		RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );
	}

	pxRFDevice->xRFCoreTaskEventGrp = xEventGroupCreate();

	if( pxRFDevice->xRFCoreTaskEventGrp == NULL )
	{
		RF_DEV_INIT_DEATH_LOOP( ulCoreId );
	}
	else
	{
		pxRFDevice->eState[ ulCoreId ] = eRFCoreTaskEventGrpCreated;
		RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );
	}

	RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );

	if( xTaskCreate( vRFCoreTask,
			 "RFcoreTask",
			 RF_CORE_TASK_STACK_SIZE,
			 ( void * )pxRFDevice,
			 RF_CORE_TASK_PRIORITY,
			 ( TaskHandle_t * )&( pxRFDevice->xRFCoreTask ) ) == pdPASS )
	{
		pxRFDevice->eState[ ulCoreId ] = eRFCoreTaskCreated;
		RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );
	}
	else
	{
		RF_DEV_INIT_DEATH_LOOP( ulCoreId );
	}

	if( iRemoteISRSetup( pxRFDevice ) )
	{
		RF_DEV_INIT_DEATH_LOOP( ulCoreId );
	}

	vSwitchLSToGpio();

    SET_HIF_MOD_RDY( pGulModPriv->pHif, HIF_MOD_READY_RFIC );
	/* TBD: Allocate IPI events and put it in respective descriptors */

	pxRFDevice->eState[ ulCoreId ] = eRFDeviceInitialized;
	RF_STATS_SET_VALUE( pxRFDevice->pxStats->init_state[ ulCoreId ], pxRFDevice->eState[ ulCoreId ] );

#ifdef YUCCA_RF
    /* Yucca RF intialization
     * return success - In case of RF card is not detected
     * return fail - Card detected and encounter error during init
     * */
    pxRFDevice->pxRFMData->rfic_type=RFIC_TYPE_YUCCA;
    if ( 0 != iYucInit((RficHandle_t)pxRFDevice))
    {
        RF_LOGERRMSG( "Yucca Init failed");
    }

#elif ICEWINGS_RF
    pxRFDevice->pxRFMData->rfic_type=RFIC_TYPE_ICW;
    if ( 0 != iIceWingsInit() )
    {
        RF_LOGERRMSG( "IceWings device Init failed");
    }
    if ( 0 != iIcwInit((RficHandle_t)pxRFDevice))
    {
        RF_LOGERRMSG( "IceWings Internal Init failed");
    }
#else
	pxRFDevice->pxRFMData->rfic_type=RFIC_TYPE_NONE;
#endif

	return pxRFDevice;
}
