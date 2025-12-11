// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "types.h"
#include "task.h"
#include "queue.h"
#include "ipiQueue.h"

#include "pmc.h"
#include "FreeRTOSConfig.h"

#include "hawk.h"

#ifndef UNUSED
#define UNUSED(_x) (void)(_x)
#endif

/* Generic bit definitions. */
#define HAWK_LOCAL_CMD_EVENT	( 0x01 )
#define HAWK_REMOTE_CMD_EVENT	( 0x02 )

/* Combinations of bits used  */
#define HAWK_CORE_TASK_EVENT_MASK ( HAWK_LOCAL_CMD_EVENT | HAWK_REMOTE_CMD_EVENT )

static HawkHandle_t pxHawkDevice = NULL;
extern gul_mod_priv_t * pGulModPriv;

static portBASE_TYPE prvHAWKCommand( char * pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char * pcCommandString );

static struct hawk_event cHAWKEventTable[] =
{
	{0, 0, "Nothing"},
	{0, 0, "Processor cycles"},
	{0, 1, "Instructions completed"},
	{1, 3, "Processor cycles with 0 instructions issued"},
	{1, 3, "Processor cycles with 1 instruction issued"},
	{1, 3, "Processor cycles with 2 instructions issued"},
	{2, 3, "Instruction words fetched"},
	{2, 3, "PM_EVENT transitions"},
	{2, 3, "PM_EVENT cycles"},
	{0, 0, "Nothing"},
	{2, 3, "Branch instructions completed"},
	{2, 3, "Branch and link type instructions completed"},
	{2, 3, "Conditional branch instructions completed"},
	{2, 3, "Taken Branch instructions completed"},
	{2, 3, "Taken Conditional Branch instructions completed"},
	{2, 3, "Load instructions completed"},
	{2, 3, "Store instructions completed"},
	{2, 3, "Integer instructions completed"},
	{2, 3, "Multiply instructions completed"},
	{2, 3, "Divide instructions completed"},
	{2, 3, "Divide instruction execution cycles"},
	{2, 3, "EFPU FP instructions completed"},
	{2, 3, "Cycles decode stalled due to no instructions available"},
	{2, 3, "Cycles issue stalled, not due to empty instruction buffer"},
	{2, 3, "Dcache linefills"},
	{2, 3, "Dcache load hits"},
	{2, 3, "Store buffer full stalls"},
	{2, 3, "Icache linefills"},
	{2, 3, "Number of Instruction fetches"},
	{2, 3, "BIU instruction-side transfers"},
	{2, 3, "BIU instruction-side cycles"},
	{2, 3, "BIU data-side transfers"},
	{2, 3, "BIU data-side cycles"},
	{2, 3, "BIU single-beat write cycles"},
	{0, 0, "PMC0 rollover"},
	{0, 0, "PMC1 rollover"},
	{0, 0, "PMC2 rollover"},
	{0, 0, "PMC3 rollover"},
	{2, 3, "Interrupts taken"},
	{2, 3, "External input interrupts taken"},
	{2, 3, "Critical input interrupts taken"},
	{2, 3, "Cycles in which MSREE=0"},
	{2, 3, "Cycles in which MSRCE=0"},
};

static const CLI_Command_Definition_t xHAWKCommand =
{
	"hawk", /* The command string to type. */
	"\r\nhawk \r\n Usage : Takes command specific arguments" \
	"\r\n To get list of supported test cases," \
	"\n\rType:" \
	"\n\r\t hawk help", \
	prvHAWKCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

void vRegisterHAWKCommands(void)
{
	FreeRTOS_CLIRegisterCommand( &xHAWKCommand );
}

static void vHAWKHelpCommand(void)
{
	log_info("Supported Commands:\r\n");
	log_info("\thawk help\r\n");
	log_info("\thawk list\r\n");
#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
	log_info("\thawk stat -e <event_output:event> <event_output:event> .. -t <seconds> -p <pid>\r\n");
#else
	log_info("\thawk stat -e <event_output:event> <event_output:event> .. -t <seconds>\r\n");
#endif
	log_info("\twhere event_output=[1 to 4]>\r\n");
	log_info("\tFor Example:\r\n");
	log_info("\tFor C0:1  , use command \"hawk stat -e 0:1 -t 1\"\r\n");
	log_info("\tFor C1-3:5, use command \"hawk stat -e 1:5 -t 1\"\r\n");
	log_info("\tOR          use command \"hawk stat -e 2:5 -t 1\"\r\n");
	log_info("\tOR          use command \"hawk stat -e 3:5 -t 1\"\r\n");
	log_info("\tFor C0:1 and C1-3:5 together,\r\n");
	log_info("              use command \"hawk stat -e 0:1 1:5 -t 1\"\r\n");
}

static void vHAWKListCommand(void)
{
	u32 i;

	log_info("Supported Events:\r\n");

	for (i = 0; i < sizeof(cHAWKEventTable) / sizeof(cHAWKEventTable[0]); i++)
	{
		log_info(" C%u-%u:%u %s\r\n", 
				cHAWKEventTable[i].event_output_start,
				cHAWKEventTable[i].event_output_end,
				i,
				cHAWKEventTable[i].event_string);

	}
}

static void vSetPMLCaEvent(u32 event_output, u32 event)
{
	switch (event_output) {
	case 0:
		PMC_SET_EVENT(PMR_PMLCa0, event);
		break;
	case 1:
		PMC_SET_EVENT(PMR_PMLCa1, event);
		break;
	case 2:
		PMC_SET_EVENT(PMR_PMLCa2, event);
		break;
	case 3:
		PMC_SET_EVENT(PMR_PMLCa3, event);
		break;
	default:
		log_err("Invalid Event Output %u\r\n", event_output);
		break;
	}
}

#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
void vSetHAWKMarker(bool bMark)
{

	if (bMark) {
		mtmsr(mfmsr() | MSR_PMM);
	} else {
		mtmsr(mfmsr() & ~MSR_PMM);
	}
}

static void vSetPMLCaMarker(u32 event_output, u32 mark0, u32 mark1)
{
	switch (event_output) {
	case 0:
		PMC_SET_MARKED(PMR_PMLCa0, mark0, mark1);
		break;
	case 1:
		PMC_SET_MARKED(PMR_PMLCa1, mark0, mark1);
		break;
	case 2:
		PMC_SET_MARKED(PMR_PMLCa2, mark0, mark1);
		break;
	case 3:
		PMC_SET_MARKED(PMR_PMLCa3, mark0, mark1);
		break;
	default:
		log_err("Invalid Event Output %u\r\n", event_output);
		break;
	}
}
#endif

static uint32_t ulGetHAWK(u32 event_output)
{
	u32 ctr = 0;

	switch (event_output) {
	case 0:
		ctr = PMC_CTR_READ(PMR_PMC0);
		break;
	case 1:
		ctr = PMC_CTR_READ(PMR_PMC1);
		break;
	case 2:
		ctr = PMC_CTR_READ(PMR_PMC2);
		break;
	case 3:
		ctr = PMC_CTR_READ(PMR_PMC3);
		break;
	default:
		log_err("Invalid Event Output %u\r\n", event_output);
		break;
	}

	return ctr;
}

static int32_t lHAWKStatCommandHandler(u32 uEventCount,  u32 *uEventOutputPtr, 
				      u32 *uEventPtr, u32 uElapsedTime,
				      u32 uTracePID)
{
	u32 ulStartCtr[uEventCount], ulEndCtr, ulElapsedCtr;
#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
	TaskHandle_t xTraceTask = NULL;
#endif
	u32 i;

	UNUSED(uTracePID);
	
	for (i = 0; i < uEventCount; i++) {
		log_dbg("uEventOutputPtr[%u] = %u, uEvent[%u] = %u \r\n", 
				i, uEventOutputPtr[i], 
				i, uEventPtr[i]);
		vSetPMLCaEvent(uEventOutputPtr[i], uEventPtr[i]);
		ulStartCtr[i] = ulGetHAWK(uEventOutputPtr[i]);
	}
	log_dbg("uElapsedTime %u \r\n", uElapsedTime);
	log_dbg("uTracePID %u \r\n", uTracePID);

#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
	if (uTracePID) {
		xTraceTask = xTaskGetHandleFromTaskNumber(uTracePID);

		if ( !xTraceTask ) {
			log_err("ERROR: Task not found !, uTracePID = %u\r\n", uTracePID);
			return -1;
		}
		/* mark the process to be traced */
		vTaskSetTrace(xTraceTask, pdTRUE);

		for (i = 0; i < uEventCount; i++) {
			vSetPMLCaMarker(uEventOutputPtr[i], 0, 1);
		}
	}
#endif

	PMC_START();

	vTaskDelay( pdMS_TO_TICKS(uElapsedTime * 1000) );

	PMC_STOP();

#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
	/* mark the process __not__ to be traced */
	if ( uTracePID && xTraceTask ) {
		vTaskSetTrace(xTraceTask, pdFALSE);
		for (i = 0; i < uEventCount; i++) {
			vSetPMLCaMarker(uEventOutputPtr[i], 1, 1);
		}
	}
#endif
	for (i = 0; i < uEventCount; i++) {
		ulEndCtr = ulGetHAWK(uEventOutputPtr[i]);

		ulElapsedCtr = (ulEndCtr > ulStartCtr[i]) ? (ulEndCtr - ulStartCtr[i]) :
			((UINT32_MAX - ulStartCtr[i]) + ulEndCtr);

		log_info("\r\tC%u:%u = %u\t%s\r\n", uEventOutputPtr[i], uEventPtr[i],
				ulElapsedCtr, cHAWKEventTable[uEventPtr[i]].event_string);
	}
	return 0;
}

u32 uEventArr[HAWK_MAX_EVENTS] = {0}, uEventOutputArr[HAWK_MAX_EVENTS] = {0};
u32 uEventCtr, uElapsedTimeVal, uTracePIDVal;

static portBASE_TYPE prvHAWKCommandHandler (BaseType_t lParameterNumber,
					   char * pcParam,
					   BaseType_t lParameterStrLen)
{
	BaseType_t xReturn = pdFALSE;
	static char cHAWKCommandString[ 16 ];
	static char cParam[ 64 ];
	static u32 uHAWKCommandEvent, uHAWKCommandElaspedTime, uHAWKCommandTracePID;
	u32 uEvent = 0, uEventOutput, uElapsedTime = 0, uTracePID = 0;

	log_dbg("lParameterNumber=%u, pcParam = %s, lParameterStrLen = %d\r\n",
			lParameterNumber, pcParam, lParameterStrLen);

	if (!lParameterNumber && !pcParam && !lParameterStrLen) {
		return xReturn;
	}

	if( strncmp( pcParam, "list", lParameterStrLen ) == 0 ) {
		log_dbg("hawk list\r\n");
		vHAWKListCommand();
		strncpy(cHAWKCommandString, pcParam, lParameterStrLen);
		xReturn = pdTRUE;
	} else if (strncmp( pcParam, "help", lParameterStrLen ) == 0 ) {
		log_dbg( "hawk help\r\n");
		vHAWKHelpCommand();
		strncpy(cHAWKCommandString, pcParam, lParameterStrLen);
		xReturn = pdTRUE;
	} else if ( strncmp( cHAWKCommandString, "stat", strlen("stat") ) == 0 || 
			strncmp( pcParam, "stat", lParameterStrLen ) == 0 ) {
		log_dbg("hawk stat \r\n");

		if (strncmp( pcParam, "stat", lParameterStrLen) == 0 ) { 
			uElapsedTimeVal = 0;
			uTracePIDVal = 0;
			strncpy(cHAWKCommandString, pcParam, lParameterStrLen);
		} else if (strncmp( pcParam, "-e", lParameterStrLen) == 0 ) { 
			log_dbg("-e \r\n");
			uHAWKCommandEvent = 1;	
		} else if ( strncmp( pcParam, "-t", lParameterStrLen) == 0 ) { 
			log_dbg("-t \r\n");
			uHAWKCommandEvent = 0;	
			uHAWKCommandElaspedTime = 1;
#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
		} else if ( strncmp( pcParam, "-p", lParameterStrLen) == 0 ) {
			log_dbg("-p \r\n");
			uHAWKCommandEvent = 0;
			uHAWKCommandTracePID = 1;
#endif
		} else {
			log_dbg("lParameterNumber=%u, pcParam = %s, lParameterStrLen = %d\r\n",
					lParameterNumber, pcParam, lParameterStrLen);

			/* take a copy before calling strtok */
			strncpy(cParam, pcParam, lParameterStrLen);
			cParam[lParameterStrLen] =  0;

			if (uHAWKCommandEvent) {
				uEventOutput = strtoul( strtok(cParam, ":"), ( char ** ) NULL, 10 );
				uEvent = strtoul( strtok(NULL, ":"), ( char ** ) NULL, 10 );

				log_dbg("uEventOutput %u, uEvent %u\r\n",
						uEventOutput, uEvent);

				if (uEventOutput > 4 || uEvent > PMLCa_EVENT_NUM_MAX) {
					log_err("Incorrect command parameter(s).\r\n");
					goto err;
				}

				uEventOutputArr[uEventCtr] = uEventOutput;
				uEventArr[uEventCtr++] = uEvent;

			} else if (uHAWKCommandElaspedTime) {
				uElapsedTime = strtoul( pcParam, ( char ** ) NULL, 10 );

				log_dbg("uElapsedTime %u \r\n", uElapsedTime);	
				uHAWKCommandElaspedTime = 0;

				uElapsedTimeVal = uElapsedTime;
			} else if (uHAWKCommandTracePID) {
				uTracePID = strtoul( pcParam, ( char ** ) NULL, 10 );

				log_dbg("uTracePID %u \r\n", uTracePID);
				uHAWKCommandTracePID = 0;

				uTracePIDVal = uTracePID;
			}
		}

		xReturn = pdTRUE;
	} else {
		log_err("Incorrect command parameter(s).\r\n");
		goto err;
	}

	return xReturn;
err:
	vHAWKHelpCommand();
	return pdFALSE;
}

static portBASE_TYPE prvHAWKCommand( char * pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char * pcCommandString )
{
	char * pcParam;
	BaseType_t lParameterStrLen, xReturn = pdFALSE;
	static BaseType_t lParameterNumber = 0;

	( void ) xWriteBufferLen;

	if( lParameterNumber == 0 ) {
		lParameterNumber = 1L;
		xReturn = pdPASS;
	} else { 

		pcParam = (char *) FreeRTOS_CLIGetParameter( pcCommandString, lParameterNumber, &lParameterStrLen );

		if (pcParam != NULL) {
			xReturn = prvHAWKCommandHandler(lParameterNumber, pcParam, lParameterStrLen);
			lParameterNumber++;
		} else {
			xReturn = prvHAWKCommandHandler(0, NULL, 0);
			pcWriteBuffer[ 0 ] = 0x00;
			lParameterNumber = 0;

			if (uEventCtr) {
				(void) lHAWKStatCommandHandler(uEventCtr, uEventOutputArr, uEventArr, uElapsedTimeVal, uTracePIDVal);
				uEventCtr = 0;
			}
		}
	}

	return xReturn;
}


static int32_t lHAWKRecordCommandHandler(struct hawk_swcmd_stat * pxHAWKStatCmdDesc, 
					bool_t bEnable)
{
	static u32 ulStartCtr[HAWK_MAX_EVENTS];
	u32 ulEndCtr, ulElapsedCtr;
	u32 i;

	if (bEnable) { 
		for (i = 0; i < pxHAWKStatCmdDesc->event_count; i++) {
			log_dbg("uEventOutputPtr[%u] = %u, uEvent[%u] = %u \r\n", 
					i, pxHAWKStatCmdDesc->event_output[i], 
					i, pxHAWKStatCmdDesc->event[i]);
			vSetPMLCaEvent(pxHAWKStatCmdDesc->event_output[i], 
					pxHAWKStatCmdDesc->event[i]);
			ulStartCtr[i] = ulGetHAWK(pxHAWKStatCmdDesc->event_output[i]);
		}
		PMC_START();
	} else {
		PMC_STOP();

		for (i = 0; i < pxHAWKStatCmdDesc->event_count; i++) {
			ulEndCtr = ulGetHAWK(pxHAWKStatCmdDesc->event_output[i]);

			ulElapsedCtr = (ulEndCtr >= ulStartCtr[i]) ? (ulEndCtr - ulStartCtr[i]) :
				((UINT32_MAX - ulStartCtr[i]) + ulEndCtr);

			log_dbg("ulStartCtr[%u] %u, ulEndCtr %u, ulElapsedCtr %u\r\n",
					i, ulStartCtr[i], ulEndCtr, ulElapsedCtr);

			pxHAWKStatCmdDesc->ctrs[crt_core_id][i] = ulElapsedCtr;
		}
	}
	return 0;
}

static int32_t lHAWKMarkCommandHandler(struct hawk_swcmd_stat * pxHAWKStatCmdDesc, 
					bool_t bEnable)
{
#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
	u32 i;

	if (bEnable) { 
		for (i = 0; i < pxHAWKStatCmdDesc->event_count; i++) {
			vSetPMLCaMarker(pxHAWKStatCmdDesc->event_output[i], 0, 1);
		}
	} else {
		for (i = 0; i < pxHAWKStatCmdDesc->event_count; i++) {
			vSetPMLCaMarker(pxHAWKStatCmdDesc->event_output[i], 1, 1);
		}
	}
#else
	UNUSED(pxHAWKStatCmdDesc);
	UNUSED(bEnable);
#endif
	return 0;
}

static int32_t prvHawkMark( HawkHandle_t xHandle,
		hawk_sw_cmd_desc_t volatile * pxHAWKSWCmdDesc, bool_t bEnable )
{
	struct hawk_swcmd_stat * pxHAWKStatCmdDesc = ( struct hawk_swcmd_stat * ) pxHAWKSWCmdDesc->data;

	(void) xHandle;

	lHAWKMarkCommandHandler(pxHAWKStatCmdDesc, bEnable);

	return HAWK_SW_CMD_RESULT_OK;
}

static int32_t prvHawkRecord( HawkHandle_t xHandle,
		hawk_sw_cmd_desc_t volatile * pxHAWKSWCmdDesc, bool_t bEnable )
{
	struct hawk_swcmd_stat * pxHAWKStatCmdDesc = ( struct hawk_swcmd_stat * ) pxHAWKSWCmdDesc->data;

	(void) xHandle;

	lHAWKRecordCommandHandler(pxHAWKStatCmdDesc, bEnable);

	return HAWK_SW_CMD_RESULT_OK;
}

int32_t iHandleHawkCmds( HawkHandle_t xHandle, hawk_sw_cmd_desc_t volatile *
		pxHAWKSWCmdDesc ) {
	
	int32_t iRet = HAWK_SW_CMD_RESULT_CMD_INVALID;

	if( pxHAWKSWCmdDesc->status[crt_core_id] != HAWK_SW_CMD_STATUS_POSTED ) { 
		log_err("ERROR: cmd status not posted\n");
		return HAWK_SW_CMD_RESULT_DESC_INVALID;
	}

	pxHAWKSWCmdDesc->status[crt_core_id] = HAWK_SW_CMD_STATUS_IN_PROGRESS;
	iRet = HAWK_SW_CMD_RESULT_BUSY;

	switch( pxHAWKSWCmdDesc->cmd ) {
		case HAWK_SW_CMD_RECORD_START:
			log_dbg("HAWK_SW_CMD_RECORD_START\r\n");
			iRet = prvHawkRecord(xHandle, pxHAWKSWCmdDesc, pdTRUE);
			break;
		case HAWK_SW_CMD_RECORD_STOP:
			log_dbg("HAWK_SW_CMD_RECORD_STOP\r\n");
			iRet = prvHawkRecord(xHandle, pxHAWKSWCmdDesc, pdFALSE);
			break;
		case HAWK_SW_CMD_MARK_SET:
			log_dbg("HAWK_SW_CMD_MARK_SET\r\n");
			iRet = prvHawkMark(xHandle, pxHAWKSWCmdDesc, pdTRUE);
			break;
		case HAWK_SW_CMD_MARK_RESET:
			log_dbg("HAWK_SW_CMD_MARK_RESET\r\n");
			iRet = prvHawkMark(xHandle, pxHAWKSWCmdDesc, pdFALSE);
			break;
		default:
			log_err("HAWK_SW_CMD_INVALID\r\n");
			iRet = HAWK_SW_CMD_RESULT_CMD_INVALID;
			break;
	}

	return iRet;
}

static volatile hawk_sw_cmd_desc_t * xGetHAWKSWCmdDesc( HawkHandle_t xHandle)
{
	return &( xHandle->pxHAWKMData->host_swcmd );
}

bool_t bHawkRemoteISR( uint32_t ulIrqNo,
		void * pvDevData )
{
	int i;
	HawkHandle_t xHandle = pvDevData;
	uint32_t msiOffset = 0x10;
	uint32_t msiNumber = ulIrqNo - MSI_INTR_START;
	volatile hawk_sw_cmd_desc_t * pxHAWKSWCmdDesc;

	pxHAWKSWCmdDesc = xGetHAWKSWCmdDesc(xHandle);
	log_dbg("bHawkRemoteISR, pxHAWKSWCmdDesc->core_mask %u\r\n", pxHAWKSWCmdDesc->core_mask);

	for (i = 0; i < E200_CORE_COUNT; i++) { 
		if ( (1 << i) & pxHAWKSWCmdDesc->core_mask) {
			if (vIPISendDatafromISR( i, IPI_EVT_HAWK, ( void * ) pxHAWKSWCmdDesc ) == pdFALSE) {	
				log_err("ERROR: IPI send data failed\r\n");
			}
		}
	}
	mpic_in32( MPIC_REGS_MSIR0 + msiNumber * msiOffset );
	return 0;
}

static int32_t hawkRegisterHostinterrupt( HawkHandle_t xHandle )
{
	int ret;

	ret = lRegisterIrq( ( uint32_t ) ( MSI_INTR_START + HOST_MSI_HAWK ), bHawkRemoteISR, ( void * )xHandle );

	if( 1 != ret )
	{
		log_err( "Hawk: IRQ register error:%d\r\n", HOST_MSI_HAWK );
		return -1;
	}

	bMpicEnable( DEVICE_SHARE_MESSAGE, HOST_MSI_HAWK );
	return 0;
}

void vHawkCoreTask( void * pvParameters )
{
	int32_t iRet;
	HawkHandle_t xHandle = pvParameters;
	hawk_sw_cmd_desc_t * pxHAWKSWCmdDesc = NULL;

	(void) xHandle;

	if (vIPIEventRegister(IPIGlobalEventID[IPI_EVT_HAWK],&pxRxQueue[ IPI_EVT_HAWK ], NULL, NULL)
			== IPI_EVT_ID_NULL) {
		log_err("ERROR: IPI event register failed\r\n");
		return;
	}
	syncUnSync();
	log_dbg("IPI event register success\r\n");

	while( 1 )
	{
		if (xQueueReceive( pxRxQueue[ IPI_EVT_HAWK ], &pxHAWKSWCmdDesc, portMAX_DELAY ) == pdPASS) {
			iRet = iHandleHawkCmds(pxHawkDevice, pxHAWKSWCmdDesc);
			pxHAWKSWCmdDesc->result[crt_core_id] = iRet;
			pxHAWKSWCmdDesc->status[crt_core_id] = HAWK_SW_CMD_STATUS_DONE;
		}

	}
}


void xHawkInit( void )
{
	pxHawkDevice = ( HawkDevice_t * ) ( pGulModPriv->pHif->hawk_hif.hawk_prv_mdata.hawkdev );
	if (crt_core_id == GEUL_E200_MASTER_CORE)
		memset( ( void * )pxHawkDevice, 0, sizeof( HawkDevice_t ) );
	pxHawkDevice->pxHAWKMData = &( pGulModPriv->pHif->hawk_hif.hawk_mdata );
	pxHawkDevice->pxPrvHAWKMData = &( pGulModPriv->pHif->hawk_hif.hawk_prv_mdata );

	if( xTaskCreate( vHawkCoreTask,
			 "HawkCoreTask",
			 HAWK_CORE_TASK_STACK_SIZE,
			 ( void * )pxHawkDevice,
			 HAWK_CORE_TASK_PRIORITY,
			 ( TaskHandle_t * )&( pxHawkDevice->xHawkCoreTask ) ) == pdPASS )
	{
		pxHawkDevice->xHawkRemoteQueue[ crt_core_id ] = xQueueCreate( 1, sizeof( uint32_t * ) );

		if( pxHawkDevice->xHawkRemoteQueue[ crt_core_id ] == NULL )
		{
			log_err("Hawk queue creation failed\r\n");
			goto fail;
		}
		log_dbg("pxHawkDevice->xHawkRemoteQueue[%d] %x\r\n", crt_core_id, pxHawkDevice->xHawkRemoteQueue[ crt_core_id ]);

		if (crt_core_id == GEUL_E200_MASTER_CORE)
			(void ) hawkRegisterHostinterrupt(pxHawkDevice);
		log_dbg("Hawk init success\r\n");
	} else {
		log_err("Hawk init failed\r\n");
		goto fail;
	}

fail:
	return;
}
