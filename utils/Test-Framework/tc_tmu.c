// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_tmu.h"
#include "debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

#if GEUL_DEMO_TMU_TEST

extern TmuRegs_t *pTmuHandle;

void tmuCallback(struct mtd_thermalEvent *temp)
{
	if (temp->tmuEvent == TMU_HIGH_CRITICAL_TEMP_EVENT)
	{
		log_info("\n\rTemperature goes above critical Temp");
		out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_LTC_DISABLE)));
		out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_ALT_DISABLE)));
	}
	else if (temp->tmuEvent == TMU_HIGH_TEMP_EVENT)
	{
		log_info("\n\rTemperature goes above high Temp");
		out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_LTC_DISABLE)));
	}
	else if (temp->tmuEvent == TMU_LOW_TEMP_EVENT)
	{
		log_info("\n\rTemperature comes below Low Temp");
		out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_LTC_DISABLE)));
	}
	else if (temp->tmuEvent == TMU_LOW_CRIICAL_TEMP_EVENT)
	{
		log_info("\n\rTemperature comes below low critical Temp");
		out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_LTC_DISABLE)));
	}
}
void vTmuTest( void )
{
	u32 current_core = ulMpicCurrentCore();
	int id;
#define ATR_ENABLE 0x80000000
	union mtdcurentTemp mtd_temp;
	int32_t temp; 
	
	mtd_temp.temp = mtdGetTemp();
	temp = mtd_temp.vspa_temp + TEMP_ADJUST;

	log_info("Current Temp: VSPA:%d°C, FECA:%d°C, PCI:%d°C, Diode: %d°C \n\r",
		       			mtd_temp.vspa_temp + TEMP_ADJUST,
					mtd_temp.feca_temp + TEMP_ADJUST,
					mtd_temp.pci_temp + TEMP_ADJUST,
					mtd_temp.diode_temp + TEMP_ADJUST);

	/*Set ACTR and AHT to little more than present temp*/
	log_dbg("\n\r&pTmuHandle->tmhtatr=0x%x", in_le32((&pTmuHandle->tmhtatr)) );
	out_le32(&pTmuHandle->tmr, TMU_TMR_DISABLE);
	out_le32(&pTmuHandle->tmhtatr, ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp+5));
	out_le32(&pTmuHandle->tmltatr, ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp+3));
	log_dbg("\n\r&pTmuHandle->tmhtatr=0x%x", in_le32((&pTmuHandle->tmhtatr)) );
	out_le32(&pTmuHandle->tmhtactr,(ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp+10)));
	out_le32(&pTmuHandle->tmltactr,(ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp+8)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_AHT_DISABLE)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_HTC_DISABLE)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_LTC_DISABLE)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_ALT_DISABLE)));
	out_le32(&pTmuHandle->tmr, TMU_TMR_ENABLE);
	vTaskDelay(3000);
	id = mtd_register(tmuCallback);

	/* Trigger AHT interrupt */	
	out_le32(&pTmuHandle->tmr, TMU_TMR_DISABLE);
	out_le32(&pTmuHandle->tmhtatr, ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp-10));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) | (TMU_TIER_AHT_ENABLE)));
	vTaskDelay(500);
	out_le32(&pTmuHandle->tmr, TMU_TMR_ENABLE);
	vTaskDelay(3000);

	/* Diable AHT & Trigger ACHT interrupt */
	out_le32(&pTmuHandle->tmr, TMU_TMR_DISABLE);
	out_le32(&pTmuHandle->tmhtatr, ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp+5));
	out_le32(&pTmuHandle->tidr, (in_le32(&pTmuHandle->tidr) | (TMU_TIER_HTC_ENABLE)));
	out_le32(&pTmuHandle->tmhtactr, ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp-5));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) | (TMU_TIER_HTC_ENABLE)));
	vTaskDelay(300);
	out_le32(&pTmuHandle->tmr, TMU_TMR_ENABLE);
	vTaskDelay(2000);


	/* Go below ALT Temperature */
	out_le32(&pTmuHandle->tmr, TMU_TMR_DISABLE);
	out_le32(&pTmuHandle->tmhtactr, ATR_ENABLE |  TEMP_CELSIUS_TO_KELVIN(temp+10));
	out_le32(&pTmuHandle->tidr, (in_le32(&pTmuHandle->tidr) | (TMU_TIER_HTC_ENABLE)));
	vTaskDelay(300);
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) | (TMU_TIER_ALT_ENABLE)));
	vTaskDelay(500);
	out_le32(&pTmuHandle->tmr, TMU_TMR_ENABLE);
	vTaskDelay(2000);

	/*  Go below Critical temp */
	out_le32(&pTmuHandle->tmr, TMU_TMR_DISABLE);
	vTaskDelay(1000);
	out_le32(&pTmuHandle->tidr, (in_le32(&pTmuHandle->tidr) | (TMU_TIER_ALT_ENABLE)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & (TMU_TIER_ALT_DISABLE)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) | (TMU_TIER_LTC_ENABLE)));
	vTaskDelay(500);
	out_le32(&pTmuHandle->tmr, TMU_TMR_ENABLE);
	vTaskDelay(2000);

	/* Restore default settings */
	out_le32(&pTmuHandle->tidr, (in_le32(&pTmuHandle->tidr) | (TMU_TIER_ALT_ENABLE)));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) | TMU_TIER_HTC_ENABLE));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) | TMU_TIER_AHT_ENABLE));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & TMU_TIER_LTC_DISABLE));
	out_le32(&pTmuHandle->tier, (in_le32(&pTmuHandle->tier) & TMU_TIER_ALT_DISABLE));
	mtd_deregister(id);
	SET_TEST_STATUS(current_core, GEUL_DEMO_TMU_TEST_STATUS);	
}
#endif
