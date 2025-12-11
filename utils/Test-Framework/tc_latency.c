// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */
#include "tc_latency.h"
#if GEUL_DEMO_LATENCY_TEST
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "gpio.h"
#include "Time.h"
#include "tbgen_new.h"
#include "ppc.h"
#include "spinlock_api.h"
#include "queue.h"
#include "ipiQueue.h"
#include "soc.h"

#define IPI_LATENCY_DEMO_EVENT_NUM    1
static enum IPIEventID g_LatencyEventTestID[ IPI_LATENCY_DEMO_EVENT_NUM ] =
{
	IPI_EVT_ID1
};
u32 uRefFreq100us;
#define SAMPLE_COUNT 250
#define IPI_LATENCY_TEST_COUNT_NUM    SAMPLE_COUNT

#define TBGEN_TEST_ISR_LATENCY 		0
#define TBGEN_TEST_SEM_RELEASE_LATENCY 	1
#define TBGEN_TEST_QUE_SEND_LATENCY 	2
#define TBGEN_TEST_TASK_NOTIFY_LATENCY 	3
#define TBGEN_TEST_ISR_TO_TASK_LATENCY 	4

#define CMD_CONTEXT_SWITCH_SEM		1
#define CMD_CONTEXT_SWITCH_NOTIFY	2
#define CMD_CONTEXT_SWITCH_QUEUE	3
#define CMD_CONTEXT_SWITCH_ISR_TO_TSAK	4

u64 vGetTimedIntOffset( uint8_t ucTbgenNo, TimerInstance_t eInstance );
extern int ipi_latency;
static int gReceived;
extern u64 ipiStart;
static uint32_t *ulTbgenCount = NULL;
static uint32_t *ulTbgenCountSem = NULL;
static uint8_t ulTbgenTestMode;
static u64 ulActualExpiryCount;
static u64 ulExpectedExpiryCount;
static u64 ulTimeStampStart;
static u64 ulTimeStampEnd;
static SemaphoreHandle_t TestSemaphoreP1;
static SemaphoreHandle_t TestSemaphoreP2;
static TaskHandle_t Task1Handle;
static TaskHandle_t Task2Handle;
static QueueHandle_t Task1CmdQueue;
static QueueHandle_t Task2CmdQueue;
static uint32_t ulTaskIndex = 0;
static QueueSetHandle_t TestQueueSet;
static QueueHandle_t TestQueue;
static QueueHandle_t QueueSendLatency;
typedef struct {
	uint32_t ulTimedIntrCount;
	SemaphoreHandle_t xTiBinarySemaphore;
} TimedIntInfo_t;

static TimerParams_t *pxTiIntParams[ RX_ALIGNMENT_MAX_INSTANCE ];
static TimedIntInfo_t xTimedIntInfo[ RX_ALIGNMENT_MAX_INSTANCE ];

void vGeulLatencyTaskP1( void *pvParameters );
void vGeulLatencyTaskP2( void *pvParameters );

#define SEMAPHORE_PROTECTION		1
/* Number of samples/time diff between pulses that can be
 * stored.
 */
#define LATENCY_MAX_INTRRUPT_COUNT	SAMPLE_COUNT

u64 vGetTimedIntOffset( uint8_t ucTbgenNo, TimerInstance_t eInstance );

int mem_alloc(uint32_t **ptr, uint32_t size);

void vGeulIPILatencyDemoEntry()
{
	u64 ipiEnd;
	uint32_t diff;
	u32 current_core = ulMpicCurrentCore();
	u8 i, j;
	BaseType_t ret;
	u32 dstCore;
	u32 sent = 0;

	gReceived = 0;
	dstCore = 1;
#ifndef IPI_FUNCTION_CALLBACK
	void * rxData;
#endif
	for( i = 0; i < IPI_LATENCY_DEMO_EVENT_NUM; i++ )
	{
#ifdef IPI_FUNCTION_CALLBACK
		vIPIEventRegister( IPIGlobalEventID[ i ], &pxRxQueue[ i ], vIPIEventCallback, NULL );
#else
		vIPIEventRegister( IPIGlobalEventID[ i ], &pxRxQueue[ i ], NULL, NULL );
#endif
		g_LatencyEventTestID[ i ] = IPIGlobalEventID[ i ];
	}

	syncUnSync();
	if( current_core == 0 )
	{
		vTaskDelay(500);
		while( 1 )
		{
			for( j = 0; j < IPI_LATENCY_DEMO_EVENT_NUM; j++ )
			{
				if( sent < ( IPI_LATENCY_TEST_COUNT_NUM * ( get_soc_numcores() - 1 ) ) )
				{
					ret = vIPISendData( dstCore, g_LatencyEventTestID[ j ], ( void * ) current_core );

					if( ret == pdTRUE )
					{
						sent++;
						vTaskDelay( 1 );
					}
					else
					{
						log_info( "send from core %d to %d, with event %d failed, sent:%d\r\n", current_core,
								dstCore, g_LatencyEventTestID[ j ], sent );
					}
				}
				else
				{
					goto send_complete;
				}
			}

			if( dstCore == ( get_soc_numcores() - 1 ) )
			{
				dstCore = 1;
			}
			else
			{
				dstCore++;
			}
			vTaskDelay( 1 );
		}
	}
send_complete:
	if( current_core != 0 )
	{
		while( 1 )
		{
			for( i = 0; i < IPI_LATENCY_DEMO_EVENT_NUM; i++ )
			{
				ret = xQueueReceive( pxRxQueue[ i ], &rxData, portMAX_DELAY );
				ipiEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
				diff = ipiEnd - ipiStart;
				if( ret == pdTRUE )
				{
					ulTbgenCount[ gReceived ] = diff;
					gReceived++;
				}
			}
			if( ( gReceived >= ( IPI_LATENCY_TEST_COUNT_NUM ) ) )
			{
				break;
			}

#ifdef IPI_GLOBAL_Q_DBG
			vIPIGlobalQStatusCheck( ( int ) current_core, -1 );
#endif
		}
	}
	sent = 0;
	gReceived = 0;
	dstCore = 0;

	if( current_core == 1 )
	{
		vTaskDelay( 500 );
		while( 1 )
		{
			for( j = 0; j < IPI_LATENCY_DEMO_EVENT_NUM; j++ )
			{
				if( sent < ( IPI_LATENCY_TEST_COUNT_NUM ) )
				{
					ret = vIPISendData( dstCore, g_LatencyEventTestID[ j ], ( void * ) current_core );

					if( ret == pdTRUE )
					{
						sent++;
						vTaskDelay(1);
					}
					else
					{
						log_info( "send from core %d to %d, with event %d failed, sent:%d\r\n", current_core,
								dstCore, g_LatencyEventTestID[ j ], sent );
					}
				}
				else
				{
					goto send_complete_core1;
				}
			}

			dstCore = 0;
			vTaskDelay( 1 );
		}
	}
send_complete_core1:
	if( current_core == 0 )
	{
		while( 1 )
		{
			ret = xQueueReceive( pxRxQueue[ 0 ], &rxData, portMAX_DELAY );
			ipiEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
			diff = ipiEnd - ipiStart;
			if( ret == pdTRUE )
			{
				ulTbgenCount[ gReceived ] = diff;
				gReceived++;
			}
			if( ( gReceived >= ( IPI_LATENCY_TEST_COUNT_NUM ) ) )
			{
				break;
			}
		}
	}
}

void prvLatencyTimedIntTimerRepeatedCallback( uint8_t ucTbgenNo, TimerInstance_t eInstance, void *data )
{
	TimedIntInfo_t *pxTimedIntInfo = &xTimedIntInfo[ eInstance ];
	uint32_t diff = 0;
	(void) data;
	u64 ulStart = 0;
	u64 ulEnd = 0;
	u64 ulTbgenTime;
	u32 uTbgenFreqKhz = uGetTbgenFreq(ucTbgenNo);
#if SEMAPHORE_PROTECTION
	static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif
	if( ulTbgenTestMode == TBGEN_TEST_ISR_LATENCY )
	{
		ulActualExpiryCount = ullTbgenGetMasterCounterRaw( ucTbgenNo );
		ulExpectedExpiryCount = vGetTimedIntOffset( ucTbgenNo, eInstance );
		diff = ulActualExpiryCount - ulExpectedExpiryCount;
		if( pxTimedIntInfo->ulTimedIntrCount < SAMPLE_COUNT )
			ulTbgenCount[ pxTimedIntInfo->ulTimedIntrCount ] = diff;
		ulTbgenTime = ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenReloadTimer( ucTbgenNo, RX_ALIGNMENT, eInstance, uTbgenFreqKhz +  ulTbgenTime );
	}

	pxTimedIntInfo->ulTimedIntrCount++;

	if( ulTbgenTestMode == TBGEN_TEST_SEM_RELEASE_LATENCY )
	{
		ulStart = ullTbgenGetMasterCounterRaw( ucTbgenNo );
		xSemaphoreGiveFromISR( TestSemaphoreP1, &xHigherPriorityTaskWoken );
		ulEnd = ullTbgenGetMasterCounterRaw( ucTbgenNo );
		diff = ulEnd - ulStart;
		if( pxTimedIntInfo->ulTimedIntrCount < SAMPLE_COUNT )
			ulTbgenCountSem[ pxTimedIntInfo->ulTimedIntrCount ] = diff;
		xSemaphoreGiveFromISR( TestSemaphoreP2, &xHigherPriorityTaskWoken );
		ulTaskIndex++;
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	if( ulTbgenTestMode == TBGEN_TEST_TASK_NOTIFY_LATENCY )
	{
		vTaskNotifyGiveFromISR(Task2Handle, &xHigherPriorityTaskWoken);
		vTaskNotifyGiveFromISR(Task1Handle, &xHigherPriorityTaskWoken);
		ulTaskIndex++;
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	if( ulTbgenTestMode == TBGEN_TEST_QUE_SEND_LATENCY )
	{
		uint32_t ulTestData = 1;
		ulStart = ullTbgenGetMasterCounterRaw( ucTbgenNo );
		xQueueSendFromISR(TestQueue, &ulTestData, &xHigherPriorityTaskWoken );
		ulEnd = ullTbgenGetMasterCounterRaw( ucTbgenNo );
		diff = ulEnd - ulStart;
		if( pxTimedIntInfo->ulTimedIntrCount < SAMPLE_COUNT )
			ulTbgenCountSem[ pxTimedIntInfo->ulTimedIntrCount ] = diff;
		xSemaphoreGiveFromISR( TestSemaphoreP2, &xHigherPriorityTaskWoken );
		ulTaskIndex++;
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	if( ulTbgenTestMode == TBGEN_TEST_ISR_TO_TASK_LATENCY )
	{
		xSemaphoreGiveFromISR( TestSemaphoreP2, &xHigherPriorityTaskWoken );
		ulTaskIndex++;
		ulTimeStampStart = ullTbgenGetMasterCounterRaw( ucTbgenNo );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	if( pxTimedIntInfo->ulTimedIntrCount >= LATENCY_MAX_INTRRUPT_COUNT )
	{
		iTbgenDisableTimer( ucTbgenNo, RX_ALIGNMENT, eInstance );
		pxTimedIntInfo->ulTimedIntrCount = 0;
#if SEMAPHORE_PROTECTION
		// releapse the semaphore.
		//log_info( "ulIndex %d\r\n", ulTaskIndex );
		ulTaskIndex = 0;
		xSemaphoreGiveFromISR( pxTimedIntInfo->xTiBinarySemaphore, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		//log_info( "\n\r[TBGEN TC] Semaphore xTiBinarySemaphore Released INST%d.\n\r",  pxTiParams->eInstance );
#endif
	}
}

u64 vGetTimedIntOffset( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	u64 ullNewOffsetValue;
	vuint32 * puiOffsetHi = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[RX_ALIGNMENT] + TIMER_HI_OFFSET + (TIMER_BLOCK_SIZE * eInstance) );
	vuint32 * puiOffsetLo = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[RX_ALIGNMENT] + TIMER_LO_OFFSET + (TIMER_BLOCK_SIZE * eInstance) );

	ullNewOffsetValue = TBGEN_READ_REGISTER( puiOffsetHi );
	ullNewOffsetValue = ullNewOffsetValue << 32;
	ullNewOffsetValue = ullNewOffsetValue | TBGEN_READ_REGISTER( puiOffsetLo );
	return ullNewOffsetValue;
}

void vTbgenLatencyTest( void )
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	int instance = TIMER_INSTANCE_0 + uiCurrentCore;
	u64 ulTbgenTime;
    uint8_t ucTbgenNo = TBGEN_2;
	u32 uTbgenFreqKhz = uGetTbgenFreq(ucTbgenNo);

    if( instance >= RX_ALIGNMENT_MAX_INSTANCE )
    {
#ifdef GEUL_LA1246
        log_info( "Cant execute on %u core, As there are only %u Tbgen2 external interrupts\r\n", instance, RX_ALIGNMENT_MAX_INSTANCE );
        return;
#endif
        log_info( "Executing using TBGEN1 on core %u, As there are only %u Tbgen2 external interrupts\r\n", instance, RX_ALIGNMENT_MAX_INSTANCE );
        instance = instance - RX_ALIGNMENT_MAX_INSTANCE;
        ucTbgenNo = TBGEN_1;
    }
#if SEMAPHORE_PROTECTION
	static int iIsInitialized = 0;
	if( !iIsInitialized )
	{
		/* Init TimedInt INSTANCE */
		pxTiIntParams[ instance ] = ( TimerParams_t * ) pvPortMalloc( sizeof( TimerParams_t ) );
		log_info( "Initialising TTI INSTANCE%d\r\n", instance );
		log_info( "[TBGEN TC] Creating Semaphore xTiBinarySemaphoreTTI%d...", instance );
		// Create the semaphore to guard a shared resource,
		xTimedIntInfo[ instance ].xTiBinarySemaphore =
			xSemaphoreCreateBinary( );
		log_info( "Done.\r\n" );

		iIsInitialized = 1;
	}
#endif
	ulTbgenTime = ullTbgenGetMasterCounter( ucTbgenNo );

	/* Configure Timed Interrupt */
	pxTiIntParams[ instance ]->ePolarity = STROBE_POL_RISING;		/* Polarity Rising */
	if( ulTbgenTestMode == TBGEN_TEST_ISR_LATENCY )
		pxTiIntParams[ instance ]->uOffset =  uTbgenFreqKhz + ulTbgenTime;
	else
		pxTiIntParams[ instance ]->uOffset =  (uTbgenFreqKhz / 8) + ulTbgenTime;
	pxTiIntParams[ instance ]->pvCb = prvLatencyTimedIntTimerRepeatedCallback;	/* Timer Cb */
	pxTiIntParams[ instance ]->eSm = STROBE_MODE_PULSE; /* Pulse Mode */
	pxTiIntParams[ instance ]->ePw = PULSE_WIDTH_CLK_CYCLE_16; /* If PulseMode = STROBE_MODE_PULSE */
	/* Pulse Mode - REPETITIVE */
	if( ulTbgenTestMode == TBGEN_TEST_ISR_LATENCY )
		pxTiIntParams[ instance ]->eTrigMode = TM_ONE_SHOT;
	else
	{
		pxTiIntParams[ instance ]->eTrigMode = TM_REPETITIVE;
		pxTiIntParams[ instance ]->uInterval = uTbgenFreqKhz / 8;
	}
	iTbgenProgramTimer( ucTbgenNo, RX_ALIGNMENT, instance, pxTiIntParams[ instance ] );
	iTbgenEnableTimer( ucTbgenNo, RX_ALIGNMENT, instance );
	while( xSemaphoreTake( xTimedIntInfo[ instance ].xTiBinarySemaphore,
				( TickType_t ) portMAX_DELAY ) == pdFALSE )
	{
		log_info( "\n\r[TBGEN TC] Waiting for semaphore to be released in Timed Interrupt ISR...", __func__ );
	};
}

void vGeulLatencyTaskP1( void *pvParameters )
{
	(void) pvParameters;
	uint32_t ulCmd = 0;
	uint32_t QueueData;
	for(; ; )
	{
		xQueueReceive( Task1CmdQueue, &ulCmd, portMAX_DELAY );
		for( ; ; )
		{
			if( ulTaskIndex == SAMPLE_COUNT - 1 )
				break;
			switch( ulCmd )
			{
				case CMD_CONTEXT_SWITCH_SEM:
					ulTimeStampStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
					xSemaphoreTake( TestSemaphoreP1, portMAX_DELAY );
					break;
				case CMD_CONTEXT_SWITCH_NOTIFY:
					ulTimeStampStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
					ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
					break;
				case CMD_CONTEXT_SWITCH_QUEUE:
					ulTimeStampStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
					xQueueSelectFromSet(TestQueueSet ,portMAX_DELAY );
					xQueueReceive(TestQueue, &QueueData, portMAX_DELAY);
					break;
				default:
					break;
			}
		}
	}
}

void vGeulLatencyTaskP2( void *pvParameters )
{
	(void) pvParameters;
	uint32_t tmp = 0;
	uint32_t result = 0;
	uint32_t ulCmd = 0;
	for(; ;)
	{
		xQueueReceive( Task2CmdQueue, &ulCmd, portMAX_DELAY );
		for( ; ; )
		{
			if( ulTaskIndex == SAMPLE_COUNT - 1 )
				break;
			switch( ulCmd )
			{
				case CMD_CONTEXT_SWITCH_ISR_TO_TSAK:
				case CMD_CONTEXT_SWITCH_SEM:
					xSemaphoreTake( TestSemaphoreP2, portMAX_DELAY );
					ulTimeStampEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
					break;
				case CMD_CONTEXT_SWITCH_NOTIFY:
					ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
					ulTimeStampEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
					break;
				case CMD_CONTEXT_SWITCH_QUEUE:
					xSemaphoreTake( TestSemaphoreP2, portMAX_DELAY );
					ulTimeStampEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
					break;
				default:
					break;
			}
			if( ulTaskIndex >= SAMPLE_COUNT -1 )
			{
				tmp = 0;
				for(uint32_t i = 1; i < SAMPLE_COUNT; i++)
				{
					tmp += ulTbgenCount[ i ];
				}
				result = ( uint32_t )( tmp/( SAMPLE_COUNT - 1 ) );
				switch( ulCmd )
				{
					case CMD_CONTEXT_SWITCH_SEM:
						PRINTF( "TaskToTask Latency(Semaphore)");
						break;
					case CMD_CONTEXT_SWITCH_NOTIFY:
						PRINTF( "TaskToTask Latency(TaskNotify)");
						break;
					case CMD_CONTEXT_SWITCH_QUEUE:
						PRINTF( "TaskToTask Latency(QueueSet)");
						break;
					case CMD_CONTEXT_SWITCH_ISR_TO_TSAK:
						PRINTF( "ISRToTask Latency(PortYield)");
						break;
				}
				PRINTF( "\t\t= %u/%u\r\n", result, uRefFreq100us );
				break;
			}
			else
			{
				if( ulTaskIndex < SAMPLE_COUNT )
				{
					ulTbgenCount[ ulTaskIndex ] = ulTimeStampEnd - ulTimeStampStart;
				}
			}
		}
	}
}

int mem_alloc(uint32_t **ptr, uint32_t size)
{
	if (*ptr == NULL) {
		*ptr = pvPortMalloc(sizeof(ptr)*size);
		if (*ptr == NULL) {
			PRINTF(" Error: %s - Unable to allocate memory \n\r",__func__);
			return pdFAIL;
		}
	}
	memset(*ptr, 0, size);
	return pdPASS;
}

void vMeasureIsrLatency( void )
{
	int i;
	uint32_t tmp = 0;
	uint32_t result = 0;

	result = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (result == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	result = mem_alloc(&ulTbgenCountSem, sizeof(ulTbgenCountSem)*SAMPLE_COUNT);
	if (result == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ulTbgenTestMode = TBGEN_TEST_ISR_LATENCY;
	vTbgenLatencyTest( );
	for( i = 0; i < SAMPLE_COUNT; i++ )
	{
		tmp += ulTbgenCount[ i ];
	}
	result = ( uint32_t )( tmp/SAMPLE_COUNT );
	PRINTF( "Tbgen ISR Latency\t\t\t= %u/%u\r\n", result, uRefFreq100us );
	PRINTF( "TaskToIsr Latency\t\t\t= %u/%u\r\n", result, uRefFreq100us );
}
void vMeasureContextLatencySemaphore( void )
{
	int i, ret = 0;
	uint32_t tmp = 0;
	uint32_t ulCmd = 0;

	ret = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}
	ret = mem_alloc(&ulTbgenCountSem, sizeof(ulTbgenCountSem)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ulCmd = CMD_CONTEXT_SWITCH_SEM;
	xQueueSend( Task1CmdQueue, &ulCmd, 1000);
	xQueueSend( Task2CmdQueue, &ulCmd, 1000);
	ulTbgenTestMode = TBGEN_TEST_SEM_RELEASE_LATENCY;
	vTbgenLatencyTest( );
	for( i = 1; i < SAMPLE_COUNT; i++ )
	{
		tmp += ulTbgenCountSem[ i ];
	}
	PRINTF( "SemReleaseFromISR\t\t\t= %u/%u\r\n", (tmp/(SAMPLE_COUNT-2)), uRefFreq100us );
}

void vMeasureContextLatencyTaskNotify( void )
{
	int ret = 0;
	uint32_t ulCmd = 0;

	ret = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}
	ret = mem_alloc(&ulTbgenCountSem, sizeof(ulTbgenCountSem)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ulCmd = CMD_CONTEXT_SWITCH_NOTIFY;
	xQueueSend( Task1CmdQueue, &ulCmd, 1000);
	xQueueSend( Task2CmdQueue, &ulCmd, 1000);
	ulTbgenTestMode = TBGEN_TEST_TASK_NOTIFY_LATENCY;
	vTbgenLatencyTest( );
}
void vMeasureContextLatencyIsrToTask( void )
{
	int ret = 0;
	uint32_t ulCmd = 0;

	ret = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ret = mem_alloc(&ulTbgenCountSem, sizeof(ulTbgenCountSem)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ulCmd = CMD_CONTEXT_SWITCH_ISR_TO_TSAK;
	xQueueSend( Task2CmdQueue, &ulCmd, 1000);
	ulTbgenTestMode = TBGEN_TEST_ISR_TO_TASK_LATENCY;
	vTbgenLatencyTest( );
}
void vMeasureContextLatencyQueue( void )
{
	int i, ret = 0;
	uint32_t tmp = 0;
	uint32_t ulCmd = 0;

	ret = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ret = mem_alloc(&ulTbgenCountSem, sizeof(ulTbgenCountSem)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ulCmd = CMD_CONTEXT_SWITCH_QUEUE;
	xQueueSend( Task1CmdQueue, &ulCmd, 1000);
	xQueueSend( Task2CmdQueue, &ulCmd, 1000);
	ulTbgenTestMode = TBGEN_TEST_QUE_SEND_LATENCY;
	vTbgenLatencyTest( );
	for( i = 1; i < SAMPLE_COUNT; i++ )
	{
		tmp += ulTbgenCountSem[ i ];
	}
	PRINTF( "QueueSendFromISR\t\t\t= %u/%u\r\n", (tmp/(SAMPLE_COUNT-2)), uRefFreq100us );
}
void vMeasureQueueSendLatency( )
{
	u64 ulTimeStart = 0;
	u64 ulTimeEnd = 0;
	uint32_t diff;
	uint32_t ulTmp = 0;
	uint32_t ulTestData = 1;
	int i;
	uint32_t result;
	for( i = 0; i < SAMPLE_COUNT; i++ )
	{
		ulTimeStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
		xQueueSend( QueueSendLatency, &ulTestData, 10);
		ulTimeEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
		diff = ulTimeEnd - ulTimeStart;
		ulTmp += (uint32_t)diff;
		if(xQueueReceive( QueueSendLatency, &ulTestData, 0 )  != pdPASS)
                    PRINTF( "QueueReceive Fail\r\n" );
	}
	result = (uint32_t)(ulTmp/SAMPLE_COUNT);
	PRINTF( "QueueSendLatency\t\t\t= %u/%u\r\n", result, uRefFreq100us );

}
void vSpinlockLatency( void )
{
	static int iIsInitialized = 0;
	uint32_t ulTmpAcq = 0;
	uint32_t ulTmpRel = 0;
	uint32_t diff = 0;
	u64 ulTimeStart = 0;
	u64 ulTimeEnd = 0;
	uint32_t result;
	static struct SpinLock * pxSpinLockVar;
	int i;

	if( !iIsInitialized )
	{
		pxSpinLockVar = pxSpinLockAlloc( NULL, NULL );
		iIsInitialized = 1;
	}
	for( i = 0; i < SAMPLE_COUNT; i++ )
	{
		ulTimeStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
		vSpinLockAcquire( pxSpinLockVar );
		ulTimeEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
		diff = ulTimeEnd - ulTimeStart;
		ulTmpAcq += (uint32_t)diff;

		ulTimeStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
		vSpinLockRelease( pxSpinLockVar );
		ulTimeEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
		diff = ulTimeEnd - ulTimeStart;
		ulTmpRel += (uint32_t)diff;
	}
	result = (uint32_t)(ulTmpAcq/SAMPLE_COUNT);
	PRINTF( "\r\nSpinLockAcqLatency\t\t\t= %u/%u\r\n", result, uRefFreq100us );
	result = (ulTmpRel/SAMPLE_COUNT);
	PRINTF( "SpinLockRelLatency\t\t\t= %u/%u\r\n", result, uRefFreq100us );
}

void gpioLatency( void )
{
	uint32_t gpioDataRegValue=0, diff;
	u64 ulTimeStart = 0;
	u64 ulTimeEnd = 0;
	uint32_t uGpioSetTime = 0;
	uint32_t uGpioGetTime = 0;
	uint32_t result;
	int i;

	if( GPIO_SUCCESS == exGpioInit(GPIO_3, 23, GPIO_OUTPUT))
	{
		for( i = 0; i < SAMPLE_COUNT; i++ )
		{
			ulTimeStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
			exGpioSetRFData(GPIO_3, 23, 1);
			ulTimeEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
			diff = ulTimeEnd - ulTimeStart;
			uGpioSetTime += (uint32_t)diff;

			ulTimeStart = ullTbgenGetMasterCounterRaw( TBGEN_2 );
			exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
			ulTimeEnd = ullTbgenGetMasterCounterRaw( TBGEN_2 );
			diff = ulTimeEnd - ulTimeStart;
			uGpioGetTime += (uint32_t)diff;
		}
		result = (uint32_t)(uGpioSetTime / SAMPLE_COUNT);
		log_info("\n\rGPIO API ExGpioSetRFData\t\t= %u/%u", result, uRefFreq100us);
		result = (uint32_t)(uGpioGetTime / SAMPLE_COUNT);
		log_info("\n\rGPIO API exGpioGetDataRegister\t\t= %u/%u", result, uRefFreq100us);
	}
	else
	{
		log_info( "\n\rGPIO init failed" );
	}
}

void vIPILatencyTest()
{
	uint32_t tmp = 0;
	int i, ret = 0;
	uRefFreq100us = uGetTbgenFreq(TBGEN_2) / 10;

	ret = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	vGeulIPILatencyDemoEntry( );
	for( i = 0; i < IPI_LATENCY_TEST_COUNT_NUM; i++ )
	{
		tmp += ulTbgenCount[ i ];
	}
	log_info("\n\rResult \t\t\t\t\t= <cycles taken>/<cycles for 100us> \r\n");
	PRINTF("IPI receive latency\t\t\t= %u/%u", (tmp/IPI_LATENCY_TEST_COUNT_NUM), uRefFreq100us );
}

void vGeulLatencyTest()
{
	int ret = 0;
	u32 current_core = ulMpicCurrentCore();
	static int iIsInitialized = 0;
	uRefFreq100us = uGetTbgenFreq(TBGEN_2) / 10;

	ret = mem_alloc(&ulTbgenCount, sizeof(ulTbgenCount)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	ret = mem_alloc(&ulTbgenCountSem, sizeof(ulTbgenCountSem)*SAMPLE_COUNT);
	if (ret == pdFAIL) {
		PRINTF("%s : Memory Allocation Failed!\n\r",__func__);
		return;
	}

	if( !iIsInitialized )
	{
		TestSemaphoreP1 = xSemaphoreCreateBinary();
		TestSemaphoreP2 = xSemaphoreCreateBinary();
		Task1CmdQueue = xQueueCreate( 1, sizeof( uint32_t ) );
		Task2CmdQueue = xQueueCreate( 1, sizeof( uint32_t ) );
		TestQueue = xQueueCreate( 1, sizeof(uint32_t) );
		TestQueueSet = xQueueCreateSet( sizeof(uint32_t) );
		xQueueAddToSet( TestQueue, TestQueueSet );
		QueueSendLatency = xQueueCreate( SAMPLE_COUNT, sizeof(uint32_t) );
		xTaskCreate(vGeulLatencyTaskP1, "LatencyP1", LATENCY_TASK1_STACKSIZE,
				NULL, LATENCY_TASK1_PRIORITY, &Task1Handle);
		xTaskCreate(vGeulLatencyTaskP2, "LatencyP2", LATENCY_TASK2_STACKSIZE,
				NULL, LATENCY_TASK2_PRIORITY, &Task2Handle);
		iIsInitialized = 1;
	}


	log_info("\n\r============Latency Table=====================\r\n");
	log_info("\n\rResult \t\t\t\t\t= <cycles taken>/<cycles for 100us> \r\n");

#if GEUL_GPIO_LATENCY_TEST
	gpioLatency();
#endif
	vSpinlockLatency( );
	vMeasureIsrLatency( );
	vMeasureContextLatencySemaphore( );
	vMeasureContextLatencyTaskNotify( );
	vMeasureContextLatencyQueue( );
	vMeasureQueueSendLatency( );
	vMeasureContextLatencyIsrToTask( );
	//TEST_COMPLETE:
	log_info("\n\r==============================================");
	SET_TEST_STATUS( current_core, GEUL_DEMO_LATENCY_TEST_STATUS );
	return;
}

#endif /* GEUL_DEMO_IPI_QUEUE_TEST */
