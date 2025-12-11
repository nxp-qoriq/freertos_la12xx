// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include "tc_qdma_legacy.h"

#if GEUL_DEMO_QDMA_LEGACY_TEST

#define BLOCK_SIZE 0x1000

void vQdmaLegacyTest(void)
{
	int ret = 0;
	u32 uiCurrentCore = ulMpicCurrentCore();

	QdmaLegacyInit();

	static BaseType_t src_addr, dest_addr;

	src_addr = (BaseType_t)pvGeulMalloc(BLOCK_SIZE);
	memset((char *)src_addr, 0xab, BLOCK_SIZE);
	dest_addr = (BaseType_t)pvGeulMalloc(BLOCK_SIZE);
	ret = QdmaLegacyCopy(src_addr, dest_addr, BLOCK_SIZE);
	log_info("%s: returned value: %d \n\r", __func__, ret);
#ifdef QDMA_LEGACY_DEBUG
	BaseType_t src_addr_val = src_addr, dest_addr_val = dest_addr;
	for(int i = 0; i < BLOCK_SIZE; i++) {
			if(!(in_le32((volatile uint32_t *)src_addr_val) == in_le32((volatile uint32_t *)dest_addr_val))) {
				log_info("QDMA Legacy mode Copy Failed\n\r");
				RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_LEGACY_TEST_STATUS);
				return;
			}
			src_addr_val++;
			dest_addr_val++;
	}
	log_info("QDMA Legacy mode Copy Success\n\r");
#endif
	if(ret < GEUL_QDMA_LEGACY_TRANSFER_ERRORS)
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_LEGACY_TEST_STATUS);
	else
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_LEGACY_TEST_STATUS);
}

#endif
