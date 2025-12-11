// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#include <common.h>
#include <stdarg.h>
#include <debug_console.h>
#include "config.h"
#include "immap.h"
#include "soc.h"
#ifdef BBDEV_IPC_MODE
    #include "bbdev_ipc.h"
#elif defined(CPE_IPC)
    #include "ipc.h"
#endif
#include <mem_log.h>

#ifdef GEUL_BOOT_MODE_PEBM
uint32_t ulIndex[GEUL_E200_CORE_GLOBAL_NUM];
char geul_pebm_logger[GEUL_E200_CORE_GLOBAL_NUM][GUL_CORE_LOGGER_SIZE] __attribute__((section (".shared.bss")));
#endif

void vMemlogWrite(void *pucData)
{
#ifdef GEUL_BOOT_MODE_XSPI
/*   As there is no use of this in FlexSPI boot. Moreover this is causing
 *  .shared.bss section to overflow.
 */
	return;
#endif
    uint32_t core_id = ulMpicCurrentCore();

#ifdef GEUL_BOOT_MODE_PEBM
	char *ucLogger_addr = geul_pebm_logger[core_id];
	static uint32_t ulen = (uint32_t) GUL_CORE_LOGGER_SIZE;
	uint32_t ulLogIndex = ulIndex[core_id];
	char *ucdbgptr = (char *) (ucLogger_addr + ulLogIndex);
#else
	volatile struct gul_hif *pxHif = &ipc_hif_area;
	volatile struct debug_log_regs *pxDbglog = &pxHif->dbg_log_regs[core_id];
	volatile uint32_t ulLogIndex = pxHif->ulLogIndex[core_id];
	uint32_t ulen = (uint32_t) in_be32(&pxHif->dbg_log_regs[core_id].len);
	char *ucdbgptr = (char *) (in_be32(&pxDbglog->buf) + ulLogIndex);

	if (!CHK_HIF_HOST_RDY(pxHif, HIF_HOST_READY_LOGGER))
		return;
#endif
	*(ucdbgptr) = *(char *) pucData;

	ulLogIndex++;
	if (ulLogIndex >= ulen)
		ulLogIndex = 0;

#ifdef GEUL_BOOT_MODE_PEBM
	ulIndex[core_id] = ulLogIndex;
#else
	pxHif->ulLogIndex[core_id] = ulLogIndex;
#endif
	return;
}
