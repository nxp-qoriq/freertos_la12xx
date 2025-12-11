// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_smem_text.h"

#ifdef GEUL_DEMO_SMEM_TEXT_TEST
#include <types.h>
#include <debug_console.h>

void  __attribute__ ((section (".smem_text"))) vGeulDemoSmemTextTest(void)
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	extern char smem_text_start __asm__ ("__smem_text_start");
	extern char smem_text_end __asm__ ("__smem_text_end");

	PRINTF( "Executing function at %p\r\n", vGeulDemoSmemTextTest );
	PRINTF( "smem text start %p smem text end %p\r\n", &smem_text_start, &smem_text_end );
	if( ( uint32_t )vGeulDemoSmemTextTest >= ( uint32_t )&smem_text_start 
			&& ( uint32_t )vGeulDemoSmemTextTest <= ( uint32_t )&smem_text_end )
	{
		PRINTF( "SMEM text test PASS\r\n" );
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_SMEM_TEXT_TEST_STATUS);
	}
	else
	{
		PRINTF( "SMEM text test FAIL\r\n" );
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_SMEM_TEXT_TEST_STATUS);
	}
}

#endif
