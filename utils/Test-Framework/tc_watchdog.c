// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#include "tc_watchdog.h"

#ifdef GEUL_DEMO_WDOG_TEST
#include "FreeRTOS.h"
#include "task.h"
#include <debug_console.h>
#include "mpic.h"
#include "semphr.h"
#include "Time.h"
#include "ipiQueue.h"
#include "soc.h"

#include "watchdog_api.h"

#define WDOG_TASK_STACKSIZE	( configMINIMAL_STACK_SIZE * 2)
#define WDOG_TASK_PRIORITY	( tskIDLE_PRIORITY + 4 )

/*
 * counter value 1 corresponds to 30.5us ,
 * So Ideally the Decimal equivalent of [Value loaded in Hex in WDOG] x 30.5
 * would give the time in microseconds
 */
#define WDOG_TIMEOUT_VALUE	(0x100000)

static TaskHandle_t TaskHandle;
volatile uint32_t core_alive[GEUL_E200_CORE_GLOBAL_NUM] __attribute__ ((section(".smem")));
uint32_t core_mask __attribute__ ((section(".smem")));

static inline int CalNumPermitCore(void) {
	uint32_t core_id = 0, sum = 0;
	for(;core_id < get_soc_numcores(); core_id++) {
		if(!(core_mask & (1 << core_id)))
			continue;
		else
			sum+= 1;
	}
	return sum;
}

/*
 * This function is just to return 0 or 1;
 * 1 for Watchdog Interrupt Handler to reload the timer
 * 0 for don't reload, so after 2nd timeout, watchdog raise interrupt to Host(LS1046)
 */
static int prvWdogTestFunc()
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	static uint32_t timeout_count = 0;
	BaseType_t ret;
	u32 dstCore, num_core_allowed;

	/* first timeout */
	if (++timeout_count == 1) {
		uint32_t retries = 5;
		core_alive[uiCurrentCore] = 1;

		/* check the health of the e200 cores */
		for (dstCore = 1; dstCore < get_soc_numcores(); dstCore++) {
			if(!(core_mask & (1 << dstCore)))
				continue;

			ret = vIPISendDatafromISR( dstCore, IPI_EVT_WDOG, ( void * ) uiCurrentCore );
			if( ret == pdFALSE )
			{
				log_err( "vGeulWdogTask: vIPISendDatafromISR: core %d to %d, with event %d failed\r\n", uiCurrentCore,
						dstCore, IPI_EVT_WDOG);
			}
		}

		num_core_allowed = CalNumPermitCore();
		while (retries)
		{
			uint32_t i, sum;
			for (i = 0, sum = 0; i < get_soc_numcores(); i++){
				if(!(core_mask & (1 << i)))
					continue;
				sum += core_alive[i];
			}
			if (sum == num_core_allowed) {
				/* reset the counter */
				timeout_count = 0;
				log_dbg( "vGeulWdogTask: Health check OK: Reload\n\r");
				/* Clear core_alive[] for next Watchdog event */
				for (i = 0; i < get_soc_numcores(); i++)
				{
					core_alive[i]=0;
				}
				/* reload the timer */
				return 1;
			}
			retries--;
			vUdelay(1000);
		}
		log_info( "vGeulWdogTask: Health check NOK: Don't Reload\n\r");
		/* don't reload the timer */
		return 0;
	} else {
		/* reset the counter */
		timeout_count = 0;
		log_info( "vGeulWdogTask: Second Timeout: Don't Reload\n\r");
		/* don't reload the timer */
		return 0;
	}
}

void vGeulWdogTask( void *pvParameters )
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	(void) pvParameters;
	BaseType_t ret;
	void * rxData;

	log_info( "[Watchdog TC] vGeulWdogTask : %s started...\n\r", __func__ );
	while( 1 )
	{
		switch(uiCurrentCore)
		{
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				ret = xQueueReceive( pxRxQueue[ IPI_EVT_WDOG ], &rxData, portMAX_DELAY );
				if( ret == pdFALSE )
				{
					core_alive[uiCurrentCore] = 0;
					log_err("vGeulWdogTask: xQueueReceive on core %d with event %d failed\n\r", uiCurrentCore, IPI_EVT_WDOG);
				}
				else
				{
					core_alive[uiCurrentCore] = 1;
				}
				break;
			default:
				vTaskDelay(1000);
				break;
		}
	}
	log_info( "[Regress TC] vGeulWdogTask : %s ends...\n\r", __func__ );
}


void vCoreMaskBasedWdogEnable(int mask)
{
	uint32_t crt_core, loadval;
	crt_core = ulMpicCurrentCore();
	if(mask & (1 << crt_core)){
		vIPIEventRegister( IPI_EVT_WDOG, &pxRxQueue[IPI_EVT_WDOG], NULL, NULL );
		xTaskCreate(vGeulWdogTask, "WdogTask", WDOG_TASK_STACKSIZE,
							NULL, WDOG_TASK_PRIORITY, &TaskHandle);
		if (crt_core == 0){
			core_mask = mask;
			loadval = WDOG_TIMEOUT_VALUE;
			int (* wdogTestCallback)() = prvWdogTestFunc;
			vWatchdogStart( loadval, wdogTestCallback );
		}
	}
	else {
		log_info("wdog not enabled for core_id: %d\n\r", ulMpicCurrentCore());
	}

	return;
}

void vWatchdogTest( void )
{
	uint32_t uiCurrentCore;
 
	uiCurrentCore = ulMpicCurrentCore();
 
	if(!(wdog_status()) && (core_mask & (1 << uiCurrentCore))) {
		log_info("WDOG already enabled on this core\n\r");
		return;
	}
 
	core_mask |= 1 << uiCurrentCore;
	log_info( "%s:WDOG Demo core id: %d\n\r", __func__, uiCurrentCore );
 
	vCoreMaskBasedWdogEnable(core_mask);
}
#endif /* ifdef GEUL_DEMO_WDOG_TEST */
