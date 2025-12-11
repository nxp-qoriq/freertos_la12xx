// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_dcs.h"

#if GEUL_DEMO_DCS_TEST
#include "FreeRTOS.h"
#include "dcs.h"


/******************************************************************
*			Global Parameters
******************************************************************/


/******************************************************************
*			API function's
******************************************************************/

static DcsParams_t pxDcsHSParam;

int iDcsHSInitTC()
{
	int uiErr = 0;
	/* Set Params for DCS HS */
	pxDcsHSParam.eDcsPllClkMode = DCS_PLL_CLK_0;	/* Configuring DCS_PLL to CLK_0 */
	pxDcsHSParam.ePrimaryClkSrc = PrimaryClkSrc_1;
	pxDcsHSParam.eClkDiv = ClkDiv_3;
	pxDcsHSParam.eConvPair = ConvPair_2;
	/* TODO [29 Jan 2020-2021]: Check if Clock divider needs to be set for Conv Pairs in HS */
	pxDcsHSParam.eDacConvPairClk = CP_ClkDiv_0;
	pxDcsHSParam.eAdcConvPairClk = CP_ClkDiv_0;

	/* Firmware Loading Check - Done inside DcsInit */
	uiErr = vDcsInit( DCS_HS, &pxDcsHSParam );
	if( uiErr ) {
		log_err("DCS_HS: Initialization failed\n\r");
		return uiErr;
	}

	return SUCCESS;
}

static DcsParams_t pxDcsParam1;
static DcsParams_t pxDcsParam2;

int iDcsLSInitTC()
{
	/* Select DCS PLL Reference Clock
		- Should be called explicitly before calling DCS init */
	vSetDcsRefClk( DCS_REF_CLK_160 );	/* Select 160 Mhz DCS Ref Clk */


	/* Set Params for DCS1 */
	pxDcsParam1.eDcsPllClkMode = DCS_PLL_CLK_0;
	pxDcsParam1.ePrimaryClkSrc = PrimaryClkSrc_1;
	pxDcsParam1.eClkDiv = ClkDiv_4;
	pxDcsParam1.eConvPair = ConvPair_Both;	/* Configure both ConversionPair 1 & 2 */
	pxDcsParam1.eDacConvPairClk = CP_ClkDiv_0; /* Apply clock divider 2 ^ CP_ClkDiv_x */
	pxDcsParam1.eAdcConvPairClk = CP_ClkDiv_1; /* Apply clock divider 2 ^ CP_ClkDiv_x */
	pxDcsParam1.eClkMode = WIRELESS_WIFI_DUAL_CH_CLK_MODE;

	/* Set Params for DCS2 */
	pxDcsParam1.eDcsPllClkMode = DCS_PLL_CLK_0;
	pxDcsParam2.ePrimaryClkSrc = PrimaryClkSrc_2;
	pxDcsParam2.eClkDiv = ClkDiv_0;
	pxDcsParam2.eConvPair = ConvPair_1;	/* Configure only ConversionPair 1 */
	pxDcsParam1.eDacConvPairClk = CP_ClkDiv_0; /* Apply clock divider 2 ^ CP_ClkDiv_x */
	pxDcsParam1.eAdcConvPairClk = CP_ClkDiv_1; /* Apply clock divider 2 ^ CP_ClkDiv_x */
	pxDcsParam1.eClkMode = WIRELESS_5G_SINGLE_CH_CLK_MODE;

	/* Initialize DCS Sub-Systems with params above */
	if( vDcsInit( DCS_LS1, &pxDcsParam1 ) ) {
		log_info("DCS_LS1: Initialization Failed\n");
	}
	if(vDcsInit( DCS_LS2, &pxDcsParam2 ) ) {
		log_info("DCS_LS1: Initialization Failed\n");
	}

	return SUCCESS;
}


void vDcsLsDemo( void )
{
	int iResult = 0;
	u32 uiCurrentCore = ( u32 ) ulMpicCurrentCore();
	
	iResult = iDcsLSInitTC();
	
	if( iResult == 0 )
	{
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_DCS_TEST_STATUS );
	}
	else
	{
		RESET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_DCS_TEST_STATUS );
	}
	
}

void vDcsHsDemo( void )
{
	int iResult = 0;
	u32 uiCurrentCore = ( u32 ) ulMpicCurrentCore();

	iResult = iDcsHSInitTC();

	if( iResult == 0 )
	{
		SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_DCS_TEST_STATUS );
	}
	else
	{
		RESET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_DCS_TEST_STATUS );
	}
}

#endif	/* GEUL_DEMO_DCS_TEST */
