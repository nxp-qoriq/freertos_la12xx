// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#include "FreeRTOS.h"
#include "rf_sw_cmds.h"
#include "rf_config.h"
#include "rf_dev.h"
#include "rf_core.h"
#include "event_groups.h"
#include "types.h"
#include "Time.h"
#include "rf_hif.h"

int32_t xRFPostLocalSWCmd( RficHandle_t xHandle,
                           volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc,
                           struct Time * pxEntryTime )
{
	int32_t iRet = RF_SW_CMD_RESULT_OK;

	BaseType_t xRet = xQueueSend( xHandle->xRFLocalQueue, &pxRFSWCmdDesc, RF_LOCAL_QUEUE_SEND_TIMEOUT );

	if( xRet == pdPASS )
	{
		xEventGroupSetBits( xHandle->xRFCoreTaskEventGrp, RF_LOCAL_CMD_EVENT );

		xRet = pdFAIL;

		while( xRet == pdFAIL )
		{
			xRet = xQueueReceive( xHandle->xRFLocalQueueResp, &iRet, RF_LOCAL_RESP_QUEUE_RECV_TIMEOUT );

			if( iHasTimeElapsedMPICGB( pxEntryTime, pxRFSWCmdDesc->timeout ) )
			{
				pxRFSWCmdDesc->status = RF_SW_CMD_STATUS_TIMEOUT;
				RF_STATS_ADD( xHandle->pxStats->local_sw_cmd_timeout_count );
				iRet = RF_SW_CMD_RESULT_TIMEOUT;
				break;
			}
		}

		pxRFSWCmdDesc->status = RF_SW_CMD_STATUS_DONE;
	}
	else
	{
		RF_STATS_ADD( xHandle->pxStats->local_queue_send_failed );
		iRet = RF_SW_CMD_RESULT_ENQUEUE_ERROR;
	}

	if( iRet != RF_SW_CMD_RESULT_TIMEOUT)
	{
		iRet = pxRFSWCmdDesc->result;
	}

	return iRet;
}
