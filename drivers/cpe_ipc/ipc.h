// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 * 
 */

#ifndef __IPC_H
#define __IPC_H
#include <geul_cpe_ipc.h>


#define MODEM_IPC_APP_MEMPOOL_SIZE (1024 * 96)

#define TEST_IPC_MAX_CHANNELS		8

#define UNUSED(x)			(void)(x)
#define pr_debug			PRINTF
#define pr_err				PRINTF
#define fsl_print			PRINTF

#define IPC1_INSTANCE_ID		0

/* Exported ipc lib variables.*/
extern ipc_t	ipc_handle;

extern struct gul_hif ipc_hif_area;
ipc_metadata_t ipc_md_area;
uint8_t ipc_mempool_area[MODEM_IPC_APP_MEMPOOL_SIZE];

uint32_t ipc_h2m_32(volatile uint32_t * addr);
void     ipc_m2h_32(uint32_t val, volatile uint32_t * addr);
int      ipc_init(gul_mod_priv_t * priv, uint8_t core_id);
#endif
