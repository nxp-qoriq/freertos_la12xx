// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "tc_system.h"

#if GEUL_SYSTEM_DEBUG_CAPABILITIES
#include "FreeRTOS.h"
#include "task.h"

extern uint32_t uiTaskStackSize( TaskHandle_t *pxTaskHandle );
void vStatsUsageCommand( void )
{

	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
	UBaseType_t x = 0;
	uint32_t ulTotalTime = 0;
	TaskHandle_t xTaskHandle = NULL;
	uint32_t uiStackSize = 0;
	eTaskState eTaskState = eInvalid;
	char * pcTaskState = NULL;

	PRINTF("\n\r======================");
	PRINTF("\n\rStack Usage Per Thread");
	PRINTF("\n\r======================");
	PRINTF("\n\rLegends : ");
	PRINTF("\n\rStack BA:Stack Base Addr | SS:Stack Size | SR:Stack Remaining\n\r");

	pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

	if( pxTaskStatusArray != NULL )
	{
		/* Generate the (binary) data. */
		uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );

		PRINTF("\n\r------------------------------------------------------------------");
		PRINTF("\n\rThrd ID\tThrd Name\tStack BA\tSS\tSR\tState");
		PRINTF("\n\r------------------------------------------------------------------\n\r");

		/* Create a human readable table from the binary data. */
		for( x = 0; x < uxArraySize; x++ )
		{
			xTaskHandle = pxTaskStatusArray[ x ].xHandle;
			uiStackSize = uiTaskStackSize(&xTaskHandle );

			eTaskState = pxTaskStatusArray[ x ].eCurrentState;
			switch( eTaskState )
			{
				case eRunning:
					pcTaskState = "RUNNING";
					break;
				case eReady:
					pcTaskState = "READY";
					break;
				case eBlocked:
					pcTaskState = "BLOCKED";
					break;
				case eSuspended:
					pcTaskState = "SUSPENDED";
					break;
				case eDeleted:
					pcTaskState = "DELETED";
					break;
				case eInvalid:
				default:
					pcTaskState = "INVALID";
					break;
			}

			PRINTF("%d\t%-15s\t0x%p\t0x%p\t0x%p\t%s\n\r",
				pxTaskStatusArray[ x ].xTaskNumber,
				pxTaskStatusArray[ x ].pcTaskName,
				pxTaskStatusArray[ x ].pxStackBase,
				uiStackSize,
				pxTaskStatusArray[ x ].usStackHighWaterMark,
				pcTaskState );
		}
		PRINTF("------------------------------------------------------------------\n\r");

		vPortFree( pxTaskStatusArray );
	}
}
#endif	/* GEUL_SYSTEM_DEBUG_CAPABILITIES */
