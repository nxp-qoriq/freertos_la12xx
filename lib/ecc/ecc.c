// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#include <types.h>
#include <FreeRTOS.h>
#include <math.h>
#include "ecc.h"

void vEccDisable()
{
	out_le32((volatile uint32_t *) ECC_CNTRL_REG1_ADDR,0xffffffff);
	out_le32((volatile uint32_t *) ECC_CNTRL_REG2_ADDR,0xffffffff);
}

void vEccEnable()
{
	RESET_BIT(ECC_CNTRL_REG2_ADDR, MULTI_BIT_SRAM_CHECK);
	RESET_BIT(ECC_CNTRL_REG2_ADDR, MULTI_BIT_PEBMEM_CHECK);

	lRegisterIrq(ECC_MULTIBIT_IRQ_NUM+INTERNAL_IRQ_OFFSET , ECCIRQHandlermulti, NULL);
	bMpicEnable(DEVICE_INTERNAL, ECC_MULTIBIT_IRQ_NUM);
}

bool_t ECCIRQHandlermulti(uint32_t ulIrq_No __attribute__((unused)), void * vDev_Data __attribute__((unused)))
{
	log_info("Interrupt number:%d,vDev_Data:%p\n",ulIrq_No,vDev_Data );
	if(CHECK_BIT(MULTI_BIT_ECC_STATUS_REG2_ADDR, MULTI_BIT_SRAM_CHECK))
		log_info(" SRAM  MULTI-BIT ECC ERROR GENERATED");
	else if(CHECK_BIT(MULTI_BIT_ECC_STATUS_REG2_ADDR, MULTI_BIT_PEBMEM_CHECK))
		log_info(" PEBMEM MULTI-BIT ECC ERROR GENERATED");
	else 
		log_info("NOT A  SRAM OR PEBMEM ERROR ");

	return 0;
}




