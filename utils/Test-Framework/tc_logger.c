// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_logger.h"

#ifdef GEUL_BOOT_MODE_PEBM
extern char geul_pebm_logger[GEUL_E200_CORE_GLOBAL_NUM][GUL_CORE_LOGGER_SIZE];
void vLaunchPEBMLogger(void )
{
	log_dbg("\r\n %s: In Core_ID\n", __func__, ulMpicCurrentCore());
	u32 ucCore = ulMpicCurrentCore();
	int iIndex = GUL_CORE_LOGGER_SIZE;
	char ucBuf_logger[BUF_MAX + 1];
	char *ucLogger_addr = geul_pebm_logger[ucCore];

	log_info("\n\r");

	while(iIndex) {

		memcpy( ( void * ) ucBuf_logger, ( void * ) ucLogger_addr,
				BUF_MAX);
		log_info("%s", ucBuf_logger);
		iIndex -= BUF_MAX;
		ucLogger_addr += BUF_MAX;
	}

	log_info("\n\r");

	SET_TEST_STATUS(ucCore, GEUL_DEMO_PEBM_LOGGER_STATUS);
	log_dbg("%s: Out\n\r", __func__);
}
#endif /* GEUL PEBM Logger */
