// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2025 NXP
 */

#include <common.h>
#include "config.h"
#include "immap.h"
#include <platform_def.h>
#include "debug_console.h"
#include "i2cAPI.h"
#include "gpio.h"
#include "soc.h"
#include "tbgen_new.h"
#if defined(GEUL_LA1238RDB) || defined(GEUL_LA1238CPE) || defined (GEUL_LA1224)
#include "pmux.h"
#endif
#include "dcs.h"
#include "scfg.h"
#include "Time.h"

#define INIT_I2C_CTRL_5_6		0

extern volatile uint32_t brd_ver;

#define TBGEN1_GPIO_EN_1V8	( 6 )

uint32_t tbgen1_div_B0 = SCFG_TBGEN_REF_CLK_DIVIDER_1;
uint32_t tbgen2_div_B0 = SCFG_TBGEN_REF_CLK_DIVIDER_1;

/* User must set TBGEN1_DIV to get required TBGEN1 Ref Freq */
volatile uint32_t tbgen1_div = REF_CLK_245_76_MHZ;

/* User must set TBGEN2_DIV to get required TBGEN2 Ref Freq */
volatile uint32_t tbgen2_div = REF_CLK_245_76_MHZ;

static int isMWrBNRC()
{
#if defined(GEUL_LA1238CPE) || defined(GEUL_LA1238RDB)
    return 1;
#endif
#ifdef GEUL_LA1224
    if (brd_ver == GEUL_HOST_REVC_VAL)
        return 1;
#endif
    return 0;
}

void vInitSCFG( void )
{
    if (!isMWrBNRC())
        return;
    struct scfg_regs *regs;
    uint32_t val;

#ifdef GEUL_BOOT_MODE_XSPI
	vuint32 * puiClkCtrl = ( vuint32 * ) ( DCS_HS_BASE + HS_CLK_CTRL_OFFSET );
#endif
    regs = (struct scfg_regs *) (SCFG_BASE_ADDR);

    val = in_le32(&regs->conf_ctrl3);
    val |= LLCP_CLK_EN;
    out_le32(&regs->conf_ctrl3, val);
    PRINTF("%s: conf_ctrl3(0x%x) 0x%x\r\n", __func__,
	     &regs->conf_ctrl3, in_le32(&regs->conf_ctrl3));
#ifdef GEUL_BOOT_MODE_XSPI
	val = in_le32(&regs->conf_ctrl6);
	val &= HSLSDCSCLKMASK;
	out_le32(&regs->conf_ctrl6, val);
	PRINTF("%s: conf_ctrl6(0x%x) 0x%x\r\n", __func__,
            &regs->conf_ctrl6, in_le32(&regs->conf_ctrl6));

	out_le32(puiClkCtrl, HSDCS_CLK_CTRL);
#endif
}

#ifndef GEUL_LA1246
static uint32_t prvGetTbgen1Div( volatile struct gul_hif *pxHif, vuint32 uiRegValue )
{
	switch( uiRegValue )
	{
		case DCS_PLL_CLK_0:
		case DCS_PLL_CLK_2:
			switch( /*TBGEN1_DIV*/tbgen1_div )
			{
				case REF_CLK_491_52_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_491_52_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_245_76_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
				case REF_CLK_61_44_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_61_44_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_8;
				case REF_CLK_30_72_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_30_72_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_16;
				default:
					log_err("Tbgen1: Not a valid Frequency\r\n");
					log_info("Tbgen1: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
			}
		case DCS_PLL_CLK_1:
		case DCS_PLL_CLK_3:
			switch( /*TBGEN1_DIV*/tbgen1_div )
			{
				case REF_CLK_245_76_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
				case REF_CLK_61_44_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_61_44_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
				case REF_CLK_30_72_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_30_72_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_8;
				default:
					log_err("Tbgen1: Not a valid Frequency\r\n");
					log_info("Tbgen1: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
			}
	}

	return SCFG_TBGEN_REF_CLK_DIVIDER_2;
}
#endif

static uint32_t prvGetTbgen2Div( volatile struct gul_hif *pxHif, vuint32 uiRegValue )
{
	switch( uiRegValue )
	{
		case DCS_PLL_CLK_0:
		case DCS_PLL_CLK_1:
			switch( /*TBGEN2_DIV*/tbgen2_div )
			{
				case REF_CLK_491_52_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_491_52_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_245_76_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
				case REF_CLK_61_44_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_61_44_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_8;
				case REF_CLK_30_72_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_30_72_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_16;
				default:
					log_err("Tbgen2: Not a valid Frequency\r\n");
					log_info("Tbgen2: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
			}
		case DCS_PLL_CLK_2:
		case DCS_PLL_CLK_3:
			switch( /*TBGEN2_DIV*/tbgen2_div  )
			{
				case REF_CLK_245_76_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
				case REF_CLK_61_44_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_61_44_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
				case REF_CLK_30_72_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_30_72_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_8;
				default:
					log_err("Tbgen2: Not a valid Frequency\r\n");
					log_info("Tbgen2: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
			}
	}

	return SCFG_TBGEN_REF_CLK_DIVIDER_2;
}

#if !TBGEN1_REF_CLK_IPG_CLK
#ifndef GEUL_LA1246
static uint32_t prvGetTbgen1DivB0( volatile struct gul_hif *pxHif, vuint32 uiRegValue )
{
	switch( uiRegValue )
	{
		case DCS_PLL_CLK_6:
		case DCS_PLL_CLK_10:
		case DCS_PLL_CLK_14:
			switch( /*TBGEN1_DIV*/tbgen1_div )
			{
				// case REF_CLK_491_52_MHZ:
				// 	pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_491_52_REF_CLK_KHZ;
				// 	return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_245_76_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
				case REF_CLK_61_44_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_61_44_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_4;
				case REF_CLK_30_72_MHZ:
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_30_72_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_8;
				default:
					log_err("Tbgen1: Not a valid Frequency\r\n");
					log_info("Tbgen1: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
		case DCS_PLL_CLK_7:
		case DCS_PLL_CLK_11:
		case DCS_PLL_CLK_15:
				log_err("Tbgen1: Setting Source Ref Clk as Platform clk\r\n");
					pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_IPG_CLK / 4;
					return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
			}
	}

	return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
}
#endif
#endif

#if !TBGEN2_REF_CLK_IPG_CLK
static uint32_t GetTbgenrefclkdiv(volatile struct gul_hif *pxHif, uint32_t uiRegValue, TbgenRefClk_t clk_div) {
	switch( uiRegValue )
	{
		case DCS_PLL_CLK_10:
		case DCS_PLL_CLK_11:
			switch( clk_div )
			{
				case REF_CLK_245_76_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
				default:
					log_err("Tbgen2: Not a valid Frequency\r\n");
					log_info("Tbgen2: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_2;
			}
		case DCS_PLL_CLK_6:
		case DCS_PLL_CLK_7:
			switch( clk_div )
			{
				case REF_CLK_122_88_MHZ:
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
				default:
					log_err("Tbgen2: Not a valid Frequency\r\n");
					log_info("Tbgen2: Setting 122.88Mhz as ref freq\r\n");
					pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
					return SCFG_TBGEN_REF_CLK_DIVIDER_1;
			}
		case DCS_PLL_CLK_13:
		case DCS_PLL_CLK_14:
		case DCS_PLL_CLK_15:
			log_err("Tbgen2:Setting Source of Ref Clk as Platform clk\r\n");
			pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_IPG_CLK / 2;
			return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
	}
	return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
}
static uint32_t prvGetTbgen2DivB0( volatile struct gul_hif *pxHif, vuint32 uiRegValue )
{
	vuint32 uiRegVal, uiFuseVal;
	TbgenRefClk_t tbgen_clk_div = 0;
	vuint32 * pulPllcr0 = ( vuint32 * ) ( CCSR_DCGU_BASE_ADDR + PLLCR0_OFFSET);
	struct ccsr_dcsr *regs = (struct ccsr_dcsr *) (DCFG_BASE_ADDR);

	/* Workaround to configure correctly 983 Msps mode TBGEN2 dividers for GulB0
 		Update required for TBGEN2 Ref Freq based on hsdcs_sps configured in YAMI. 
		If HS not set the returned values should be used in TBGEN1 enum LS context 
   		The TBGEN2 clock can be set to max 245MHz (1966MHz / 8) so the REF_CLK_XXX_XX_MHZ 
		defines should be used/scaled accordingly for HS */

	uint32_t pll_val = 0;
	vuint32 ulSvr = in_le32(&regs->ulSvr);
	pll_val = in_le32(pulPllcr0);
	if ((pll_val & (1 << 14)) && (pll_val & (1<<15)) && ((ulSvr & GEUL_SVR_REV_MASK) == GEUL_SVR_REVB_VAL))
	{
		tbgen_clk_div = REF_CLK_122_88_MHZ;
	}
	else
	{
		tbgen_clk_div = REF_CLK_245_76_MHZ;
	}

	uiFuseVal = in_le32(&regs->ulFusesr);
	uiFuseVal = (uiFuseVal >> 12) & 0xf;

	if ( uiFuseVal == FUSE_LA1200 || uiFuseVal == FUSE_LA1201 ||
		uiFuseVal == FUSE_LA1212 || uiFuseVal == FUSE_LA1214 ||
		uiFuseVal == FUSE_LA1223 || uiFuseVal == FUSE_LA1232 ||
		uiFuseVal == FUSE_LA1234 ) {
			pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_IPG_CLK / 2;
			return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
	} else {
		uiRegVal = in_le32( pulPllcr0 );
		uiRegVal &= (CLK_SEL_TOP_MASK << CLK_SEL_TOP);
		if (uiRegVal == (CLK_SEL_TOP_SET_983 << CLK_SEL_TOP)) {
			pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_122_88_REF_CLK_KHZ;
			if((pGulModPriv->pHif->soc_rev & GEUL_SVR_REV_MASK) == GEUL_SVR_REVB_VAL)
			{
				tbgen2_div = REF_CLK_122_88_MHZ;
				/* Override TBGEN_PLL configuration to be sure it's DCS_PLL_CLK_6, => Geul_B0: HS:983.04 MHz  LS: 491.52 MHz */
				vuint32 * puiPORSR1 = ( vuint32 * ) ( DCFG_BASE_ADDR + DCFG_PORSR1_OFFSET );
				vuint32 * puiPORCR1 = ( vuint32 * ) ( DCSR_BASE_ADDR + DCFG_PORCR1_OFFSET );
				uint32_t val = in_le32( puiPORSR1 );
				val |= 0xF00;
				val &= 0xFFFFF6FF; // DCS_PLL_CLK_6 mask for GeulB0
				out_le32(puiPORCR1, val);
				uiRegValue = in_le32( puiPORSR1 );
				uiRegValue = ( uiRegValue >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;
			}
		} else if (uiRegVal == (CLK_SEL_TOP_SET_1966 << CLK_SEL_TOP)) {
			pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_245_76_REF_CLK_KHZ;
			if((pGulModPriv->pHif->soc_rev & GEUL_SVR_REV_MASK) == GEUL_SVR_REVB_VAL)
			{
				/* Override TBGEN_PLL configuration to be sure it's DCS_PLL_CLK_10, => Geul_B0: HS:1966.08 MHz  LS: 491.52 MHz */
				vuint32 * puiPORSR1 = ( vuint32 * ) ( DCFG_BASE_ADDR + DCFG_PORSR1_OFFSET );
				vuint32 * puiPORCR1 = ( vuint32 * ) ( DCSR_BASE_ADDR + DCFG_PORCR1_OFFSET );
				uint32_t val = in_le32( puiPORSR1 );
				val |= 0xF00;
				val &= 0xFFFFFAFF; // DCS_PLL_CLK_10 mask for GeulB0
				out_le32(puiPORCR1, val);
				uiRegValue = in_le32( puiPORSR1 );
				uiRegValue = ( uiRegValue >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;
			}
		} else {
			log_err("Tbgen2:Setting Source of Ref Clk as Platform clk\r\n");
			pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_IPG_CLK / 2;
			return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
		}
		return GetTbgenrefclkdiv(pxHif, uiRegValue, tbgen_clk_div);
	}

	return SCFG_INVALID_TBGEN_REF_CLK_DIVIDER;
}
#endif

#if (TBGEN1_REF_CLK_IPG_CLK || TBGEN2_REF_CLK_IPG_CLK)
static uint8_t prvGetTbgenIPGDivB0( uint8_t ucIPGDiv )
{
    switch( ucIPGDiv )
    {
	case REF_CLK_IPG_1_DIV:
	    return 0;
	case REF_CLK_IPG_2_DIV:
	    return 1;
	case REF_CLK_IPG_4_DIV:
	    return 2;
	case REF_CLK_IPG_8_DIV:
	    return 3;
	case REF_CLK_IPG_16_DIV:
	    return 4;
	case REF_CLK_IPG_32_DIV:
	    return 5;
	default:
	    return 0;
    }
}
#endif

void vSCFGInitTbgenClk(volatile struct gul_hif *pxHif, int iLSDCSInitStatus, int * iHsDcsIPClkEn )
{
	struct scfg_regs *regs;
	uint32_t val;
	vuint32 uiRegValue = 0;
	vuint32 * puiPORSR1 = ( vuint32 * ) ( DCFG_BASE_ADDR + DCFG_PORSR1_OFFSET );
	uint32_t timeout = 10000;
#if !(TBGEN1_REF_CLK_IPG_CLK && TBGEN2_REF_CLK_IPG_CLK)
    uint32_t temp = 0;
#endif

    while(timeout--)
    {
       if(CHK_HIF_HOST_RDY(pxHif, HIF_HOST_READY_HS_TBGEN2)) {
           *iHsDcsIPClkEn = 1;
           break;
       }
       vUdelay(100);
    };
    regs = (struct scfg_regs *) (SCFG_BASE_ADDR);
    val = in_le32(&regs->conf_ctrl1);
	if ( get_soc_revision() == GEUL_SVR_REVB_VAL )
    {
        val &= ~(TBGEN1_REF_CLK_SEL);
#if TBGEN1_REF_CLK_IPG_CLK
	pxHif->tbgen_clk_info.tbgen1_freq_khz = (TBGEN_IPG_CLK / 4) / TBGEN1_IPG_DIV;
	pxHif->tbgen_clk_info.tbgen1_clk_src = IPG_CLK;
        val |= TBGEN1_CD_EN |  (prvGetTbgenIPGDivB0(TBGEN1_IPG_DIV) << TBGEN1_REF_CLK_RAT);
#else
        /* iLSDCSInitStatus  = 0, implies LS-DCS initialization Success */
        /* iLSDCSInitStatus != 0, implies LS-DCS initialization failure */
        if( iLSDCSInitStatus )
        {
            val |= TBGEN1_CD_EN ; /* Fallback Mechanism to IPG Clk */
	    pxHif->tbgen_clk_info.tbgen1_freq_khz = TBGEN_IPG_CLK / 4;
	    pxHif->tbgen_clk_info.tbgen1_clk_src = IPG_CLK;
            log_err("Tbgen1 is running at IPG clk and not from Dcs clk\r\n");
        }
        else
        {
#ifndef GEUL_LA1246
            /* Read PORSR1[11:8] - DCFG Block */
            uiRegValue = in_le32( puiPORSR1 );
            uiRegValue = ( uiRegValue >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;
            temp = prvGetTbgen1DivB0( pxHif, uiRegValue );
            pxHif->tbgen_clk_info.tbgen1_clk_src = LS_DCS_OUTPUT;
			tbgen1_div_B0 = (uint8_t)temp;

            if( temp != SCFG_INVALID_TBGEN_REF_CLK_DIVIDER )
            {
                val |= (temp<< TBGEN1_REF_CLK_RAT) | TBGEN1_REF_CLK_SEL;
            }
            else
            {
                pxHif->tbgen_clk_info.tbgen1_clk_src = IPG_CLK;
            }
            val |= TBGEN1_CD_EN ;
#endif
        }
#endif /* TBGEN1_REF_CLK_IPG_CLK */

	if(!CHK_HIF_HOST_RDY(pxHif, HIF_HOST_DISABLE_TBGEN2)) {
#if TBGEN2_REF_CLK_IPG_CLK
		*iHsDcsIPClkEn = 1;
		pxHif->tbgen_clk_info.tbgen2_freq_khz = (TBGEN_IPG_CLK / 2) / TBGEN2_IPG_DIV;
		pxHif->tbgen_clk_info.tbgen2_clk_src = IPG_CLK;
		val |= TBGEN2_CD_EN | TBGEN2_REF_CLK_SEL_IPG_CLK_2 | (prvGetTbgenIPGDivB0(TBGEN2_IPG_DIV) << TBGEN2_REF_CLK_RAT);
#else
		if( *iHsDcsIPClkEn )
		{
			/* Read PORSR1[11:8] - DCFG Block */
			uiRegValue = in_le32( puiPORSR1 );
			uiRegValue = ( uiRegValue >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;
			temp = prvGetTbgen2DivB0( pxHif, uiRegValue );
			pxHif->tbgen_clk_info.tbgen2_clk_src = HS_DCS_OUTPUT;
			tbgen2_div_B0 = (uint8_t)temp;

			if( temp != SCFG_INVALID_TBGEN_REF_CLK_DIVIDER )
			{
				val |= temp << TBGEN2_REF_CLK_RAT;
			}
			else
			{
				pxHif->tbgen_clk_info.tbgen2_clk_src = IPG_CLK;
				val |= TBGEN2_REF_CLK_SEL_IPG_CLK_2;
				PRINTF("WARNING - TBGEN2 is now configured to ipg_clk/2 instead of TBgen_PLL_Output2 (platform_clock = 614.4) \r\n");
			}
			val |= TBGEN2_CD_EN;
		}
		else
		{
			*iHsDcsIPClkEn = 1; /* Fallback Mechanism to IPG Clk */
			pxHif->tbgen_clk_info.tbgen2_freq_khz = TBGEN_IPG_CLK / 2;
			pxHif->tbgen_clk_info.tbgen2_clk_src = IPG_CLK;
			val |= TBGEN2_CD_EN | TBGEN2_REF_CLK_SEL_IPG_CLK_2;
			log_err("Tbgen2 is running at IPG clk and not from Dcs clk\r\n");
		}
#endif /* TBGEN2_REF_CLK_IPG_CLK */
	}
    }
    else
    {
        /* Read PORSR1[11:8] - DCFG Block */
        uiRegValue = in_le32( puiPORSR1 );
        uiRegValue = ( uiRegValue >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;

#ifndef GEUL_LA1246
        val |= prvGetTbgen1Div( pxHif, uiRegValue ) << TBGEN1_REF_CLK_RAT;
        pxHif->tbgen_clk_info.tbgen1_clk_src = LS_DCS_OUTPUT;
        val |=  TBGEN1_CD_EN | TBGEN1_REF_CLK_SEL;
#endif
	if( *iHsDcsIPClkEn ) {
	    val |= prvGetTbgen2Div( pxHif, uiRegValue ) << TBGEN2_REF_CLK_RAT;
	    pxHif->tbgen_clk_info.tbgen2_clk_src = HS_DCS_OUTPUT;
	    val |= TBGEN2_CD_EN | TBGEN2_REF_CLKSEL_DCS_CLK1;
	}
    }
    (void) iLSDCSInitStatus;

	out_le32(&regs->conf_ctrl1, val);
	log_info("%s: conf_ctrl1(0x%x) 0x%x\r\n", __func__,
			&regs->conf_ctrl1, in_le32(&regs->conf_ctrl1));
}

void vBoardConfigPinMux()
{
    GpioStatusCode_t retStatus;
    if (!isMWrBNRC())
        return;
/**
 * By default all the GPIO Pins will be in GPIO mode
 */
#if 0
    struct ccsr_pmux * pmux = ( struct ccsr_pmux * ) ( PMUXCR_BASE_ADDR_BANK1 );
    enum pmux_num num = PMUX_3;

    out_le32( &( pmux->ulPMuxCR[ num ] ), PMUX_LS_GPIO_MODE_MW );
    out_le32( &( pmux->ulPMuxCR[ num + 1 ] ), PMUX_LS_MODE_ALL );
#endif
    retStatus = exGpioInit( GPIO_2, TBGEN1_GPIO_EN_1V8, GPIO_OUTPUT );
    if( GPIO_SUCCESS != retStatus )
    {
        PRINTF( "\n GPIO init failed" );
    }
    else
    {
        retStatus = exGpioSetData( GPIO_2, TBGEN1_GPIO_EN_1V8, 1 );
        if( GPIO_SUCCESS != retStatus )
        {
            PRINTF( "\n GPIO set data  failed" );
        }
    }
}

uint8_t get_board_version(void)
{
#if !defined(GEUL_LA1224) && !defined(GEUL_LA1224CPE)
	uint32_t gpio3DataReg;
	GpioStatusCode_t ret = GPIO_SUCCESS;

	ret |= exGpioInit(GPIO_3, 29, GPIO_INPUT);
	ret |= exGpioInit(GPIO_3, 30, GPIO_INPUT);
	ret |= exGpioInit(GPIO_3, 31, GPIO_INPUT);

	if (ret != GPIO_SUCCESS)
		PRINTF("\n Invalid board version");

	ret = exGpioGetDataRegister(GPIO_3, &gpio3DataReg);
	if (ret != GPIO_SUCCESS)
                PRINTF("\n GPIO get data register failed");

	return (gpio3DataReg & 0x7);
#else
	uint8_t ucCore = (uint8_t)ulMpicCurrentCore();
	uint8_t ucData = 0xff;
	uint8_t ucVal;
	uint32_t ucAddr = IO_EXAPNDER_CONF_REG;
	int iRet = 0;
	if(ucCore == GEUL_E200_MASTER_CORE) {
		iRet = iI2C_Write( I2C1_BASE_ADDR, PCAL6524_BASE_ADDR, ucAddr, 1, &ucData, 1);
		if(iRet >= 0) {
			ucAddr = IO_EXAPNDER_INPUT_REG;
			iRet = iI2C_Read( I2C1_BASE_ADDR, PCAL6524_BASE_ADDR, ucAddr, 1, &ucVal, 1);
			if(iRet == 1) {
				ucVal = ((ucVal >> BOARD_REV_SHIFT_MASK) & BOARD_REV_MASK);
			if(ucVal == 0x1)
				return GEUL_HOST_REVC_VAL;
			}
		}
	}
#endif
	return 0;
}

void vRedirectModemLogToHost(uint8_t core_id)
{
    uint32_t uart_base = 0;

    switch (core_id)
    {
    case 0:
	uart_base = UART_BASE_ADDR_CORE1; break;
    case 1:
	uart_base = UART_BASE_ADDR_CORE0; break;
    default: break;
    }

    xDebugConsoleInit((void *)uart_base, UART_CLOCK_FREQUENCY, UART_BAUDRATE);

}

void vBoardEarlyInit(uint8_t core_id)
{
	int ulRet;
    uint32_t uart_base = 0;

    switch (core_id)
    {
    case 0:
	#if GEUL_LA12XX_MODEM_LOG_AT_HOST_ENABLE == 1
		uart_base = UART_BASE_ADDR_CORE1; break;
	#else
		uart_base = UART_BASE_ADDR_CORE0; break;
	#endif
    case 1:
	#if GEUL_LA12XX_MODEM_LOG_AT_HOST_ENABLE == 1
		uart_base = UART_BASE_ADDR_CORE0; break;
	#else
		uart_base = UART_BASE_ADDR_CORE1; break;
	#endif

    case 2: uart_base = UART_BASE_ADDR_CORE2; break;
    case 3: uart_base = UART_BASE_ADDR_CORE3; break;
    /* core 5 , core 6 console prints are diverted towards UART2 and UART3 */
    case 4: uart_base = UART_BASE_ADDR_CORE2; break;
    case 5: uart_base = UART_BASE_ADDR_CORE3; break;
    default: break;
    }

    xDebugConsoleInit((void *)uart_base, UART_CLOCK_FREQUENCY, UART_BAUDRATE);

	/* Init I2C */
    if (core_id == GEUL_E200_MASTER_CORE)
    {
	    ulRet = iI2C_Init(I2C1_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
	    if (ulRet == 1)
		    DPRINTF("I2C1 init successful\n\r");
	    else
		    PRINTF("I2C1 init failed\n\r");

	    ulRet = iI2C_Init(I2C2_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
	    if (ulRet == 1)
		    DPRINTF("I2C2 init successful\n\r");
	    else
		    PRINTF("I2C2 init failed\n\r");
	    ulRet = iI2C_Init(I2C3_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
	    if (ulRet == 1)
		    DPRINTF("I2C3 init successful\n\r");
	    else
		    PRINTF("I2C3 init failed\n\r");
    }

    /* get_board_version uses i2c1 so we need to keep this after i2c1 init */
    uint8_t ucCore = (uint8_t)ulMpicCurrentCore();
    if(ucCore == GEUL_E200_MASTER_CORE) {
	    brd_ver=get_board_version();
    }
    PRINTF("Modem Board Revision");
#if !defined(GEUL_LA1224) && !defined(GEUL_LA1224CPE)
    switch(brd_ver)
    {
	case 0x00:PRINTF(" A \n\r");break;
	case 0x01:PRINTF(" B \n\r");break;
	default: PRINTF(" Unknown 0x%x\n\r", brd_ver); break;
    }
#else
    switch(brd_ver)
    {
	case GEUL_HOST_REVA_VAL: PRINTF(" A \n\r");break;
	case GEUL_HOST_REVB_VAL: PRINTF(" B \n\r");break;
	case GEUL_HOST_REVC_VAL: PRINTF(" C \n\r");break;
	default: PRINTF(" Unknown 0x%x\n\r", brd_ver); break;
    }
#endif


    if (core_id == GEUL_E200_MASTER_CORE)
    {
        vBoardConfigPinMux();
        if (isMWrBNRC())
        {
            ulRet = iI2C_Init(I2C3_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
            if (ulRet == 1)
                DPRINTF("I2C3 init successful\n\r");
            else
                PRINTF("I2C3 init failed\n\r");

            ulRet = iI2C_Init(I2C4_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
            if (ulRet == 1)
                DPRINTF("I2C4 init successful\n\r");
            else
                PRINTF("I2C4 init failed\n\r");

            vInitSCFG();
        }
#if	INIT_I2C_CTRL_5_6
        ulRet = iI2C_Init(I2C5_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
        if (ulRet == 1)
            DPRINTF("I2C5 init successful\n\r");
        else
            PRINTF("I2C5 init failed\n\r");

        ulRet = iI2C_Init(I2C6_BASE_ADDR, I2C_CLK_FREQ, I2C_FREQ);
        if (ulRet == 1)
            DPRINTF("I2C6 init successful\n\r");
        else
            PRINTF("I2C6 init failed\n\r");
#endif
    }
}

void vBoardFinalInit(void)
{

}

void vPrintTbgenClkinfo(void)
{
	uint32_t uTbgenFreq;
	enum tbgen_clk_source eClkSrc;

	uTbgenFreq = uGetTbgenFreq(TBGEN_1);
	if(uTbgenFreq) {
		eClkSrc = eGetTbgenClkSrc(TBGEN_1);
		switch( eClkSrc ) {
			case LS_DCS_OUTPUT:
				log_info("Tbgen1 clk source: LS_DCS Output\r\n");
				break;
			case IPG_CLK:
				log_info("Tbgen1 clk source: Platform clk\r\n");
				break;
			default:
				log_info("Invalid clk source\r\n");
		}
		log_info("Tbgen1 Frequecncy: %u KHz\r\n", uTbgenFreq);
	} else {
		log_info("Tbgen1: Disabled\r\n");
	}
	uTbgenFreq = uGetTbgenFreq(TBGEN_2);
	if(uTbgenFreq) {
		eClkSrc = eGetTbgenClkSrc(TBGEN_2);
		switch( eClkSrc ) {
			case HS_DCS_OUTPUT:
				log_info("Tbgen2 clk source: HS_DCS Output\r\n");
				break;
			case IPG_CLK:
				log_info("Tbgen2 clk source: Platform clk\r\n");
				break;
			default:
				log_info("Invalid clk source\r\n");
		}
		log_info("Tbgen2 Frequecncy: %u KHz \r\n", uTbgenFreq);
	} else {
		log_info("Tbgen2: Disabled\r\n");
	}
}
