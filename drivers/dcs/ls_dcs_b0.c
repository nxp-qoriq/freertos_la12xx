// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2023 NXP
 */

#include <stdint.h>
#include <types.h>
#include <FreeRTOS.h>
#include <task.h>
#include <Time.h>

#include "dcs_plat_config.h"
#include "dcs_regs_b0.h"
#include "dcs.h"
#include "Time.h"
#include "gul_host_if.h"
#include "immap.h"
#include "i2cAPI.h"

uint32_t lsdac_mask = 0;
uint32_t lsadc_mask = 0;

void lsdcs_i2c_write_cus_reg(uint32_t regoff, uint8_t *regval,uint32_t chaddr)
{
	int ret;

	ret = iI2C_Write( I2C7_BASE_ADDR, chaddr, regoff,
				 I2C_DEV_OFFSET_LEN_1_BYTE, regval, 1 );
	if( ret < 0 )
	{
		log_err( "%s(): i2c_write failed %d\n\r",__func__,ret );
		return;
	}

	return;
}

uint8_t lsdcs_i2c_read_cus_reg(uint32_t regoff,uint32_t chaddr)
{
	int ret;
	uint8_t ucval=0;

	ret = iI2C_Read( I2C7_BASE_ADDR, chaddr, regoff,
				 I2C_DEV_OFFSET_LEN_1_BYTE, &ucval, 1 );
	if( ret < 0 )
	{
		log_err( "%s(): i2c_read failed. Reg[0x%x] Err:[%d]\n\r",__func__,regoff,ret );
		return ucval;
	}

	return ucval;
}

static void LsADC_I2C_Config(volatile struct gul_hif *pxHif)
{
	uint32_t i2c_ch_select = 0;
	uint8_t reg_21;

	(void)pxHif;
	reg_21 = LSADC_I2C_CONTROL_REG_21_VAL;
	for (int i = 0; i < 4; i++) {
		if (lsadc_mask & (1 << i)) {
			i2c_ch_select = DCS_VAL_I2C_ADC_EN_BASE|(i * 2);
			OUT_32( DCS_I2C_OPCTL, i2c_ch_select);
			lsdcs_i2c_write_cus_reg( DCS_I2C_CHANGE_CAL_FREQ, &reg_21,i2c_ch_select);
			i2c_ch_select = DCS_VAL_I2C_ADC_EN_BASE|(i * 2 + 1);
			OUT_32( DCS_I2C_OPCTL, i2c_ch_select);
			lsdcs_i2c_write_cus_reg( DCS_I2C_CHANGE_CAL_FREQ, &reg_21,i2c_ch_select);
		}
	}
	return;
}

static inline int LsDcsSyncCheck(uint32_t addr, uint32_t val, char * code,uint32_t setval)
{
    uint32_t ulRetry = DCS_SYNC_RETRY;

    if(setval == SET_N_SYNC)
        OUT_32(addr, val);

    if(setval == SYNC_TRUE)
    {
        while((val & IN_32((uint32_t *)addr)) != val) {
            if(!(ulRetry--)) {
                log_err("\r\nSync %s failed",code);
                return -DCS_WAIT_TIMEOUT;
            }
        }
        return DCS_SUCCESS;
    }

    /* ONLY_SYNC. Check for bit clearing */
    while(val & IN_32((uint32_t *)addr)) {
          if(!(ulRetry--)) {
              log_err("\r\nSync %s failed",code);
              return -DCS_WAIT_TIMEOUT;
          }
    }
    return DCS_SUCCESS;
}

static int LsDcsInitCLK(volatile struct gul_hif *pxHif)
{

	uint32_t ulRegVal = 0;
	int uiErr = 0;

    /* Divider for TBGEN clk generation Divide by 2, PLL_LOCK */
	OUT_32( DCS_MISC_CTRL_REG, DCS_VAL_MISC_CTRL_REG);
	lsdac_mask = in_le32(&pxHif->lsdac_mask);
	lsadc_mask = in_le32(&pxHif->lsadc_mask);
	/* Check for DAC CLK diviser configuration */
    switch(in_le32(&pxHif->ls_dac_sps))
    {
        case LS_DAC_SPS_61:
            ulRegVal |= LS_DAC_CLK_DIV_8 << LS_DAC_CLK_DIV_POS;
            break;
        case LS_DAC_SPS_122:
            ulRegVal |= LS_DAC_CLK_DIV_4 << LS_DAC_CLK_DIV_POS;
            break;
        case LS_DAC_SPS_245:
            ulRegVal |= LS_DAC_CLK_DIV_2 << LS_DAC_CLK_DIV_POS;
            break;
        case LS_DAC_SPS_491:
        default:
            ulRegVal |= LS_DAC_CLK_DIV_1 << LS_DAC_CLK_DIV_POS;
            break;
    }

    /* Config Control - Power down*/
    if(lsadc_mask & 0x1)
	    OUT_32( DCS_ADC0_CFGCTL, DCS_VAL_ADC_N_CFGCTLV0);
    if(lsadc_mask & 0x2)
	    OUT_32( DCS_ADC1_CFGCTL, DCS_VAL_ADC_N_CFGCTLV0);
    if(lsadc_mask & 0x4)
	    OUT_32( DCS_ADC2_CFGCTL, DCS_VAL_ADC_N_CFGCTLV0);
    if(lsadc_mask & 0x8)
	    OUT_32( DCS_ADC3_CFGCTL, DCS_VAL_ADC_N_CFGCTLV0);
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, TRGR_ADC_SYNC, "CFGCTL_ADC",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

    /* Configure LS_ADC_SPS_245 for calibration. reconfigure the actual Divider later */       
            ulRegVal |= LS_ADC_CLK_DIV_2;
	/* corresponding to all the channels (0~3) */
	OUT_32( DCS_CH0_CLKCFG1, ulRegVal);
	if(lsdac_mask & 0x1)
		OUT_32( DCS_CH0_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
	if(lsdac_mask & 0x2)
		OUT_32( DCS_CH1_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
	if(lsdac_mask & 0x4)
		OUT_32( DCS_CH2_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
	if(lsdac_mask & 0x8)
		OUT_32( DCS_CH3_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
    /*  TBGEN_CLK generation is enabled */
	OUT_32( DCS_MISC_CTRL_REG, DCS_MISC_CTRL_REG_TBGEN_EN);

    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, TRGR_CLKDIV_SYNC, "CLKDIV2",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

    return DCS_SUCCESS;

}

static int LsDcsInitADC_DAC(volatile struct gul_hif *pxHif)
{
    int uiErr = 0;
    uint32_t ulRegVal = 0;
    uint32_t ls_adc_sps = 0;

    /* ADC/DAC Enable Control Register */
    OUT_32(DCS_ADC_ENCTL, DCS_VAL_ADC_ENCTL & lsadc_mask);
    OUT_32(DCS_DAC_ENCTL , DCS_VAL_DAC_ENCTL & lsdac_mask);
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_ADC_SYNC|TRGR_DAC_SYNC), "ENCTL_ADC_DAC",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

     /* Reset Control */
	OUT_32( DCS_ADC_RSTCTL, DCS_VAL_ADC_RSTCTL & lsadc_mask);
	OUT_32( DCS_DAC_RSTCTL, DCS_VAL_DAC_RSTCTL & lsdac_mask);
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_ADC_SYNC|TRGR_DAC_SYNC), "RSTCTL_REGSPACE_ADC_DAC",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
    uiErr = LsDcsSyncCheck(DCS_ADC_RSTCTL, DCS_VAL_ADC_RSTCTL & lsadc_mask, "RSTCTL_ADC", ONLY_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
    uiErr = LsDcsSyncCheck(DCS_DAC_RSTCTL, DCS_VAL_DAC_RSTCTL & lsdac_mask, "RSTCTL_DAC", ONLY_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

    /* ADC Config Control - OPM active */
    if(lsadc_mask & 0x1)
	    OUT_32(DCS_ADC0_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
    if(lsadc_mask & 0x2)
	    OUT_32(DCS_ADC1_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
    if(lsadc_mask & 0x4)
	    OUT_32(DCS_ADC2_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
    if(lsadc_mask & 0x8)
	    OUT_32(DCS_ADC3_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_ADC_SYNC), "ADC_CFGCTL",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

    uiErr = LsDcsSyncCheck(DCS_ADC_STAT, ADC_RDY_STATS & lsadc_mask, "ADC_RDY_STAT",SYNC_TRUE);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

	/* I2C ADC registers configuration */
	LsADC_I2C_Config(pxHif);

    /* ADC Calibration Control */
	OUT_32( DCS_ADC_CALCTL, DCS_VAL_ADC_CALCTL & lsadc_mask);
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_ADC_SYNC), "ADC_CALCTL_REG",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
    uiErr = LsDcsSyncCheck(DCS_ADC_CALCTL, DCS_VAL_ADC_CALCTL & lsadc_mask, "ADC_CALCTL", ONLY_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
	/* ADC sampling rate < 245. update the divider in sleep state */
	/* Check ADC CLK diviser and do the re-configuration */
	ls_adc_sps = in_le32(&pxHif->ls_adc_sps);
	if(ls_adc_sps != LS_ADC_SPS_245)
	{
		/* ADC Config Control - OPM Sleep*/
		if(lsadc_mask & 0x1)
			OUT_32( DCS_ADC0_CFGCTL, DCS_VAL_ADC_N_CFGCTLV1);
		if(lsadc_mask & 0x2)
			OUT_32( DCS_ADC1_CFGCTL, DCS_VAL_ADC_N_CFGCTLV1);
		if(lsadc_mask & 0x4)
			OUT_32( DCS_ADC2_CFGCTL, DCS_VAL_ADC_N_CFGCTLV1);
		if(lsadc_mask & 0x8)
			OUT_32( DCS_ADC3_CFGCTL, DCS_VAL_ADC_N_CFGCTLV1);
		uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_ADC_SYNC), "ADC_CFGCTL",SET_N_SYNC);
		CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

		ulRegVal = IN_32((uint32_t *)DCS_CH0_CLKCFG1);

		ulRegVal &= ~LS_ADC_CLK_DIV_2;

		switch(ls_adc_sps) {
			case LS_ADC_SPS_61:
				ulRegVal |= LS_ADC_CLK_DIV_8;
			break;
			case LS_ADC_SPS_122:
				ulRegVal |= LS_ADC_CLK_DIV_4;
			break;
			default:
				log_err("%s()LSDCS: Invalid CLKCFG1: 0x%x \r\n",__func__,ls_adc_sps);
			break;
		}
		log_info("%s()LSDCS: CLKCFG1: 0x%x \r\n",__func__,ulRegVal);

		/* Disable DAC CLK divider */
		if(lsdac_mask & 0x1)
			OUT_32( DCS_CH0_CLKCTRL, DCS_VAL_DAC_CH_N_CLKCTRL);
		if(lsdac_mask & 0x2)
			OUT_32( DCS_CH1_CLKCTRL, DCS_VAL_DAC_CH_N_CLKCTRL);
		if(lsdac_mask & 0x4)
			OUT_32( DCS_CH2_CLKCTRL, DCS_VAL_DAC_CH_N_CLKCTRL);
		if(lsdac_mask & 0x8)
			OUT_32( DCS_CH3_CLKCTRL, DCS_VAL_DAC_CH_N_CLKCTRL);
		uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, TRGR_CLKDIV_SYNC, "CLKDIV_RECONF0",SET_N_SYNC);
		CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

		OUT_32( DCS_CH0_CLKCFG1, ulRegVal);

		/* Enable ADC CLK divider */
		if(lsadc_mask & 0x1)
			OUT_32( DCS_CH0_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
		if(lsadc_mask & 0x2)
			OUT_32( DCS_CH1_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
		if(lsadc_mask & 0x4)
			OUT_32( DCS_CH2_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
		if(lsadc_mask & 0x8)
			OUT_32( DCS_CH3_CLKCTRL, DCS_VAL_CH_N_CLKCTRL);
		uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, TRGR_CLKDIV_SYNC, "CLKDIV_RECONF1",SET_N_SYNC);
		CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

		/* ADC Config Control - OPM active */
		if(lsadc_mask & 0x1)
			OUT_32( DCS_ADC0_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
		if(lsadc_mask & 0x2)
			OUT_32( DCS_ADC1_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
		if(lsadc_mask & 0x4)
			OUT_32( DCS_ADC2_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
		if(lsadc_mask & 0x8)
			OUT_32( DCS_ADC3_CFGCTL, DCS_VAL_ADC_N_CFGCTLV2);
		uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_ADC_SYNC), "ADC_CFGCTL",SET_N_SYNC);
		CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
	}
    uiErr = LsDcsSyncCheck(DCS_ADC_STAT, ADC_RDY_STATS & lsadc_mask, "ADC_RDY_STAT",SYNC_TRUE);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
    log_dbg("%s: ADC Stats Reg:0x%x",__func__,IN_32((uint32_t *)DCS_ADC_STAT));
    if(lsdac_mask & 0x1)
    {
	    OUT_32( DCS_DAC0_CFGCTL1, DCS_VAL_DAC_N_CFGCTL1);
	    OUT_32( DCS_DAC0_CFGCTL2, DCS_VAL_DAC_N_CFGCTL2);
    }
    if(lsdac_mask & 0x2)
    {
	    OUT_32( DCS_DAC1_CFGCTL1, DCS_VAL_DAC_N_CFGCTL1);
	    OUT_32( DCS_DAC1_CFGCTL2, DCS_VAL_DAC_N_CFGCTL2);
    }
    if(lsdac_mask & 0x4)
    {
	    OUT_32( DCS_DAC2_CFGCTL1, DCS_VAL_DAC_N_CFGCTL1);
	    OUT_32( DCS_DAC2_CFGCTL2, DCS_VAL_DAC_N_CFGCTL2);
    }
    if(lsdac_mask & 0x8)
    {
	    OUT_32( DCS_DAC3_CFGCTL1, DCS_VAL_DAC_N_CFGCTL1);
	    OUT_32( DCS_DAC3_CFGCTL2, DCS_VAL_DAC_N_CFGCTL2);
    }
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_DAC_SYNC), "DAC_CFGCTL",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
    uiErr = LsDcsSyncCheck(DCS_DAC_STAT, DAC_RDY_STATS & lsdac_mask, "DAC_RDY_STAT",SYNC_TRUE);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)
    log_dbg("\r\n%s: DAC Stats Reg:0x%x",__func__,IN_32((uint32_t *)DCS_DAC_STAT));

	OUT_32( DCS_ADC_RDYCTRL, DCS_VAL_ADC_RDYCTRL & lsadc_mask );
	OUT_32( DCS_DAC_RDYCTRL, DCS_VAL_DAC_RDYCTRL & lsdac_mask );
    uiErr = LsDcsSyncCheck(DCS_REG_SPACE_SYNC, (TRGR_RDYCTL_SYNC), "ADC_DAC_RDYCTL",SET_N_SYNC);
    CHECK_FOR_ERROR_CODE_AND_RETURN(uiErr)

    log_dbg("\r\n%s: OPM REG Val: 0x%x ",__func__,IN_32((uint32_t *)DCS_ADC_DAC_OPM_STATUS_REG));
    log_info("%s() Done\r\n",__func__);
    return DCS_SUCCESS;
}

void iCML2CMOSConfig( void )
{
    vuint32 * ulCtrl_0_reg = (vuint32 *) (SCFG_BASE_ADDR  +
            SCFG_CONFIG_CTRL0_OFFSET);
    vuint32 * ulCtrl_1_reg = (vuint32 *) (SCFG_BASE_ADDR  +
            SCFG_CONFIG_CTRL1_OFFSET);
    vuint32 * ulCtrl_5_reg = (vuint32 *) (SCFG_BASE_ADDR  +
            SCFG_CONFIG_CTRL5_OFFSET);
    uint32_t ulRegVal = 0;

    ulRegVal = READ_REGISTER(ulCtrl_5_reg);
    ulRegVal |= LS_DCS_SYNC_BYPASS_EN;
    WRITE_REGISTER(ulCtrl_5_reg, ulRegVal);

    ulRegVal = READ_REGISTER(ulCtrl_0_reg);
    ulRegVal |= LS_DCS_C2C_EN;
    WRITE_REGISTER(ulCtrl_0_reg, ulRegVal);
    vUdelay(2);

    ulRegVal = READ_REGISTER(ulCtrl_1_reg);
    ulRegVal |= LS_DCS_C2C_CLK_EN;
    WRITE_REGISTER(ulCtrl_1_reg, ulRegVal);

    return;
}

int vLSDcsInit_b0(volatile struct gul_hif *pxHif)
{
	int uiErr = 0;

    /* Sleep now from DCS and wakeup on possible PLL stable */
    vUdelay(200);

    iCML2CMOSConfig();

    uiErr = LsDcsInitCLK(pxHif);
	if(uiErr) {
		log_err("DCS Init CLK Failed\n\r");
		return uiErr;
	}

    uiErr = LsDcsInitADC_DAC(pxHif);
	if(uiErr) {
		log_err("DCS ADC_DAC Init Failed\n\r");
		return uiErr;
	}

    return 0;
}
