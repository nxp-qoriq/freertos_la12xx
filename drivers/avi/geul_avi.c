// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "debug_console.h"
#include "spinlock_api.h"
#ifdef BBDEV_IPC_MODE
#include "bbdev_ipc.h"
#else
#include "ipc.h"
#endif
#include "Time.h"

volatile struct gul_hif *pxHif = &ipc_hif_area;
/******************************************************************************
*************************** Global Parameters *********************************
******************************************************************************/
static AviIntr_t xVspaIntrNo[ VSPA_CORE_MAX ][ VSPA_GROUP_MAX ] =
						{	/*Group A*/    /*Group B */
							{   66,           67    }, /* VSPA 0 */
							{   69,           70    }, /* VSPA 1 */
							{   72,           73    }, /* VSPA 2 */
							{   75,           76    }, /* VSPA 3 */
							{   78,           79    }, /* VSPA 4 */
							{   81,           82    }, /* VSPA 5 */
							{   84,           85    }, /* VSPA 6 */
							{   87,           88    }, /* VSPA 7 */
						};
struct SpinLock *pxAviSpinLock;
AviHandle_t xAviHandle __attribute__ ((section (".smem")));
AviHandle_t *pxAviHandle = &xAviHandle;

/**********************************************************************************
*************************** 	Static API Functions 		 ******************
**********************************************************************************/
#if VSPA_BACKDOOR_HANDSHAKE
static void vGeulVSPABootHandshake( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore )
{

	VspaRegs_t *pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );
	uint32_t ulVspaReadMsb, ulVspaReadLsb;
	if( !( pxAviHandle->ucVspaHandshake & ( uint8_t )( 1 << eVspaCore ) ) )
	{
		/* Wait till status bit to get set */
		while( !( IN_32( &pxVspaRegs->ulHostMboxStatus ) & HOST_MBOX_MSG_IN_0_VALID ) );

		ulVspaReadMsb = IN_32( &pxVspaRegs->ulHostIn0Msb );
		ulVspaReadLsb = IN_32( &pxVspaRegs->ulHostIn0Lsb );

		if( VSPA_BOOT_OK != ulVspaReadMsb )
		{
			log_err( "%s: Error in receiving 0x%x\n\r", __func__, VSPA_BOOT_OK  );
		}

		log_dbg( "%s: MSB: 0x%x LSB: 0x%x\n\r",__func__, ulVspaReadMsb, ulVspaReadLsb );

		OUT_32( &pxVspaRegs->ulHostOut0Msb, SPM_BUFFER_BYTES );
		OUT_32( &pxVspaRegs->ulHostOut0Lsb, 0 );

		/* Wait till status bit to get set */
		while( !( IN_32( &pxVspaRegs->ulHostMboxStatus ) & HOST_MBOX_MSG_IN_0_VALID ) );

		ulVspaReadMsb = IN_32( &pxVspaRegs->ulHostIn0Msb );
		ulVspaReadLsb = IN_32( &pxVspaRegs->ulHostIn0Lsb );
		if( VSPA_SPM_BUF_ACK != ulVspaReadMsb )
		{
			log_err( "%s: Error in receiving 0x%x\n\r", __func__, VSPA_SPM_BUF_ACK );
		}
		log_dbg( "%s: MSB: 0x%x LSB: 0x%x\n\r",__func__, ulVspaReadMsb, ulVspaReadLsb );
		( void )ulVspaReadLsb;
		log_dbg( "%s: VSPA instance = %d handshake is done...\n\r", __func__, eVspaCore );

		pxAviHandle->ucVspaHandshake = ( pxAviHandle->ucVspaHandshake | ( uint8_t )( 1 << eVspaCore ) );
	}
}
#endif

/**********************************************************************************
*************************** 	API Functions 		 **************************
**********************************************************************************/
uint32_t uiCheckVSPABoot( VspaCore_t eVspaCore )
{
	VspaRegs_t *pxVspaRegs;

	if( eVspaCore >= VSPA_CORE_MAX )
	{
		log_err( "%s: Wrong Instanse\n", __func__ );
		return 0;
	}

	pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );
	return IN_32( &pxVspaRegs->ulSwVersion );
}

AviStatusCodes_t exGeulAviHostSendFastMboxToVspa( AviHandle_t *pxAviHandle,
		VspaCore_t eVspaCore,
		VspaMboxIndex_t eVspaMboxIndex,
		AviMboxData_t xAviMboxData )
{

	VspaRegs_t *pxVspaRegs = NULL;
	if( ( pxAviHandle == NULL ) || ( eVspaCore >= VSPA_CORE_MAX ) )
	{
		log_err( "AVI mailbox error, pxAviHandle = %p, eVspaCore = %d\n\r", pxAviHandle, eVspaCore );
		return AVI_INIT_NOT_DONE;
	}

	pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );
	log_dbg( "%s: vspa instance = %d base addr = %p\n\r", __func__, eVspaCore, pxVspaRegs );

	switch (eVspaMboxIndex) {
	case VSPA_MBOX_0:
		OUT_32( &pxVspaRegs->ulHostOut0Msb, xAviMboxData.ulMsb );
		sync(  );
		OUT_32( &pxVspaRegs->ulHostOut0Lsb, xAviMboxData.ulLsb );
		break;
	case VSPA_MBOX_1:
		OUT_32( &pxVspaRegs->ulHostOut1Msb, xAviMboxData.ulMsb );
		sync(  );
		OUT_32( &pxVspaRegs->ulHostOut1Lsb, xAviMboxData.ulLsb );
		break;
	default:
		log_err( "ERR: Invalid MboxIndex= %d\n\r", eVspaMboxIndex );
		return AVI_INVALID_MBOX_INDEX;
	}
#if GUL_AVI_STATS_ENABLE
	pxHif->stats.vspa_avi_stats[eVspaCore].avi_E200_mbox0_tx_cnt++;
#endif
	return AVI_SUCCESS;
}

AviStatusCodes_t exGeulAviHostSendSlowMboxToVspa( AviHandle_t *pxAviHandle,
		VspaCore_t eVspaCore,
		VspaMboxIndex_t eVspaMboxIndex,
		AviMboxData_t xAviMboxData )
{

	VspaRegs_t *pxVspaRegs = NULL;
	bool status = 1;

	if( ( pxAviHandle == NULL ) || ( eVspaCore >= VSPA_CORE_MAX ) )
	{
		log_err( "AVI mailbox error, pxAviHandle = %p, eVspaCore = %d\n\r", pxAviHandle, eVspaCore );
		return AVI_INIT_NOT_DONE;
	}

	pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );
	log_dbg( "%s: vspa instance = %d base addr = %p\n\r", __func__, eVspaCore, pxVspaRegs );

	/*Acquire spinlock */
	vSpinLockAcquire( pxAviHandle->pxMailboxSpinlock[ eVspaCore ] );
	switch (eVspaMboxIndex) {
	case VSPA_MBOX_0:
		OUT_32( &pxVspaRegs->ulHostOut0Msb, xAviMboxData.ulMsb );
		sync(  );
		OUT_32( &pxVspaRegs->ulHostOut0Lsb, xAviMboxData.ulLsb );
		break;
	case VSPA_MBOX_1:
		OUT_32( &pxVspaRegs->ulHostOut1Msb, xAviMboxData.ulMsb );
		sync(  );
		OUT_32( &pxVspaRegs->ulHostOut1Lsb, xAviMboxData.ulLsb );
		break;
	default:
		log_err( "ERR: Invalid MboxIndex= %d\n\r", eVspaMboxIndex );
		status = 0;
		break;
	}
	/* Release spinlock */
	vSpinLockRelease( pxAviHandle->pxMailboxSpinlock[ eVspaCore ] );
	if (!status)
		return AVI_INVALID_MBOX_INDEX;
#if GUL_AVI_STATS_ENABLE
	pxHif->stats.vspa_avi_stats[eVspaCore].avi_E200_mbox1_tx_cnt++;
#endif
	return AVI_SUCCESS;
}

AviStatusCodes_t exGeulAviHostHandleMboxIrq( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore,
                VspaMboxIndex_t eVspaMboxIndex, AviMboxData_t *pAviMboxData )
{
	AviStatusCodes_t exReturn = AVI_INIT_NOT_DONE;
	VspaRegs_t *pxVspaRegs = NULL;
	bool QueueInit =  pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaMboxIndex ].QueueInit;
	QueueHandle_t currHandle = NULL;

        /* Check if AVI init is done or not */
	if ( NULL == pxAviHandle || eVspaCore >= VSPA_CORE_MAX || eVspaMboxIndex >= VSPA_MBOX_MAX  )
	{
                log_err( "ERR: in AVI parameter : %p core = %u pAviMboxData = %u\n\r", pxAviHandle, eVspaCore, eVspaMboxIndex );
                exReturn = AVI_INIT_NOT_DONE;
                goto hndl_exReturn;
        }
	pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );
        if ( VSPA_MBOX_0 == eVspaMboxIndex )
	{
		if ( ( IN_32( &pxVspaRegs->ulVspaStatus ) & VSPA_MBOX0_STATUS ) == VSPA_MBOX0_STATUS )
		{
			pAviMboxData->ulMsb = IN_32( &pxVspaRegs->ulHostIn0Msb );
			pAviMboxData->ulLsb = IN_32( &pxVspaRegs->ulHostIn0Lsb );
			currHandle =  pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaMboxIndex ].VspaToCm4QMbox;

			if( QueueInit && (pdTRUE != xQueueSendToBackFromISR( currHandle,
							pAviMboxData, NULL)))
			{
				log_dbg( "ERR:%s: VspaToCm4QMbox0 Full or Not Initialized\n\r", __func__ );
			}

			OUT_32( &pxVspaRegs->ulVspaStatus, ( uint32_t )( VSPA_MBOX0_STATUS ) );
			exReturn = AVI_SUCCESS;
#if GUL_AVI_STATS_ENABLE
			pxHif->stats.vspa_avi_stats[eVspaCore].avi_E200_mbox0_rx_cnt++;
#endif
		}
		else
		{
			exReturn = AVI_NO_MESSAGE_IN_MBOX0;
		}
        }
	else
	{
		if ( ( IN_32( &pxVspaRegs->ulVspaStatus ) & VSPA_MBOX1_STATUS ) == VSPA_MBOX1_STATUS )
		{
			pAviMboxData->ulMsb = IN_32( &pxVspaRegs->ulHostIn1Msb );
			pAviMboxData->ulLsb = IN_32( &pxVspaRegs->ulHostIn1Lsb );
			currHandle =  pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaMboxIndex ].VspaToCm4QMbox;

			if( QueueInit && (pdTRUE != xQueueSendToBackFromISR( currHandle,
						pAviMboxData, NULL)))
			{
				log_dbg( "ERR:%s: VspaToCm4QMbox0 Full\n\r", __func__ );
			}
			OUT_32( &pxVspaRegs->ulVspaStatus, ( uint32_t )( VSPA_MBOX1_STATUS ) );
#if GUL_AVI_STATS_ENABLE
			pxHif->stats.vspa_avi_stats[eVspaCore].avi_E200_mbox1_rx_cnt++;
#endif
			exReturn = AVI_SUCCESS;
		}
		else
		{
			exReturn = AVI_NO_MESSAGE_IN_MBOX1;
		}
        }

hndl_exReturn:
        return exReturn;
}

void vGeulAviVspaHwVer( void )
{
	VspaRegs_t *pxVspaRegs = ( VspaRegs_t * )VSPA_BASE_ADDR;

	/* To remove unused variable warning */
	(void)pxVspaRegs;
	log_info( "INFO: VSPA Hw VER: 0x%x\n\r", pxVspaRegs->ulHwVersion );
}

AviStatusCodes_t exGeulRegisterVspaInterrupt( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore,
	VspaMboxIndex_t eVspaGroup, VspaCallbackFn pvIsr, VspaCallbackData pvData,
	int dir , bool QueueInit )
{
	VspaRegs_t *pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );
	AviStatusCodes_t exReturn= AVI_INIT_NOT_DONE;
	if( ( pxAviHandle == NULL ) || ( eVspaCore >= VSPA_CORE_MAX ) || ( eVspaGroup >= VSPA_GROUP_MAX ) )
        {
                log_err( "AVI mailbox error, pxAviHandle = %p, eVspaCore = %d\n\r", pxAviHandle, eVspaCore );
                return AVI_INIT_NOT_DONE;
        }

	if ( (dir & VSPA_MBOX_W) == VSPA_MBOX_W )
		QueueInit = false;

	if (QueueInit) {
		/*   Create VSPA to CM4 queues */
		pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].VspaToCm4QMbox
			= xQueueCreate( VSPA_CM4_Q_LEN, sizeof( AviMboxData_t ) );

		pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].QueueInit = QueueInit;
	}

	vSpinLockAcquire( pxAviHandle->pxMailboxSpinlock[ eVspaCore ] );

        if( pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqState == IRQ_UNREGISTERED )
        {
                AviIntr_t xIrqNo = pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo;
		/* Enable the irq_enable register bits */
		uint32_t ulRegRead = IN_32( &pxVspaRegs->ulVspaIrqEn );
		uint32_t ulMask = 0;

		if (dir & 0x1)
			ulMask |= ( eVspaGroup + 1 ) * IRQEN_EN_MBOX_R;

		if (dir & 0x2)
			ulMask |= ( eVspaGroup + 1 ) * IRQEN_EN_MBOX_W;

		uint32_t ulVspaStatusBits = eVspaGroup ? ( E200_MBOX1_STATUS | VSPA_MBOX1_STATUS ) : ( E200_MBOX0_STATUS | VSPA_MBOX0_STATUS );
		/* Clear if any pending vspa status bits */
		OUT_32( &pxVspaRegs->ulVspaStatus, ulVspaStatusBits );
		/* Enable IRQ_EN reg bits */
		OUT_32( &pxVspaRegs->ulVspaIrqEn, ( ulRegRead| ulMask ) );
                int ulReturn = lRegisterIrq( ( uint32_t )( INTERNAL_IRQ_OFFSET + xIrqNo ), pvIsr, pvData );
                if ( ulReturn > 0 )
                {
                        pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqState = IRQ_REGISTERED;
                        log_dbg( "%s: Registered core=%d,group=%d,irq=%d:%d,addr=0x%p\n\r",
			__func__, eVspaCore, eVspaGroup, pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo,
				pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo+INTERNAL_IRQ_OFFSET , pvIsr );

			bMpicEnable( DEVICE_INTERNAL, xIrqNo );
			exReturn = AVI_SUCCESS;
                }
                else
                {
                        log_err( "%s: IRQ registartion failure : VSPA core = %d, VSPA group = %d, irq = %d:%d\n\r",
                        __func__, eVspaCore, eVspaGroup, pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo,
			pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo + INTERNAL_IRQ_OFFSET );
			exReturn = AVI_IRQ_REG_FAILURE;
                }
        }
        else
        {
                log_dbg( "%s: IRQ is already registered : VSPA core = %d, VSPA group = %d, irq = %d:%d\n\r",
                __func__, eVspaCore, eVspaGroup, pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo,
			pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo + INTERNAL_IRQ_OFFSET );
		exReturn = AVI_IRQ_ALREADY_REGISTERED;
        }
	vSpinLockRelease( pxAviHandle->pxMailboxSpinlock[ eVspaCore ] );
        return exReturn;
}

AviStatusCodes_t exGeulUnRegisterVspaInterrupt( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore, VspaMboxIndex_t eVspaGroup, int dir )
{
        VspaRegs_t *pxVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );

	if( ( pxAviHandle == NULL ) || ( eVspaCore >= VSPA_CORE_MAX ) )
        {
                log_err( "AVI mailbox error, pxAviHandle = %p, eVspaCore = %d\n\r", pxAviHandle, eVspaCore );
                return AVI_INIT_NOT_DONE;
        }

	vSpinLockAcquire( pxAviHandle->pxMailboxSpinlock[ eVspaCore ] );
        if( pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqState == IRQ_REGISTERED )
        {
		uint32_t ulRegRead;
		uint32_t ulMask = 0;
		uint32_t ulVspaStatusBits;
                uint32_t xIrqNo = pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo;

		/* MPIC interrupt disable */
		bMpicDisable( DEVICE_INTERNAL, xIrqNo );
                vUnregisterIrq( xIrqNo + INTERNAL_IRQ_OFFSET );

                pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqState = IRQ_UNREGISTERED;
		ulVspaStatusBits = eVspaGroup ? ( E200_MBOX1_STATUS | VSPA_MBOX1_STATUS ) : ( E200_MBOX0_STATUS | VSPA_MBOX0_STATUS );
		/* Clear if any pending vspa status bits */
		OUT_32( &pxVspaRegs->ulVspaStatus, ulVspaStatusBits );
		/* Clear the irq_enable register bits */
		ulRegRead = IN_32( &pxVspaRegs->ulVspaIrqEn );
		if (dir & 0x1)
			ulMask |= ( eVspaGroup + 1 ) * IRQEN_EN_MBOX_R;

		if (dir & 0x2)
			ulMask |= ( eVspaGroup + 1 ) * IRQEN_EN_MBOX_W;
		OUT_32( &pxVspaRegs->ulVspaIrqEn, ( ulRegRead & ~ulMask ) );
                log_dbg( "%s: Unregistered VSPA core=%d, VSPA group=%d, irq=%d:%d\n\r",
			__func__, eVspaCore, eVspaGroup, pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo,
		pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].xIrqNo+INTERNAL_IRQ_OFFSET );
        }
	vSpinLockRelease( pxAviHandle->pxMailboxSpinlock[ eVspaCore ] );

	if (pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].QueueInit) {
		vQueueDelete(pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaGroup ].VspaToCm4QMbox);
	}
        return AVI_SUCCESS;
}


AviHandle_t *pxGeulAviGetHandle( void )
{
	AviHandle_t *pxAvihndlr = NULL;
	/* Global spinlock will be assigned only inside iGeulInit */
	if( pxAviSpinLock != NULL )
	{
		vSpinLockAcquire( pxAviSpinLock );
		pxAvihndlr = pxAviHandle->bVspaBoot ? pxAviHandle : ( AviHandle_t * )NULL;
		vSpinLockRelease( pxAviSpinLock );
	}
	return pxAvihndlr;
}

AviStatusCodes_t exGeulAviHostRecvMboxFromVspa(VspaCore_t eVspaCore, VspaMboxIndex_t eVspaMboxIndex, AviMboxData_t *mbox ) {

	QueueHandle_t currHandle = NULL;
	if (pxAviHandle == NULL) {
		log_err( "AVI mailbox error, pxAviHandle = %p\n\r",
				pxAviHandle);
		return AVI_INIT_NOT_DONE;
	}

	log_dbg("\nWaiting for malbox from VSPA... \n\r");

	if (eVspaMboxIndex == VSPA_MBOX_1) {
		currHandle =  pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaMboxIndex ].VspaToCm4QMbox;
		if( pdPASS == xQueueReceive( currHandle,
					(void *)mbox, portMAX_DELAY ))
		{
			log_dbg( "\nRcvd VSPA MBOX[%d] msb_lsb 0x%x 0x%x\n\r",
				eVspaMboxIndex, mbox->ulMsb, mbox->ulLsb);
		}

		if( mbox->ulLsb == 0x1 && mbox->ulMsb == 0x0B << 24 )
			ulVspaResp++;
		else {
			log_err( "\n%s: eVspaCore %d : response expected : ulMsb : 0x0B << 24 : ulLsb = 0x1, received ulMsb = 0x%x ulLsb = 0x%x\r\n", __func__, eVspaCore, mbox->ulMsb , mbox->ulLsb);
		}

	} else if (eVspaMboxIndex == VSPA_MBOX_0) {
	currHandle =  pxAviHandle->xVspaIntrNo[ eVspaCore ][ eVspaMboxIndex ].VspaToCm4QMbox;
		if( pdPASS == xQueueReceive( currHandle,
					(void *)mbox, portMAX_DELAY ))
		{
			log_dbg( "\nRcvd VSPA MBOX[%d] msb_lsb 0x%x 0x%x\n\r",
				eVspaMboxIndex, mbox->ulMsb, mbox->ulLsb);
		}

		if( mbox->ulLsb == 0x1 && mbox->ulMsb == 0x0A << 24 )
			ulVspaResp++;
		else if ((mbox->ulLsb == 0x01 && mbox->ulMsb == 0x0C << 24) ||
				(mbox->ulLsb == 0x01 && mbox->ulMsb == 0x0D << 24))
			ulOverlay_resp++;
		else {
			ulOverlay_resp = -1;
			log_err( "\n%s: eVspaCore %d : response expected : ulMsb : 0x0B << 24 : ul Lsb = 0x1, received ulMsb = 0x%x ulLsb = 0x%x\n\r", __func__, eVspaCore, mbox->ulMsb , mbox->ulLsb);
		}

	} else {
		log_err( "ERR: Invalid MBOX index [%d]\n\r", eVspaMboxIndex);
		return AVI_INVALID_MBOX_INDEX;
	}

	return AVI_SUCCESS;
}

AviHandle_t *pxGeulAviInit( TaskHandle_t xHandle )
{
	AviMboxData_t xAviMboxData;
	pxAviSpinLock = pxSpinLockGet( xHandle, &xAviMboxData, SPINLOCK_VSPA );
	uint32_t ulCount = 0;
	uint32_t ulVspaTry = 0;

	/* Acquire spinlock */
	for( ulCount = VSPA_CORE_0; ulCount < VSPA_CORE_MAX; ulCount++ )
	{
		ulVspaTry = 50;
		while( !uiCheckVSPABoot( ulCount ) )
		{
			log_info( "INFO: WAITING FOR VSPA FW TO LOAD\n\r" );
			ulVspaTry--;
			vUdelay(1000000);
			if( ulVspaTry == 0 )
				goto vspa_boot_error;
		}
	}
	log_dbg( "%s: Spinlock init = %u\n\r", __func__, pxAviHandle->bVspaBoot );
	vSpinLockAcquire( pxAviSpinLock );
	if( false == pxAviHandle->bVspaBoot )
	{
#if VSPA_BACKDOOR_HANDSHAKE
		pxAviHandle->ucVspaHandshake = 0;
#endif
		for( ulCount = VSPA_CORE_0; ulCount < VSPA_CORE_MAX; ulCount++ )
		{
#if VSPA_BACKDOOR_HANDSHAKE
			vGeulVSPABootHandshake( pxAviHandle, ulCount );
#endif
			pxAviHandle->pxMailboxSpinlock[ ulCount ] = pxSpinLockAlloc( xHandle, &xAviMboxData );
			/* Group A interrupt */
			pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_A ].xIrqNo = xVspaIntrNo[ ulCount ][ VSPA_GROUP_A ];
			pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_A ].xIrqState = IRQ_UNREGISTERED;
			log_dbg( "%s:VSPA core = %d, xIrqNo = %u, xIrqState =%u\n\r", __func__, ulCount,
					pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_A ].xIrqNo, pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_A ].xIrqState );
			/* Group B interrupt */
			pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_B ].xIrqNo = xVspaIntrNo[ ulCount ][VSPA_GROUP_B ];
			pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_B ].xIrqState = IRQ_UNREGISTERED;
			log_dbg( "%s:VSPA core = %d, xIrqNo = %u, xIrqState =%u\n\r", __func__, ulCount,
					pxAviHandle->xVspaIntrNo[ ulCount ][VSPA_GROUP_B ].xIrqNo, pxAviHandle->xVspaIntrNo[ ulCount ][ VSPA_GROUP_B ].xIrqState );
		}
		pxAviHandle->bVspaBoot = true;
	}
	for( ulCount = VSPA_CORE_0; ulCount < VSPA_CORE_MAX; ulCount++ )
	{
		log_dbg( "%s : Mailbox spinlock [%d] = %p\n\r", __func__, ulCount, pxAviHandle->pxMailboxSpinlock[ ulCount ] );
	}
	vSpinLockRelease( pxAviSpinLock );
	return pxAviHandle;
vspa_boot_error:
	log_err( "%s : VSPA FW is not booted, coming out of wait\n\r", __func__  );
	return ( AviHandle_t * )NULL;
}
