// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#include "FreeRTOS.h"
#include "mpic.h"
#include "Time.h"
#include "task.h"
#include "types.h"
#include "tbgen_new.h"

static uint8_t ucTbgenHandle;
static uint32_t tbgen_ref_clock = 0;

#define TBGEN_TIMER_COUNT_TO_US(count) ((count) / tbgen_ref_clock )

struct Time pxTimeStruct[GEUL_E200_CORE_GLOBAL_NUM] __attribute__ ((section (".smem")));

#ifdef TBGEN_ENABLED
void timeLibInit(uint8_t tbgen_id)
{
	ucTbgenHandle = tbgen_id;
	if (tbgen_id == TBGEN_1)
	{
		tbgen_ref_clock = TBGEN1_REF_CLK;
	}
	else if(tbgen_id == TBGEN_2)
	{
		tbgen_ref_clock = TBGEN2_REF_CLK;
		PRINTF("Debug: %s tbgen_ref_clock = %d; \r\n", __func__, tbgen_ref_clock);
	}
}

/* Note : While meauring the latencies, only one task from a E200 core can invoke this function at a time.
 *	  To measue latencies for multiple task from same core yet to be implemented*/
void vGetCurrentTimeMulticore(void )
{
	uint32_t coreId = ulMpicCurrentCore();
	pxTimeStruct[coreId].ulCurrentTickCount = ullTbgenGetMasterCounter(ucTbgenHandle);
}
/* Note : While meauring the latencies, only one task from a E200 core can invoke this function at a time.
 *	  To measue latencies for multiple task from same core yet to be implemented*/
uint32_t ulGetElapsedTimeMulticore(uint32_t coreId)
{
	u64 endtime = 0;
	endtime = ullTbgenGetMasterCounter(ucTbgenHandle);

	return ((uint32_t) TBGEN_TIMER_COUNT_TO_US(endtime - pxTimeStruct[coreId].ulCurrentTickCount));
}

void vGetCurrentTime( struct Time *pxTime )
{
	if( pxTime )
	{
		pxTime->ulCurrentTickCount = ullTbgenGetMasterCounter( ucTbgenHandle );
	}
}
#endif

void vGetCurrentTimeMpic( struct Time *pxTime )
{
	uint32_t ulCurrentTimerCounter = 0, ulCurrentTickCount = 0, ulCurrentTickCountAgain = 0;

	if( pxTime )
	{
redo_read:
		ulCurrentTickCount = xTaskGetTickCount();
		ulCurrentTimerCounter = ulGetMpicCurrentTickTimerCount();
		ulCurrentTickCountAgain = xTaskGetTickCount();

		if ( ulCurrentTickCountAgain != ulCurrentTickCount )
			goto redo_read;

		pxTime->ulCurrentTickCountMpic = ulCurrentTickCount;
		pxTime->ulCurrentTimerCounter = ulCurrentTimerCounter;
	}
}

#ifdef TBGEN_ENABLED
uint32_t ulGetElapsedTime( struct Time *pxTime )
{
	if( !pxTime )
	{
		return 0xFFFFFFFFul;
	}
	u64 endtime = 0;
	endtime = ullTbgenGetMasterCounter( ucTbgenHandle );

	return ( ( uint32_t ) TBGEN_TIMER_COUNT_TO_US(endtime - pxTime->ulCurrentTickCount ) );
}
#endif

uint32_t ulGetElapsedTimeMpic( struct Time *pxTime )
{
	if( !pxTime )
	{
		return 0xFFFFFFFFul;
	}
	uint32_t ulCurrentTimerCounter = 0, ulCurrentTickCount = 0, ulCurrentTickCountAgain = 0, ulTotalCountPassed = 0;

redo_read:
	ulCurrentTickCount = xTaskGetTickCount();
	ulCurrentTimerCounter = ulGetMpicCurrentTickTimerCount();
	ulCurrentTickCountAgain = xTaskGetTickCount();

	if ( ulCurrentTickCountAgain != ulCurrentTickCount )
		goto redo_read;

	/* Check about MPIC timer count roll over */
	if( ulCurrentTickCount > pxTime->ulCurrentTickCountMpic )
	{
		ulTotalCountPassed = pxTime->ulCurrentTimerCounter +
			( MPIC_SYS_TICK_COUNT - ulCurrentTimerCounter ) +
			( ( ulCurrentTickCount - pxTime->ulCurrentTickCountMpic - 1 ) * MPIC_SYS_TICK_COUNT );
	}
	/* Check about MPIC tick count roll over ( Approx every 49 days )*/
	else if( ulCurrentTickCount < pxTime->ulCurrentTickCountMpic)
	{
		ulTotalCountPassed = pxTime->ulCurrentTimerCounter +
			( MPIC_SYS_TICK_COUNT - ulCurrentTimerCounter ) +
			( ( 0xFFFFFFFFUL - pxTime->ulCurrentTickCountMpic + ulCurrentTickCount ) * MPIC_SYS_TICK_COUNT );
	}
	else
	{
		ulTotalCountPassed = pxTime->ulCurrentTimerCounter - ulCurrentTimerCounter;
	}

	return TIMER_COUNT_TO_US( ulTotalCountPassed );
}

int32_t iHasTimeElapsed( struct Time *pxTime, uint32_t ulTimeToCheckInUS )
{
	if( ( ulTimeToCheckInUS == 0 ) || !pxTime )
	{
		return 1;
	}

	return ( ulGetElapsedTime( pxTime ) > ulTimeToCheckInUS );
}

int32_t iHasTimeElapsedMpic( struct Time *pxTime, uint32_t ulTimeToCheckInUS )
{
	if( ( ulTimeToCheckInUS == 0 ) || !pxTime )
	{
		return 1;
	}

	return ( ulGetElapsedTimeMpic( pxTime ) > ulTimeToCheckInUS );
}

void vBusyWait( uint32_t ulUS )
{
	struct Time xTime;

	vGetCurrentTime( &xTime );

	while( iHasTimeElapsed( &xTime, ulUS ) == 0 );
}

void vBusyWaitMpic( uint32_t ulUS )
{
	struct Time xTime;

	vGetCurrentTimeMpic( &xTime );

	while( iHasTimeElapsedMpic( &xTime, ulUS ) == 0 );
}

void vUdelay( uint32_t usec )
{
	uint32_t bp_en = mfspr(SPR_BUCSR) & BUCSR_BPEN;
	uint32_t cycles, err_adjust;

	if (bp_en) {
		cycles = UDELAY_BP_CYCLES;
		err_adjust = UDELAY_BP_ERR_ADJ(usec);
	} else {
		cycles = UDELAY_CYCLES;
		err_adjust = UDELAY_ERR_ADJ(usec);
	}

       __asm__ volatile (
							"1:						\n"
							"e_subi    	%0, %0, 1	\n"
							"e_cmpi		 0, %0, 0	\n"
							"e_bne		1b			\n"
							:
							: "r"   	(USEC2LOOPCOUNT(usec, cycles, err_adjust))
							: "cr0"
						);
}

void vGetCurrentTimeMPICGB( struct Time *pxTime )
{
	if( pxTime )
	{
		pxTime->ulGlobalTimerBCount = ulGetMpicGloablTimerBCurrentCount();
	}
}

uint32_t ulGetElapsedTimeMPICGB( struct Time *pxTime )
{
	if( !pxTime )
	{
		return 0xFFFFFFFFul;
	}

	u64 ulGlobalTimerBCurrentTime = ulGetMpicGloablTimerBCurrentCount();
	u64 ulGloablBTimeDiff = ulGlobalTimerBCurrentTime - pxTime->ulGlobalTimerBCount;

	return ( uint32_t )( ulGloablBTimeDiff );
}

int32_t iHasTimeElapsedMPICGB( struct Time *pxTime, uint32_t ulTimeToCheckInUS )
{
	if( ( ulTimeToCheckInUS == 0 ) || !pxTime )
	{
		return 1;
	}

	return ( ulGetElapsedTimeMPICGB( pxTime ) > ulTimeToCheckInUS );
}

