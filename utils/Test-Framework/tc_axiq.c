// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "tc_axiq.h"

#ifdef GEUL_DEMO_AXIQ_TEST
#include "FreeRTOS.h"
#include "task.h"

#include "axiq.h"
#include <types.h>

static bool_t bAxiqIntHandler_H(uint32_t Irq, void *dev_data )
{
	/* To remove unused variable warnings */
	(void)Irq;
	(void)dev_data; 
	log_dbg("\n%s entering in %d %p\n\r", __func__, Irq, dev_data);
	return true;
}

static bool_t bAxiqIntHandler_L0(uint32_t Irq, void *dev_data )
{
	/* To remove unused variable warnings */
	(void)Irq;
	(void)dev_data; 
	log_dbg("\n%s entering in %d %p\n\r", __func__, Irq, dev_data);
	return true;
}

static bool_t bAxiqIntHandler_L1(uint32_t Irq, void *dev_data )
{
	/* To remove unused variable warnings */
	(void)Irq;
	(void)dev_data; 
	log_dbg("\n%s entering in %d %p\n\r", __func__, Irq, dev_data);
	return true;
}

void vAxiqInitDemo( void )
{
    AxiqLibHndlr_t *pxAxiqLib = pxAxiqInit();
    void *pvAxiqIntData = NULL;

	/* Configure AXIQ_H as inter loopback  */
	vLbConfigAxiqH( pxAxiqLib, INTER_LOOPBACK_MODE_ENABLED_1 );

	/* Configure AXIQ_L0 as intra loopback  */
	vLbConfigAxiqL0( pxAxiqLib, INTRA_LOOPBACK_MODE_ENABLED_2 );

	/* Configure AXIQ_L1 as intra loopback	*/
	vLbConfigAxiqL1( pxAxiqLib, INTRA_LOOPBACK_MODE_ENABLED_2 );

	vRegisterAxiqErrorInterrupt( pxAxiqLib, (bIsrFunc)bAxiqIntHandler_H, (void *)pvAxiqIntData, AXIQ_H );
	vRegisterAxiqErrorInterrupt( pxAxiqLib, (bIsrFunc)bAxiqIntHandler_L0, (void *)pvAxiqIntData, AXIQ_L0 );
	vRegisterAxiqErrorInterrupt( pxAxiqLib, (bIsrFunc)bAxiqIntHandler_L1, (void *)pvAxiqIntData, AXIQ_L1 );

	vEnableAxiqHSubsystem2( pxAxiqLib );
	vEnableAxiqLSubsystem1( pxAxiqLib );
}

void vAxiqExitDemo( void )
{
	AxiqLibHndlr_t *pxAxiqLib = pxAxiqInit();
	/* Configure AXIQ_L1 as loopback disabled */
	vLbConfigAxiqL1( pxAxiqLib, LOOPBACK_MODE_DISABLED_0 );

	/* Configure AXIQ_L0 as loopback disabled */
	vLbConfigAxiqL0( pxAxiqLib, LOOPBACK_MODE_DISABLED_0 );

	/* Configure AXIQ_H as loopback disabled  */
	vLbConfigAxiqH( pxAxiqLib, LOOPBACK_MODE_DISABLED_0 );

	/* Disable Device */
	vDisableAxiqLSubsystem1( pxAxiqLib );
	vDisableAxiqHSubsystem2( pxAxiqLib );

	//vUnRegisterAxiqErrorInterrupt( pxAxiqLib, AXIQ_H );
	//vUnRegisterAxiqErrorInterrupt( pxAxiqLib, AXIQ_L0 );
	//vUnRegisterAxiqErrorInterrupt( pxAxiqLib, AXIQ_L1 );

	vAxiqClose();
}

void vGeulRFLpbkTsk( void *pvParameters )
{
	if( pvParameters )
		log_info("\n%s \n\r", (char *)pvParameters);

#ifdef GEUL_DEMO_AXIQ_TEST
		vAxiqInitDemo();
		vAxiqExitDemo();
#endif

	log_info("\nHigh WATERMARK for Task %s %ld \n\r",
            __func__, uxTaskGetStackHighWaterMark( NULL ) );
	log_info("\n%s entering in while (1) \n\r", __func__);
	while( 1 )
        vTaskDelay(500);
}

static TaskHandle_t xHandlerTask;
static char *pcTextForRfLbTask;

void vRfLbTaskDemo( void )
{
	pcTextForRfLbTask = "RF loopback task is running\r\n";
	int	iRc = xTaskCreate( vGeulRFLpbkTsk, "RFLoopbkTsk", RF_LOOPBACK_TASK_STACKSIZE,
		pcTextForRfLbTask, RF_LOOPBACK_TASK_PRIORITY, &xHandlerTask );

	if( iRc != pdPASS )
	{
		log_info("Failed to create RF LoopBack demo thread.\n\r");
	}
}

#endif	/* GEUL_DEMO_AXIQ_TEST */
