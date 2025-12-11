/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2021 NXP
 */


#include "tc_core_smem_text.h"

#ifdef GEUL_DEMO_CORE_SMEM_TEXT_TEST
#include <types.h>
#include <debug_console.h>
#include "soc.h"

u32 core_smem_array[GEUL_E200_CORE_GLOBAL_NUM][1024]
        __attribute__ ((section (".core_smem")));

void  __attribute__ ((section (".core_smem_text"))) vGeulDemoCoreSmemTextTest1(void)
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	extern char core_smem_text_start __asm__ ("__core_smem_text_start");
	extern char core_smem_text_end __asm__ ("__core_smem_text_end");

	PRINTF( "Executing function at %p\r\n", vGeulDemoCoreSmemTextTest1 );
	PRINTF( "core smem text start %p smem text end %p\r\n", &core_smem_text_start, &core_smem_text_end );
	if( ( uint32_t )vGeulDemoCoreSmemTextTest1 >= ( uint32_t )&core_smem_text_start
			&& ( uint32_t )vGeulDemoCoreSmemTextTest1 <= ( uint32_t )&core_smem_text_end )
	{
		PRINTF( "Core SMEM text test PASS\r\n" );
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_CORE_SMEM_TEXT_TEST_STATUS);
	}
	else
	{
		PRINTF( "Core SMEM text test FAIL\r\n" );
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_CORE_SMEM_TEXT_TEST_STATUS);
	}
}

void  vGeulDemoCoreSmemTextTest(void)
{
	u32 uiCurrentCore = ulMpicCurrentCore();

	if ( get_soc_revision() == GEUL_SVR_REVB_VAL ) {
		memset(core_smem_array[uiCurrentCore], 0xa5, 1024 * sizeof(u32));
		vGeulDemoCoreSmemTextTest1();
	} else
		PRINTF( "Core SMEM text test not supported on Geul revA\r\n" );
}

#endif
