// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_cpu_loader.h"

#ifdef GEUL_DEMO_CPU_LOADER_TEST
#include "Time.h"

void vCpuLoaderTestCommand( void )
{
	PRINTF("\n\rLoad Generator Started");
	vBusyWait(500000); //0.5sec
	PRINTF("\n\rLoad Generator Ended...");
}
#endif
