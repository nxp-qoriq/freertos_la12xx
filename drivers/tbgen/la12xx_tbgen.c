// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2024 NXP
 */

#include <types.h>
#include <FreeRTOS.h>
#include <task.h>
#include <math.h>
#include <tbgen_new.h>
#include "la12xx_tbgen.h"
#include "dcs.h"
#include <mpic.h>
#include "pmux.h"
#include "Time.h"
#include "ppc.h"
#include "ppu_intrinsics.h"
#include "scfg.h"

const uint32_t uTimerCtrlOffset[MAX_TIMER_TYPES] = { AXRF_TIMER_CTRL_OFFSET,
													 RX_ALIGNMENT_TIMER_CTRL_OFFSET,
													 SRX_ALIGNMENT_TIMER_CTRL_OFFSET,
													 SPI_TRIGGER_TIMER_CTRL_OFFSET,
													 AGC_TIMER_CTRL_OFFSET,
													 TIMED_INT_TIMER_CTRL_OFFSET,
													 GPE_TIMER_CTRL_OFFSET,
													 TDD_TIMER_CTRL_OFFSET };

const uint8_t ucMaxTimerInstances[MAX_TIMER_TYPES] = { 	AXRF_MAX_INSTANCE,
													 	RX_ALIGNMENT_MAX_INSTANCE,
													 	SRX_ALIGNMENT_MAX_INSTANCE,
														SPI_TRIGGER_MAX_INSTANCE,
														AGC_ENABLE_MAX_INSTANCE,
														TIMED_INT_MAX_INSTANCE,
														GPE_MAX_INSTANCE,
														TDD_MAX_INSTANCE };

const TbgenPmuxInfo_t Tbgen1SPIInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_5 ] = {
	{ PMUX_4, PMUX4_GPIO_2_27 },
	{ PMUX_4, PMUX4_GPIO_2_28 },
	{ PMUX_4, PMUX4_GPIO_2_29 },
};

const TbgenPmuxInfo_t Tbgen2SPIInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_5 ] = {
	{ PMUX_5, PMUX5_GPIO_3_12 },
	{ PMUX_5, PMUX5_GPIO_3_13 },
	{ PMUX_5, PMUX5_GPIO_3_14 },
};

const TbgenPmuxInfo_t Tbgen1SRXInfo[ TIMER_INSTANCE_4 - TIMER_INSTANCE_2 ] = {
	{ PMUX_3, PMUX3_GPIO_2_9 },
	{ PMUX_3, PMUX3_GPIO_2_10 },
};

const TbgenPmuxInfo_t Tbgen2SRXInfo[ TIMER_INSTANCE_3 - TIMER_INSTANCE_2 ] = {
	{ PMUX_5, PMUX5_GPIO_3_15 },
};

const TbgenPmuxInfo_t Tbgen1AGCInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_0 ] = {
	{ PMUX_4, PMUX4_GPIO_2_19 },
	{ PMUX_4, PMUX4_GPIO_2_20 },
	{ PMUX_4, PMUX4_GPIO_2_21 },
	{ PMUX_4, PMUX4_GPIO_2_22 },
	{ PMUX_4, PMUX4_GPIO_2_23 },
	{ PMUX_4, PMUX4_GPIO_2_24 },
	{ PMUX_4, PMUX4_GPIO_2_25 },
	{ PMUX_4, PMUX4_GPIO_2_26 },
};

const TbgenPmuxInfo_t Tbgen2AGCInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_0 ] = {
	{ PMUX_6, PMUX6_GPIO_3_24 },
	{ PMUX_6, PMUX6_GPIO_3_25 },
	{ PMUX_6, PMUX6_GPIO_3_26 },
	{ PMUX_6, PMUX6_GPIO_3_27 },
	{ PMUX_6, PMUX6_GPIO_3_28 },
	{ PMUX_6, PMUX6_GPIO_3_29 },
	{ PMUX_6, PMUX6_GPIO_3_30_31 },
	{ PMUX_6, PMUX6_GPIO_3_30_31 },
};

const TbgenPmuxInfo_t Tbgen1GPEInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_0 ] = {
	{ PMUX_3, PMUX3_GPIO_2_11 },
	{ PMUX_3, PMUX3_GPIO_2_12 },
	{ PMUX_3, PMUX3_GPIO_2_13 },
	{ PMUX_3, PMUX3_GPIO_2_14 },
	{ PMUX_3, PMUX3_GPIO_2_15 },
	{ PMUX_4, PMUX4_GPIO_2_16 },
	{ PMUX_4, PMUX4_GPIO_2_17 },
	{ PMUX_4, PMUX4_GPIO_2_18 },
};

const TbgenPmuxInfo_t Tbgen2GPEInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_0 ] = {
	{ PMUX_6, PMUX6_GPIO_3_16 },
	{ PMUX_6, PMUX6_GPIO_3_17 },
	{ PMUX_6, PMUX6_GPIO_3_18 },
	{ PMUX_6, PMUX6_GPIO_3_19 },
	{ PMUX_6, PMUX6_GPIO_3_20 },
	{ PMUX_6, PMUX6_GPIO_3_21 },
	{ PMUX_6, PMUX6_GPIO_3_22 },
	{ PMUX_6, PMUX6_GPIO_3_23 },
};

const TbgenPmuxInfo_t Tbgen2AXRFInfo[ TIMER_INSTANCE_10 - TIMER_INSTANCE_2 ] = {
	{ PMUX_5, PMUX5_GPIO_3_0 },
	{ PMUX_5, PMUX5_GPIO_3_1 },
	{ PMUX_5, PMUX5_GPIO_3_2 },
	{ PMUX_5, PMUX5_GPIO_3_3 },
	{ PMUX_5, PMUX5_GPIO_3_4 },
	{ PMUX_5, PMUX5_GPIO_3_5 },
	{ PMUX_5, PMUX5_GPIO_3_6 },
	{ PMUX_5, PMUX5_GPIO_3_7 },
};

const TbgenPmuxInfo_t Tbgen2TDDTxInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_4 ] = {
	{ PMUX_5, PMUX5_GPIO_3_8 },
	{ PMUX_5, PMUX5_GPIO_3_9 },
	{ PMUX_5, PMUX5_GPIO_3_10 },
	{ PMUX_5, PMUX5_GPIO_3_11 },
};

const TbgenPmuxInfo_t Tbgen2TDDRxInfo[ TIMER_INSTANCE_8 - TIMER_INSTANCE_4 ] = {
	{ PMUX_5, PMUX5_GPIO_3_12 },
	{ PMUX_5, PMUX5_GPIO_3_13 },
	{ PMUX_5, PMUX5_GPIO_3_14 },
	{ PMUX_5, PMUX5_GPIO_3_15 },
};

uint32_t uGetTbgenFreq( uint8_t ucTbgenNo )
{
	volatile struct gul_hif * pxHif = bsp_get_hif();

	if( ucTbgenNo == TBGEN_1 )
		return pxHif->tbgen_clk_info.tbgen1_freq_khz;
	else
		return pxHif->tbgen_clk_info.tbgen2_freq_khz;
}

enum tbgen_clk_source eGetTbgenClkSrc( uint8_t ucTbgenNo )
{
	volatile struct gul_hif * pxHif = bsp_get_hif();

	if( ucTbgenNo == TBGEN_1 )
		return pxHif->tbgen_clk_info.tbgen1_clk_src;
	else
		return pxHif->tbgen_clk_info.tbgen2_clk_src;
}

int iConfPMuxModeTbgenTdd( TimerInstance_t eInstance, uint8_t ucTxRx )
{
	enum pmux_num enumber = -1;
	enum pmux_index eindex = -1;;

	if( ucTxRx )
	{
		if( eInstance >= 4 && eInstance <=7 )
		{
			enumber = Tbgen2TDDTxInfo[eInstance - 4].num;
			eindex = Tbgen2TDDTxInfo[eInstance - 4].index;
		}
	}
	else
	{
		if( eInstance >= 4 && eInstance <=7 )
		{
			enumber = Tbgen2TDDRxInfo[eInstance - 4].num;
			eindex = Tbgen2TDDRxInfo[eInstance - 4].index;
		}
	}
	if( enumber >= 0 && eindex >= 0 )
	{
		switchPMuxMode( enumber, eindex, ALT_MODE2 );
		return 0;
	}

	return -1;
}

int iConfPMuxModeTbgen( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	enum pmux_num enumber = -1;
	enum pmux_index eindex = -1;;

	if( ucTbgenNo == TBGEN_1 )
	{
		switch( eTimerType )
		{
			case SRX_ALIGNMENT:
				if( eInstance >= 2 && eInstance <=3 )
				{
					enumber = Tbgen1SRXInfo[eInstance - 2].num;
					eindex = Tbgen1SRXInfo[eInstance - 2].index;
				}
				break;
			case SPI_TRIGGER:
				if( eInstance >= 5 && eInstance <=7 )
				{
					enumber = Tbgen1SPIInfo[eInstance - 5].num;
					eindex = Tbgen1SPIInfo[eInstance - 5].index;
				}
				break;
			case AGC_ENABLE:
				if( eInstance >= 0 && eInstance <=7 )
				{
					enumber = Tbgen1AGCInfo[eInstance].num;
					eindex = Tbgen1AGCInfo[eInstance].index;
				}
				break;
			case GPE:
				if( eInstance >= 0 && eInstance <=7 )
				{
					enumber = Tbgen1GPEInfo[eInstance].num;
					eindex = Tbgen1GPEInfo[eInstance].index;
				}
				break;
			default:
				break;
		}
	}
	else if( ucTbgenNo == TBGEN_2 )
	{
		switch( eTimerType )
		{
			case AXRF:
				if( eInstance >= 2 && eInstance <=9 )
				{
					enumber = Tbgen2AXRFInfo[eInstance - 2].num;
					eindex = Tbgen2AXRFInfo[eInstance - 2].index;
				}
				break;
			case SRX_ALIGNMENT:
				if( eInstance == 2 )
				{
					enumber = Tbgen2SRXInfo[eInstance - 2].num;
					eindex = Tbgen2SRXInfo[eInstance - 2].index;
				}
				break;
			case SPI_TRIGGER:
				if( eInstance >= 5 && eInstance <=7 )
				{
					enumber = Tbgen2SPIInfo[eInstance - 5].num;
					eindex = Tbgen2SPIInfo[eInstance - 5].index;
				}
				break;
			case AGC_ENABLE:
				if( eInstance >= 0 && eInstance <=7 )
				{
					enumber = Tbgen2AGCInfo[eInstance].num;
					eindex = Tbgen2AGCInfo[eInstance].index;
				}
				break;
			case GPE:
				if( eInstance >= 0 && eInstance <=7 )
				{
					enumber = Tbgen2GPEInfo[eInstance].num;
					eindex = Tbgen2GPEInfo[eInstance].index;
				}
				break;
			default:
				break;
		}
	}
	if( enumber >= 0 && eindex >= 0 )
	{
		switchPMuxMode( enumber, eindex, ALT_MODE2 );
		return 0;
	}

	return -1;
}

static inline int prvCheckTimerConf( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	if( ucTbgenNo != TBGEN_1 && ucTbgenNo != TBGEN_2 )
	{
		return INVALID_TBGEN_NO;
	}
	if( eTimerType >= MAX_TIMER_TYPES || eTimerType < 0 )
	{
		return INVALID_TIMER_TYPE;
	}
	if( eInstance >= ucMaxTimerInstances[eTimerType] || eInstance < 0 )
	{
		return INVALID_TIMER_INSTANCE;
	}

	return 0;
}

static inline int prvCheckTddTimerConf( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	if( ucTbgenNo != TBGEN_1 && ucTbgenNo != TBGEN_2 )
	{
		return INVALID_TBGEN_NO;
	}
	if( eInstance >= ucMaxTimerInstances[TDD] && eInstance < 0 )
	{
		return INVALID_TIMER_INSTANCE;
	}

	return 0;
}

static inline int prvCheckTddNo( uint8_t ucTbgenNo )
{
	if( ucTbgenNo != TBGEN_1 && ucTbgenNo != TBGEN_2 )
	{
		return INVALID_TBGEN_NO;
	}

	return 0;
}

static inline void prvProgramTimedIntCtrlReg( vuint32 * puCtrlOffset, TriggerMode_t eTrigMode )
{
	TBGEN_WRITE_REGISTER( puCtrlOffset, (TBGEN_READ_REGISTER( puCtrlOffset ) & CTRL_TRIGGER_BIT_CLR) | (eTrigMode << CTRL_TRIGGER_BIT) );
}

static inline void prvProgramAXRFCtrlReg( vuint32 * puCtrlOffset, StrobePolarity_t ePolarity )
{
	TBGEN_WRITE_REGISTER( puCtrlOffset, (TBGEN_READ_REGISTER( puCtrlOffset ) & CTRL_POLARITY_BIT_CLR) | (ePolarity << CTRL_POLARITY_BIT) );
}

static void prvProgramCtrlReg( vuint32 * puCtrlOffset, StrobeMode_t eSm, PulseWidth_t ePw, TriggerMode_t eTrigMode, StrobePolarity_t ePolarity )
{
	vuint32 uRegValue;

	uRegValue = TBGEN_READ_REGISTER( puCtrlOffset );
	uRegValue = (uRegValue & CTRL_STROBE_MODE_BIT_CLR) | (eSm << CTRL_STROBE_MODE_BIT);
	if ( eSm == STROBE_MODE_PULSE )
	{
		uRegValue = (uRegValue & CTRL_PULSE_WIDTH_BIT_CLR) | (ePw << CTRL_PULSE_WIDTH_BIT);
	}
	uRegValue = (uRegValue & CTRL_TRIGGER_BIT_CLR) | (eTrigMode << CTRL_TRIGGER_BIT);
	uRegValue = (uRegValue & CTRL_POLARITY_BIT_CLR) | (ePolarity << CTRL_POLARITY_BIT);
	TBGEN_WRITE_REGISTER( puCtrlOffset, uRegValue );
}

static inline void prvEnableTimerInstance( vuint32 * puCtrlOffset )
{
	TBGEN_WRITE_REGISTER( puCtrlOffset, (TBGEN_READ_REGISTER( puCtrlOffset ) | CTRL_TMR_ENABLE_MASK) );
}

static inline void prvDisableTimerInstance( vuint32 * puCtrlOffset )
{
	TBGEN_WRITE_REGISTER( puCtrlOffset, ( TBGEN_READ_REGISTER( puCtrlOffset ) & CTRL_TMR_DISABLE_MASK ) );
}

static inline void prvConfigOffset( vuint32 * puOffsetHi, vuint32 * puOffsetLo, u64 uOffset )
{
	TBGEN_WRITE_REGISTER( puOffsetHi, ( u32 ) ( ( uOffset & HI_WORD_MASK ) >> HI_WORD_SHIFT_BITS ) );
	TBGEN_WRITE_REGISTER( puOffsetLo, ( u32 ) ( uOffset & LO_WORD_MASK ) );
}

static inline void prvConfigIntrvl( vuint32 * puIntrvl, u32 uInterval )
{
	TBGEN_WRITE_REGISTER( puIntrvl, uInterval );
}

static void prvConfigTddDurMode( vuint32 * puTddModeBase, vuint32 * puTddDurationBase, TddDuration_t * pxCsgDur, u8 ucCSGSeqSteps )
{
	u8 ucSeqIndex = 0;
	vuint32 uRegValue = 0;
	u32 uMask = 0;

	uRegValue = TBGEN_READ_REGISTER( puTddModeBase );
	/* Write duration & mode sequence for valid steps */
	for( ucSeqIndex = 0; ucSeqIndex < ucCSGSeqSteps; ucSeqIndex++ )
	{
		/* Program TDDx Mode Register */
		uMask = ~ ( ( u32 ) TDD_MODE_BIT_MASK << ( TDD_MODE_BIT_MASK_SIZE * ucSeqIndex ) );
		uRegValue &= uMask;
		uMask = ( u32 ) pxCsgDur[ ucSeqIndex ].eMode << ( TDD_MODE_BIT_MASK_SIZE * ucSeqIndex );
		uRegValue |= uMask;
		/* Program TDDx Durationx Register */
		TBGEN_WRITE_REGISTER( ( puTddDurationBase + ucSeqIndex ), pxCsgDur[ ucSeqIndex ].uDur );
	}
	TBGEN_WRITE_REGISTER( puTddModeBase, uRegValue );
}

static void prvProgramTddCtrlReg( vuint32 * puTddCtrlOffset, TddPulseMode_t eTddPm, u16 usPw, TriggerMode_t eTm, u8 ucBufLength )
{
	vuint32 uRegValue = 0x0;

	uRegValue = TBGEN_READ_REGISTER( puTddCtrlOffset );
	uRegValue = ( uRegValue & TDD_CTRL_PULSE_MODE_BIT_CLR ) | ( eTddPm << TDD_CTRL_PULSE_MODE_BIT );
	if ( eTddPm == TDD_PULSE_MODE_01 )
	{
		uRegValue = ( uRegValue & TDD_CTRL_PULSE_WIDTH_BIT_CLR ) | ( usPw << TDD_CTRL_PULSE_WIDTH_BIT );
	}
	uRegValue = ( uRegValue & TDD_CTRL_BUFLENGTH_BIT_CLR ) | ( (ucBufLength - 1) << TDD_CTRL_BUFLENGTH_BIT );
	uRegValue = ( uRegValue & TDD_CTRL_CONTSEQ_BIT_CLR ) | ( eTm << TDD_CTRL_CONTSEQ_BIT );
	TBGEN_WRITE_REGISTER( puTddCtrlOffset, uRegValue );
}

static inline void prvConfigTddTxRxEn( vuint32 * puTddCtrl, TddDurationMode_t eRxTxEnManual )
{
	TBGEN_WRITE_REGISTER( puTddCtrl, (eRxTxEnManual << TDD_CTRL_TXRXEN_BIT) );
}

static void prvConfigTddMcuInterrupts( uint8_t ucTbgenNo, u8 ucInstance, RegSetReset_t eSetReset )
{
	vuint32 * puTbgenCtrl1 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL1_OFFSET );
	u32 uRegValue = 0;

	uRegValue = TBGEN_READ_REGISTER( puTbgenCtrl1 );
	uRegValue &= ~( ( u32 ) TBGEN_CTRL_TDD_INT_BIT_MASK << ( TBGEN_CTRL_TDD_INT_BIT_START + ucInstance ) );
	uRegValue |= ( u32 ) eSetReset << ( TBGEN_CTRL_TDD_INT_BIT_START + ucInstance );

	TBGEN_WRITE_REGISTER( puTbgenCtrl1, uRegValue );
}

static void prvConfigGpeMcuInterrupts( uint8_t ucTbgenNo, u8 ucInstance, RegSetReset_t eSetReset )
{
	vuint32 * puTbgenCtrl1 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL1_OFFSET );
	u32 uRegValue = 0;

	uRegValue = TBGEN_READ_REGISTER( puTbgenCtrl1 );
	uRegValue &= ~( ( u32 ) TBGEN_CTRL_GPE_INT_BIT_MASK << ( TBGEN_CTRL_GPE_INT_BIT_START + ucInstance ) );
	uRegValue |= ( u32 ) eSetReset << ( TBGEN_CTRL_GPE_INT_BIT_START + ucInstance );

	TBGEN_WRITE_REGISTER( puTbgenCtrl1, uRegValue );
}

static void prvConfigRFGFSMcuInterrupts( uint8_t ucTbgenNo, RegSetReset_t eSetReset )
{
	vuint32 * puTbgenCtrl1 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL1_OFFSET );
	u32 uRegValue = 0;

	uRegValue = TBGEN_READ_REGISTER( puTbgenCtrl1 );
	uRegValue &= ~( ( u32 ) TBGEN_CTRL_RFG_FS_INT_BIT_MASK << TBGEN_CTRL_RFG_FS_INT_BIT_START );
	uRegValue |= ( u32 ) eSetReset << TBGEN_CTRL_RFG_FS_INT_BIT_START;

	TBGEN_WRITE_REGISTER( puTbgenCtrl1, uRegValue );
	return;
}

void vConfigTsMcu( uint8_t ucTbgenNo, u8 ucInstance, RegSetReset_t eSetReset )
{
	vuint32 * puTbgenCtrl0 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL0_OFFSET );
	u32 uRegValue = 0;

	uRegValue = TBGEN_READ_REGISTER( puTbgenCtrl0 );
	uRegValue &= ~( ( u32 ) TBGEN_CTRL_TS_EN_BIT_MASK << ( TBGEN_CTRL_TS_EN_BIT_START + ( ucInstance * 2 ) ) );
	uRegValue |= ( u32 ) eSetReset << ( TBGEN_CTRL_TS_EN_BIT_START + ( ucInstance * 2 ) );
	TBGEN_WRITE_REGISTER( puTbgenCtrl0, uRegValue );
}

void vConfigTsMcuInterrupts( uint8_t ucTbgenNo, u8 ucInstance, RegSetReset_t eSetReset )
{
	vuint32 * puTbgenCtrl1 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL1_OFFSET );
	u32 uRegValue = 0;

	vConfigTsMcu(ucTbgenNo, ucInstance, eSetReset);

	uRegValue = TBGEN_READ_REGISTER( puTbgenCtrl1 );
	uRegValue &= ~( ( u32 ) TBGEN_CTRL_TS_INT_BIT_MASK << ( TBGEN_CTRL_TS_INT_BIT_START + ucInstance ) );
	uRegValue |= ( u32 ) eSetReset << ( TBGEN_CTRL_TS_INT_BIT_START + ucInstance );
	TBGEN_WRITE_REGISTER( puTbgenCtrl1, uRegValue );
}

static void prvAckTddTimerInstance( uint8_t ucTbgenNo, u8 ucTddTmrIndex )
{
	vuint32 * puTbgenIntstat = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_INTSTAT_OFFSET );
	vuint32 uRegValue = 0;

	uRegValue = ( u32 ) REG_RESET << ( TBGEN_CTRL_TDD_INT_BIT_START + ucTddTmrIndex );
	TBGEN_WRITE_REGISTER( puTbgenIntstat, uRegValue );
}

static void prvAckGpeTimerInstance( uint8_t ucTbgenNo, u8 ucTddTmrIndex )
{
	vuint32 * puTbgenIntstat = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_INTSTAT_OFFSET );
	vuint32 uRegValue = 0;

	uRegValue = ( u32 ) REG_RESET << ( TBGEN_CTRL_GPE_INT_BIT_START + ucTddTmrIndex );
	TBGEN_WRITE_REGISTER( puTbgenIntstat, uRegValue );
}

void vAckTsInstance( uint8_t ucTbgenNo, u8 ucTsIndex )
{
	vuint32 * puTbgenIntstat = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_INTSTAT_OFFSET );
	vuint32 uRegValue = 0;

	uRegValue = ( u32 ) REG_RESET << ( TBGEN_CTRL_TS_INT_BIT_START + ucTsIndex );
	TBGEN_WRITE_REGISTER( puTbgenIntstat, uRegValue );
}

static void  prvCallRFGIntCb( uint8_t ucTbgenNo )
{
	vuint32 uRegValue = 0;
	vuint32 * puTbgenIntstat = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_INTSTAT_OFFSET );

	uRegValue = TBGEN_READ_REGISTER( puTbgenIntstat );
	uRegValue &= TBGEN_CTRL_RFG_FS_INT_BIT_MASK << TBGEN_CTRL_RFG_FS_INT_BIT_START;

	if( uRegValue )
	{
		TBGEN_WRITE_REGISTER( puTbgenIntstat, uRegValue );
		if( xRFGIntCb[ ucTbgenNo - 1 ].pvCb != NULL )
		{
			xRFGIntCb[ ucTbgenNo - 1 ].pvCb( ucTbgenNo );
		}
	}
}

static void prvCallTddIntCb( uint8_t ucTbgenNo )
{
	vuint32 uRegValue = 0;
	vuint32 * puTbgenIntstat = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_INTSTAT_OFFSET );
	u8 ucIntSrcMask = 0x0;
	u8 ucIndex = 0;

	uRegValue = TBGEN_READ_REGISTER( puTbgenIntstat );
	uRegValue &= TDD_INT_STATUS_FLAG_MASK;
	uRegValue >>= TBGEN_CTRL_TDD_INT_BIT_START;

	ucIntSrcMask = ( u8 ) uRegValue;
	while ( ucIntSrcMask )
		{
			if ( ucIntSrcMask & ( 1 << ucIndex ) )
			{
				uRegValue = ( u32 ) REG_RESET << ( TBGEN_CTRL_TDD_INT_BIT_START + ucIndex );
				TBGEN_WRITE_REGISTER( puTbgenIntstat, uRegValue );
				if ( xTddIntCb[ ucTbgenNo - 1 ][ ucIndex ].pvCb != NULL )
				{
					xTddIntCb[ ucTbgenNo - 1 ][ ucIndex ].pvCb ( ucTbgenNo, ucIndex, xTddIntCb[ ucTbgenNo - 1 ][ ucIndex ].pvConfData );
				}
			}
			ucIntSrcMask = ucIntSrcMask & ( ( u8 ) ~( 1 << ucIndex ) ) ;
			ucIndex++;
		}
}

static void prvCallGpeIntCb( uint8_t ucTbgenNo )
{
	vuint32 uRegValue = 0;
	vuint32 * puTbgenIntstat = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_INTSTAT_OFFSET );
	u8 ucIntSrcMask = 0x0;
	u8 ucIndex = 0;

	uRegValue = TBGEN_READ_REGISTER( puTbgenIntstat );
	uRegValue &= GPE_INT_STATUS_FLAG_MASK;
	uRegValue >>= TBGEN_CTRL_GPE_INT_BIT_START;

	ucIntSrcMask = ( u8 ) uRegValue;
	while ( ucIntSrcMask )
		{
			if ( ucIntSrcMask & ( 1 << ucIndex ) )
			{
				uRegValue = ( u32 ) REG_RESET << ( TBGEN_CTRL_GPE_INT_BIT_START + ucIndex );
				TBGEN_WRITE_REGISTER( puTbgenIntstat, uRegValue );
				if ( xGpeIntCb[ ucTbgenNo - 1 ][ ucIndex ].pvCb != NULL )
				{
					xGpeIntCb[ ucTbgenNo - 1 ][ ucIndex ].pvCb ( ucTbgenNo, ucIndex, xGpeIntCb[ ucTbgenNo - 1 ][ ucIndex ].pvConfData );
				}
			}
			ucIntSrcMask = ucIntSrcMask & ( ( u8 ) ~( 1 << ucIndex ) ) ;
			ucIndex++;
		}
}

static bool_t prvTbgenIsrHandler( uint32_t ulirq, void * pvDevHandle )
{
	u8 ucTbgenNo = TBGEN_1;

	(void) pvDevHandle;

	if( ulirq == (MPIC_INTERNAL_TBGEN_IRQ(TBGEN_2) + INTERNAL_IRQ_OFFSET) )
	{
		ucTbgenNo = TBGEN_2;
	}
	prvCallTddIntCb( ucTbgenNo );
	prvCallRFGIntCb( ucTbgenNo );
	prvCallGpeIntCb( ucTbgenNo );

	return true;
}

static void prvRegisterCallbackTddInt( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddTimerParams_t *pxTddParams )
{
	if ( pxTddParams->pvCb != NULL )
	{
		xTddIntCb[ ucTbgenNo - 1 ][ eInstance ].pvCb = pxTddParams->pvCb;
		xTddIntCb[ ucTbgenNo - 1 ][ eInstance ].pvConfData = pxTddParams;
		bMpicEnable( DEVICE_INTERNAL, MPIC_INTERNAL_TBGEN_IRQ(ucTbgenNo) );
	}
}

static void prvUnRegisterCallbackTddInt ( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	xTddIntCb[ ucTbgenNo - 1 ][ eInstance ].pvCb = NULL;
	xTddIntCb[ ucTbgenNo - 1 ][ eInstance ].pvConfData = NULL;
}

static void prvRegisterCallbackGpeInt( uint8_t ucTbgenNo, TimerInstance_t eInstance, TimerParams_t *pxParams )
{

	if ( pxParams->pvCb != NULL )
	{
		xGpeIntCb[ ucTbgenNo - 1 ][ eInstance ].pvCb = pxParams->pvCb;
		xGpeIntCb[ ucTbgenNo - 1 ][ eInstance ].pvConfData = pxParams;
		bMpicEnable( DEVICE_INTERNAL, MPIC_INTERNAL_TBGEN_IRQ(ucTbgenNo) );
	}
}

static void prvUnRegisterCallbackGpeInt( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	xGpeIntCb[ ucTbgenNo - 1 ][ eInstance ].pvCb = NULL;
	xGpeIntCb[ ucTbgenNo - 1 ][ eInstance ].pvConfData = NULL;
}

static uint32_t prvExtIrqLine( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	uint32_t uIrqline = 0;

	switch( ucTbgenNo )
	{
		case TBGEN_1:
			uIrqline = MPIC_EXTERNAL_TBGEN1_IRQ_STROBE( eInstance );
			break;
		case TBGEN_2:
			uIrqline = MPIC_EXTERNAL_TBGEN2_IRQ_STROBE( eInstance );
			break;
	}

	return uIrqline;
}

static bool_t prvRxAlignmentIntIsrHandler( uint32_t ulirq, void * data )
{
	uint8_t ucTbgenNo = TBGEN_1;
	uint8_t ucInstance = 3;
	( void ) data;

	if( ulirq >= 8 )
	{
		ucTbgenNo = TBGEN_2;
		ucInstance = ulirq - 8;
	}
	else if( ulirq != 4)
	{
		ucInstance = ulirq - 5;
	}
	if ( xRxAlignIntCb[ ucTbgenNo - 1 ][ ucInstance ].pvCb != NULL )
	{
		xRxAlignIntCb[ ucTbgenNo - 1 ][ ucInstance ].pvCb( ucTbgenNo, ucInstance,xRxAlignIntCb[ ucTbgenNo - 1 ][ ucInstance ].pvConfData );
	}

	return true;
}

static int prvRegisterCallbackRxAlignmentInt( uint8_t ucTbgenNo, TimerInstance_t eInstance, TimerParams_t * pxParams )
{
	uint32_t uIrqline = 0;
	int32_t iRetIrqVal = 0;

	uIrqline = prvExtIrqLine( ucTbgenNo, eInstance );
	if ( pxParams->pvCb != NULL )
	{
		xRxAlignIntCb[ ucTbgenNo - 1 ][ eInstance ].pvCb  = pxParams->pvCb;
		xRxAlignIntCb[ ucTbgenNo - 1 ][ eInstance ].pvConfData = pxParams;
	}
	/* Register Xternal IRQs with FreeRTOS */
	iRetIrqVal = Request_Irq( uIrqline, prvRxAlignmentIntIsrHandler, ( void * ) pxParams );
	bMpicEnable( DEVICE_EXTERNAL, uIrqline );

	return iRetIrqVal;
}

static void prvUnRegisterCallbackRxAlignmentInt( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	uint32_t uIrqline = 0;

	uIrqline = prvExtIrqLine( ucTbgenNo, eInstance );

	/* First UnRegister Xternal IRQs with FreeRTOS */
	bMpicDisable( DEVICE_EXTERNAL, uIrqline );
	Free_Irq( uIrqline );
	xRxAlignIntCb[ ucTbgenNo - 1 ][ eInstance ].pvCb  = NULL;
	xRxAlignIntCb[ ucTbgenNo - 1 ][ eInstance ].pvConfData = NULL;
}

int iTbgenDevOpen( uint8_t ucTbgenNo )
{
	int iRet = 0;
	vuint32 * puRfgcr = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_RFGCR_OFFSET );

	TBGEN_WRITE_REGISTER( puRfgcr, (TBGEN_READ_REGISTER( puRfgcr ) | (1 << RFG_REF_SYNC_BIT)) );
	iRet = Request_Irq( MPIC_INTERNAL_TBGEN_IRQ(ucTbgenNo) + INTERNAL_IRQ_OFFSET, prvTbgenIsrHandler, ( void * ) TBGEN_BASE(ucTbgenNo) );

	return iRet;
}

void vTbgenDevClose( uint8_t ucTbgenNo )
{
	bMpicDisable( DEVICE_INTERNAL, MPIC_INTERNAL_TBGEN_IRQ(ucTbgenNo) );
	Free_Irq( MPIC_INTERNAL_TBGEN_IRQ(ucTbgenNo) + INTERNAL_IRQ_OFFSET );
}

u64 ullTbgenGetMasterCounter( uint8_t ucTbgenNo )
{
	vuint32 * puRegAddrHi = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_MSTRCNTHI_OFFSET );
	vuint32 * puRegAddrLo = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_MSTRCNTLO_OFFSET );
	u64 ullTimestamp = 0;
	u32 tmpTimeStampHi = 0, tmpTimeStampLo = 0;
	u32 tmpTimeStampHi1 = 0, tmpTimeStampLo1 = 0;

	tmpTimeStampHi = TBGEN_READ_REGISTER( puRegAddrHi );
	tmpTimeStampLo = TBGEN_READ_REGISTER( puRegAddrLo );
	tmpTimeStampHi1 = TBGEN_READ_REGISTER( puRegAddrHi );

	if( tmpTimeStampHi1 != tmpTimeStampHi )
	{
		tmpTimeStampLo1 = TBGEN_READ_REGISTER( puRegAddrLo );
		ullTimestamp = ((u64)tmpTimeStampHi1 << 32) | tmpTimeStampLo1;
	}
	else
	{
		ullTimestamp = ((u64)tmpTimeStampHi << 32) | tmpTimeStampLo;
	}

	return ullTimestamp;
}

u64 ullTbgenGet10MSCounter( uint8_t ucTbgenNo )
{
	vuint32 * puRegAddrHi = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_TS10MSHI_OFFSET );
	vuint32 * puRegAddrLo = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_TS10MSLO_OFFSET );
	u64 ullTimestamp = 0;

	ullTimestamp = ((u64)TBGEN_READ_REGISTER( puRegAddrHi ) << 32) | TBGEN_READ_REGISTER( puRegAddrLo );

	return ullTimestamp;
}

u64 ullTbgenGetMasterCounterRaw( uint8_t ucTbgenNo )
{
	uint32_t * puRegAddrHi = ( uint32_t * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_MSTRCNTHI_OFFSET );
	uint32_t * puRegAddrLo = ( uint32_t * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_MSTRCNTLO_OFFSET );
	u64 ullTimestamp = 0;

	ullTimestamp = ((u64)__lwbrx(puRegAddrHi) << 32) | __lwbrx(puRegAddrLo);

	return ullTimestamp;
}

int iTbgenTimerInterruptEn( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	switch ( eTimerType )
	{
		case TDD:
			prvConfigTddMcuInterrupts( ucTbgenNo, eInstance, REG_RESET );
			break;
		case GPE:
			prvConfigGpeMcuInterrupts( ucTbgenNo, eInstance, REG_RESET );
			break;
		default:
			break;
	}

	return 0;
}

int iTbgenTimerInterruptDis( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	switch ( eTimerType )
	{
		case TDD:
			prvConfigTddMcuInterrupts( ucTbgenNo, eInstance, REG_SET );
			break;
		case GPE:
			prvConfigGpeMcuInterrupts( ucTbgenNo, eInstance, REG_SET );
			break;
		default:
			break;
	}

	return 0;
}

int iTbgenTimerInterruptClr( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	switch ( eTimerType )
	{
		case TDD:
			prvAckTddTimerInstance( ucTbgenNo, eInstance );
			break;
		case GPE:
			prvAckGpeTimerInstance( ucTbgenNo, eInstance );
			break;
		default:
			break;
	}

	return 0;
}

int iTbgenPerformSwRst( uint8_t ucTbgenNo )
{
	vuint32 * puiCntrl0;
	int ret;

	ret = prvCheckTddNo( ucTbgenNo );
	if( ret )
	{
		return ret;
	}
	puiCntrl0 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL0_OFFSET );
	/* Caution : This will reset all the Timers for passed TBGEN handle */
	TBGEN_WRITE_REGISTER( puiCntrl0, ( TBGEN_READ_REGISTER(puiCntrl0) | (REG_RESET << TBGEN_CTRL_SWRST_BIT_START)) );

	return 0;
}

int iTbgenProgramTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddTimerParams_t * pxTddTimerParams )
{
	vuint32 * puTimerCtrl;
	TriggerMode_t eTm = TM_REPETITIVE;
	int ret;

	ret = prvCheckTddTimerConf( ucTbgenNo, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * eInstance) );

	//TO DO - Check Whether We need to disable Timer before Programming the timer
	prvDisableTimerInstance( puTimerCtrl );
	prvConfigTddDurMode( (vuint32 *)((u8 *)puTimerCtrl + TDD_TIMER_MODE_OFFSET), (vuint32 *)((u8 *)puTimerCtrl + TDD_TIMER_DURATION_OFFSET),  &pxTddTimerParams->xDuration[0], pxTddTimerParams->ucTddSeqSteps );
	/* Set Offset Value */
	prvConfigOffset( (vuint32 *)((u8 *)puTimerCtrl + TDD_TIMER_HI_OFFSET), (vuint32 *)((u8 *)puTimerCtrl + TDD_TIMER_LO_OFFSET), pxTddTimerParams->uOffset );
	if( pxTddTimerParams->pvCb != NULL )
	{
		prvRegisterCallbackTddInt( ucTbgenNo, eInstance, pxTddTimerParams );
		prvConfigTddMcuInterrupts( ucTbgenNo, eInstance, REG_RESET );
	}
	if( pxTddTimerParams->eTrigMode == TM_REPETITIVE )
	{
		eTm = TM_ONE_SHOT;
	}
	prvProgramTddCtrlReg( puTimerCtrl, pxTddTimerParams->ePm, pxTddTimerParams->uPw, eTm, pxTddTimerParams->ucTddSeqSteps );

	return 0;
}

int iTbgenEnableTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTddTimerConf( ucTbgenNo, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * eInstance) );

	prvEnableTimerInstance( puTimerCtrl );

	return 0;
}

int iTbgenDisableTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTddTimerConf( ucTbgenNo, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * eInstance) );

	prvDisableTimerInstance( puTimerCtrl );
	prvConfigTddMcuInterrupts( ucTbgenNo, eInstance, REG_SET );
	prvUnRegisterCallbackTddInt( ucTbgenNo, eInstance );

	return 0;
}

int iTbgenProgramTddTimerTxRxManual( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddDurationMode_t eRxTxEnManual )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTddTimerConf( ucTbgenNo, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * eInstance) );
	prvConfigTddTxRxEn( puTimerCtrl, eRxTxEnManual );

	return 0;
}

int iTbgenReloadTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance, u64 uOffset )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTddTimerConf( ucTbgenNo, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[TDD] + (TDD_TIMER_BLOCK_SIZE * eInstance) );
	prvConfigOffset( (vuint32 *)((u8 *)puTimerCtrl + TDD_TIMER_HI_OFFSET), (vuint32 *)((u8 *)puTimerCtrl + TDD_TIMER_LO_OFFSET), uOffset );

	return 0;
}

int iTbgenConfigTimerIntrvl( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, u32 uInterval )
{
	vuint32 * puIntrvl;
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	puIntrvl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[eTimerType] + TIMER_INTRVL_OFFSET + (TIMER_BLOCK_SIZE * eInstance) );
	prvConfigIntrvl( puIntrvl, uInterval );

	return 0;
}

int iTbgenProgramTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, TimerParams_t * pxTimerParams )
{
	vuint32 * puIntrvl;
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[eTimerType] + (TIMER_BLOCK_SIZE * eInstance) );
	puIntrvl = ( vuint32 * ) ( (u8 *)puTimerCtrl + TIMER_INTRVL_OFFSET );
	//TO DO - Check Whether We need to disable Timer before Programming the timer
	prvDisableTimerInstance( puTimerCtrl );
	prvConfigOffset( (vuint32 *) ((u8 *)puTimerCtrl + TIMER_HI_OFFSET), (vuint32 *)((u8 *)puTimerCtrl + TIMER_LO_OFFSET), pxTimerParams->uOffset );
	switch( eTimerType )
	{
		case AXRF:
			/* Configure AXRF Timer Polarity and return */
			prvProgramAXRFCtrlReg( puTimerCtrl, pxTimerParams->ePolarity );
			return 0;
		case TIMED_INT:
			/* Configure Interval and Triggure Mode of Timer and Return */
			if ( pxTimerParams->eTrigMode == TM_REPETITIVE )
			{
				prvConfigIntrvl( puIntrvl, pxTimerParams->uInterval );
			}
			prvProgramTimedIntCtrlReg( puTimerCtrl, pxTimerParams->eTrigMode );
			return 0;
		case RX_ALIGNMENT:
			if( pxTimerParams->pvCb != NULL )
			{
				prvRegisterCallbackRxAlignmentInt( ucTbgenNo, eInstance, pxTimerParams );
			}
			break;
		case GPE:
			if( pxTimerParams->pvCb != NULL )
			{
				prvRegisterCallbackGpeInt( ucTbgenNo, eInstance, pxTimerParams );
				prvConfigGpeMcuInterrupts( ucTbgenNo, eInstance, REG_RESET );
			}
			break;
		default:
			break;
	}
	/* Except AXRF and TIMED_INT Timers, below functions will be called */
	if ( pxTimerParams->eTrigMode == TM_REPETITIVE )
	{
		prvConfigIntrvl( puIntrvl, pxTimerParams->uInterval );
	}
	prvProgramCtrlReg( puTimerCtrl, pxTimerParams->eSm, pxTimerParams->ePw, pxTimerParams->eTrigMode, pxTimerParams->ePolarity );

	return 0;
}

int iTbgenEnableTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[eTimerType] + (TIMER_BLOCK_SIZE * eInstance) );
	prvEnableTimerInstance( puTimerCtrl );

	return 0;
}

int iTbgenDisableTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[eTimerType] + (TIMER_BLOCK_SIZE * eInstance) );
	prvDisableTimerInstance( puTimerCtrl );
	switch( eTimerType )
	{
		case RX_ALIGNMENT:
			prvUnRegisterCallbackRxAlignmentInt( ucTbgenNo, eInstance );
			break;
		case GPE:
			prvUnRegisterCallbackGpeInt( ucTbgenNo, eInstance );
			prvConfigGpeMcuInterrupts( ucTbgenNo, eInstance, REG_SET );
			break;
		default:
			break;
	}

	return 0;
}

int iTbgenReloadTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, u64 uOffset )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[eTimerType] + (TIMER_BLOCK_SIZE * eInstance) );
	prvConfigOffset( (vuint32 *)((u8 *)puTimerCtrl + TIMER_HI_OFFSET), (vuint32 *)((u8 *)puTimerCtrl + TIMER_LO_OFFSET), uOffset );

	return 0;
}

int iTbgenReloadTimerAndPolarity( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, StrobePolarity_t ePolarity, u64 uOffset )
{
	vuint32 * puTimerCtrl;
	int ret;

	ret = prvCheckTimerConf( ucTbgenNo, eTimerType, eInstance );
	if( ret )
	{
		return ret;
	}
	puTimerCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + uTimerCtrlOffset[eTimerType] + (TIMER_BLOCK_SIZE * eInstance) );
	TBGEN_WRITE_REGISTER( puTimerCtrl, (TBGEN_READ_REGISTER( puTimerCtrl ) & CTRL_POLARITY_BIT_CLR) | (ePolarity << CTRL_POLARITY_BIT) );
	prvConfigOffset( (vuint32 *)((u8 *)puTimerCtrl + TIMER_HI_OFFSET), (vuint32 *)((u8 *)puTimerCtrl + TIMER_LO_OFFSET), uOffset );

	return 0;
}

static void prvSetRFGRefClkPer10MS( uint8_t ucTbgenNo )
{
	vuint32 * puRFGRefClkPer10MS = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_REFCLK_PER_10MS_OFFSET );
    vuint32 uRegValue = 0;
	double fTbgenFreq = TBGEN2_REF_CLK;

	if( ucTbgenNo == TBGEN_1 )
	{
		fTbgenFreq = TBGEN1_REF_CLK;
	}
    uRegValue = ( vuint32 ) (  fTbgenFreq * 10000 );
    TBGEN_WRITE_REGISTER( puRFGRefClkPer10MS, uRegValue );
}

static void prvConfigFrameSycnOutput( uint8_t ucTbgenNo, FrameSyncSelect_t eFrameSyncSel )
{
	vuint32 * puTbgenCtrl0 = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_CNTRL0_OFFSET );
    vuint32 uRegValue = 0;

    uRegValue = TBGEN_READ_REGISTER( puTbgenCtrl0 );
    uRegValue &= ~( ( u32 ) RFG_FRAME_SYNC_SOURCE_MASK <<
               RFG_FRAME_SYNC_SOURCE_BIT );
    uRegValue |= ( u32 ) eFrameSyncSel << RFG_FRAME_SYNC_SOURCE_BIT;
    TBGEN_WRITE_REGISTER( puTbgenCtrl0, uRegValue );
}

static void prvConfigAndEnableRFG( uint8_t ucTbgenNo, RFGParams_t * pxRFGParams )
{
	vuint32 * puRFGCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_RFGCR_OFFSET );
    vuint32 uRegValue = 0;
    u32 uMask = 0;

	prvSetRFGRefClkPer10MS( ucTbgenNo );
    prvConfigRFGFSMcuInterrupts( ucTbgenNo, REG_RESET );
	if( pxRFGParams->pvCb != NULL )
	{
		xRFGIntCb[ ucTbgenNo - 1 ].pvCb = pxRFGParams->pvCb;
		bMpicEnable( DEVICE_INTERNAL, MPIC_INTERNAL_TBGEN_IRQ(ucTbgenNo) );
	}
    uRegValue = TBGEN_READ_REGISTER( puRFGCtrl );

    switch( pxRFGParams->eRefSyncSel )
    {
        case RFG_SYSREF_IN:
        case RFG_CPRI_RX:
            uMask = ~( ( u32 ) RFG_REF_SYNC_MASK << RFG_REF_SYNC_BIT );
            uRegValue &= uMask;
            uMask = ( u32 ) pxRFGParams->eRefSyncSel << RFG_REF_SYNC_BIT;
            break;

        case RFG_INTERNAL:
            uMask = ~( ( u32 ) RFG_START_INT_RAD_FRM_MASK <<
                       RFG_START_INT_RAD_FRM_BIT );
            uRegValue &= uMask;
            uMask = ( u32 ) REG_RESET << RFG_START_INT_RAD_FRM_BIT;
            break;
    }

    uRegValue |= uMask;
    uMask = ~( ( u32 ) RFG_SYNCOUT_CTRL_MASK << RFG_SYNCOUT_CTRL_BIT );
    uRegValue &= uMask;

    switch( pxRFGParams->eSyncOut )
    {
        case RFG_SYSREF_IN_SYNC_OUT:
        case RFG_CPRI_RX_SYNC_OUT:
            uMask = ( u32 ) RFG_SYSREF_IN_SYNC_OUT << RFG_SYNCOUT_CTRL_BIT;
            break;

        case RFG_GENERATED_SYNC_OUT:
            uMask = ( u32 ) RFG_GENERATED_SYNC_OUT << RFG_SYNCOUT_CTRL_BIT;
            break;
    }

    uRegValue |= uMask;
    uRegValue |= ( RFG_FIX_ERR_MASK << RFG_FIX_ERR_BIT ) | ( REG_RESET << RFG_INIT_BIT );
    TBGEN_WRITE_REGISTER( puRFGCtrl, uRegValue );
}

int iDisableRFG( uint8_t ucTbgenNo )
{
	int ret;
	vuint32 * puRFGCtrl;

	ret = prvCheckTddNo( ucTbgenNo );
	if( ret )
	{
		return ret;
	}
	puRFGCtrl = ( vuint32 * ) ( TBGEN_BASE( ucTbgenNo ) + TBGEN_RFGCR_OFFSET );
    TBGEN_WRITE_REGISTER( puRFGCtrl, REG_SET << RFG_INIT_BIT );
    prvConfigRFGFSMcuInterrupts( ucTbgenNo, REG_SET );

	return ret;
}

int iInitRFG( uint8_t ucTbgenNo, RFGParams_t * pxRFGParams )
{
	int ret;

	ret = prvCheckTddNo( ucTbgenNo );
	if( ret )
	{
		return ret;
	}
    prvConfigFrameSycnOutput( ucTbgenNo, pxRFGParams->eFrameSyncSel );
    prvConfigAndEnableRFG( ucTbgenNo, pxRFGParams );

	return ret;
}

void vEnableTbgen( uint8_t ucTbgenNo )
{
	vuint32 uiRegValue = 0;
	struct scfg_regs *regs;

	regs = (struct scfg_regs *) (SCFG_BASE_ADDR);
	uiRegValue = in_le32(&regs->conf_ctrl1);
	if( TBGEN_1 == ucTbgenNo ) {
		uiRegValue |= ( TBGEN1_CD_EN );
	} else {
		uiRegValue |= ( TBGEN2_CD_EN );
	}
	out_le32(&regs->conf_ctrl1, uiRegValue);

	return;
}

void vDisableTbgen( uint8_t ucTbgenNo )
{
	vuint32 uiRegValue = 0;
	struct scfg_regs *regs;

	regs = (struct scfg_regs *) (SCFG_BASE_ADDR);
	uiRegValue = in_le32(&regs->conf_ctrl1);
	if( TBGEN_1 == ucTbgenNo ) {
		uiRegValue &= ( ~TBGEN1_CD_EN );
	} else {
		uiRegValue &= ( ~TBGEN2_CD_EN );
	}
	out_le32(&regs->conf_ctrl1, uiRegValue);

	return;
}
