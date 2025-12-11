// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#include "tc_msi.h"

#ifdef GEUL_DEMO_MSI_TEST
#include <types.h>
#include <debug_console.h>
#include "mpic_regs.h"
#include "spinlock_api.h"

#define SOC_MSIIR_SRS5		0xA0000000
#define SOC_MSIIR_SRS5_IBS	0x1B000000

struct SpinLock *pxMSILock = NULL;
static volatile int flag;

static bool_t msi_test_interrupt_handler( uint32_t ulIrq_No, void *vDev_Data )
{
	u32 uiCurrentCore = ulMpicCurrentCore();
	
	/* To remove unused variable warnings */
	(void)ulIrq_No;
	(void)vDev_Data; 
	log_dbg("%s: msi test handler irq = %d data = %p \r\n", __func__,ulIrq_No, vDev_Data);

	/* receive msg from interrupt, read will also clear interrupt */
	in_be32(MPIC_REGS_MSIR5);
	SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_MSI_TEST_STATUS);
	flag = 0;
	return true;
}

void vGeulDemoMSITest(void)
{
	if ( pxMSILock == NULL )
	{
		pxMSILock = pxSpinLockGet( xTaskGetCurrentTaskHandle(), NULL, SPINLOCK_MSI_TEST);
	}

	flag = 1;

	lRegisterIrq(145 + INTERNAL_IRQ_OFFSET, msi_test_interrupt_handler, NULL);

	vSpinLockAcquire( pxMSILock );

	log_info("%s: enable msi\r\n", __func__);
	bMpicEnable(DEVICE_SHARE_MESSAGE, 5);

	log_info("%s: generate msi\r\n", __func__);
	out_be32(MPIC_REGS_MSIIR, SOC_MSIIR_SRS5 | SOC_MSIIR_SRS5_IBS);

	while(flag);
	vSpinLockRelease( pxMSILock );
	log_info("LA12XX MSI Test Completed\r\n");
}
#endif


