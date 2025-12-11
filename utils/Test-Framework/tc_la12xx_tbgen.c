// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2024 NXP
 */

#include "tc_la12xx_tbgen.h"
#include "FreeRTOS.h"
#include "task.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "tbgen_new.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "platform_def.h"
#include "gul_host_if.h"

/* Time defined in terms of TBGEN1 Ref clock */
#define TBGEN1_25_US		( TBGEN1_REF_CLK * 25 )
#define TBGEN1_125_US		( TBGEN1_25_US * 5 )
#define TBGEN1_250_US		( TBGEN1_125_US * 2 )
#define TBGEN1_500_US		( TBGEN1_250_US * 2 )
#define TBGEN1_1000_US		( TBGEN1_500_US * 2 )
#define TBGEN1_2000_US		( TBGEN1_1000_US * 2 )

/* Time defined in terms of TBGEN2 Ref clock */
#define TBGEN2_25_US		( TBGEN2_REF_CLK * 25 )
#define TBGEN2_125_US		( TBGEN2_25_US * 5 )
#define TBGEN2_250_US		( TBGEN2_125_US * 2 )
#define TBGEN2_500_US		( TBGEN2_250_US * 2 )
#define TBGEN2_1000_US		( TBGEN2_500_US * 2 )
#define TBGEN2_2000_US		( TBGEN2_1000_US * 2 )

/* TBGEN2 external interrupt line for RX_ALIGNMENT Instance 0 */
#define TBGEN2_XIRQ_INT_STROBE_0		( 8 )
/* TBGEN1 external interrupt line for RX_ALIGNMENT Instance 0 */
#define TBGEN1_XIRQ_INT_STROBE_0		( 5 )

#define TM_REP_TEST_CYCLES				100
/* Number of samples/time diff between pulses that can be
 * stored.
 */
#define TIME_DIFF_BUF_SIZE		250
/* Number of samples to perform test
 * out of MAX_INTRRUPT_COUNT last TIME_DIFF_BUF_SIZE
 * will be stored.
 */
#define MAX_INTRRUPT_COUNT		5000

static uint32_t tti_interval_div = TBGEN_TTI_INTERVAL_125_DIV;
typedef struct {
	struct Time TimedIntPrevTime;
	uint32_t *pulTimeDiffBuff;
	uint32_t ulTimedIntrCount;
	SemaphoreHandle_t xTiBinarySemaphore;
} RxAlignIntInfo_t;
static RxAlignIntInfo_t xRxAlignIntInfo;

TddTimerParams_t TddParams;

uint32_t uIsRFG1Initialized = 0;
uint32_t uIsRFG2Initialized = 0;
gul_mod_priv_t * pGulModPriv;

static u32 uiMinDiff = 0, uiMaxDiff = 0, uiAvgDiff = 0, uiDiffSum = 0;
static int iJitter = 0, iMinJitter = 0, iMaxJitter = 0, iAvgJitter = 0, iJitterSum = 0;
static u32 ulTiDiff = 0;

extern int TbgenTotalInstGenHostTTI[ TBGEN_MAX ];
extern TbgenHostTTIConf_t xTbgen1HostTTIConf[2];
extern TbgenHostTTIConf_t xTbgen2HostTTIConf[2];
extern volatile uint32_t brd_ver;

SemaphoreHandle_t xCSGBinarySemaphore = NULL;
SemaphoreHandle_t xTiBinarySemaphore = NULL;

volatile u64 *ulRxAlignTimeStamp = NULL;
volatile uint8_t uiCompletionFlag = 0;
volatile uint8_t ulCounter = 0;

void vTddIntCb( uint8_t ucTbgenNo, TimerInstance_t eInstance, void * pxTimerParams )
{
	static BaseType_t xHigherPriorityTaskWoken;

	(void)pxTimerParams;
	log_dbg("\n\r[TBGEN TC] Entered TBGEN TDD Cb...Instance -> %d", eInstance);

	iTbgenTimerInterruptClr( ucTbgenNo, TDD, eInstance );
	iTbgenDisableTddTimer( ucTbgenNo, eInstance );
	// release the semaphore.
	xSemaphoreGiveFromISR( xCSGBinarySemaphore, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	log_info("\n\rSemaphore xCSGBinarySemaphore Released.\n\r");
}

static void prvTbgenInitCsgParams( uint8_t ucTbgenNo, TimerInstance_t eTddInstance, TddTimerParams_t * pxParams )
{
	u8 ucSeqIndex = 0;
	u32 uDuration = TBGEN1_REF_CLK * 8;
	u64 uOffset = TBGEN1_1000_US;

	if( ucTbgenNo == TBGEN_2 )
	{
		uDuration = TBGEN2_REF_CLK * 8;
		uOffset = TBGEN2_1000_US;
	}
	iConfPMuxModeTbgenTdd( eTddInstance, 1 );
	pxParams->ucTddSeqSteps = MAX_TDD_SEQUENCE_STEPS;
	for ( ucSeqIndex = 0; ucSeqIndex < MAX_TDD_SEQUENCE_STEPS; ucSeqIndex++ )
	{
		if((ucSeqIndex % 2) == 0) {
			pxParams->xDuration[ucSeqIndex].uDur = uDuration;
			pxParams->xDuration[ucSeqIndex].eMode = TDD_MODE_01;
		}
		else {
			pxParams->xDuration[ucSeqIndex].uDur = uDuration;
			pxParams->xDuration[ucSeqIndex].eMode = TDD_MODE_10;
		}
	}
	pxParams->uOffset = uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
	pxParams->ePm = TDD_PULSE_MODE_00;
	pxParams->uPw = 16;
	pxParams->eTrigMode = TM_ONE_SHOT;
	pxParams->pvCb = ( TimerCallbackFn ) vTddIntCb;
}

void vTddInitSeq( uint8_t ucTbgenNo )
{
	u32 uiCurrentCore = (u32)ulMpicCurrentCore();
	static int iIsCSGInitialized = 0;

	if( !iIsCSGInitialized )
	{
		log_info( "\n\r[TBGEN TC] : Creating Semaphore xCSGBinarySemaphore..." );
		// Create the semaphore to guard a shared resource,
		xCSGBinarySemaphore = xSemaphoreCreateBinary();
		log_info("Done.");
		iIsCSGInitialized = 1;
	}

	log_info("\n\r[TBGEN TC] Entered : %s", __func__);
	prvTbgenInitCsgParams( ucTbgenNo, TIMER_INSTANCE_4, &TddParams );
	iTbgenProgramTddTimer( ucTbgenNo, TIMER_INSTANCE_4, &TddParams );
	iTbgenEnableTddTimer( ucTbgenNo, TIMER_INSTANCE_4 );

	// and wait for it to be acquired
	while( xSemaphoreTake( xCSGBinarySemaphore, ( TickType_t ) 10 ) == pdFALSE )
	{
		log_info( "\n\r[TBGEN TC] : Waiting for CSG semaphore to be released in CSG ISR...", __func__ );
	};
	log_info( "Semaphore Obtained for instance %d\r\n", TIMER_INSTANCE_4 );
	log_info( "\n\rTested Tbgen2 TIMER_INSTANCE_4 with 16 TDD steps");
	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
	SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_TBGEN2_TDD_TEST_STATUS);
}

void vTddInitManual( uint8_t ucTbgenNo )
{
	u32 uiCurrentCore = (u32)ulMpicCurrentCore();

	log_info("\n\r[TBGEN TC] Entered : %s \n\r", __func__);

	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_0, TDD_MODE_10 );
	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_1, TDD_MODE_10 );

	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_2, TDD_MODE_01 );
	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_3, TDD_MODE_01 );

	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_4, TDD_MODE_00 );
	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_5, TDD_MODE_00 );

	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_6, TDD_MODE_11 );
	iTbgenProgramTddTimerTxRxManual( ucTbgenNo, TIMER_INSTANCE_7, TDD_MODE_11 );

	log_info( "\n\r[TBGEN TC] Tested below Tbgen 2 Timer Instances:");
	log_info( "\n\rTIMER_INSTANCE_0: TDD_MODE_10");
	log_info( "\n\rTIMER_INSTANCE_1: TDD_MODE_10");
	log_info( "\n\rTIMER_INSTANCE_2: TDD_MODE_01");
	log_info( "\n\rTIMER_INSTANCE_3: TDD_MODE_01");
	log_info( "\n\rTIMER_INSTANCE_4: TDD_MODE_00");
	log_info( "\n\rTIMER_INSTANCE_5: TDD_MODE_00");
	log_info( "\n\rTIMER_INSTANCE_6: TDD_MODE_11");
	log_info( "\n\rTIMER_INSTANCE_7: TDD_MODE_11");
	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
	SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_TBGEN2_TDD_MANUAL_TEST_STATUS);
}

void vTestTbgen2TddTimer( void )
{
	uint32_t uiCurrentCore = ulMpicCurrentCore();

#if defined(GEUL_LA1238RDB) || defined(GEUL_LA1238CPE)
	if(brd_ver == MW_REVB_VERSION)
	{
		log_info("\r\nTbgen TDD Test cases can not execute on MW RevB\r\n");
		return;
	}
#endif
	if (uiCurrentCore != 0)
	{
		log_info("\r\ntbgen2_tdd test runs only on Core0\r\n");
		return;
	}

	vTddInitSeq( TBGEN_2 );
}

void vTestTbgen2TddTimerManual( void )
{
	uint32_t uiCurrentCore = ulMpicCurrentCore();

#if defined(GEUL_LA1238RDB) || defined(GEUL_LA1238CPE)
	if(brd_ver == MW_REVB_VERSION)
	{
		log_info("\r\nTbgen TDD Test cases can not execute on MW RevB\r\n");
		return;
	}
#endif
	if (uiCurrentCore != 0)
	{
		log_info("\r\ntbgen2_tdd_manual test runs only on Core0\r\n");
		return;
	}

	vTddInitManual( TBGEN_2 );
}

void vRxAlignIntCb( uint8_t ucTbgenNo, TimerInstance_t eInstance, void * pxTimerParams )
{
	TimerParams_t * pxTiParams = ( TimerParams_t * ) pxTimerParams;
	static BaseType_t xHigherPriorityTaskWoken;

	if (ulRxAlignTimeStamp == NULL) {
		ulRxAlignTimeStamp = pvPortMalloc(sizeof(u64)*TM_REP_TEST_CYCLES);
		if (ulRxAlignTimeStamp == NULL) {
			log_err(" Error: %s - Unable to allocate memory \n\r",__func__);
			return;
		}
		memset((void *)ulRxAlignTimeStamp, 0, sizeof(u64)*TM_REP_TEST_CYCLES);
	}

	if( pxTiParams->eTrigMode == TM_REPETITIVE )
	{
		if( ulCounter >= TM_REP_TEST_CYCLES )
		{
			iTbgenDisableTimer( ucTbgenNo, RX_ALIGNMENT, eInstance );

			if ( pxTiParams )
				vPortFree( pxTiParams );
			// release the semaphore.
			xSemaphoreGiveFromISR( xTiBinarySemaphore, &xHigherPriorityTaskWoken );
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			log_info("\n\r[TBGEN TC] Semaphore xTiBinarySemaphore Released.\n\r");
			uiCompletionFlag = 1;
		}
		else
			ulRxAlignTimeStamp[ ulCounter++ ] = ullTbgenGetMasterCounter( ucTbgenNo );
	} else {

		iTbgenDisableTimer( ucTbgenNo, RX_ALIGNMENT, eInstance );
		if ( pxTiParams )
			vPortFree( pxTiParams );
		// release the semaphore.
		xSemaphoreGiveFromISR( xTiBinarySemaphore, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		log_info("\n\r[TBGEN TC] Semaphore xTiBinarySemaphore Released.\n\r");
		uiCompletionFlag = 1;
	}
}

void vTbgenExternalIRQTest( uint8_t ucTbgenNo )
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	u32 uiMinDiff = 0, uiMaxDiff = 0, uiAvgDiff = 0, uiDiffSum = 0;
	int iJitter = 0, iMinJitter = 0, iMaxJitter = 0, iAvgJitter = 0, iJitterSum = 0;
	u64 ulTiDiff = 0;
	TimerParams_t * pxParams = ( TimerParams_t * ) pvPortMalloc( sizeof( TimerParams_t ) );
	static int iIsInitialized = 0;

	log_info("\n\r[TBGEN TC] Entered : %s", __func__);

	if (ulRxAlignTimeStamp == NULL) {
			ulRxAlignTimeStamp = pvPortMalloc(sizeof(u64)*TM_REP_TEST_CYCLES);
			if (ulRxAlignTimeStamp == NULL) {
				log_err(" Error: %s - Unable to allocate memory \n\r",__func__);
				return;
			}
			memset((void *)ulRxAlignTimeStamp, 0, sizeof(u64)*TM_REP_TEST_CYCLES);
		}

	if( !iIsInitialized )
	{
		log_info( "\n\r[TBGEN TC] Creating Semaphore xTiBinarySemaphore..." );
		// Create the semaphore to guard a shared resource,
		xTiBinarySemaphore = xSemaphoreCreateBinary();
		log_info("Done.");

		log_info( "\n\r[TBGEN TC] Increasing Semaphore Count for first time..." );
		xSemaphoreGive( xTiBinarySemaphore );
		log_info( "Released." );

		iIsInitialized = 1;
	}
	// and wait for it to be acquired
	while( xSemaphoreTake( xTiBinarySemaphore, ( TickType_t ) 10 ) == pdFALSE )
	{
		log_info( "\n\r[TBGEN TC] Waiting for semaphore to be released in External Int...", __func__ );
	};
	log_info( "Semaphore Obtained." );
	pxParams->ePolarity = STROBE_POL_RISING;
	pxParams->pvCb = vRxAlignIntCb;
	pxParams->eSm = STROBE_MODE_PULSE;
	pxParams->ePw = PULSE_WIDTH_CLK_CYCLE_16; /* If PulseMode = STROBE_MODE_PULSE */
	pxParams->eTrigMode = TM_REPETITIVE;
	if( TBGEN_2 == ucTbgenNo )
	{
	    pxParams->uInterval = TBGEN2_125_US;
		pxParams->uOffset = ullTbgenGetMasterCounter( ucTbgenNo ) + TBGEN2_1000_US;
	}
	else
	{
	    pxParams->uInterval = TBGEN1_500_US;
		pxParams->uOffset = ullTbgenGetMasterCounter( ucTbgenNo ) + TBGEN1_1000_US;
	}

	iTbgenProgramTimer( ucTbgenNo, RX_ALIGNMENT, TIMER_INSTANCE_0, pxParams );
	iTbgenEnableTimer( ucTbgenNo, RX_ALIGNMENT, TIMER_INSTANCE_0 );

	/* Wait for the ISR to be completed */
	while ( 0 == uiCompletionFlag );
	uiCompletionFlag = 0;

	if( pxParams->eTrigMode == TM_REPETITIVE )
	{
		if( TBGEN_2 == ucTbgenNo )
		{
			log_info("\r\n Tbgen2 RX_ALIGNMENT Instance 0 generate interrupt after every 125us interval");
		}
		else
		{
			log_info("\r\n Tbgen1 RX_ALIGNMENT Instance 0 generate interrupt after every 500us interval ");
		}
		for( ulCounter = 0; ulCounter < ( TM_REP_TEST_CYCLES - 1 ); ulCounter++ )
		{
			ulTiDiff = ( ulRxAlignTimeStamp[ ulCounter + 1 ] - ulRxAlignTimeStamp[ ulCounter ] );
			if( TBGEN_2 == ucTbgenNo )
			{
			    iJitter = (u32)ulTiDiff - ( TBGEN2_125_US );
			}
			else
			{
			    iJitter = (u32)ulTiDiff - ( TBGEN1_500_US );
			}
			log_info("\r\n Counter[%d - %d] \t diff : %x%x Jitter : %d", ( ulCounter + 1 ), ulCounter,
					(u32)( ulTiDiff  >> 32 ), (u32)ulTiDiff, iJitter);
			if( 0 == ulCounter) {
				uiMinDiff = (u32)ulTiDiff;
				uiMaxDiff = (u32)ulTiDiff;
				uiDiffSum = (u32)ulTiDiff;
				iMinJitter = iJitter;
				iMaxJitter = iJitter;
				iJitterSum = iJitter;
			}
			else {
				if( uiMinDiff > ulTiDiff )
					uiMinDiff = (u32)ulTiDiff;
				if( uiMaxDiff < ulTiDiff )
					uiMaxDiff = (u32)ulTiDiff;
				if( iMinJitter > iJitter )
					iMinJitter = iJitter;
				if( iMaxJitter < iJitter )
					iMaxJitter = iJitter;
				uiDiffSum += ulTiDiff;
				iJitterSum += iJitter;
			}
		}
		uiAvgDiff = ( uiDiffSum / ( TM_REP_TEST_CYCLES - 1 ) );
		iAvgJitter = ( iJitterSum / ( TM_REP_TEST_CYCLES - 1 ) );
		log_info( "\n\r[TBGEN TC] MinDiff = %x, MaxDiff = %x, MinJitter = %d, MaxJitter = %d, AvgDiff = %x, AvgJitter = %d", uiMinDiff, uiMaxDiff, iMinJitter, iMaxJitter, uiAvgDiff, iAvgJitter);
		memset((void *)ulRxAlignTimeStamp, 0, sizeof(u64)*TM_REP_TEST_CYCLES);
		ulCounter = 0;
	}

	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
	if( TBGEN_2 == ucTbgenNo )
	{
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN2_RX_ALIGN_INT_TEST_STATUS );
	}
	else
	{
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN1_RX_ALIGN_INT_TEST_STATUS );
	}
}

void vTbgen1RxAlignTimerTest( void )
{
    vTbgenExternalIRQTest( TBGEN_1 );
}

void vTbgen2RxAlignTimerTest( void )
{
    vTbgenExternalIRQTest( TBGEN_2 );
}

void vTbgen1RFGTest()
{
	RFGParams_t RFGParams;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("\n\r[TBGEN TC] Entered : %s", __func__);
	if( !uIsRFG1Initialized )
	{
		RFGParams.eSyncOut = RFG_GENERATED_SYNC_OUT;
		RFGParams.eRefSyncSel = RFG_SYSREF_IN;
		RFGParams.eFrameSyncSel = FRAME_SYNC_SRC_CPRI_RX_RFG_0b10;
		RFGParams.pvCb = NULL;
	    iInitRFG( TBGEN_1, &RFGParams );
	    log_info("\n\r Tbgen1 RFG generates pulse after every 10ms");
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN1_RFG_TEST_STATUS );
	    uIsRFG1Initialized = 1;
	}
	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
}

void vTbgen1DisableRFGTest()
{
	log_info("\n\r Using [TBGEN%d]\n\r", TBGEN_1);
	iDisableRFG( TBGEN_1 );
	uIsRFG1Initialized = 0;
}

void vTbgen2RFGTest()
{
	RFGParams_t RFGParams;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("\n\r[TBGEN TC] Entered : %s", __func__);
	if( !uIsRFG2Initialized )
	{
		RFGParams.eSyncOut = RFG_GENERATED_SYNC_OUT;
		RFGParams.eRefSyncSel = RFG_SYSREF_IN;
		RFGParams.eFrameSyncSel = FRAME_SYNC_SRC_CPRI_RX_RFG_0b10;
		RFGParams.pvCb = NULL;
	    log_info("\n\r Using [TBGEN%d]\n\r", TBGEN_2);
	    iInitRFG( TBGEN_2, &RFGParams );
	    log_info("\n\r Tbgen2 RFG generates pulse after every 10ms");
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN2_RFG_TEST_STATUS );
	    uIsRFG2Initialized = 1;
	}
	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
}

void vTbgen2DisableRFGTest()
{
	log_info("\n\r Using [TBGEN%d]\n\r", TBGEN_1);
	iDisableRFG( TBGEN_2 );
	uIsRFG2Initialized = 0;
}

void vRxAlignTTIMsiCb( __attribute__((unused))uint8_t ucTbgenNo,
		__attribute__((unused))TimerInstance_t eInstance,
		__attribute__((unused))void * pxTimerParams )
{
	uint32_t msiLine = 0xFFFFFFFF;
	struct gul_msi_info * pMsiInfo;

	pMsiInfo = &pGulModPriv->msi_info[ MSI_IRQ_MUX ];
	msiLine = in_le32( &pGulModPriv->pHif->msi_tti );
	if( msiLine < GUL_MSI_MAX_CNT )
	{
		out_le32( pMsiInfo[ msiLine ].addr, pMsiInfo[ msiLine ].data );
	}
}

void vRxAlignTTIIntCb( uint8_t ucTbgenNo, TimerInstance_t eInstance, void * pxTimerParams )
{
	RxAlignIntInfo_t *pxRxAlignIntInfo = &xRxAlignIntInfo;
	TimerParams_t *timeParams = (TimerParams_t *)pxTimerParams;
	uint32_t ulTimeDiffUS;
	uint32_t ulIndex;
	static uint32_t ulInvalidSamples;
	static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	pxRxAlignIntInfo->ulTimedIntrCount++;

	if( pxRxAlignIntInfo->ulTimedIntrCount == 1 )
	{
		vGetCurrentTimeMpic( &pxRxAlignIntInfo->TimedIntPrevTime );
	}
	else
	{
		ulTimeDiffUS = ulGetElapsedTimeMpic( &pxRxAlignIntInfo->TimedIntPrevTime );
		ulTiDiff = ulTimeDiffUS ;
		iJitter = (int)ulTiDiff - (int)1000/(uGetTbgenFreq(ucTbgenNo)/timeParams->uInterval);
		if( 2 == pxRxAlignIntInfo->ulTimedIntrCount ) {
			uiMinDiff = (u32)ulTiDiff;
			uiMaxDiff = (u32)ulTiDiff;
			uiDiffSum = (u32)ulTiDiff;
			iMinJitter = iJitter;
			iMaxJitter = iJitter;
			iJitterSum = iJitter;
			ulInvalidSamples = 0;
		}
		else {
			if( ulTiDiff < (125 + 1000 )) {
				if( uiMinDiff > ulTiDiff )
					uiMinDiff = (u32)ulTiDiff;
				if( uiMaxDiff < ulTiDiff )
					uiMaxDiff = (u32)ulTiDiff;
				if( iMinJitter > iJitter )
					iMinJitter = iJitter;
				if( iMaxJitter < iJitter )
					iMaxJitter = iJitter;
				uiDiffSum += ulTiDiff;
				iJitterSum += iJitter;
			}
			else {
				ulInvalidSamples++;
			}
		}

		vGetCurrentTimeMpic( &pxRxAlignIntInfo->TimedIntPrevTime );
		ulIndex = pxRxAlignIntInfo->ulTimedIntrCount - 2;
		pxRxAlignIntInfo->pulTimeDiffBuff[ ( ulIndex )%TIME_DIFF_BUF_SIZE ] = ulTimeDiffUS;
	}

	if( pxRxAlignIntInfo->ulTimedIntrCount >= MAX_INTRRUPT_COUNT )
	{
		iTbgenDisableTimer( ucTbgenNo, RX_ALIGNMENT, TIMER_INSTANCE_0 );
		pxRxAlignIntInfo->ulTimedIntrCount = 0;
		uiAvgDiff = ( uiDiffSum / ( MAX_INTRRUPT_COUNT - ( ulInvalidSamples + 2 ) ) );
		iAvgJitter = ( iJitterSum / ( int )( MAX_INTRRUPT_COUNT - (ulInvalidSamples + 2 ) ) );
		log_info( "\n\r[TBGEN TC] MinDiff = %u, MaxDiff = %u, MinJitter = %d, MaxJitter = %d, AvgDiff = %u, AvgJitter = %d\r\n", uiMinDiff, uiMaxDiff, iMinJitter, iMaxJitter, uiAvgDiff, iAvgJitter);
	// release the semaphore.
		xSemaphoreGiveFromISR( pxRxAlignIntInfo->xTiBinarySemaphore, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		log_info( "\n\r[TBGEN TC] Semaphore xTiBinarySemaphore Released INST%d.\n\r", eInstance );
	}
}

static int prvConfigHostTTIEvent( u8 ucTbgenNo, u64 uOffset )
{
	int i = 0;
	static int iIsInitialized = 0;
	static TimerParams_t * pxParams;
	int iTimerInst;
	int iTimerType;
	u32 uTbgenFreqKhz = uGetTbgenFreq(ucTbgenNo);
	uint32_t disable_sideband = in_le32( &pGulModPriv->pHif->disable_sideband);

	if( !iIsInitialized )
	{
		pxParams = ( TimerParams_t * ) pvPortMalloc( sizeof( TimerParams_t ) );
		iIsInitialized = 1;
	}
	if( pxParams == NULL )
	{
		log_info( "%s: memory allocation fail\r\n" );
		return -1;
	}

	if( TbgenTotalInstGenHostTTI[ ucTbgenNo - 1 ] == 0 )
	{
		log_err(" TBGEN %d do not support HOST TTI\r\n", ucTbgenNo );
		return -1;
	}

	for( i = 0; i < TbgenTotalInstGenHostTTI[ ucTbgenNo - 1 ]; i++ )
	{
		pxParams->ePolarity = STROBE_POL_RISING;
		pxParams->eSm = STROBE_MODE_PULSE;
		pxParams->ePw = PULSE_WIDTH_CLK_CYCLE_15;
		pxParams->eTrigMode = TM_REPETITIVE;
		if (disable_sideband)
			pxParams->pvCb = vRxAlignTTIMsiCb;
		else
			pxParams->pvCb = NULL;
		pxParams->uInterval = uTbgenFreqKhz / tti_interval_div;
		pxParams->uOffset = uOffset + (uTbgenFreqKhz * 2) ;
		if( ucTbgenNo == TBGEN_1 )
		{
			if (disable_sideband)
				iTimerType = RX_ALIGNMENT;
			else
				iTimerType = xTbgen1HostTTIConf[i].etype;
			iTimerInst = xTbgen1HostTTIConf[i].eInst;
		}
		else
		{
			if (disable_sideband)
				iTimerType = RX_ALIGNMENT;
			else
				iTimerType = xTbgen2HostTTIConf[i].etype;
			iTimerInst = xTbgen2HostTTIConf[i].eInst;
		}
		iConfPMuxModeTbgen( ucTbgenNo, iTimerType, iTimerInst );
		iTbgenProgramTimer( ucTbgenNo, iTimerType, iTimerInst, pxParams );
		iTbgenEnableTimer( ucTbgenNo, iTimerType, iTimerInst );
	}

	return 0;
}

void vTbgenHostTTIEventTest( u8 ucTbgenNo )
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	uint32_t ulOrigPriority = 0;
	uint32_t *pulBuffer = NULL;
	u64 uOffset;
	int ret = 0;
	TimerParams_t  pxRxAlignParams;
	static int iIsInitialized = 0;

	u32 uTbgenFreqKhz = uGetTbgenFreq(ucTbgenNo);
	if( !uTbgenFreqKhz )
	{
		log_info("tbgen %d not initiated\n\r", ucTbgenNo);
		return; //TO handle the personalities for which tbgen not initiated
	}
	log_info("\n\r[TBGEN TC] Entered : %s ", __func__);
	if( !iIsInitialized )
	{
		log_info( "Initialising TTI INSTANCE%d\r\n", TIMER_INSTANCE_0 );
		log_info( "[TBGEN TC] Creating Semaphore xTiBinarySemaphoreTTI%d...", TIMER_INSTANCE_0 );
		// Create the semaphore to guard a shared resource,
		xRxAlignIntInfo.xTiBinarySemaphore = xSemaphoreCreateBinary( );
		log_info( "Done.\r\n" );

		pulBuffer = pvGeulMalloc( TIME_DIFF_BUF_SIZE * sizeof( uint32_t ) );
		if( pulBuffer == NULL)
		{
			log_err(" %s Memory Allocation Fail\r\n", __func__);
			tti_interval_div = TBGEN_TTI_INTERVAL_125_DIV;
			return;
		}
		xRxAlignIntInfo.pulTimeDiffBuff = pulBuffer;
		memset( pulBuffer, 0, TIME_DIFF_BUF_SIZE * sizeof( uint32_t ) );

		iIsInitialized = 1;
	}

	uOffset = ullTbgenGetMasterCounter( ucTbgenNo );

	ret = prvConfigHostTTIEvent( ucTbgenNo, uOffset );
	if( ret )
	{
		tti_interval_div = TBGEN_TTI_INTERVAL_125_DIV;
		return;
	}

	memset( xRxAlignIntInfo.pulTimeDiffBuff, 0, TIME_DIFF_BUF_SIZE * sizeof( uint32_t ) );
	pxRxAlignParams.ePolarity = STROBE_POL_RISING;
	pxRxAlignParams.eTrigMode = TM_REPETITIVE;
	pxRxAlignParams.eSm = STROBE_MODE_PULSE;
	pxRxAlignParams.ePw = PULSE_WIDTH_CLK_CYCLE_16; /* If PulseMode = STROBE_MODE_PULSE */
	pxRxAlignParams.pvCb = vRxAlignTTIIntCb;
	pxRxAlignParams.uInterval = uTbgenFreqKhz / tti_interval_div;
	pxRxAlignParams.uOffset = uOffset + (uTbgenFreqKhz * 2);
	if( ucTbgenNo == TBGEN_1 )
	{
		/* Store original interrupt priority */
		ulOrigPriority = ulMpicGetDevicePriority( DEVICE_EXTERNAL, TBGEN1_XIRQ_INT_STROBE_0 );
		/* Set interrupt priority < TIMER_PRIOIRY to use MPIC timestamp priority reliably */
		bMpicSetDevicePriority(DEVICE_EXTERNAL, TBGEN1_XIRQ_INT_STROBE_0, TIMER_PRIORITY - 1 );
	}
	else
	{
		/* Store original interrupt priority */
		ulOrigPriority = ulMpicGetDevicePriority( DEVICE_EXTERNAL, TBGEN2_XIRQ_INT_STROBE_0 );
		/* Set interrupt priority < TIMER_PRIOIRY to use MPIC timestamp priority reliably */
		bMpicSetDevicePriority(DEVICE_EXTERNAL, TBGEN2_XIRQ_INT_STROBE_0, TIMER_PRIORITY - 1 );
	}

	iTbgenProgramTimer( ucTbgenNo, RX_ALIGNMENT, TIMER_INSTANCE_0, &pxRxAlignParams );
	iTbgenEnableTimer( ucTbgenNo, RX_ALIGNMENT, TIMER_INSTANCE_0 );
	while( xSemaphoreTake( xRxAlignIntInfo.xTiBinarySemaphore, ( TickType_t ) 10 ) == pdFALSE )
	{
		log_info( "\n\r[TBGEN TC] Waiting for semaphore to be released in Host TTI Cb...", __func__ );
	};
	log_info("\n\r RX_ALIGNMENT instance 0 generates interrupt after every %dus to Modem",
			TBGEN_TTI_INTERVAL_125 * (TBGEN_TTI_INTERVAL_125_DIV/tti_interval_div));

	if( ucTbgenNo == TBGEN_1 )
	{
		bMpicSetDevicePriority(DEVICE_EXTERNAL, TBGEN1_XIRQ_INT_STROBE_0, ulOrigPriority );
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN1_HOST_TTI_TEST_STATUS);
	}
	else
	{
		bMpicSetDevicePriority(DEVICE_EXTERNAL, TBGEN2_XIRQ_INT_STROBE_0, ulOrigPriority );
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN2_HOST_TTI_TEST_STATUS);
	}
	/*Restore the default value of the tti interval divisor*/
	tti_interval_div = TBGEN_TTI_INTERVAL_125_DIV;
	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
}

void vTbgen1HostTTIEventTest( )
{
    vTbgenHostTTIEventTest( TBGEN_1 );
}

void vConfigurable_host_tbgentti_interval_test(uint8_t intvl_div, uint8_t num_tbgen)
{
	tti_interval_div = intvl_div;
	vTbgenHostTTIEventTest( num_tbgen );
}

void vTbgen2HostTTIEventTest( )
{
    vTbgenHostTTIEventTest( TBGEN_2 );
}

void vTbgenNonTddTimerTest( u8 ucTbgenNo )
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	TimerParams_t * pxParams = ( TimerParams_t * ) pvPortMalloc( sizeof( TimerParams_t ) );
	int iIndex = TIMER_INSTANCE_0;
	int iSrxMaxIndex = TIMER_INSTANCE_4;
	u64 uOffset = TBGEN1_1000_US;;
	u32 uInterval = TBGEN1_125_US;

	if( pxParams == NULL )
	{
		log_err( "%s: memory allocation fail\r\n" );
		return;
	}

	log_info("\n\r[TBGEN TC] Entered : %s", __func__);
	if( ucTbgenNo == TBGEN_2 )
	{
		uOffset = TBGEN2_1000_US;
		uInterval = TBGEN2_125_US;
		iSrxMaxIndex = TIMER_INSTANCE_3;
	}
	pxParams->ePolarity = STROBE_POL_RISING;
	pxParams->eSm = STROBE_MODE_PULSE;
	pxParams->ePw = PULSE_WIDTH_CLK_CYCLE_16;
	pxParams->eTrigMode = TM_REPETITIVE;
	pxParams->uInterval = uInterval;
	pxParams->pvCb = NULL;
	for( iIndex = TIMER_INSTANCE_0; iIndex < TIMER_INSTANCE_2; iIndex++ )
	{
		iConfPMuxModeTbgen( ucTbgenNo, GPE, iIndex );
	    pxParams->uOffset = uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenProgramTimer( ucTbgenNo, GPE, iIndex, pxParams );
		iTbgenEnableTimer( ucTbgenNo, GPE, iIndex );
	}
	log_info("\n\r GPE Instance 0 and 1 programmed in repetitive mode and expire after 125us");

	for( iIndex = TIMER_INSTANCE_5; iIndex < TIMER_INSTANCE_8; iIndex++ )
	{
		iConfPMuxModeTbgen( ucTbgenNo, SPI_TRIGGER, iIndex );
	    pxParams->uOffset =  uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenProgramTimer( ucTbgenNo, SPI_TRIGGER, iIndex, pxParams );
		iTbgenEnableTimer( ucTbgenNo, SPI_TRIGGER, iIndex );
	}
	log_info("\n\r SPI_TRIGGER Instance 5,6,7 programmed in repetitive mode and expire after 125us");

	for( iIndex = TIMER_INSTANCE_2; iIndex < iSrxMaxIndex; iIndex++ )
	{
		iConfPMuxModeTbgen( ucTbgenNo, SRX_ALIGNMENT, iIndex );
	    pxParams->uOffset =  uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenProgramTimer( ucTbgenNo, SRX_ALIGNMENT, iIndex, pxParams );
		iTbgenEnableTimer( ucTbgenNo, SRX_ALIGNMENT, iIndex );
	}
	if( ucTbgenNo == TBGEN_2 )
	{
		log_info("\n\r SRX_ALIGNMENT Instance 2 programmed in repetitive mode and expire after 125us");
	}
	else
	{
		log_info("\n\r SRX_ALIGNMENT Instance 2 and 3 programmed in repetitive mode and expire after 125us");
	}

	for( iIndex = TIMER_INSTANCE_0; iIndex < TIMER_INSTANCE_5; iIndex++ )
	{
		iConfPMuxModeTbgen( ucTbgenNo, AGC_ENABLE, iIndex );
	    pxParams->uOffset =  uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenProgramTimer( ucTbgenNo, AGC_ENABLE, iIndex, pxParams );
		iTbgenEnableTimer( ucTbgenNo, AGC_ENABLE, iIndex );
	}
	log_info("\n\r AGC_ENABLE Instance 0,1,2,3,4 programmed in repetitive mode and expire after 125us");

	for( iIndex = TIMER_INSTANCE_2; iIndex < TIMER_INSTANCE_4; iIndex++ )
	{
		iConfPMuxModeTbgen( ucTbgenNo, AXRF, iIndex );
	    pxParams->uOffset =  uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenProgramTimer( ucTbgenNo, AXRF, iIndex, pxParams );
		iTbgenEnableTimer( ucTbgenNo, AXRF, iIndex );
	}
	log_info("\n\r AXRF Instance 2 and 3 programmed in ONE SHOT");

	for( iIndex = TIMER_INSTANCE_2; iIndex < TIMER_INSTANCE_4; iIndex++ )
	{
	    pxParams->uOffset =  uOffset + ullTbgenGetMasterCounter( ucTbgenNo );
		iTbgenProgramTimer( ucTbgenNo, TIMED_INT, iIndex, pxParams );
		iTbgenEnableTimer( ucTbgenNo, TIMED_INT, iIndex );
	}
	log_info("\n\r TIMED_INT Instance 2 and 3 programmed in repetitive mode and expire after 125us");

	if( pxParams )
	{
	    vPortFree( pxParams );
	}
	log_info( "\n\r[TBGEN TC] Test : %s ends...", __func__ );
	if( ucTbgenNo == TBGEN_2 )
	{
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN2_NON_TDD_TEST_STATUS);
	}
	else
	{
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_TBGEN1_NON_TDD_TEST_STATUS);
	}
}

void vTbgen1NonTddTimerTest( )
{
	vTbgenNonTddTimerTest( TBGEN_1 );
}

void vTbgen2NonTddTimerTest( )
{
    vTbgenNonTddTimerTest( TBGEN_2 );
}
