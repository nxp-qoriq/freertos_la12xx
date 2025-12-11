// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#include "tc_overlay_data.h"

#if (GEUL_DEMO_OVERLAY_REUSE_TEST) && !defined(OVERLAY_REUSE_ENABLED)
#include <types.h>
#include <debug_console.h>

uint8_t __attribute__ ((section (".ov.data"))) ucOvDatabuf[64] = "Reusing VSPA overlay area.....\n";
uint8_t __attribute__ ((section (".ov.bss"))) ucOvBssbuf[64];

void  vGeulDemoOvReuseTest()
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	extern char ov_data_start __asm__ ("__ov_data_start");
	extern char ov_data_end __asm__ ("__ov_data_end");
	extern char ov_bss_start __asm__ ("__ov_bss_start");
	extern char ov_bss_end __asm__ ("__ov_bss_end");

	PRINTF( "\r\nData String \" %s \" stored at %p\r\n", ucOvDatabuf, &ucOvDatabuf[0]);
	PRINTF( "BSS variable stored at %p\r\n", &ucOvBssbuf[0]);
	PRINTF( "Overlay data start %p overlay data end %p\r\n", &ov_data_start, &ov_data_end );
	PRINTF( "Overlay bss start %p overlay bss end %p\r\n", &ov_bss_start, &ov_bss_end );
	if( (( uint32_t )ucOvDatabuf >= ( uint32_t )&ov_data_start)
			&& (( uint32_t )ucOvDatabuf < ( uint32_t )&ov_data_end )
			&& (( uint32_t )ucOvBssbuf >= ( uint32_t )&ov_bss_start )
			&& (( uint32_t )ucOvBssbuf < ( uint32_t )&ov_bss_end ) )
	{
		PRINTF( "Overlay reuse test PASS\r\n" );
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_OVERLAY_REUSE_TEST_STATUS);
	}
	else
	{
		PRINTF( "Overlay reuse test FAIL\r\n" );
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_OVERLAY_REUSE_TEST_STATUS);
	}
}
#endif
