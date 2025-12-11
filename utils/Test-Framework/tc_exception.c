/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2020-2021 NXP
 */

#include "tc_exception.h"


#if GEUL_DEMO_DATA_EXCEPTION
#include <types.h>
#include <debug_console.h>

/* External References */
extern void vEnableEfpuExceptions( void );
extern void vEnableEfpuFinxeException( void );

static void prvTestDataStorageException( void )
{
	log_info( "\nInside invalid meory test fun.\r\n" );

	volatile uint32_t *point = ( uint32_t * )0xE03FFFFF;
	*point = 20;
	log_info( "\n ulAddress=%u \r\n", *point );
}

void vGeulDataExceptionTest( void )
{
	prvTestDataStorageException( );
}
#endif	/* GEUL_DEMO_DATA_EXCEPTION */

#if GEUL_DEMO_PROGRAM_EXCEPTION
static void prvTestProgramException( void )
{
	/* Run trap instruction to generate program exception. */
	__asm__ __volatile__ ("twi 15,%0,%1" : : "r" (0x10), "i" (0x10): );
}

void vGeulProgramExceptionTest( void )
{
	prvTestProgramException( );
}

#endif	/* GEUL_DEMO_PROGRAM_EXCEPTION */


#if GEUL_DEMO_ALIGNMENT_EXCEPTION
static void prvTestAlignmentException( void )
{
	log_info( "\n Alignment access test fun.\r\n" );
	__asm__ __volatile__ ( "dcbz 30, 8" );
}

void vGeulAlignExceptionTest( void )
{
	prvTestAlignmentException( );
}

#endif	/* GEUL_DEMO_ALIGNMENT_EXCEPTION */


#if GEUL_DEMO_INSTR_STORAGE_EXCEPTION
static void prvTestInstrStorageException( void )
{
	void ( *func )( void ) = ( void * )0xFFFFFFFF; // choose jump address, which is not known to MPU or execute permission are disabled for region
	log_info( "\n Instruction storage exception test fun.\r\n" );
	func( );
}

void vGeulInstrStorageExceptionTest( void )
{
	prvTestInstrStorageException( );
}

#endif	/* GEUL_DEMO_INSTR_STORAGE_EXCEPTION */

#if GEUL_DEMO_EFPU_DATA_EXCEPTION
static void prvTestEfpuDataException( void )
{
	float fTestVar;

	(void)fTestVar;
	log_info( "\n EFPU data exception test fun.\r\n" );

	vEnableEfpuExceptions( );

	fTestVar = ( float ) ( +0.0/+0.0 );
	log_info( "\n EFPU floating point result %lf \n", ( double )fTestVar );
}

void vGeulEFPUDataExceptionTest( void )
{
	prvTestEfpuDataException( );
}

#endif	/* GEUL_DEMO_EFPU_DATA_EXCEPTION */

#if GEUL_DEMO_EFPU_ROUND_EXCEPTION
static void prvTestEfpuRoundException( void )
{
	float fTestVar = 10;
	int i;

	log_info( "\n EFPU round exception test fun.\r\n" );
	vEnableEfpuFinxeException( );

	for( i = 0; i < 100; i++ )
	{
		fTestVar = fTestVar * 10;
	}
	log_info( "\n EFPU floating pointa rounded result %lf \n", ( double )fTestVar );
}

void vGeulEFPURoundExceptionTest( void )
{
	prvTestEfpuRoundException( );
}
#endif	/* GEUL_DEMO_EFPU_ROUND_EXCEPTION */
