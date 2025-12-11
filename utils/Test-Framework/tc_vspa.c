// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#include "tc_vspa.h"

#if GEUL_VSPAMBOX_TEST
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "geul_error_codes.h"
#include "spinlock_api.h"
#include "spinlock.h"

#include "tc_avi.h"
/********************************************************************
*			Extern Variables
********************************************************************/
extern struct SpinLock * pxAviSpinLock;
/********************************************************************
*			Global Variables
********************************************************************/
uint32_t ulVspaResp;

/********************************************************************
*			Static function prototypes
********************************************************************/
static bool_t iGeulVspaCore( uint32_t ulIrq,VspaCore_t *peVspaCore );
/* For AVI demo we are using AVI Spinlock */

/********************************************************************
*		Static API function's
********************************************************************/
static bool_t iGeulVspaCore( uint32_t ulIrq, VspaCore_t *peVspaCore )
{
	ulIrq = ulIrq - INTERNAL_IRQ_OFFSET;
        switch( ulIrq )
        {
                case 66 :
                case 67 :
                        *peVspaCore = VSPA_CORE_0;
                        break;

                case 69 :
                case 70 :
                        *peVspaCore = VSPA_CORE_1;
                        break;

                case 72 :
                case 73 :
                        *peVspaCore = VSPA_CORE_2;
                        break;

                case 75 :
                case 76 :
                        *peVspaCore = VSPA_CORE_3;
                        break;

                case 78 :
                case 79 :
                        *peVspaCore = VSPA_CORE_4;
                        break;

                case 81 :
                case 82 :
                        *peVspaCore = VSPA_CORE_5;
                        break;

                case 84 :
                case 85 :
                        *peVspaCore = VSPA_CORE_6;
                        break;

                case 87 :
                case 88 :
                        *peVspaCore = VSPA_CORE_7;
                        break;

                default :
                        log_isr( "%s : Invalid interrupt number :%u\n\r", __func__, ulIrq );
                        return false;
        }
        return true;
}

/********************************************************************
*		API function's
********************************************************************/
#if GEUL_VSPAMBOX_TEST
int32_t iVSPAMBOXTest( void )
{
	AviMboxData_t xAviMboxSendData;
	uint32_t exVspaMboxIndex;
	AviHandle_t *pxAviHandle= NULL;
	int32_t iStatus = -1;
	uint32_t eVspaCore = 0;
	uint32_t ulCnt = 0;
	uint32_t ulErrCnt = 0;
	uint32_t ulSndCnt = 0;
	uint32_t ulLoopCnt = 0;
	pxAviHandle = pxGeulAviGetHandle(  );

	log_dbg( "%s: AVI Loop Test..Entry..\n\r", __func__ );
	if( NULL == pxAviHandle )
	{
		log_err( "%s: VSPA FW is not booted, AVI hndlr = %p\n\r", __func__, pxAviHandle );
		return 0;
	}
	for( ulLoopCnt = 0; ulLoopCnt < VSPA_MAX_LOOP; ulLoopCnt++ )
	{
		vSpinLockAcquire( pxAviSpinLock );
		for( eVspaCore = VSPA_CORE_0; eVspaCore < VSPA_CORE_MAX; eVspaCore++ )
		{
			exGeulRegisterVspaInterrupt( pxAviHandle, eVspaCore,
					VSPA_MBOX_0, bVSPA0GroupAInterrupt, pxAviHandle, VSPA_MBOX_RW, true);
			exGeulRegisterVspaInterrupt( pxAviHandle, eVspaCore,
					VSPA_MBOX_1, bVSPA0GroupBInterrupt, pxAviHandle, VSPA_MBOX_RW, false);
			for( ulCnt = 0; ulCnt < MAX_MBOX_SEND; ulCnt++ )
			{
				log_dbg( "VSPA INST = %d ulCnt = %d\n\r", eVspaCore, ulCnt );
				for( exVspaMboxIndex = 0; exVspaMboxIndex < NUMBER_OF_VSPA_MAILBOXES; exVspaMboxIndex++ )
				{
					/*Set the ulLsb value */
					xAviMboxSendData.ulLsb = OVERLAY_SECTION_OFFSET;         /* For offset value */
					ulSndCnt++;

					switch( exVspaMboxIndex )
					{
						case VSPA_MBOX_0:
							xAviMboxSendData.ulMsb = 0x0A << 24;                     /*Opcode */
							iStatus =exGeulAviHostSendFastMboxToVspa ( pxAviHandle, eVspaCore, exVspaMboxIndex, xAviMboxSendData );
							if ( AVI_SUCCESS != iStatus )
							{
								ulErrCnt++;
								log_err( "ERR: Fail to send MBox%d message iStatus = %d\n\r", exVspaMboxIndex, iStatus );
								break;
							}
							break;
						case VSPA_MBOX_1:
							xAviMboxSendData.ulMsb = 0x0B << 24 ;
							iStatus =exGeulAviHostSendSlowMboxToVspa ( pxAviHandle, eVspaCore, exVspaMboxIndex, xAviMboxSendData );
							if ( AVI_SUCCESS != iStatus )
							{
								ulErrCnt++;
								log_err( "ERR: Fail to send MBox%d message iStatus = %d\n\r", exVspaMboxIndex, iStatus );
								break;
							}
							break;
						default :
								log_err( "ERR: Invalid MboxIndex= %d\n\r", exVspaMboxIndex );
								break;
					}

					if (pxAviHandle->xVspaIntrNo[ eVspaCore ][ exVspaMboxIndex ].QueueInit)
						exGeulAviHostRecvMboxFromVspa(eVspaCore, exVspaMboxIndex, &xAviMboxSendData);
				}
			}
		}
		for( eVspaCore = VSPA_CORE_0; eVspaCore < VSPA_CORE_MAX; eVspaCore++ )
		{
			exGeulUnRegisterVspaInterrupt( pxAviHandle, eVspaCore, VSPA_MBOX_0, VSPA_MBOX_RW );
			exGeulUnRegisterVspaInterrupt( pxAviHandle, eVspaCore, VSPA_MBOX_1, VSPA_MBOX_RW );
		}
		vSpinLockRelease( pxAviSpinLock );
	}
	/*
	  Receive ulCnt might be lesser than sent ulCnt due to below reasons :
	  1. There is a possibility of in test framework, we might unregister the ISR before it is completed
	  2. Below print might get printed before ISR is handled
	*/
        if( ulSndCnt != ulVspaResp )
	{
                iStatus = 1;
	}
        else
	{
                iStatus = 0;
	}
	log_info( "%s : Total message sent = %d successful sent  = %d, received = %d\n\r",__func__, ulSndCnt, ulSndCnt-ulErrCnt, ulVspaResp );
        /* ulVspaResp ( global variable ) is reintialized to 0 for next iteration */
        ulVspaResp = 0;

	log_dbg( "%s : AVI test completed\n\r", __func__ );

	return iStatus;
}

bool_t bVSPA0GroupAInterrupt( uint32_t ulIrq, void *pvDevData )
{
	VspaRegs_t *pVspaRegs = NULL;
	VspaCore_t eVspaCore;
	bool_t iStatus  = false;
	AviHandle_t * pxAviHandler = ( AviHandle_t * )pvDevData;
	AviMboxData_t mbox;

	iStatus = iGeulVspaCore( ulIrq, &eVspaCore );
	if( false == iStatus )
	{
		log_isr( "%s : Invalid interrupt generated\n\r", __func__ );
		return iStatus;
	}

	pVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );


	if ( IN_32( &pVspaRegs->ulVspaStatus ) & E200_MBOX0_STATUS )
	{
		log_dbg( "%s: VSPA : [%d] E200_MBOX0_STATUS, intr = %u\n\r", __func__, eVspaCore, ulIrq );

		OUT_32( &pVspaRegs->ulVspaStatus, E200_MBOX0_STATUS );
		iStatus = true;
	}
	else if ( IN_32( &pVspaRegs->ulVspaStatus ) & VSPA_MBOX0_STATUS )
	{
		exGeulAviHostHandleMboxIrq( pxAviHandler, eVspaCore, VSPA_MBOX_0, &mbox );
		if (!pxAviHandler->xVspaIntrNo[ eVspaCore ][ VSPA_MBOX_0 ].QueueInit) {
			log_dbg( "%s: VSPA : [%d] VSPA_MBOX0_STATUS, intr = %u\n\r", __func__, eVspaCore, ulIrq );
			if( mbox.ulLsb == 0x1 && mbox.ulMsb == 0x0A << 24 )
				ulVspaResp++;
			else if ((mbox.ulLsb == 0x01 && mbox.ulMsb == 0x0C << 24) ||
				(mbox.ulLsb == 0x01 && mbox.ulMsb == 0x0D << 24))
				ulOverlay_resp++;
			else {
				ulOverlay_resp = -1;
				log_err( "\n%s: eVspaCore %d : response expected : ulMsb : 0x0B << 24 : ulLsb = 0x1, received ulMsb = 0x%x ulLsb = 0x%x\n\r",
					__func__, eVspaCore, mbox.ulMsb , mbox.ulLsb);
			}
		}
		iStatus = true;
	}
	else
	{
		log_err( "%s ERR: Invalid VSPA iStatus\n\r", __func__ );
		iStatus = false;
	}
	return iStatus;
}

bool_t bVSPA0GroupBInterrupt( uint32_t ulIrq, void *pvDevData )
{
	VspaRegs_t *pVspaRegs = NULL;
	VspaCore_t eVspaCore;
	bool_t iStatus  = false;
	AviHandle_t *pxAviHandler = ( AviHandle_t * ) pvDevData;
	AviMboxData_t mbox;

	iStatus = iGeulVspaCore( ulIrq, &eVspaCore );
	if( false == iStatus )
	{
		log_isr( "%s : Invalid interrupt generated\n\r", __func__ );
		return iStatus;
	}
	pVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR( eVspaCore );

	if ( IN_32( &pVspaRegs->ulVspaStatus ) & E200_MBOX1_STATUS )
	{
		log_dbg( "%s: VSPA : [%d] E200_MBOX1_STATUS, intr = %u\n\r", __func__, eVspaCore, ulIrq );

		OUT_32( &pVspaRegs->ulVspaStatus, E200_MBOX1_STATUS );
		iStatus = true;
	}
	else if ( IN_32( &pVspaRegs->ulVspaStatus ) & VSPA_MBOX1_STATUS )
	{
		exGeulAviHostHandleMboxIrq( pxAviHandler, eVspaCore, VSPA_MBOX_1, &mbox );
		if (!pxAviHandler->xVspaIntrNo[ eVspaCore ][ VSPA_MBOX_1 ].QueueInit) {
			log_dbg( "%s: VSPA : [%d] VSPA_MBOX1_STATUS, intr = %u\n\r", __func__, eVspaCore, ulIrq );
			if( mbox.ulLsb == 0x1 && mbox.ulMsb == 0x0B << 24 )
				ulVspaResp++;
			else {
				log_err( "\n%s: eVspaCore %d : response expected : ulMsb : 0x0B << 24 : ulLsb = 0x1, received ulMsb = 0x%x ulLsb = 0x%x\r\n",
					__func__, eVspaCore, mbox.ulMsb , mbox.ulLsb);
			}
		}
		iStatus = true;
	}
	else
	{
		log_err( "%s ERR: Invalid VSPA iStatus\n\r", __func__ );
		iStatus = false;
	}
	return iStatus;
}
#endif
#endif
