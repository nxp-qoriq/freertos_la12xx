// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#include "tc_avi.h"
#include "tc_vspa.h"

#if GEUL_DEMO_AVI_TEST
#include "FreeRTOS.h"
#include "semphr.h"
#include "geul_avi.h"


/******************************************************************
*			Global Parameters
******************************************************************/
SemaphoreHandle_t xSemaphore = NULL;
TickType_t xTicksToWait	= 10;

int32_t ulOverlay_resp;
static AviHandle_t *pxAviHandle = NULL;
static TaskHandle_t xHandlerTask;
static char *pcTextForTask = NULL;

/******************************************************************
*			API function's
******************************************************************/
void vGeulDemoVspaAviTest( void )
{
	int32_t iStatus;
	u32 uiCurrentCore = ulMpicCurrentCore();

#if GEUL_VSPAMBOX_TEST
	iStatus = iVSPAMBOXTest();
#endif
	log_dbg("%s : iStatus = %u\n\r", __func__, iStatus);
	if ( !iStatus )
	{
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_VSPA_AVI_TEST_STATUS);
	}
}

static void vGeulAviDemo( void *pvParameters )
{
	pxAviHandle = NULL;
	if ( pvParameters )
	{
		log_dbg("\n%s \n\r", ( char * )pvParameters);
	}

#if GEUL_DEMO_AVI_TEST
	pxAviHandle = pxGeulAviInit( xHandlerTask );
	if( pxAviHandle == NULL )
	{
		log_err("ERR: %s: AVI Initialization is failed \n\r",__func__);
	}
	else
	{
		log_dbg("%s: AVI Initialization : %p\n\r",__func__, pxAviHandle);
		vGeulDemoVspaAviTest();
	}
#endif

	log_info("\nHigh WATERMARK for Task %s %ld\n\r",
            pcTaskGetName( NULL ), uxTaskGetStackHighWaterMark( NULL ) );
	xSemaphoreGive( xSemaphore );
}
void vLaunchAviTestTask( void )
{
	pcTextForTask = "Task vGeulAviDemo is running\r\n";
	static int iIsInitialized = 0;
	if( !iIsInitialized )
	{
		xSemaphore = xSemaphoreCreateBinary();
		if( xSemaphore )
		{
			/* Keep the sema count as 1 initially */
			xSemaphoreGive( xSemaphore );
			log_info("%s , Semaphore is given\n\r", __func__);
		}
		else
		{
			log_err("%s:%d Failed to create semaphore \r\n", __func__, __LINE__);
			return;
		}

		xHandlerTask = xTaskGetCurrentTaskHandle();
		iIsInitialized = 1;
	}
	/* Before proceeding further, first acquire sema */
	while( xSemaphoreTake( xSemaphore, xTicksToWait) == pdFALSE )
	{
		log_info("%s : Waiting for Semaphore... \n\r", __func__);
	}
	vGeulAviDemo( pcTextForTask );
}
#endif	/* GEUL_DEMO_AVI_TEST */

#if GEUL_DEMO_OVERLAY_TEST
int iTriggerHostOverlay( void )
{
	AviMboxData_t xHostSendMbox;
	uint8_t ucCore = 0;
	int iStatus;

	pxAviHandle = pxGeulAviGetHandle();

	/* Sending mailbox as per design to trigger overlay */
	xHostSendMbox.ulMsb = 0x0C << 24;
	xHostSendMbox.ulLsb = 0x0;

	for( ucCore = 0; ucCore < VSPA_MAX_CORES; ucCore++ )
	{
		while( !uiCheckVSPABoot( ( uint32_t ) ucCore ) )
		{
			vTaskDelay( 20 );
		}

		while( exGeulRegisterVspaInterrupt( pxAviHandle, ucCore, VSPA_MBOX_0,
				bVSPA0GroupAInterrupt, pxAviHandle, VSPA_MBOX_RW, false ))
		{
			vTaskDelay( 5 );
		}

		iStatus = exGeulAviHostSendFastMboxToVspa( pxAviHandle, ucCore,
				VSPA_MBOX_1, xHostSendMbox );
		if( 0 != iStatus)
		{
			log_err("Error[H][%d]: Sending Overlay MBOX failed\n",
				ucCore);
			goto Overlay_Error_H;
		}

		while (!ulOverlay_resp) {
			log_dbg("[H]: Waiting for Overlay response...\n\r");
			vTaskDelay(1);
		}

		if (ulOverlay_resp < 0)
			log_info("[H]: Host Overlay failed for core %d\n\r", ucCore);
		else
			log_info("[H]: Host Overlay passed for core %d\n\r", ucCore);

		ulOverlay_resp = 0;

		while( exGeulUnRegisterVspaInterrupt( pxAviHandle, ucCore, VSPA_MBOX_0, VSPA_MBOX_RW ) )
		{
			vTaskDelay( 5 );
		}
	}
	return iStatus;

Overlay_Error_H:
	exGeulUnRegisterVspaInterrupt( pxAviHandle, ucCore, VSPA_MBOX_0, VSPA_MBOX_RW );
	return iStatus;
}

int iTriggerPEBMOverlay( void )
{
	AviMboxData_t ulPebmSndMbox;
	int iStatus;
	uint8_t ucCore = 0;

	pxAviHandle = pxGeulAviGetHandle();

	/* Sending mailbox as per design to trigger PEBM overlay */
	ulPebmSndMbox.ulMsb = 0x0D << 24;
	ulPebmSndMbox.ulLsb = 0x0;

	for( ucCore = 0; ucCore < VSPA_MAX_CORES; ucCore++ )
	{
		while( !uiCheckVSPABoot( ( uint32_t )ucCore ) )
		{
			vTaskDelay(20);
		}

		while( exGeulRegisterVspaInterrupt( pxAviHandle, ucCore, VSPA_MBOX_1,
				bVSPA0GroupAInterrupt, pxAviHandle, VSPA_MBOX_RW, false ) )
		{
			vTaskDelay(5);
		}

		iStatus = exGeulAviHostSendFastMboxToVspa( pxAviHandle, ucCore,
				VSPA_MBOX_1, ulPebmSndMbox );
		if( 0 != iStatus )
		{
			log_info("Error[P][%d]: Sending Overlay MBOX failed\n",
					ucCore);
			goto Overlay_Error_P;
		}

		while (!ulOverlay_resp) {
			log_dbg("[P]: Waiting for Overlay response...\n\r");
			vTaskDelay(1);
		}

		if (ulOverlay_resp < 0)
			log_info("[P]: PEBM Overlay failed for core %d\n\r", ucCore);
		else
			log_info("[P]: PEBM Overlay passed for core %d\n\r", ucCore);

		ulOverlay_resp = 0;

		while( exGeulUnRegisterVspaInterrupt( pxAviHandle, ucCore, VSPA_MBOX_1, VSPA_MBOX_RW ) )
		{
			vTaskDelay(5);
		}
	}
	return iStatus;

Overlay_Error_P:
	exGeulUnRegisterVspaInterrupt( pxAviHandle, ucCore, VSPA_MBOX_1, VSPA_MBOX_RW );
	return iStatus;
}

void vGeulOverlayTest( void )
{
	pcTextForTask = "Task vGeulOverlayTest is running\r\n";
	static int iIsInitialized;
	int iStatus = 0;
	u32 uiCurrentCore = ulMpicCurrentCore();

	pxAviHandle = pxGeulAviInit(xHandlerTask);
	if (pxAviHandle == NULL)
		log_err("ERR: %s: AVI Initialization is failed \n\r", __func__);
	else
		log_dbg("%s: AVI Initialization : %p\n\r", __func__,
				pxAviHandle);

	if (!iIsInitialized) {
		xSemaphore = xSemaphoreCreateBinary();
		if (xSemaphore) {
			/* Keep the sema count as 1 initially */
			xSemaphoreGive (xSemaphore);
			log_info("%s , Semaphore is given\n\r", __func__);
		}
		else
		{
			log_err("%s:%d Failed to create semaphore \r\n", __func__, __LINE__);
			return;
		}
		xHandlerTask = xTaskGetCurrentTaskHandle();
		iIsInitialized = 1;
	}

	/* Before proceeding further, first acquire sema */
	while (xSemaphoreTake (xSemaphore, xTicksToWait) == pdFALSE) {
		log_info("Waiting for Semaphore... \n\r");
	}

#ifndef GEUL_BOOT_MODE_PEBM
	iStatus = iTriggerHostOverlay();
#endif

#ifdef VSPA_OV_PEBM_ENABLED
	if( iStatus == 0 )
	{
		iStatus = iTriggerPEBMOverlay();
	}
#endif

	/* Releasing Semaphore */
	xSemaphoreGive(xSemaphore);
	if( iStatus == 0 )
	{
		SET_TEST_STATUS(uiCurrentCore,
			GEUL_DEMO_VSPA_OVERLAY_TEST_STATUS);
	}
}
#endif

#if GEUL_VSPA_LOG
#include "geul_avi_ds.h"
int msg_rcvd = 0;
bool_t vspa_irq_handler(uint32_t ulIrq, void *pvDevData)
{
	VspaRegs_t *pVspaRegs = NULL;
	VspaCore_t eVspaCore = VSPA_CORE_0;
	bool_t iStatus  = false;
	AviHandle_t * pxAviHandler = ( AviHandle_t * )pvDevData;
	AviMboxData_t mbox;
	uint8_t *str;

	ulIrq = ulIrq - INTERNAL_IRQ_OFFSET;
	if (ulIrq != 66)
		log_isr( "%s : expected IrqNo: 66 got %d\n\r", __func__, ulIrq);

	pVspaRegs = ( VspaRegs_t * )VSPA_INST_BASE_ADDR(eVspaCore);

	log_dbg( "%s : VSPA Status 0x%X\n\r", __func__, IN_32(&pVspaRegs->ulVspaStatus));
	if (IN_32(&pVspaRegs->ulVspaStatus) & E200_MBOX0_STATUS) {
		log_dbg( "%s: VSPA : [%d] E200_MBOX0_STATUS, intr = %u\n\r", __func__, eVspaCore, ulIrq );
		OUT_32( &pVspaRegs->ulVspaStatus, E200_MBOX0_STATUS );
		iStatus = true;
	}
	if (IN_32( &pVspaRegs->ulVspaStatus ) & VSPA_MBOX1_STATUS) {
		exGeulAviHostHandleMboxIrq(pxAviHandler, eVspaCore, VSPA_MBOX_1, &mbox );
		log_dbg( "%s : VSPA[%d]  VSPA_MBOX1_STATUS, intr = %u mbox.ulMsb = 0x%x mbox.ulLsb = 0x%x\n\r",
							__func__, eVspaCore, ulIrq, mbox.ulMsb, mbox.ulLsb);
		str = (uint8_t *)(0xE1000000 | mbox.ulLsb);
		msg_rcvd++;
		PRINTF( "VSPA[%d]:MBOX1:  %s \n\r", eVspaCore, str);
		iStatus = true;
	}
	if (IN_32( &pVspaRegs->ulVspaStatus ) & VSPA_MBOX0_STATUS) {
		exGeulAviHostHandleMboxIrq(pxAviHandler, eVspaCore, VSPA_MBOX_0, &mbox );
		log_dbg( "%s : VSPA[%d]  VSPA_MBOX0_STATUS, intr = %u mbox.ulMsb = 0x%x mbox.ulLsb = 0x%x\n\r",
							__func__, eVspaCore, ulIrq, mbox.ulMsb, mbox.ulLsb);
		str = (uint8_t *)(0xE1000000 | mbox.ulLsb);
		msg_rcvd++;
		PRINTF( "VSPA[%d]:MBOX0:  %s \n\r", eVspaCore, str);
		iStatus = true;
	} else {
		if (!msg_rcvd) {
			log_err( "%s ERR: Invalid VSPA Status\n\r", __func__);
			iStatus = false;
		}
	}
	return iStatus;
}

void vLaunchVspaLogs(void)
{
	pcTextForTask = "Task vLaunchVspaLogs is running...\r\n";
	static int iIsInitialized = 0;
	AviMboxData_t xAviMboxSendData;
	AviHandle_t *pxAviHandle= NULL;
	int32_t iStatus = -1;
	uint32_t eVspaCore = 0;
	u8 core_id = (u8)ulMpicCurrentCore();

	if (core_id != 0)
		return;

	log_dbg("\n%s \n\r", ( char * )pcTextForTask);
	if( !iIsInitialized )
	{
		xSemaphore = xSemaphoreCreateBinary();
		if( xSemaphore )
		{
			/* Keep the sema count as 1 initially */
			xSemaphoreGive( xSemaphore );
			log_info("%s , Semaphore is given\n\r", __func__);
		}
		else
		{
			log_err("%s:%d Failed to create semaphore \r\n", __func__, __LINE__);
			return;
		}
		xHandlerTask = xTaskGetCurrentTaskHandle();
		iIsInitialized = 1;
	}
	/* Before proceeding further, first acquire sema */
	while( xSemaphoreTake( xSemaphore, xTicksToWait) == pdFALSE )
	{
		log_info("%s : Waiting for Semaphore... \n\r", __func__);
	}

	/* Init AVI */
	pxAviHandle = pxGeulAviInit(xHandlerTask);
	if (pxAviHandle == NULL) {
		log_err("ERR: %s: AVI Initialization is failed \n\r",__func__);
		goto exit;
	}
	log_dbg("%s: AVI Initialization done : %p\n\r",__func__, pxAviHandle);

	for (eVspaCore = VSPA_CORE_0; eVspaCore < VSPA_CORE_MAX; eVspaCore++) {
		exGeulRegisterVspaInterrupt(pxAviHandle, eVspaCore, VSPA_MBOX_0, vspa_irq_handler, pxAviHandle, VSPA_MBOX_RW, false);
		exGeulRegisterVspaInterrupt(pxAviHandle, eVspaCore, VSPA_MBOX_1, vspa_irq_handler, pxAviHandle, VSPA_MBOX_RW, false);
	}

	/* VSPA-0 DEMEM */
	xAviMboxSendData.ulMsb = 0xF;    /*Opcode to enable logs */
	xAviMboxSendData.ulLsb = 0x1;
	eVspaCore = VSPA_CORE_0;
	iStatus = exGeulAviHostSendFastMboxToVspa(pxAviHandle, eVspaCore, VSPA_MBOX_1, xAviMboxSendData);
	if ( AVI_SUCCESS != iStatus ) {
		log_err( "ERR: Fail to send MBox0 message iStatus = %d\n\r", iStatus );
		goto exit;
	}

	log_err( "Sent sample MSG on VSPA Core%d MBox0 --> MSB 0x%X LSB 0x%X : iStatus = %d\n\r",
				eVspaCore, xAviMboxSendData.ulMsb, xAviMboxSendData.ulLsb, iStatus);
exit:
	for (eVspaCore = VSPA_CORE_0; eVspaCore < VSPA_CORE_MAX; eVspaCore++) {
		exGeulUnRegisterVspaInterrupt(pxAviHandle, eVspaCore, VSPA_MBOX_0, VSPA_MBOX_RW);
		exGeulUnRegisterVspaInterrupt(pxAviHandle, eVspaCore, VSPA_MBOX_1, VSPA_MBOX_RW);
	}
	xSemaphoreGive( xSemaphore );
	return;
}
#endif
