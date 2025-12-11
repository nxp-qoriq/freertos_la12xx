/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2021 NXP
 */


#include "tc_peb_port3.h"

#ifdef GEUL_DEMO_PEBM_PORT3_TEST
#include <types.h>
#include <debug_console.h>
#include "soc.h"

void  vGeulDemoPEBPort3Test(void)
{
	u32 uiCurrentCore = ulMpicCurrentCore();

	if ( get_soc_revision() == GEUL_SVR_REVB_VAL ) {
		if ( memcmp((void *)PEBM_PORT3_BASE_ADDR, (void *)PEBM_BASE_ADDR, 4096) == 0)
		{
			PRINTF( "PEBM port3 test PASS\r\n" );
			SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_PEB_PORT3_TEST_STATUS);
		}
		else
		{
			PRINTF( "PEBM port3 test FAIL\r\n" );
			RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_PEB_PORT3_TEST_STATUS);
		}
	} else
		PRINTF( "PEBM port3 test not supported on Geul revA\r\n" );
}

#endif
