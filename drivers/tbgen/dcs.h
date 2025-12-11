// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef _DCS_H_
#define _DCS_H_

#include <types.h>
#include <dcs_regs.h>
#include <dcs_hs_regs.h>

/******************************************************************************

 					DCS Subsystem
					=============

DCS Subsystem is comprised of 2 subsystems (supplied by IQ Analog) :
==================================================================
1) High Speed Data Converter Subsystem (HS_DCS)			x 1 instance
	- High Speed macro includes --
		* integrated ADCs/DACs 
		* Clock Distribution
		* Configuration Interface (APB) 
		* DFT implementation.
	- Includes one(1) UART interface, which is used for debug of embedded
	RISC-V core.
	- It consists of 4 ADCs & 4 DACs, which can support upto 2 channels (2T2R) 
	wireless transceivers. Each channel supports I & Q
	- DACs & ADCs support Sampling clock upto 3.932 GHz
	- ADCs & DACs support 10bits resolution.

2) Low Speed Data Converter Subsystem (LS_DCS)			x 2 instance
	- Low Speed macro includes -- 
		* integrated ADCs/DACs 
		* Clock Distribution
		* Configuration Interface (APB)
		* DFT implementation
	-  consists of total 8 ADCs & 8 DACs (4 ADCs & 4 DACs per Macro), which can 
	support upto 4 channels (4T 4R) wireless transceivers. 
	- Each channel supports I & Q.
	- DACs supports Sampling clock upto 491.52 MHz 
	- ADCs supports Sampling clock upto 245.76 MHz (Non Interleaving mode)
		& 491.52 MHz (Interleaving mode)


All the IQ Analog Macros (Both High Speed Macro & Low Speed Macros) support APB 
interface for access to configuration/status registers.



AXIQ_L Clocks enabling & disabling scheme (txclk_dn & rxclk_dn)
===============================================================

==================== ENABLING CLOCKS =========================
There are two modes of synchronization to enable these AXIQ_L clocks.

Mode0: Clocks to both AXIQ_L0 & AXIQ_L1 enabled (released) in the same clock cycle.
=====

Following is the sequence of SW programming for enabling the TX clocks (DACs).
	1) Enable the Power down signals for TX_CLK for both the Macros
		(TX_LS_DCS0_CLK_DIS & TX_LS_DCS1_CLK_DIS bits of CONFIG_CONTROL6 register)
	2) Set the SYNC register bit
		(TX_CLK_SYNC_EN bit of CONFIG_CONTROL5 register)
To enable the RX (ADCs) clocks, the similar SW programming sequence should be followed.




Mode1: Clocks to single instance (i.e.AXIQ_L0 or AXIQ_L1) released in the same clock cycle
=====
tx_clk_d1/d2 of one LS_DCS macro is released in the same cycle, but there is
separate control for second LS_DCS Macro. Same applies for rx_clk1_d1/d2

Following is the sequence of SW programming for enabling the TX clocks.
	1) Set the global TX ENABLE register bit
		(TX_CLK_SYNC_EN bit of CONFIG_CONTROL5 register)
	2) Then enable the Power down signals for TX_CLK of desired DCS Macros
		(TX_LS_DCS0_CLK_DIS / TX_LS_DCS1_CLK_DIS bits of CONFIG_CONTROL6 register).

To enable the RX clocks, the similar SW programming sequence should be
followed.


==================== DISABLING CLOCKS =========================

To disable the AXIQ_L clocks, following SW sequence should be followed. This is
common for both the modes.

Following is the sequence for disabling TX clocks.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
DISABLE THE TX CLOCK FOR INDIVIDUAL LS_DCS Macro:
	--> Disable Power Down signal of desired LS_DCS Macro
		(TX_LS_DCS0_CLK_DIS & TX_LS_DCS1_CLK_DIS bits of CONFIG_CONTROL6 register)

DISABLE THE TX CLOCK FOR Both LS_DCS Macros:
	--> Reset the global TX ENABLE register bit (TX_CLK_SYNC_EN bit of CONFIG_CONTROL5 register)


There is a similar sequence for disabling RX clocks.



PCLK clocks to LS_DCS0, LS_DCS1, HS_DCS
=======================================
PCLK is the APB interface clock to these macros, which is generated from ipg_clk
(Platform Clock).
PCLK to each Macro (LS_DCS0, LS_DCS1, HS_DCS) is clock gated out of Reset.

There are 3 SCFG bits provisioned (One per Macro) to enable the PCLK to each
Macro.

During PCLK gating, no APB transaction to the macros from e200 cores OR other
Geul masters should hang. They should be gracefully terminated.

Data Converter Subsystem (DCS) Low Power modes - NOT Needed as of now.
==============================================
* Both Low Speed DCS Macros & High Speed DCS Macro support Low Power modes of ADCs & DACs.

* Low Power modes control various levels of Power saving features:
	- LDO enable/disable
	- Analog Core enable/disable
	- Digital logic enable/disable.

* Low Power modes are supported by both the macros for ADC & DACs separately per TX & RX channel basis.
* Low Power modes are supported by IP configuration register bits (*_low_pwr_cfg).
* Additionally there are Input pins (txlp/rxlp) to the DCS macros which control entry & exit of Low Power modes.

* Most of PRL listed Geul Use Cases are TDD systems (Time Division Duplexing)
	- mmWave 5G-NR
	- Sub 6 GHz 5G-NR
	- 802.11AD.

* In TDD systems, Transmit & Receive operations are mutually exclusive on Air Interface. \
* In such TDD systems, there may be a need for ADCs & DACs to dynamically entry & exit from low power
modes in each/few TX TTI (Time Slot) or RX TTI (Time Slot) to save Power.

*** Geul doesn’t support such dynamic Low Power entry/exit of ADCs & DACs ***

* However ADCs & DACs can be semi statically put in low power states by appropriately programming the LS_DCS/HS_DCS IP & with TBGEN1/2 generated triggers.
* Similarly ADCs & DACs can semi statically exit from Low Power states.

* It is recommended that System Software runs TX path delay calibration procedure after each exit from Low Power states.

For a typical application following are the sequence of events to enter & exit ADCs/DCAs low power modes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* IP is configured for a given low power mode using P configuration register bits (*_low_pwr_cfg) once.
* Then low power mode entry & exit are controlled by txlp/rxlp signals, which are driven from TBGEN timers

NOTE:(For Post Silicon Validation)
====
Though dynamic Low Power Entry/Exit is de-featured for HS DCS Macro due to unknown IP issues through CCB#18, it is suggested
that dynamic low Power feature of HS_DCS be validated on Silicon.
If it is found to be working without Phase alignment calibration, it can be featured again for Geul.


Reset input (rst_n) of Data Converter Subsystems (DCS)
======================================================
* There could be a possibility of high frequency pulses on dcs_clk_ls & dcs_clk_hs, to propagate to digital logic in DCS Macros (LS_DCS1, LS_DCS2, HS_DCS, AXIQ_L/AXIQ_H,
TBGEN1/TBGEN2) till DCS_PLL is locked.
* To avoid such cases, hard reset input of all three DCS macros should be stretched (Kept asserted) till DCS_PLL is locked. DCS CLKGEN IP provides such signal
called “reset_done”.
* SCFG bits are provisioned to provide Software controllable Reset on “rst_n” pins. There is one SCFG bit to assert/de-assert “rst_n” pin of HS_DCS and
one SCFG bit to assert/de-assert “rst_n” pins of LS_DCS1 & LS_DCS2 together.
* These SCFG bits don’t assert “rst_n” input of DCS Macros out of Reset.


Reset input (PRESETn) of Data Converter Subsystems (DCS)
========================================================
* Software controllable Reset assertion & de-assertion on PRESETn pin of each DCS
macro (LS_DCS0, LS_DCS1, HS_DCS) is provided. There are 3 SCFG bits
provisioned, one per DCS macro.
* When SCFG bit is set to 1’b1, PRESETn of the DCS macro is asserted.
* When SCFG bit is 1’b0, other controls of PRESETn takes over. If SCFG bit is 1’b0
& other controls of PRESETn are de-asserted, PRESETn to the macro will be
de-asserted.


LS_DCS TX PATH Delay calibration
================================
* It is not ascertained by design that DAC path delays would be a fixed delay across
TX Ch 0 to TX Ch 3 after Reset signal de-assertion (rst_n) OR Digital
synchronization of ADC/DAC dividers (using SYNCA/SYNCB OR DIV_RST) ---TKT0521918

* This can result in phase difference between transmitted TX waveforms through
different Antennas/channels at the Air interface.

* It is recommended that software runs TX path delay calibration procedure as
outlined in TKT0521918 after Power On Reset OR Low Power Mode Exit OR
Digital Synchronization using IP’s SYNCA/SYNCB OR DIV_RST mechanism.



Geul Data Conversion Subsystem Use Cases
========================================
* Geul supports 5G protocol for both Small Cell & CPE Use Cases for both sub 6GHz
bands & mmWave bands.

* Additionally it supports LTE & 802.11ad protocols.

* RF Transceiver interface for 5G-mmWave & 802.11ad protocols are typically supported
using HS_DCS subsystem, TBGEN2, LLCP1, LLCP2 & SPI interfaces.

* While RF Transceiver interface for 5G sub 6 GHz, LTE is supported using LS_DCS
subsystem, TBGEN1 & SPI Interfaces.



Low Power Bits Programming
==========================
* The register LOW_PWR_CONFIG defines bits which when set, will disable the corresponding
core/block when the appropriate rxlp and/or txlp inputs are asserted. Bits are defined for
disabling each converter pair ADC, ADC BG Cal(Background Calibration), DAC,
DAC DEM (Dynamic Element Matching logic) Analog and Digital Cores.

* Recovery back to normal operation after low power mode depends on the cores/blocks
	that were disabled. ADC recalibration may be needed in certain conditions

******************************************************************************/





/******************************************************************************
*
* 					Conditional Compilation
*
******************************************************************************/
#define DCS_ENABLE_DEBUG_INFO				1	/* Enable for debug messages */
#define DCS_ENABLE_REG_TRX_INFO				1	/* Enables register TRX messages */
#define DCS_ENABLE_EXCEPTION_TRAP			1	/* Enable for debug messages */



#define DCS_SUPPORT_API						1
#ifdef DCS_SUPPORT_API
	#define DCS_SUPPORT_API_INTERNAL
#endif


#define DCS_API_FUNCTIONS					1
#ifdef DCS_API_FUNCTIONS
	#define DCS_API_FUNCTIONS_INTERNAL
#endif

#define WAIT_LIMIT_MAX 0x10000

/******************************************************************************
*
* 					Defines
*
******************************************************************************/
#define SET		1
#define RESET	0

#define HS_DCS_MAX_INSTANCE		1		/* DCS_HS has ONE instances */
#define LS_DCS_MAX_INSTANCE		2		/* DCS_LS has TWO instances */

#define IP_ID_MAGIC_WORD_LS		0x1C04092D	/* Default Value of IP ID reg */
#define IP_ID_MAGIC_WORD_HS		0x1C020929	/* Default Value of IP ID reg for HS */

/* Macro to Clear specific bits of register and write 'value' to it */
#define DCS_SET_REG_BITFIELD( addr, start_bit, mask, value )	\
				out_le32( addr, ( in_le32( addr ) & \
					( ~ ( ( u32 ) mask << start_bit ) ) ) | \
					( ( u32 ) value << start_bit ) )

/* Macro to Read specific bits of register */
#define DCS_GET_REG_BITFIELD( addr, start_bit, mask )	\
				( ( in_le32( addr ) & \
					( ( u32 ) mask << start_bit ) ) >> start_bit )

/* Macro to Clear specific bits of register as per mask provided */
#define DCS_RESET_REG_BITFIELD( addr, start_bit, mask )	\
	out_le32( addr, in_le32( addr ) & \
					( ~ ( ( u32 ) mask << start_bit ) ) )


/* Read from Register */
#define READ_REGISTER( ADDR )		DCS_GET_REG_BITFIELD( ADDR, 0, 0xFFFFFFFF )

/* Write to Register */
#define WRITE_REGISTER( ADDR, VALUE )	DCS_SET_REG_BITFIELD( ADDR, 0, 0xFFFFFFFF, VALUE )

/* Write Mask to Register */
#define SET_REG_MASK( ADDR, MASK_VALUE )	DCS_SET_REG_BITFIELD( ADDR, 0, MASK_VALUE, MASK_VALUE )

/* Clear Register bits as per Mask */
#define RESET_REG_MASK( ADDR, MASK_VALUE )	DCS_RESET_REG_BITFIELD( ADDR, 0, MASK_VALUE )


#define DELAY_2MS					2
#define DELAY_4MS					4
#define DELAY_FW_LOAD				DELAY_2MS
#define DELAY_ENABLE_4G_DAC_BLOCK	DELAY_2MS



/******************************************************************************
*
* 					Error Codes
*
******************************************************************************/
#define SUCCESS					 0
#define FAILURE					-1
#define ERR_INVALID_HANDLE		-2

/******************************************************************************
*
* 					Enum Types
*
******************************************************************************/
/* ADC / DAC Channel Instance (bit wise operations) */
typedef enum DAC_CH_Instance {
	DAC_CH_0 = 1,
	DAC_CH_1 = 2,
	DAC_CH_2 = 4,
	DAC_CH_3 = 8
} DAC_CH_Instance_t;

typedef enum ADC_CH_Instance {
	ADC_CH_0 = 16,
	ADC_CH_1 = 32,
	ADC_CH_2 = 64,
	ADC_CH_3 = 128
} ADC_CH_Instance_t;

/* Used to get handle to DCS IP Block instance HS / LS */
typedef enum DcsBlock {
	DCS_INVALID,

    /* Supports relatively low Sampling clk.
     * DACs supports upto 491.52 MHz and ADCs support upto 245.76 MHz.
     * LS_DCS can support upto four-channel (4I + 4Q) wireless transceivers.*/
    DCS_LS1,

	DCS_LS2,

    /* Supports high Sampling clk viz upto 3.932 GHz.
     * HS_DCS can support upto two-channel (2I + 2Q) wireless transceivers. */
    DCS_HS,

	DCS_MAX
} DcsBlock_t;

typedef enum PrimaryClockSrc
{
	PrimaryClkSrc_1,
	PrimaryClkSrc_2
}  PrimaryClockSrc_t;

/* ADC / DAC Pair */
typedef enum ConvPair
{
	ConvPair_1,
	ConvPair_2,
	ConvPair_Both,
}  ConvPair_t;


/* ADC / DAC Clock Divider */
typedef enum ConvPairClk
{
	CP_ClkDiv_0,	/* Divide by 1 */
	CP_ClkDiv_1,	/* Divide by 2 */
	CP_ClkDiv_2,	/* Divide by 4 */
	CP_ClkDiv_3,	/* Divide by 8 */
}  ConvPairClk_t;

/* Primary Clock Divider */
typedef enum MacroClkDiv
{
/*
A General Note on both 2 bits or 3 bits Divide Values (div_sel_ below) :
Divider Output = 2 ^ "Register bits Value".
*/
	ClkDiv_0,	/* LS: Divide by 1   , HS: Divide by 4 */
	ClkDiv_1,	/* LS: Divide by 2   , HS: Divide by 8 */
	ClkDiv_2,	/* LS: Divide by 4   , HS: Divide by 16 */
	ClkDiv_3,	/* LS: Divide by 8   , HS: Divide by 1 */
	ClkDiv_4,	/* LS: Divide by 16  , HS: Divide by 2 */
	ClkDiv_5,	/* LS: Divide by 32  , HS: Divide by 11 */
	ClkDiv_6,	/* LS: Divide by 64  , HS: Divide by 2 */
	ClkDiv_7,	/* LS: Divide by 128 , HS: Divide by 11 */
}  MacroClkDiv_t;

typedef enum LS_AMB_CLK_MODE
{
	GFAST_212_CLK_MODE = 0,				// DCS CLKIN 847.22MHz, DAC/ADC rate 424MHz
	GFAST_106_CLK_MODE,					// DCS CLKIN 847.22MHz, DAC/ADC rate 424MHz
	VDSL_35_CLK_MODE,					// DCS CLKIN 635.4MHz, DAC/ADC rate 318MHz
	WIRELESS_5G_DUAL_CH_CLK_MODE,		// DCS CLKIN 983.04 MHz,
	WIRELESS_WIFI_DUAL_CH_CLK_MODE, 	// DCS CLKIN 640 MHz,
	WIRELESS_5G_SINGLE_CH_CLK_MODE,		// DCS CLKIN 983.04 MHz,
	WIRELESS_WIFI_SINGLE_CH_CLK_MODE	// DCS CLKIN 640 MHz,
} LS_AMB_CLK_MODE_t;

typedef enum STGx_EOC_SEL
{
	STGx_EOC_SEL_VALUE = 0x2,
} STGx_EOC_SEL_t;


typedef enum DAC_LFSR_PROG
{
	LFSR_DAC_PROG_GFAST	= 0xe,
	LFSR_DAC_PROG_VDSL 	= 0xe,
	LFSR_DAC_PROG_WIFI 	= 0x3e,
} DAC_LFSR_PROG_t;

typedef enum DCS_REF_CLK
{
	DCS_REF_CLK_125 	= 0b00001,	/* 122.88 MHz or 125 MHz */
	DCS_REF_CLK_156_25 	= 0b00010,	/* 156.25 MHz */
	DCS_REF_CLK_160 	= 0b01000,	/* 160 MHz */
} DCS_REF_CLK_t;


/******************************************************************************
*
* 							Unions
*
******************************************************************************/




/******************************************************************************
*
* 							Data Structures
*
******************************************************************************/
typedef void * DcsDevHandle_t;



/******************************************************************************
* STRUCT DCS_PARAMS
******************************************************************************/
typedef struct DcsParams {
	DcsPllClk_t eDcsPllClkMode;
	PrimaryClockSrc_t ePrimaryClkSrc;
	MacroClkDiv_t eClkDiv;
	ConvPair_t eConvPair;
	ConvPairClk_t eDacConvPairClk;
	ConvPairClk_t eAdcConvPairClk;
	bool_t	bInterleaveEn;

	/* LS Macro */
	LS_AMB_CLK_MODE_t eClkMode;
} DcsParams_t;


/******************************************************************************
*
* 					Function prototypes
*
******************************************************************************/
void vConfigureDcsPllClk( DcsPllClk_t eDcsPllClkMode );
void vSetDcsRefClk( DCS_REF_CLK_t eDcsRefClk );
DcsPllClk_t eGetDCSPllValue( void );
int uiLoadFw();


/******************************************************************************
* DCS INITIALIZATION AND PLL CONFIGURATION
******************************************************************************/
int iDcsInit( volatile struct gul_hif * pxHif );
int vLSDcsInit_a0( volatile struct gul_hif * pxHif, DcsBlock_t eDcsBlock );
int vLSDcsInit_b0( volatile struct gul_hif * pxHif);
int uiCheckDcsLsConfig( volatile struct gul_hif *pxHif );

#endif /* _DCS_H_ */
