// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2014-2017 Freescale Semiconductor, Inc.
 * Copyright 2020-2021, 2023 NXP
 */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "common.h"
#include "config.h"
#include "immap.h"
#include "mpic.h"
#include "ppc.h"
#include "interrupt_event_handler.h"

static void prvShowCallStack( struct StackFrame *pxSf );
#if ( SHOW_STACK_FRAMES != 0 )
static void prvShowStackFrames( struct StackFrame *pxSf );
#endif
void InterruptHandler(long cause)
{
	switch(cause)
	{
		case 0x0000:
			PRINTF("\r Cause: Critical Input Exception \r\n");
		break;

		case 0x0010:
			PRINTF("\r Cause: Machine check Exception \r\n");
		break;

		case 0x0030:
			PRINTF("\r Cause: Instruction storage Exception \r\n");
		break;

		case 0x0050:
			PRINTF("\r Cause: Alignment handler Exception \r\n");
		break;

		case 0x0070:
			PRINTF("\r Cause: Performance Monitor handler Exception \r\n");
		break;

		case 0x0090:
			PRINTF("\r Cause: Debug handler Exception \r\n");
		break;

		case 0x00A0:
			PRINTF("\r Cause: Embedded Floating point handler Exception \r\n");
		break;

		case 0x00B0:
			PRINTF("\r Cause: Embedded Floating point round handler Exception \r\n");
		break;

		default:
			PRINTF("\r Unknown Exception \r\n");
		break;
	}

	while( true );
}

void vDisableInterrupts( void )
{
       uint32_t ulMsr;

       ulMsr = mfmsr( );

       ulMsr = ulMsr & ~(MSR_EE); //disable External interrupt
       ulMsr = ulMsr & ~(MSR_CE); //disable Critical interrupt

       mtmsr( ulMsr );
}

static void prvStackFrameDump( struct StackFrame *pxSf )
{
	int i;
	volatile struct gul_hif *pHif;
	uint8_t core_id = (uint8_t)ulMpicCurrentCore();

	pHif = pGulModPriv->pHif;
	SET_CORE_STATUS_STOPPED(core_id);

	PRINTF( "\r SRR0: 0x%08x \t SRR1: 0x%08x \t CR: 0x%08x \r\n",
			pxSf->ulSrr0, pxSf->ulSrr1, pxSf->ulCr );

	PRINTF( "\r LR: 0x%08x \t CTR: 0x%08x \t XER: 0x%08x \r\n"
			, pxSf->ulLR, pxSf->ulCtr, pxSf->ulXer );

	PRINTF( "\r GPR0: 0x%08x SP: 0x%08x\r\n"
			, pxSf->ulGpr_3_12[ 0 ], pxSf->ulSp );

	PRINTF( "\r GPR3: 0x%08x GPR4: 0x%08x GPR5: 0x%08x GPR6: 0x%08x\r\n",
			pxSf->ulGpr_3_12[ 1 ], pxSf->ulGpr_3_12[ 2 ],
			pxSf->ulGpr_3_12[ 3 ], pxSf->ulGpr_3_12[ 4 ] );

	PRINTF( "\r GPR7: 0x%08x GPR8: 0x%08x GPR9: 0x%08x GPR10: 0x%08x\r\n",
			pxSf->ulGpr_3_12[ 5 ], pxSf->ulGpr_3_12[ 6 ]
			, pxSf->ulGpr_3_12[ 7 ], pxSf->ulGpr_3_12[ 8 ] );

	PRINTF( "\r GPR11: 0x%08x GPR12: 0x%08x \r\n", pxSf->ulGpr_3_12[ 9 ],
			pxSf->ulGpr_3_12[ 10 ] );

	for( i = 0; i < 18; i += 3 )
	{
		PRINTF( "\r GPR%d: 0x%08x GPR%d: 0x%08x GPR%d: 0x%08x  \r\n",
				i + 14, pxSf->ulGpr_14[ i ], i + 15, pxSf->ulGpr_14[ i + 1 ],
				i + 16, pxSf->ulGpr_14[ i + 2 ] );
	}

#if ( SHOW_STACK_FRAMES != 0 )
	prvShowStackFrames( pxSf );
#endif
	prvShowCallStack( pxSf );
}

void vDataExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulDear, ulEsr;
       ulDear = mfspr( 61 );
       ulEsr = mfspr( 62 );
       PRINTF( "\r Cause: Data Storage Exception: Stack Dump \r\n" );
       PRINTF( "\r DEAR: 0x%08x \t ESR: 0x%08x \t\r\n", ulDear, ulEsr );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vProgramExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulEsr;
       ulEsr = mfspr( 62 );
       PRINTF( "\r Cause: Program Exception: Stack Dump \r\n" );
       PRINTF( "\r ESR: 0x%08x \t\r\n", ulEsr );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vAlignmentExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulDear, ulEsr;
       ulDear = mfspr( 61 );
       ulEsr = mfspr( 62 );
       PRINTF( "\r Cause: Alignment Exception: Stack Dump \r\n" );
       PRINTF( "\r DEAR: 0x%08x \t ESR: 0x%08x \t\r\n", ulDear, ulEsr );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vCriticalExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulCsrr0, ulCsrr1;
       ulCsrr0 = mfspr( 58 );
       ulCsrr1 = mfspr( 59 );
       PRINTF( "\r Cause: Critical Exception: Stack Dump \r\n" );
       PRINTF( "\r CSRR0: 0x%08x \t CSRR1: 0x%08x \t\r\n", ulCsrr0, ulCsrr1 );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vMachineCheckExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulMcsrr0, ulMcsrr1, ulMcsr;
       ulMcsrr0 = mfspr( 570 );
       ulMcsrr1 = mfspr( 571 );
       ulMcsr   = mfspr( 572 );
       PRINTF( "\r Cause: MachineCheck Exception: Stack Dump \r\n" );
       PRINTF( "\r MCSRR0: 0x%08x \t MCSRR1: 0x%08x MCSR:0x%08x \t\r\n", ulMcsrr0, ulMcsrr1, ulMcsr );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vInstructionStoreExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulSrr0, ulSrr1;
       ulSrr0 = mfspr( 26 );
       ulSrr1 = mfspr( 27 );
       PRINTF( "\r Cause: InstructioinStore Exception: Stack Dump \r\n" );
       PRINTF( "\r SRR0: 0x%08x \t SRR1: 0x%08x \t\r\n", ulSrr0, ulSrr1 );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vPerformanceExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulDsrr0, ulDsrr1;
       ulDsrr0 = mfspr( 574 );
       ulDsrr1 = mfspr( 575 );
       PRINTF( "\r Cause: PerformanceMonitor Exception: Stack Dump \r\n" );
       PRINTF( "\r DSRR0: 0x%08x \t DSRR1: 0x%08x \t\r\n", ulDsrr0, ulDsrr1 );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vDebugHandlerExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulDsrr0, ulDsrr1, ulDbsr;
       ulDsrr0 = mfspr( 574 );
       ulDsrr1 = mfspr( 575 );
       ulDbsr  = mfspr( 304 );
       PRINTF( "\r Cause: DebugHandler Exception: Stack Dump \r\n" );
       PRINTF( "\r DSRR0: 0x%08x \t DSRR1: 0x%08x DBSR: 0x%08x \t\r\n", ulDsrr0, ulDsrr1, ulDbsr );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vEFPHandlerExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulEsr;
       ulEsr = mfspr( 62 );
       PRINTF( "\r Cause: Embedded FloatingPoint Exception: Stack Dump \r\n" );
       PRINTF( "\r ESR: 0x%08x \t\r\n", ulEsr );
       PRINTF( "\r \n \r\n");

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vEFPRoundHandlerExceptionDump( struct StackFrame *pxSf )
{
       uint32_t ulEsr;
       ulEsr = mfspr( 62 );
       PRINTF( "\r Cause: Embedded FloatingPointRound Exception: Stack Dump \r\n" );
       PRINTF( "\r ESR: 0x%08x \t\r\n", ulEsr );
       PRINTF( "\r \n \r\n" );

       prvStackFrameDump( pxSf );

       vDisableInterrupts( );
       while( true );
}

void vEnableEfpuExceptions( void )
{
	uint32_t ulSpeFscr;

	/* Enable Floating point exceptions */
	ulSpeFscr = mfspr( 512 );
	ulSpeFscr = ulSpeFscr | FINXE | FINVE | FDBZE | FUNFE | FOVFE; //Enable FINXE|FINVE|FDBZE|FUNFE|FOVFE exceptions
	mtspr( 512, ulSpeFscr );
}

void vEnableEfpuFinxeException( void )
{
	uint32_t ulSpeFscr;

	/* Enable Floating point exceptions */
	ulSpeFscr = mfspr( 512 );
	ulSpeFscr = ulSpeFscr | FINXE | FINVE; //Enable FINXE|FINVE, do not enable FUNFE and FOVFE otherwise EFPU Data exception will be taken
	mtspr( 512, ulSpeFscr );
}

static void prvShowCallStack( struct StackFrame *pxSf )
{
	unsigned long ulSp;
	unsigned long ulNewSp;
	unsigned long *pulStack;
	int iCount = 0;
	ulSp = pxSf->ulSp;
	PRINTF( " Call Stack:\r\n" );
	do{
		if( ulSp == 0 )
			break;
		pulStack = ( unsigned long * )ulSp;
		ulNewSp = pulStack[ 0 ];
		PRINTF( " 0x%08x\r\n", pulStack[ 1 ] );
		ulSp = ulNewSp;
	}while( iCount++ < MAX_STACKTRACE_DEPTH );
}

#if (SHOW_STACK_FRAMES != 0)
static void prvShowStackFrames( struct StackFrame *pxSf )
{
	unsigned long ulSp;
	unsigned long ulNewSp;
	unsigned long *pulStack;
	int iCount = 0;
	unsigned long ulLoop;

	ulSp = pxSf->ulSp;

	PRINTF( "stack frames\r\n" );
	do{
		if( ulSp == 0 )
			break;
		pulStack = ( unsigned long * )ulSp;
		ulNewSp = pulStack[ 0 ];
		if( ulNewSp )
		{
			PRINTF( " --------Frame %d--------\r\n", iCount + 1 );
			for( ulLoop = 0; ulLoop <= ( ulNewSp - ulSp )/sizeof( unsigned long ) + 1; ulLoop++ )
			{
				PRINTF( " 0x%08x 0x%08x\r\n",( pulStack + ulLoop ), pulStack[ ulLoop ] );
			}
		}
		ulSp = ulNewSp;
	}while( iCount++ < MAX_STACKTRACE_DEPTH );

}
#endif

