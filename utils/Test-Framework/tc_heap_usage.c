// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#include "tc_heap_usage.h"
#include "debug_console.h"
#include "FreeRTOS.h"

#ifdef GEUL_HEAP_USAGE_TEST

#define HEAP_META_DATA_SIZE 32
#define MAX_HEAP_SAMPLES 10

void vHeapMemoryUsageInfo(void)
{
	volatile size_t xFreeHeapSpace = 0;

	u32 current_core = ulMpicCurrentCore();
	log_info(" Current Core: %d \n\r", current_core);

	log_info(" Total Heap Memory: %d \n\r", configTOTAL_HEAP_SIZE);

	xFreeHeapSpace = xPortGetFreeHeapSize();
	log_info(" Available Heap Memory: %dBytes \n\r", xFreeHeapSpace);

	xFreeHeapSpace = xPortGetMinimumEverFreeHeapSize();
	log_info(" Minimum Ever Heap Memory Size: %dBytes \n\r", xFreeHeapSpace);

	SET_TEST_STATUS( current_core, GEUL_HEAP_USAGE_TEST_STATUS );
}

void vHeapMemoryUsage( void )
{
	volatile size_t xFreeHeapSpace = 0, memDiff = 0, xHeapSpaceStart = 0;
	int **ptr = NULL, i = 0;

	u32 current_core = ulMpicCurrentCore();
	log_info(" Current Core: %d \n\r", current_core);

	log_info(" Total Heap Memory: %d \n\r", configTOTAL_HEAP_SIZE);

	xFreeHeapSpace = xPortGetFreeHeapSize();
	log_info(" Available Heap Memory : %dBytes \n\r", xFreeHeapSpace);
	xHeapSpaceStart = xFreeHeapSpace;

	xFreeHeapSpace = xPortGetMinimumEverFreeHeapSize();
	log_info(" Minimum Ever Heap Memory Size Before Test: %dBytes \n\r", xFreeHeapSpace);

	ptr = pvPortMalloc(sizeof(int *)*MAX_HEAP_SAMPLES);
	xFreeHeapSpace = xPortGetFreeHeapSize();
	log_info(" Available Heap Memory after array allocation : %dBytes \n\r", xFreeHeapSpace);

	log_info("------Test Start --------\n\r");
	log_info(" Allocating memory\n\r");

	for (i=0; i<MAX_HEAP_SAMPLES; i++) {
		xFreeHeapSpace = xPortGetFreeHeapSize();
		log_info(" Allocating : %d Bytes, Available Mem Before: %dBytes, ",
				HEAP_META_DATA_SIZE* (i+1), xFreeHeapSpace);
		memDiff = xFreeHeapSpace;
		ptr[i] = pvPortMalloc(HEAP_META_DATA_SIZE * (i+1));
		xFreeHeapSpace = xPortGetFreeHeapSize();
		log_info(" After : %dBytes, Difference: %dBytes , metadataSize: %dBytes\n\r",
				xFreeHeapSpace,
				memDiff - xFreeHeapSpace,
				memDiff - xFreeHeapSpace - HEAP_META_DATA_SIZE * (i+1) );
	}

	log_info(" Allocation Done\n\r");
	log_info(" Free Memory\n\r");

	xFreeHeapSpace = xPortGetFreeHeapSize();
	log_info(" Available Heap Memory before Free: %dBytes \n\r", xFreeHeapSpace);

	for (i=0; i<MAX_HEAP_SAMPLES; i++) {
		xFreeHeapSpace = xPortGetFreeHeapSize();
		log_info(" Free : %d Bytes, Available Mem Before: %dBytes, ",
				HEAP_META_DATA_SIZE* (i+1), xFreeHeapSpace);
		memDiff = xFreeHeapSpace;
		vPortFree(ptr[i]);
		xFreeHeapSpace = xPortGetFreeHeapSize();
		log_info(" After : %dBytes, Difference: %dBytes , metadataSize: %dBytes\n\r",
				xFreeHeapSpace,
				xFreeHeapSpace - memDiff,
				xFreeHeapSpace - memDiff - HEAP_META_DATA_SIZE * (i+1) );
	}
	log_info(" Free Complete\n\r");
	log_info("------ Test End  ---------\n\r");

	log_info(" Free array memory \n\r");
	vPortFree(ptr);

	xFreeHeapSpace = xPortGetMinimumEverFreeHeapSize();
	log_info(" Minimum Ever Heap Memory Size: %dBytes \n\r", xFreeHeapSpace);

	xFreeHeapSpace = xPortGetFreeHeapSize();
	log_info(" Available Heap Memory : %dBytes \n\r", xFreeHeapSpace);

	if (xFreeHeapSpace == xHeapSpaceStart) {
		log_info(" PASS\n\r");
	} else {
		log_info(" FAIL : Heap size is not equal before and after test.\n\r");
	}
	SET_TEST_STATUS( current_core, GEUL_HEAP_USAGE_TEST_STATUS );
}
#endif /* GEUL_HEAP_USAGE_TEST */
