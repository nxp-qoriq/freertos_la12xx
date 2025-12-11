// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_pcimsi.h"
#include "FreeRTOS.h"

#ifdef GEUL_DEMO_PCIMSI_TEST
#include <task.h>

extern gul_mod_priv_t *pGulModPriv;
void vGeulDemoPCIMSITest(void)
{
        struct gul_msi_info *pMsiInfo;
	u32 uiCurrentCore = ulMpicCurrentCore();
        int i;
        pMsiInfo=&pGulModPriv->msi_info[MSI_IRQ_MUX];
        for (i = 0; i < GUL_MSI_MAX_CNT; i++) {
                log_info("%s: PCIMSI [%d], addr 0x%x, data 0x%x\n\r", __func__,
                                i, pMsiInfo[i].addr, pMsiInfo[i].data);
		out_le32( pMsiInfo[i].addr, pMsiInfo[i].data);
		vTaskDelay(2);
		#if 0
		log_info("in_le32( pMsiInfo[i].addr )=0x%x\r\n",in_le32( pMsiInfo[i].addr ));
		if( (in_le32( pMsiInfo[i].addr )) != pMsiInfo[i].data)
		{
			log_err(" Not Able to raise PCIMSI interrupt addr 0x%x, data 0x%x\n\r", 
				pMsiInfo[i].addr, pMsiInfo[i].data);
			RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_PCIMSI_TEST_STATUS);
			return;
		}
		#endif
        };
	SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_PCIMSI_TEST_STATUS);	
	return;
}
#endif


