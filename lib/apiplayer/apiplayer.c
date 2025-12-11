// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2022 NXP
 */

#include "FreeRTOS.h"
#include "Time.h"
#include "ipiQueue.h"
#include "apiplayer.h"
#include "rfapiplayer.h"

/* APIPlayerDev_t pxApiPlayer = {0}; */
static TaskHandle_t xAPIPlayerCoreTask;
static bool_t stopAPIPlayer = pdFALSE;

xAPIGroupHandler apiGroupHandlers[] = 
{
    rfgroup_api_player, /* APIPLAYER_GOURP_RF */
};

/* start API player, call this with 0 addr to stop any running api player */
void xStartAPIPlayer(uint32_t ulAPIListAddr)
{
	int i;

	for (i = 0; i < E200_CORE_COUNT; i++) { 
        if (vIPISendData( i, IPI_EVT_APIPLAYER, ( void * ) ulAPIListAddr ) == pdFALSE) {	
            log_err("ERROR: API Player IPI send data failed\r\n");
        }
        log_dbg("\r\nSent API Player events core(%d) scraddr(%d)",i, ulAPIListAddr);
	}
}

int32_t iRunAPIs(uint32_t ulAPIListAddr)
{
    apiplayer_api_t *api = (apiplayer_api_t *)(ulAPIListAddr);
    uint32_t ulCoreId = ulMpicCurrentCore();
    uint32_t prevts = 0;
    while((stopAPIPlayer != pdTRUE))
    {
        if (ulCoreId == api->core) {
            if (api->group == APIPLAYER_GROUP_END) {
                /* mark as done so that host can continue its validations */
                api->state = API_DONE;
                break;
            }

            /* task delay takes in ticks, in current architecture its 1ms */
            /* api contains timestamps in micro seconds. */
            /* so convert time stamp diff to milli seconds before delaying */
            /* or use vUdelay to exactly wait that much time ( vUdelay doesnt scheduleout ) */
            if (api->ts > prevts)
                vUdelay((api->ts - prevts));
            else
                log_err("\r\nPrevious API took more time than expected, so ignoring time stamp");

            log_dbg("\r\nRunning API on core(%d) group(%d) id(%d)", ulCoreId, api->group, api->id);
            api->state = API_RUNNING;
            apiGroupHandlers[api->group](api);
            api->state = API_DONE;
            prevts = api->ts;
        }
        api++;
    }
    return 0;
}

void vApiPlayerCoreTask( void * pvParameters )
{
    UNUSED(pvParameters);
    uint32_t ulAPIListAddr;

	if (vIPIEventRegister(IPIGlobalEventID[IPI_EVT_APIPLAYER],&pxRxQueue[ IPI_EVT_APIPLAYER ], NULL, NULL)
			== IPI_EVT_ID_NULL) {
		log_err("ERROR: APIPlayer IPI event register failed\r\n");
		return;
	}
	syncUnSync();
	log_dbg("APIPlayer IPI event register success\r\n");

	while( 1 )
	{
        if (xQueueReceive( pxRxQueue[ IPI_EVT_APIPLAYER ], &ulAPIListAddr, portMAX_DELAY ) == pdPASS) 
        {
            log_dbg("\r\nAPI Player event on core(%d) with scraddr(%d)", ulMpicCurrentCore(), ulAPIListAddr);
            if (ulAPIListAddr == 0)
            {
                stopAPIPlayer = pdTRUE;
            }
            else
            {
                stopAPIPlayer = pdFALSE;
                iRunAPIs(ulAPIListAddr);
            }
        }
	}
}

void xApiPlayerInit( void )
{
	if( xTaskCreate( vApiPlayerCoreTask,
			 "ApiplayerCoreTask",
			 APIPLAYER_CORE_TASK_STACK_SIZE,
             NULL,
			 (APIPLAYER_CORE_TASK_PRIORITY),
			 ( TaskHandle_t * )&(xAPIPlayerCoreTask) ) == pdPASS )
	{
		log_dbg("APIPlayer init success\r\n");
	} else {
		log_err("APIPlayer init failed\r\n");
	}
}
