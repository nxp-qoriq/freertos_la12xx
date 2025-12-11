// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2022 NXP
 */

#include <types.h>
#include <FreeRTOS.h>
#include <task.h>
#include <Time.h>

#include "dcs_hs.h"
#include "dcs_plat_config.h"
#include "dcs_regs.h"
#include "dcs.h"
#include "Time.h"
#include "immap.h"

#define T_OUT_CNT 	10000
#define CAL_DELAY	1000
#define RE_CALIBRATION_CNT	5

static void dcs_idle(volatile u32 num)
{
	while(num--) { };
}

static error_t adc_calibration(DcsDevHandle_t pvDevHandle) {
	u32 adci1_fail = 0, adcq1_fail = 0, adci2_fail = 0, adcq2_fail = 0;
	u32 done = 0, t_out = 0, recali_cnt = 0;
	do {
		adci1_fail = 0;
		done = 0;
		t_out = 0;
		// PAIR 1 ADC I COMP1 CAL
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x41118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_I)
				& DCS_PAIR_1_ADC_COMP1_CAL_DONE_I);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 1 ADC I COMP1 CAL fail\n\r");
			adci1_fail = 1;
		} else {
			log_info("PAIR 1 ADC I COMP1 CAL success\n\r");
		}

		// PAIR 1 ADC I COMP2 CAL
		done = 0;
		t_out = 0;
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x42118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_I)
					& DCS_PAIR_1_ADC_COMP2_CAL_DONE_I);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 1 ADC I COMP2 CAL fail\n\r");
			adci1_fail = 1;
		} else {
			log_info("PAIR 1 ADC I COMP2 CAL success\n\r");
		}

		// PAIR 1 ADC I RA_OFFSET CAL
		dcs_out_le32(0x44118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
		dcs_idle(CAL_DELAY);
		dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
		dcs_idle(CAL_DELAY);
		done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_I)
			& DCS_PAIR_1_ADC_RA_OFFSET_CAL_DONE_I);
		if (done == 0) {
			log_info("PAIR 1 ADC I RA_OFFSET CAL Failure/Timeout\n\r");
			adci1_fail = 1;
		} else {
			log_info("PAIR 1 ADC I RA_OFFSET CAL success\n\r");
		}
		// PAIR 1 ADC I Fore_GAIN CAL
		done = 0;
		t_out = 0;
		while(done == 0 && t_out < T_OUT_CNT){
			dcs_out_le32(0x48118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			done = dcs_in_le32((char *)pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_I) & DCS_PAIR_1_ADC_FORE_GAIN_CAL_DONE_I;
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 1 ADC I Fore_GAIN CAL Failure/Timeout3\n\r");
			adci1_fail = 1;
		} else {
			log_info("PAIR 1 ADC I Fore_GAIN CAL success\n\r");
		}
		recali_cnt++;
	} while ((adci1_fail != 0) && (recali_cnt < RE_CALIBRATION_CNT));

	/*============================================================================================*/

	recali_cnt = 0;
	do {
		adcq1_fail = 0;
		done = 0;
		t_out = 0;
		// PAIR 1 ADC Q COMP1 CAL
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x41118200, (void *)((char *) pvDevHandle +DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle +DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_Q)
					& (DCS_PAIR_1_ADC_COMP1_CAL_DONE_Q));
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 1 ADC Q COMP1 CAL fail\n\r");
			adcq1_fail = 1;
		} else {
			log_info("PAIR 1 ADC Q COMP1 CAL success\n\r");
		}

		// PAIR 1 ADC Q COMP2 CAL
		dcs_out_le32(0x42118200, (void *)((char *) pvDevHandle +DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
		done = 0;
		t_out = 0;
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle +DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_Q)
					& (DCS_PAIR_1_ADC_COMP2_CAL_DONE_Q));
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 1 ADC Q COMP2 CAL fail\n\r");
			adcq1_fail = 1;
		} else {
			log_info("PAIR 1 ADC Q COMP2 CAL success\n\r");
		}

		// PAIR 1 ADC Q RA_OFFSET CAL
		dcs_out_le32(0x44118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
		dcs_idle(CAL_DELAY);
		dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
		done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_Q)
				& (DCS_PAIR_1_ADC_RA_OFFSET_CAL_DONE_Q));
		if (done == 0) {
			log_info("PAIR 1 ADC Q RA_OFFSET CAL Failure/Timeout\n\r");
			adcq1_fail = 1;
		} else {
			log_info("PAIR 1 ADC Q RA_OFFSET CAL success\n\r");
		}

		// PAIR 1 ADC Q Fore_GAIN CAL
		done = 0;
		t_out = 0;
		while(done == 0 && t_out < T_OUT_CNT){
			dcs_out_le32(0x48118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			done = dcs_in_le32((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_Q)
				& (DCS_PAIR_1_ADC_FORE_GAIN_CAL_DONE_Q);
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 1 ADC Q Fore_GAIN CAL Failure/Timeout\n\r");
			adcq1_fail = 1;
		} else {
			log_info("PAIR 1 ADC Q Fore_GAIN CAL success\n\r");
		}
		recali_cnt++;
	} while ((adcq1_fail != 0) && (recali_cnt < RE_CALIBRATION_CNT));
	// Clear All PAIR_1 Q Channel CAL Enable bits
	dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MANUAL_CALIBRAION_Q));

	/*============================================================================================*/
	recali_cnt = 0;
	do {
		adci2_fail = 0;
		done = 0;
		t_out = 0;
		// PAIR 2 ADC I COMP1 CAL
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x41118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_I)
					& (DCS_PAIR_2_ADC_COMP1_CAL_DONE_I));
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 2 ADC I COMP1 CAL fail\n\r");
			adci2_fail = 1;
		} else {
			log_info("PAIR 2 ADC I COMP1 CAL success\n\r");
		}

		// PAIR 2 ADC I COMP2 CAL
		done = 0;
		t_out = 0;
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x42918200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_I)
					& (DCS_PAIR_2_ADC_COMP2_CAL_DONE_I));
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 2 ADC I COMP2 CAL fail\n\r");
			adci2_fail = 1;
		} else {
			log_info("PAIR 2 ADC I COMP2 CAL success\n\r");
		}

		// PAIR 2 ADC I RA_OFFSET CAL
		dcs_out_le32(0x44918200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
		dcs_idle(CAL_DELAY);
		dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
		done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_I)
				&(DCS_PAIR_2_ADC_RA_OFFSET_CAL_DONE_I));
		if (done == 0) {
			log_info("PAIR 2 ADC I RA_OFFSET CAL Failure/Timeout\n\r");
			adci2_fail = 1;
		} else {
			log_info("PAIR 2 ADC I RA_OFFSET CAL success\n\r");
		}

		// PAIR 2 ADC I Fore_GAIN CAL
		done = 0;
		t_out = 0;
		while(done == 0 && t_out < T_OUT_CNT){
			dcs_out_le32(0x48918200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_I));
			dcs_idle(CAL_DELAY);
			done = dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_I)
				& (DCS_PAIR_2_ADC_FORE_GAIN_CAL_DONE_I);
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 2 ADC I Fore_GAIN CAL Failure/Timeout1\n\r");
			adci2_fail = 1;
		} else {
			log_info("PAIR 2 ADC I Fore_GAIN CAL success\n\r");
		}
		recali_cnt++;
	} while ((adci2_fail != 0) && (recali_cnt < RE_CALIBRATION_CNT));

	/*============================================================================================*/
	recali_cnt = 0;
	do {
		adcq2_fail = 0;
		done = 0;
		t_out = 0;
		// PAIR 2 ADC Q COMP1 CAL
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x41118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_Q)
					& (DCS_PAIR_2_ADC_COMP1_CAL_DONE_Q));
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 2 ADC Q COMP1 CAL fail\n\r");
			adcq2_fail = 1;
		} else {
			log_info("PAIR 2 ADC Q COMP1 CAL success\n\r");
		}

		// PAIR 2 ADC Q COMP2 CAL
		done = 0;
		t_out = 0;
		while (done == 0 && t_out < T_OUT_CNT) {
			dcs_out_le32(0x42118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
			dcs_idle(CAL_DELAY);
			done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_Q)
					& (DCS_PAIR_2_ADC_COMP2_CAL_DONE_Q));
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 2 ADC Q COMP2 CAL fail\n\r");
			adcq2_fail = 1;
		} else {
			log_info("PAIR 2 ADC Q COMP2 CAL success\n\r");
		}

		// PAIR 2 ADC Q RA_OFFSET CAL
		dcs_out_le32(0x44118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
		dcs_idle(CAL_DELAY);
		dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
		dcs_idle(CAL_DELAY);
		done = (dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_Q)
				& (DCS_PAIR_2_ADC_RA_OFFSET_CAL_DONE_Q));
		if (done == 0) {
			log_info("PAIR 2 ADC Q RA_OFFSET CAL Failure/Timeout\n\r");
			adcq2_fail = 1;
		} else {
			log_info("PAIR 2 ADC Q RA_OFFSET CAL success\n\r");
		}

		// PAIR 2 ADC Q Fore_GAIN CAL
		done = 0;
		t_out = 0;
		dcs_out_le32(0x48118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
		dcs_idle(CAL_DELAY);
		dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));
		while(done == 0 && t_out < T_OUT_CNT){
			done = dcs_in_le32((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_Q)
				& (DCS_PAIR_2_ADC_FORE_GAIN_CAL_DONE_Q);
			dcs_idle(CAL_DELAY);
			t_out++;
		}
		if (done == 0) {
			log_info("PAIR 2 ADC Q Fore_GAIN CAL Failure/Timeout\n\r");
			adcq2_fail = 1;
		} else {
			log_info("PAIR 2 ADC Q Fore_GAIN CAL success\n\r");
		}
		recali_cnt++;
	} while ((adcq2_fail != 0) && (recali_cnt < RE_CALIBRATION_CNT));

	// Clear All PAIR_2 Q Channel CAL Enable bits
	dcs_out_le32(0x40118200, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MANUAL_CALIBRAION_Q));

	/*============================================================================================*/
	// Now Remove the FIFO reset and continue to Data stage
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_GLOBAL_SYNC_CTRL));

	if (adci1_fail != 0 || adcq1_fail != 0 || adci2_fail != 0 || adcq2_fail != 0)
		return FAILURE;
	else
		return SUCCESS;
}

void vSetDcsRefClk( DCS_REF_CLK_t eDcsRefClk )
{
	vuint32 * puiPllConfigAddr = ( vuint32 * ) ( DCS_PLL_CONFIG_ADDR );
	vuint32 uiRegVal = 0;

	/* Clear out previous DCS Pll ref clk value */
	RESET_REG_MASK( puiPllConfigAddr, BIT_MASK( REFCLK_SEL, REFCLK_SEL_MASK ) );

	/* Read DCS Config register */
	uiRegVal = READ_REGISTER( puiPllConfigAddr );

	switch( eDcsRefClk )
	{
		/* 122.88 MHz or 125 MHz */
		case DCS_REF_CLK_125:
			uiRegVal |= ( DCS_REF_CLK_125 << REFCLK_SEL );
			break;

		/* 156.25 MHz */
		case DCS_REF_CLK_156_25:
			uiRegVal |= ( DCS_REF_CLK_156_25 << REFCLK_SEL );
			break;

		/* 160 MHz */
		case DCS_REF_CLK_160:
		default:
			uiRegVal |= ( DCS_REF_CLK_160 << REFCLK_SEL );
			break;
	};

	WRITE_REGISTER( puiPllConfigAddr, uiRegVal );
}

static void dcs_calibration_5g( DcsDevHandle_t pvDevHandle) 
{
	//# Carry out Basic ADC Calibrations
	dcs_out_le32(0x12, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_IQPAIR_MODE_CONTROL));
	dcs_out_le32(0x12, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_IQPAIR_MODE_CONTROL));

	// Stefano & Mikko Mods Start
	dcs_out_le32(0x00018, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CONFIGURATION_I));
	dcs_out_le32(0x00018, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CONFIGURATION_Q));
	dcs_out_le32(0x00018, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CONFIGURATION_I));
	dcs_out_le32(0x00018, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CONFIGURATION_Q));
	dcs_out_le32(0x38, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x38, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));
	dcs_out_le32(0x38, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x38, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));
#ifdef GEUL_LA1238RDB
	if ((u32) pvDevHandle != ( u32 ) DCS_LS1_BASE) {
		dcs_out_le32(0xd4a60, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
		dcs_out_le32(0xd4a60, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));
	} else {
		dcs_out_le32(0xd4a70, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
		dcs_out_le32(0xd4a70, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));
	}
#else
	if ((u32) pvDevHandle != ( u32 ) DCS_LS1_BASE) {
		dcs_out_le32(0xd4b60, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
		dcs_out_le32(0xd4b60, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));
	} else {
		dcs_out_le32(0xd4b70, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
		dcs_out_le32(0xd4b70, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));
	}
#endif

	/* These statements are new */
	dcs_out_le32(0x10000000, (void *)((char *) pvDevHandle + DCS_BG_CAL_CORR_AVG_COUNT));
	dcs_out_le32(0x234, (void *)((char *) pvDevHandle + DCS_ADC_CONFIG_RA_GAIN));
	// Stefano Mods End
}

char * pcDcsInstanceToString( DcsBlock_t eDcsBlock )
{
	char * pcDcsInstStr = NULL;

	switch ( eDcsBlock )
	{
		case DCS_HS:
			pcDcsInstStr = "DCS HS";
			break;
		case DCS_LS1:
			pcDcsInstStr = "DCS LS-1";
			break;
		case DCS_LS2:
			pcDcsInstStr = "DCS LS-2";
			break;
		default:
			pcDcsInstStr = "NULL";
#if DCS_ENABLE_DEBUG_INFO
			log_info("\n\r[DCS] Invalid instance passed in pcDcsInstanceToString()");
#endif
			break;
	}

	return pcDcsInstStr;
}

static int prvCheckIPID( DcsDevHandle_t pvDevHandle )
{
	vuint32 uRegValue = 0;
	vuint32 * puiIpId = ( vuint32 * ) ( ( u8 * ) pvDevHandle + IP_ID_OFFSET );

	/* Read Magic Word from IP ID Register  */
	uRegValue = in_le32( puiIpId );

	if( IP_ID_MAGIC_WORD_LS == uRegValue )
	{
#if DCS_ENABLE_DEBUG_INFO
		log_info("[DCS LS] ...SUCCESS.");
		return 0;
#endif	/* DCS_ENABLE_DEBUG_INFO */
	}
	else if ( IP_ID_MAGIC_WORD_HS == uRegValue)
	{
		log_info("[DCS HS] ...SUCCESS");
		return 0;
	}
	else
	{
#if DCS_ENABLE_DEBUG_INFO
		log_info("...FAILED.: Val = %x\n\r",
				uRegValue);
#endif	/* DCS_ENABLE_DEBUG_INFO */
	}
	return -DCS_ID_MISMATCH;
}

DcsDevHandle_t pvDcsGetHandle( DcsBlock_t eInstance )
{
	DcsDevHandle_t pvDevHandle = NULL;

	switch ( eInstance )
	{
		case DCS_HS:
			pvDevHandle = ( DcsDevHandle_t ) DCS_HS_BASE;
			break;
		case DCS_LS1:
			pvDevHandle = ( DcsDevHandle_t ) DCS_LS1_BASE;
			break;
		case DCS_LS2:
			pvDevHandle = ( DcsDevHandle_t ) DCS_LS2_BASE;
			break;

		default:
#if DCS_ENABLE_DEBUG_INFO
			log_info("\n\r[DCS] Invalid instance passed in pvDcsGetHandle()");
#endif
			break;
	}

	return pvDevHandle;
}

static void LsDcsInit( DcsDevHandle_t pvDevHandle)
{
	dcs_out_le32(0x00000008, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
	dcs_out_le32(0x00000008, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));
	dcs_out_le32(0x00004008, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
	dcs_out_le32(0x00004008, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));
	dcs_out_le32(0x00004028, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
	dcs_out_le32(0x00004028, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));

	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CALIBRATION_DATA_I));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CALIBRATION_DATA_Q));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CALIBRATION_DATA_I));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CALIBRATION_DATA_Q));

	dcs_out_le32(0x00000001, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x00000001, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));
	dcs_out_le32(0x00000001, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x00000001, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));

	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_I));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CAL_STATUS_Q));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_I));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CAL_STATUS_Q));

	dcs_out_le32(0x00000010, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CONFIGURATION_I));
	dcs_out_le32(0x00000010, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_CONFIGURATION_Q));
	dcs_out_le32(0x00000010, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CONFIGURATION_I));
	dcs_out_le32(0x00000010, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_CONFIGURATION_Q));

	dcs_out_le32(0x00014028, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_MISC_CTRL));
	dcs_out_le32(0x00014028, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_MISC_CTRL));

	dcs_out_le32(0x00000020, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x00000020, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));
	dcs_out_le32(0x00000020, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_I));
	dcs_out_le32(0x00000020, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_RESIDUAL_AMPLIFIER_CTRL_Q));

	dcs_out_le32(0x00000019, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_RESET));
	dcs_out_le32(0x00000004, (void *)((char *) pvDevHandle + DCS_GLOBAL_SYNC_CTRL));

	//dcs_out_le32(0x000001a5, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_SEL)); /* Divide by 4 */
	dcs_out_le32(0x00000193, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_SEL)); /* Divide by 2 */

	/* soc_dig_clk1/2 not connected */
	//dcs_out_le32(0x00000001, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_SEL));

	dcs_out_le32(0x0000001f, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_RESET));

	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_IQ_PAIR_CLK_SEL_1));

	// A setting to fix DAC output order swap from IQA Ken on 0214
	//dcs_out_le32(0x00000004, (void *)((char *) pvDevHandle + DCS_ADC_CONFIG_RESERVE_2));
	dcs_out_le32(0x0000003a, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_IQPAIR_MODE_CONTROL));
	dcs_out_le32(0x00000007, (void *)((char *) pvDevHandle + DCS_PAIR_1_ADC_ENABLE));
	dcs_out_le32(0x0000003a, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_IQPAIR_MODE_CONTROL));
	dcs_out_le32(0x00000007, (void *)((char *) pvDevHandle + DCS_PAIR_2_ADC_ENABLE));

/*	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_DAC_I_CONFIG));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_1_DAC_Q_CONFIG));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_DAC_I_CONFIG));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_PAIR_2_DAC_Q_CONFIG));
*/
	// Enable Pair 1 DAC
	dcs_out_le32(0x00004051, (void *)((char *) pvDevHandle + DCS_PAIR_1_DAC_CTRL));

	// Enable Pair 2 DAC
	dcs_out_le32(0x00004051, (void *)((char *) pvDevHandle + DCS_PAIR_2_DAC_CTRL));

	dcs_out_le32(0x00000004, (void *)((char *) pvDevHandle + DCS_ADC_CONFIG_RESERVE_2));

	dcs_out_le32(0x00000019, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_RESET));

	log_info("LS DCS: DAC configuration Done..!!\r\n");
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_CLK_DIVIDER_RESET));
	dcs_out_le32(0x00000000, (void *)((char *) pvDevHandle + DCS_GLOBAL_SYNC_CTRL));
}

static void prvClkGenStopPll()
{
	vuint32 * puiPllReset = ( vuint32 * ) ( DCS_PLL_RESET_ADDR );

	/* Stop the PLL by setting PLLRSTCTL[STP_REQ]=1 */
	DCS_SET_REG_BITFIELD( puiPllReset, STP_REQ, STP_REQ_MASK, SET );

#if DCS_ENABLE_DEBUG_INFO
	log_info("\n\r[DCS] Waiting for Pll Status Control - STOP REQUEST to be cleared");
#endif	/* DCS_ENABLE_DEBUG_INFO */

	/* Wait for STP_REQ to be cleared by hardware PLLRSTCTL[STP_REQ]==0 */
	while( DCS_GET_REG_BITFIELD( puiPllReset, STP_REQ, STP_REQ_MASK ) != 0 );

#if DCS_ENABLE_DEBUG_INFO
	log_info("...CLEARED.");
#endif	/* DCS_ENABLE_DEBUG_INFO */
}

static void prvClkGenStartPll()
{
	vuint32 * puiPllReset = ( vuint32 * ) ( DCS_PLL_RESET_ADDR );

	/* Enable the PLL by setting PLLRSTCTL[RST_REQ]=1 */
	DCS_SET_REG_BITFIELD( puiPllReset, RST_REQ, RST_REQ_MASK, SET );

#if DCS_ENABLE_DEBUG_INFO
	log_info("\n\r[DCS] Waiting for Pll Status Control - RESET REQUEST to finish");
#endif	/* DCS_ENABLE_DEBUG_INFO */

	/* Wait for Reset to finish RST_DONE==1 */
	while( DCS_GET_REG_BITFIELD( puiPllReset, RST_DONE, RST_DONE_MASK ) != 1 );

#if DCS_ENABLE_DEBUG_INFO
	log_info("...RESET Complete.");
#endif	/* DCS_ENABLE_DEBUG_INFO */
}

/******************************************************************************
*************************** 		APIs 			 **************************
******************************************************************************/
#if DCS_API_FUNCTIONS
void vConfigureDcsPllClk( DcsPllClk_t eDcsPllClkMode )
{
	vuint32 * puiPllConfigAddr = ( vuint32 * ) ( DCS_PLL_CONFIG_ADDR );
	u32 uiRegVal = READ_REGISTER( puiPllConfigAddr );

	/* Stop DCS Pll before configuring */
	prvClkGenStopPll();

	/* Turn Off CLK_SEL_TOP & CLK_SEL_SIDE */
	RESET_REG_MASK( puiPllConfigAddr, BIT_MASK( CLK_SEL_SIDE, CLK_SEL_SIDE_MASK ) | BIT_MASK( CLK_SEL_TOP, CLK_SEL_TOP_MASK ));

	/* Configure DCS Pll as per requirement */
	switch( eDcsPllClkMode )
	{
#if ( DCS_PLATFORM == GEUL )
		case DCS_PLL_CLK_0:	/* HS: 3932.16 MHz	LS: 983.04 MHz*/
			uiRegVal |= (0b00 << CLK_SEL_SIDE);	// bit[10 09]=00, divided by 8		CLK_SEL_SIDE	- LS
			uiRegVal |= (0b01 << CLK_SEL_TOP);	// bit[15 14]=01, divided by 2		CLK_SEL_TOP	- HS
			break;
		case DCS_PLL_CLK_1:	/* HS: 3932.16 MHz	LS: 491.52 MHz*/
			uiRegVal |= (0b11 << CLK_SEL_SIDE);	// bit[10 09]=11, divided by 16		CLK_SEL_SIDE	- LS
			uiRegVal |= (0b01 << CLK_SEL_TOP);	// bit[15 14]=01, divided by 2		CLK_SEL_TOP	- HS
			break;
		case DCS_PLL_CLK_2: /* HS: 1966.08 MHz	LS: 983.04 MHz*/
			uiRegVal |= (0b00 << CLK_SEL_SIDE);	// bit[10 09]=00, divided by 8		CLK_SEL_SIDE	- LS
			uiRegVal |= (0b10 << CLK_SEL_TOP);	// bit[15 14]=10, divided by 4		CLK_SEL_TOP	- HS
			break;
		case DCS_PLL_CLK_3:	/* HS: 1966.08 MHz	LS: 491.52 MHz*/
			uiRegVal |= (0b11 << CLK_SEL_SIDE);	// bit[10 09]=11, divided by 16		CLK_SEL_SIDE	- LS
			uiRegVal |= (0b10 << CLK_SEL_TOP);	// bit[15 14]=10, divided by 4		CLK_SEL_TOP	- HS
			break;
        case DCS_PLL_CLK_6:  /* Geul_B0: HS:983.04 MHz  LS: 491.52 MHz  */
			uiRegVal |= (0b11 << CLK_SEL_SIDE);	
			uiRegVal |= (0b11 << CLK_SEL_TOP);
            break;
        case DCS_PLL_CLK_7:  /* Geul_B0: HS:983.04 MHz  LS: OFF         */
			uiRegVal |= (0b00 << CLK_SEL_SIDE);	
			uiRegVal |= (0b11 << CLK_SEL_TOP);
            break;
		case DCS_PLL_CLK_8:	/* HS: 3520 MHz LS: Off */
			uiRegVal |= (0b00 << CLK_SEL_SIDE);	// bit[10 09]=00, divided by 8		CLK_SEL_SIDE	- LS
			uiRegVal |= (0b01 << CLK_SEL_TOP);	// bit[15 14]=01, divided by 2		CLK_SEL_TOP	- HS		/* TODO: Check if this entry is fine in Geul's Lynx BG. No seperate entry for 3520 MHz clock freq */
			break;
        case DCS_PLL_CLK_10: /* Geul_B0: HS:1966.08 MHz LS: 491.52 MHz  */
			uiRegVal |= (0b11 << CLK_SEL_SIDE);	// bit[10 09]=11, divided by 16		CLK_SEL_SIDE- LS
			uiRegVal |= (0b10 << CLK_SEL_TOP);	// bit[15 14]=10, divided by 4		CLK_SEL_TOP	- HS
            break;
        case DCS_PLL_CLK_11: /* Geul_B0: HS:1966.08 MHz LS: OFF         */
			uiRegVal |= (0b00 << CLK_SEL_SIDE);	
			uiRegVal |= (0b10 << CLK_SEL_TOP);
            break;
        case DCS_PLL_CLK_13: /* Geul_B0: HS:OFF         LS: 491.52 MHz  */
			uiRegVal |= (0b11 << CLK_SEL_SIDE);	
			uiRegVal |= (0b00 << CLK_SEL_TOP);
            break;
        case DCS_PLL_CLK_14: /* Geul_B0: HS:OFF         LS: 491.52 MHz  */
			uiRegVal |= (0b11 << CLK_SEL_SIDE);	
			uiRegVal |= (0b00 << CLK_SEL_TOP);	
            break;
        case DCS_PLL_CLK_15: /* Geul_B0: HS:OFF         LS: OFF         */
			uiRegVal |= (0b00 << CLK_SEL_SIDE);	
			uiRegVal |= (0b00 << CLK_SEL_TOP);
            break;

#elif ( DCS_PLATFORM == WDL )
		case DCS_PLL_CLK_0:	/* LS: 640 MHzs */
			uiRegVal |= (0b01 << CLK_SEL_SIDE);	// bit[12 11]=01, divided by 9
			break;
		case DCS_PLL_CLK_1:	/* LS: 983.04 MHz*/
			uiRegVal |= (0b10 << CLK_SEL_SIDE);	// bit[12 11]=10, divided by 12
			break;
		case DCS_PLL_CLK_2: /* LS: 635.40 MHz*/
			uiRegVal |= (0b10 << CLK_SEL_SIDE);	// bit[12 11]=10, divided by 12
			break;
		case DCS_PLL_CLK_3:	/* LS: 847.20 MHz*/
			uiRegVal |= (0b10 << CLK_SEL_SIDE);	// bit[12 11]=10, divided by 12
			break;
#endif
        default:
            log_err(" Invalid DCS CLK Mode configuration \r\n");
            return;
	};

	uiRegVal |= BIT_MASK( CLKOUT_SIDE_EN, CLKOUT_SIDE_EN_MASK );
	WRITE_REGISTER( puiPllConfigAddr, uiRegVal );

	/* Start DCS Pll again after new configuration */
	prvClkGenStartPll();
}

DcsPllClk_t eGetDCSPllValue( void )
{
	vuint32 uiRegValue = 0;
	DcsPllClk_t eDcsClk = DCS_PLL_CLK_0;

#if ( DCS_PLATFORM == GEUL )
	vuint32 * puiPORSR1 = ( vuint32 * ) ( DCFG_BASE_ADDR + DCFG_PORSR1_OFFSET );

	/* Read PORSR1[11:8] - DCFG Block */
	uiRegValue = in_le32( puiPORSR1 );
	uiRegValue = ( uiRegValue >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;
#elif ( DCS_PLATFORM == WDL )
	vuint32 * puiPORSR2 = ( vuint32 * ) ( DCFG_BASE_ADDR + DCFG_PORSR2_OFFSET );

	/* Read PORSR2[9:8] - DCFG Block */
	uiRegValue = in_le32( puiPORSR2 );
	uiRegValue = ( uiRegValue >> DCFG_PORSR2_DCS_PLL_START_BIT ) & DCFG_PORSR2_DCS_PLL_MASK;
#endif

	/* Set the HS & LS Clock freq in Floating number */
	switch( uiRegValue )
	{
		case 0b0000:
			eDcsClk = DCS_PLL_CLK_0;
			break;
		case 0b0001:
			eDcsClk = DCS_PLL_CLK_1;
			break;
		case 0b0010:
			eDcsClk = DCS_PLL_CLK_2;
			break;
		case 0b0011:
			eDcsClk = DCS_PLL_CLK_3;
			break;
#if ( DCS_PLATFORM == GEUL )
		case 0b1000:
			eDcsClk = DCS_PLL_CLK_8;
			break;
#endif
	}

	return eDcsClk;
}

/* Config Check for DCS-LS block */
int uiCheckDcsLsConfig( volatile struct gul_hif *pxHif )
{
#ifdef GEUL_BOOT_MODE_XSPI
/*  As for FlexSPI boot, there is no host side communication,
 *  So No need to wait for checking HOST Ready flag.
 */
	return SUCCESS;
#endif
	uint32_t timeout = 10000;

	log_dbg("Waiting for host to Set DCS_PLL\r\n");
	while(timeout--)
	{
		if(CHK_HIF_HOST_RDY(pxHif, HIF_HOST_READY_DCS_LS))
			return SUCCESS;	

		vUdelay(100);
	};

	return FAILURE;
}

static void prvSetCalStatus(DcsBlock_t eDcsBlock)
{
	DcsDevHandle_t pvDevHandle = pvDcsGetHandle( eDcsBlock);
	vuint32 * ulRiscvMboxReg0Resp = (vuint32 *) ((u8 *) pvDevHandle +
					RISCV_MAILBOX_REG0_RESP_OFFSET);
	vuint32 * ulAdcEnableReg = (vuint32 *) ((u8 *) pvDevHandle +
					ADC_ENABLE_CTRL_OFFSET);
	vuint32 ulStatus = 0;
	int iCount = 0;

	/* Checking 4th bit of RISCV_MAILBOX_REG0_RESP Register */
	do {
		vTaskDelay(5);
		ulStatus = READ_REGISTER(ulRiscvMboxReg0Resp);
		iCount++;
		if(iCount > INIT_CAL_COUNT_EXCEED) {
			log_err("[INIT CAL]: ADC Enablement Failed\n");
			goto OUT_INIT_CAL;
		}
	} while(!(ulStatus & INIT_CAL_DONE));

	/* Disable the 16G ADC */
	DCS_SET_REG_BITFIELD(ulAdcEnableReg, ENABLE_16G_ADC, ENABLE_16G_ADC_MASK,
			RESET);

OUT_INIT_CAL:
	return;
}

int vReInitiateCal(DcsBlock_t eDcsBlock)
{
	DcsDevHandle_t pvDevHandle = pvDcsGetHandle( eDcsBlock);
	vuint32 * ulRiscvMboxReg1 = (vuint32 *) ((u8 *) pvDevHandle +
					RISCV_MAILBOX_REG1_OFFSET);
	vuint32 * ulAdcEnableReg = (vuint32 *) ((u8 *) pvDevHandle +
					ADC_ENABLE_CTRL_OFFSET);

	/* Write ADC_ENABLE_CONTROL = 0x1F (to enable ADC 4G & 16G as per FW) */
	SET_REG_MASK(ulAdcEnableReg, HS_ENABLE_RXI1_ADC | HS_ENABLE_RXI2_ADC |
			HS_ENABLE_RXQ1_ADC | HS_ENABLE_RXQ2_ADC |
			HS_ENABLE_16G_ADC);

	/* Initiate the Calibration by Writing RISCV_MBOX_REG1 = 1->0 */
	WRITE_REGISTER(ulRiscvMboxReg1, SET);
	WRITE_REGISTER(ulRiscvMboxReg1, RESET);

	prvSetCalStatus(eDcsBlock);

	return 0;
}

int vLSDcsInit_a0(volatile struct gul_hif *pxHif, DcsBlock_t eDcsBlock)
{
	int uiErr = 0;
	DcsDevHandle_t pvDevHandle = pvDcsGetHandle( eDcsBlock );

	UNUSED(pxHif);
	log_dbg("Sleep now from DCS and wakeup on possible PLL stable\n\r");
	vUdelay(200);
#if ENABLE_EXTRA_DCS_CONFIG
	/* This function is being used for changing the Divider of DCS PLL clock.
	 * default value of PLLCR0 = 0x0001_4100 (HS:- 5G/LTE 3932.156 MHz HS clock: 01)
	 * (LS:-IGNORE(8th bit set))".
	 *
	 * Function could be used if we want to change the divider by changing the
	 * eDcsPllClkMode, Currently it is DCS_PLL_CLK_0 */
	vConfigureDcsPllClk(pxDcsParam->eDcsPllClkMode);
	log_err("ENABLE_EXTRA_DCS_CONFIG\r\n");
#endif

	/* Dump IP_ID for LS/HS macro */
	uiErr = prvCheckIPID( pvDevHandle );
	if ( uiErr ) {
		log_err("prvCheckIPID: Failed\n\r");
		return uiErr;
	}

	/* Load Fw only in case of DCS-HS */
	if( DCS_LS1 == eDcsBlock || DCS_LS2 == eDcsBlock)
	{
		/* Initialize AMB sub-system */
		LsDcsInit( pvDevHandle );

		/* For ADC */
		dcs_calibration_5g (pvDevHandle);
		adc_calibration(pvDevHandle);
	}

	return 0;
}

#endif	/* DCS_API_FUNCTIONS */
