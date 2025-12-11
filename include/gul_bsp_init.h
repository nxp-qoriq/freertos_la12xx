// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 *
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

 /******************************************************************************
 *
 * See the following URL for information on the commands defined in this file:
 * http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/Embedded_Ethernet_Examples/Ethernet_Related_CLI_Commands.shtml
 *
 ******************************************************************************/


/* FreeRTOS includes. */
#ifndef __GUL_BSP_INIT_H__
#define __GUL_BSP_INIT_H__

#include <gul_host_if.h>
#ifndef CPE_IPC
#include "geul_ipc.h"
#else
#include "geul_cpe_ipc.h"
#endif
#include "immap.h"

//#define LS1046_HOST_MSI_RAISE

struct gul_msi_info {
        volatile uint32_t addr;
        uint32_t data;
};

/* DO NOT CHANGE ORDER OF MEM REGIONS
 * any modification in this enum should also reflect
 * in host_mem_region_id defined on gul_host_if.h */
enum mem_region_id {
	MOD_MEM_DMEM_0 = 0,
	MOD_MEM_DMEM_1,
	MOD_MEM_DMEM_2,
	MOD_MEM_DMEM_3,
	MOD_MEM_SMEM,
	MOD_MEM_PEBM,
	MOD_MEM_HUGE_PAGE_BUF,
	MOD_MEM_SCRATCH_BUF,
	MOD_MEM_FECA_AXI_SLAVE,
	MOD_MEM_FECA_APB_SLAVE,
	MOD_MEM_END
};

enum mem_type_id {
	MOD_MEM_TYPE_SMEMTEXT = 0,
	MOD_MEM_TYPE_SMEM,
	MOD_MEM_TYPE_CORESMEMTEXT,
	MOD_MEM_TYPE_CORESMEM,
	MOD_MEM_TYPE_END
};

typedef struct mod_mem_region {
	uint32_t addr_p;
	uint32_t size;
	uint32_t addr_v;
}mod_mem_region_t;

typedef struct gul_mod_priv {
	volatile struct gul_hif *pHif;
	ipc_metadata_t *ipc_md;
	mod_mem_region_t mem_region[MOD_MEM_END];
	struct gul_msi_info msi_info[GUL_MSI_MAX_CNT];
}gul_mod_priv_t;

/*Global variable*/
extern gul_mod_priv_t *pGulModPriv;

extern volatile struct gul_hif *bsp_get_hif();
extern ipc_metadata_t *bsp_get_ipc_md();
extern gul_mod_priv_t *bsp_get_mod_priv();
uint8_t get_board_version(void);
/*Function Declaration*/
mod_mem_region_t *bsp_get_mem_region(enum mem_region_id reg_id);
uint32_t get_modem_share_area_size(void);
uint32_t get_modem_host_data_size(void);
uint32_t get_modem_rf_data_size(void);
/* host data offset from the start of scratch buffer */
uint32_t get_modem_host_data_offset(void);
/* rf data offset from the start of scratch buffer */
uint32_t get_modem_rf_data_offset(void);


#define RESET_CORE_STATUS() out_le32(&pHif->core_status_flag, 0x00000000)

#define SET_CORE_STATUS_RUNNING(core_id) \
	out_le32(&pHif->core_status_flag, \
		(in_le32(&pHif->core_status_flag) \
			| 1 << (core_id * 2)))

#define SET_CORE_STATUS_STOPPED(core_id) \
         out_le32(&pHif->core_status_flag, \
		 (in_le32(&pHif->core_status_flag) \
                         | 1 << ((core_id * 2) + 1)))
#endif
