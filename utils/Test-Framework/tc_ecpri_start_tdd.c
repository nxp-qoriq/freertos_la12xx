/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 NXP
 */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "tdd_app.h"
#include "tc_ecpri_start_tdd.h"

extern void vTbgenTddStart(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void vPollEcpriTddReady( void )
{

        tdd_ready_params_t *tdd_params;
        uint32_t xDlSlotNum = 0;
        uint32_t xDlSymsNum = 0;
        uint32_t xUlSlotNum = 0;
        uint32_t xUlSymsNum = 0;
        uint32_t xDcs = 0;
        uint32_t xAntennaId = 0;
        
        PRINTF("\n\rPolling on ecpri tdd ready");

        mod_mem_region_t *modem_phy_addr = (mod_mem_region_t *)bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
        tdd_params = (tdd_ready_params_t *)(modem_phy_addr->addr_v + ECPRI_TDD_PARAMS_OFFSET);

        while(1) {
                if(in_le32(&tdd_params->ecpri_tdd_data_ready) == 1) {
                        xDlSlotNum = in_le32(&tdd_params->DlSlotNum);
                        xDlSymsNum = in_le32(&tdd_params->DlSymsNum);
                        xUlSlotNum = in_le32(&tdd_params->UlSlotNum);
                        xUlSymsNum = in_le32(&tdd_params->UlSymsNum);
                        xDcs       = in_le32(&tdd_params->Dcs);
                        xAntennaId = in_le32(&tdd_params->AntennaId);
                        vTbgenTddStart( xDlSlotNum, xDlSymsNum, xUlSlotNum, xUlSymsNum, xDcs, xAntennaId );
                        out_le32(&tdd_params->ecpri_tdd_data_ready,0);
                }
        }
}
