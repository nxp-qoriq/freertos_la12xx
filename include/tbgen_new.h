// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2024 NXP
 */

#ifndef _TBGEN_NEW_H_
#define _TBGEN_NEW_H_

/**
 * @file        tbgen.h
 * @brief       TBGEN-related APIs.
 * @addtogroup  TBGEN_API
 * @{
 */

#include "pmux.h"
/*
 * Summary
 * ===============
 * la12xx e200 FreeRTOS code provides TBGEN driver.
 * This TBGEN driver provides APIs to control and configure TBGEN
 * Hardware IP for various use-cases and functionality.
 *
 * Code for TBGEN driver is present in drivers/tbgen directory of
 * FREERTOS code.
 *
 * For details on TBGEN APIs, refer to
 * a)TBGEN API document or
 * include/tbgen_new.h header file of FREERTOS code.
 *
 *
 *
 * Introduction
 * ===============
 * la12xx SoC supports two TBGEN controllers:
 * a. TBGEN1 Controller
 * b. TBEGN2 Controller
 *
 * Each TBGEN (TBGEN1, TBGEN2) Controller supports
 * multiple Hardware(HW) timers which can be configured as per
 * use-case.
 *
 * Note: Timer names, such as AXRF/AGC are only for
 * reference purpose. No HW limitation in timers to be used for any
 * specific purpose. They can be used as per use-case/requirement.
 *
 * Various TBGEN Hardware timers can be classified into two types:
 * 1. TDD timers (Control sequence generator timers)
 * 2. Non-TDD timers: All other timers like AXRF, RX/SRX Alignment,
 * 	SPI Trigger, AGC, TimedInt, and GPE Timers
 *
 * Each timer type supports multiple instances.
 * To get maximum instance supported, check for
 * "MAX_INSTANCE" string in code or refer to hardware manuals.
 *
 *
 *
 * Generic TBGEN APIs
 * ===================
 * This section contains summary of Generic TBGEN APIS.
 *
 * 1) API to Read Master counter value:
 * Can be called from Multiples cores at same instant.
 *
 * u64 ullTbgenGetMasterCounter( uint8_t ucTbgenNo );
 *
 * 2) API to Read Master counter value:
 * Should be called only from one core at particular instant,
 * else it can return indeterministic value.
 *
 * u64 ullTbgenGetMasterCounterRaw( uint8_t ucTbgenNo );
 *
 * 3) API to Initialize and Enable RFG (10 ms Sync Frame events
 * Generator):
 *
 * int iInitRFG( uint8_t ucTbgenNo, RFGParams_t * pxRFGParams );
 *
 * 4) API to Read TS10MS counter which captures Master Counter value
 * at last 10MS RF event:
 *
 * u64 ullTbgenGet10MSCounter( uint8_t ucTbgenNo );
 *
 * 5) API to Disable the RFG(10 ms Sync Frame Generator):
 *
 * int iDisableRFG( uint8_t ucTbgenNo );
 *
 * 6) API to Reset all the timers of particular TBGEN Controller
 * int iTbgenPerformSwRst( uint8_t ucTbgenNo );
 *
 *
 *
 * TDD Timer
 * =========
 * This section contains summary on TDD Timers and related APIS.
 *
 * TDD Timer configures the HW TDD Tx/Rx enable switching units.
 * Each TBGEN controller supports 8 TDD timer instances to control
 * up to eight transceivers.
 * Each TDD timer instance(TDDx) can drive two signals:
 * 	tx_enable[x] and rx_enable[x] signals.
 *
 * Each TDD Timer instance can be configured in either of below mode:
 * a) In CSG mode: To support sequence of up to 16 steps or
 * b) In Manual mode: To control TX and RX enable outputs
 *   when TDD switching is disabled.
 *
 * TDD Timer Related APIs
 * ---------------------
 * 1) API to Configure TDD Timer in CSG mode:
 *
 * int iTbgenProgramTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddTimerParams_t * pxTddTimerParams );
 *
 * 2) API to Enable TDD Timer:
 *
 * int iTbgenEnableTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance );
 *
 * 3) API to Disable TDD Timer:
 *
 * int iTbgenDisableTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance );
 *
 * 4) API to Restart already programmed timer by reloading the offset
 * Register value:
 *
 * int iTbgenReloadTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance, u64 uOffset );
 *
 * 5) API to Configure TDD Timer in Manual mode (TDD switching mode
 * disabled):
 *
 * int iTbgenProgramTddTimerTxRxManual( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddDurationMode_t eRxTxEnManual );
 *
 * 6)TDD Timer supports interrupt generation mode on Timer expiry.
 * API to enable Interrupt:
 *
 * int iTbgenTimerInterruptEn( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eTimerInstance );
 *
 * API to Disable Interrupt:
 *
 * int iTbgenTimerInterruptDis( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eTimerInstance );
 *
 * API to Acknowledge Interrupt:
 *
 * int iTbgenTimerInterruptClr( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eTimerInstance );
 *
 *
 *
 * Non-TDD Timer
 * ==============
 * This section contains summary on non-TDD Timers and related APIS.
 *
 * All other timers except TDD like AXRF, RX/SRX Alignment, SPI Trigger, AGC,
 * TimedInt and GPE Timers belongs to this category.
 * Each TBGEN controller supports multiple instances of these timers.
 *
 * Non-TDD Timer Related APIs
 * --------------------------
 * 1) API to Configure non-TDD Timer:
 *
 * int iTbgenProgramTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, TimerParams_t * pxTimerParams );
 *
 * 2)  API to Enable non-TDD Timer:
 *
 * int iTbgenEnableTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );
 *
 * 3) API to Disable non-TDD Timer:
 *
 * int iTbgenDisableTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );
 *
 * 4) API to Restart already programmed timer by reloading the offset
 * Register value:
 *
 * int iTbgenReloadTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, u64 uOffset );
 *
 * 5)GPE Timer supports interrupt generation mode on Timer expiry.
 * Relevant APIs below
 * API to Enable Interrupt:
 *
 * int iTbgenTimerInterruptEn( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eTimerInstance );
 *
 * API to Disable Interrupt:
 *
 * int iTbgenTimerInterruptDis( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eTimerInstance );
 *
 * API to Acknowledge interrupt:
 *
 * int iTbgenTimerInterruptClr( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eTimerInstance );
 *
 *
 *
 *
 * How to program any Timer
 * ========================
 * This section contains summary on how to program various timers.
 *
 * For example/test code, user can refer TBGEN test application in
 * FreeRTOS code: utils/Test-Framework/tc_la12xx_tbgen.c
 *
 * 1) For TDD timer programming in CSG mode, refer vTddInitSeq() in
 * test application.
 * 2) For TDD timer programming in manual mode, refer vTddInitManual()
 * in test application.
 * 3) For non-TDD timer programming, refer vTbgenNonTddTimerTest()
 * in test application.
 * 4) For TTI Timer programming which is used to generate TTI events
 * to host, refer vTbgenHostTTIEventTest() in test application.
 * 5) For RX Alignment timer programming to be used for external IRQ,
 * refer to vTbgenExternalIRQTest() in test application.
 * 6) For RFG programming, refer to vTbgen1RFGTest() or vTbgen2RFGTest()
 * in test application.
 */

/******************************************************************************/

#include <types.h>
#include <tbgen_regs_new.h>

/*
 * MAX TBGEN Controller Instances
 */
#define TBGEN_MAX								2
/* TBGEN1 Controller */
#define TBGEN_1									1
/* TBGEN2 Controller */
#define TBGEN_2									2

/*
 * MAX Instances of particular timer type
 */
/* MAX Instances of AXRF Timers */
#define AXRF_MAX_INSTANCE						10
/* MAX Instances of RX Alignment Timers */
#define RX_ALIGNMENT_MAX_INSTANCE				4
/* MAX Instances of SRX Alignment Timers */
#define SRX_ALIGNMENT_MAX_INSTANCE				4
/* MAX Instances of SPI Trigger Timers */
#define SPI_TRIGGER_MAX_INSTANCE				8
/* MAX Instances of AGC Enable Timers */
#define AGC_ENABLE_MAX_INSTANCE					8
/* MAX Instances of Timed Int Timers */
#define TIMED_INT_MAX_INSTANCE					11
/* MAX Instances of GPE Timers */
#define GPE_MAX_INSTANCE						8
/* MAX Instances of TDD Timers */
#define TDD_MAX_INSTANCE						8

/* TDD TX/RX switching unit sequence Max Steps */
#define MAX_TDD_SEQUENCE_STEPS    				16
#define ENABLE_DEBUG_INFO						0
#define DEBUG_RFG								0
#define LO_WORD_MASK							0x00000000FFFFFFFF
#define HI_WORD_MASK							0xFFFFFFFF00000000
#define HI_WORD_SHIFT_BITS						32
#define TBGEN_WRITE_REGISTER( ADDR, VALUE )		out_ppc_le32( (volatile uint32_t *)ADDR, VALUE )
#define TBGEN_READ_REGISTER( ADDR )				in_ppc_le32( (volatile uint32_t *)ADDR )

/*
 * TBGEN ERROR CODES
 */
#define INVALID_TBGEN_NO						-1
#define INVALID_TIMER_TYPE						-2
#define INVALID_TIMER_INSTANCE					-3

/**
 * \struct TbgenPmuxInfo_t
 * Use to store Tbgen Pmux Control Register number and pin no
 */
typedef struct TbgenPmuxInfo {
	enum pmux_num num;			/**< Pmux register no */
	enum pmux_index index;		/**< Pin no to configure as GPIO */
} TbgenPmuxInfo_t;

/*
 * \enum TbgenRefClk_t
 * It represents possible values for TBGEN Ref clock and corresponding divider.
 *
 */
typedef enum TbgenRefClk
{
    REF_CLK_491_52_MHZ = 1,
    REF_CLK_245_76_MHZ = 2,
    REF_CLK_122_88_MHZ = 4,
    REF_CLK_61_44_MHZ = 8,
    REF_CLK_30_72_MHZ = 16,
} TbgenRefClk_t;

/**
 * \enum TbgenIPGRefClkDiv_t
 * B0: It represents possible values for Tbgen1 and Tbgen2 dividers for IPG clk.
 * By default for Tbgen1, Source clk is ipg_clk/4.
 * By default for Tbgen2, Source clk is ipg_clk/2.
 * These dividers will divide the source clk(that is, ipg_clk/4 for Tbgen1 and ipg_clk/2 for Tbgen2).
 * For example, When TbgenIPGRefClkDiv_t = REF_CLK_IPG_1_DIV,
 *            Tbgen1 ref clk will be (ipg_clk/4) / 1
 *            Tbgen2 ref clk will be (ipg_clk/2) / 1
 * For example, When TbgenIPGRefClkDiv_t = REF_CLK_IPG_2_DIV,
 *            Tbgen1 ref clk will be (ipg_clk/4) / 2
 *            Tbgen2 ref clk will be (ipg_clk/2) / 2
 */
typedef enum TbgenIPGRefClkDiv
{
    REF_CLK_IPG_1_DIV = 1,
    REF_CLK_IPG_2_DIV = 2,
    REF_CLK_IPG_4_DIV = 4,
    REF_CLK_IPG_8_DIV = 8,
    REF_CLK_IPG_16_DIV = 16,
    REF_CLK_IPG_32_DIV = 32,
} TbgenIPGRefClkDiv_t;

/* For Geul B0, Source of Tbgen1 ref Clk can either be ipg_clk/4 or soc_clk_lsdcs output of ls_dcs_subsystem */
/* For Geul B0, Source of Tbgen2 ref Clk can either be ipg_clk/2 or clk_d1 output of HS_DCS macro */
/* For Geul B0, By default, Tbgen1 and Tbgen2 ref clk is platform clk */
/* For Geul B0, For Tbgen1 ref clk as source of output of ls_dcs_subsystem, User must set value of TBGEN1_REF_CLK_IPG_CLK as 0 */
/* For Geul B0, For Tbgen2 Ref clk as source of output of HS_DCS macro, User must set value of TBGEN2_REF_CLK_IPG_CLK as 0 */
#define TBGEN1_REF_CLK_IPG_CLK			0
#define TBGEN2_REF_CLK_IPG_CLK			0

/* User must set TBGEN1_DIV to get required TBGEN1 Ref Freq */
//#define TBGEN1_DIV			REF_CLK_245_76_MHZ
extern volatile uint32_t tbgen1_div;

/* User must set TBGEN2_DIV to get required TBGEN2 Ref Freq */
//#define TBGEN2_DIV			REF_CLK_245_76_MHZ
extern volatile uint32_t tbgen2_div;

/** If user select TBGEN1_REF_CLK_IPG_CLK or TBGEN2_REF_CLK_IPG_CLK, then User must set TBGEN1_IPG_DIV or TBGEN2_IPG_DIV to any TbgenIPGRefClkDiv_t divider
 *  By default, TBGEN1_IPG_DIV is REF_CLK_IPG_1_DIV, that is, Tbgen1 Ref clk is (ipg_clk/4) / 1
 *  By default, TBGEN2_IPG_DIV is REF_CLK_IPG_1_DIV, that is, Tbgen2 Ref clk is (ipg_clk/2) / 1
 */
#define TBGEN1_IPG_DIV			REF_CLK_IPG_1_DIV
#define TBGEN2_IPG_DIV			REF_CLK_IPG_1_DIV

#define TBGEN_MAX_FREQ		( 491.52 )

#if TBGEN1_REF_CLK_IPG_CLK
#define TBGEN1_REF_CLK		(((PLAT_FREQ / 4) / TBGEN1_IPG_DIV) / 1000000)
#else
//#define TBGEN1_REF_CLK		TBGEN_MAX_FREQ / TBGEN1_DIV
#define TBGEN1_REF_CLK		TBGEN_MAX_FREQ / tbgen1_div
#endif

#if TBGEN2_REF_CLK_IPG_CLK
#define TBGEN2_REF_CLK		(((PLAT_FREQ / 2) / TBGEN2_IPG_DIV) / 1000000)
#else
//#define TBGEN2_REF_CLK		TBGEN_MAX_FREQ / TBGEN2_DIV
#define TBGEN2_REF_CLK		TBGEN_MAX_FREQ / tbgen2_div
#endif

#define TBGEN_491_52_REF_CLK_KHZ		491520
#define TBGEN_245_76_REF_CLK_KHZ		245760
#define TBGEN_122_88_REF_CLK_KHZ		122880
#define TBGEN_61_44_REF_CLK_KHZ			61440
#define TBGEN_30_72_REF_CLK_KHZ			30720
#define TBGEN_IPG_CLK				PLAT_FREQ / 1000

/**
 * \enum RegSetReset_t
 * Used to Set and Reset Bits and Registers
 */
typedef enum RegSetReset
{
    REG_SET,
    REG_RESET,
} RegSetReset_t;

/* Classify the Timer in 2 parts
 * 1. Non-TDD Timer(for example, AXRF, RX Alignment, SRX Alignment, SPI Trigger, Timed Interrupt, AGC and GPE)
 * 2. TDD Timer
 */

/* Non-TDD Timers Parameters */
/**
 * \enum PulseWidth_t
 * Used to configure the timer output pulse width, when the Non-TDD timer is programmed for Pulse Mode.
 * The timer output pulse width is programmed in units of reference clock cycles.
 *
 * PULSE_WIDTH_CLK_CYCLE_16 - 16 reference clock cycles (default)
 */
typedef enum PulseWidth
{
    PULSE_WIDTH_CLK_CYCLE_16 = 0,
    PULSE_WIDTH_CLK_CYCLE_1,
    PULSE_WIDTH_CLK_CYCLE_2,
    PULSE_WIDTH_CLK_CYCLE_3,
    PULSE_WIDTH_CLK_CYCLE_4,
    PULSE_WIDTH_CLK_CYCLE_5,
    PULSE_WIDTH_CLK_CYCLE_6,
    PULSE_WIDTH_CLK_CYCLE_7,
    PULSE_WIDTH_CLK_CYCLE_8,
    PULSE_WIDTH_CLK_CYCLE_9,
    PULSE_WIDTH_CLK_CYCLE_10,
    PULSE_WIDTH_CLK_CYCLE_11,
    PULSE_WIDTH_CLK_CYCLE_12,
    PULSE_WIDTH_CLK_CYCLE_13,
    PULSE_WIDTH_CLK_CYCLE_14,
    PULSE_WIDTH_CLK_CYCLE_15,
} PulseWidth_t;

/**
 * \enum StrobePolarity_t
 * Used to configure timer output polarity.
 */
typedef enum StrobePolarity
{
    STROBE_POL_RISING,		/**< Timer output starts at 0 and transitions to 1 upon triggering */
    STROBE_POL_FALLING,		/**< Timer output starts at 1 and transitions to 0 upon triggering */
} StrobePolarity_t;

/**
 * \enum StrobeMode_t
 * Timer Output Strobe Mode.
 */
typedef enum StrobeMode
{
    STROBE_MODE_TOGGLE,		/**< Timer in toggle mode (output toggles when the timer expires) */
    STROBE_MODE_PULSE,		/**< Timer in pulse mode (output is a pulse PulseWidth_t cycles wide) */
    STROBE_MODE_CYCLE,		/**< Timer in cycle mode (output is a 50 % duty cycle pulse with a period of TIMER(INTERVAL[31:0])) */
} StrobeMode_t;

/**
 * \enum TriggerMode_t
 * Timer One-shot mode
 */
typedef enum TriggerMode
{
    TM_REPETITIVE,		/**< Timer output triggers continuously based on the programmed interval value. */
    TM_ONE_SHOT,		/**Timer output triggers once and goes to sleep. */
} TriggerMode_t;

/* TDD Timer Parameters */
/**
 * \enum TddDurationMode_t
 * TDD TX/RX Enable Output Mode
 */
typedef enum TddDurationMode
{
    TDD_MODE_00,		/**< TX enable = 0, RX enable = 0 */
    TDD_MODE_01,		/**< TX enable = 0, RX enable = 1 */
    TDD_MODE_10,		/**< TX enable = 1, RX enable = 0 */
    TDD_MODE_11,		/**< TX enable = 1, RX enable = 1 */
} TddDurationMode_t;

/**
 * \enum TddPulseMode_t
 * TDD Timer Pulse Mode Configuration
 */

typedef enum TddPulseMode
{
    TDD_PULSE_MODE_00,	/**< TDD Timer uses TDD Mode to determine the output values when executing a step in a defined buffer sequence */
    TDD_PULSE_MODE_01,	/**< TDD Timer generates a programmable-width pulse on the TX Enable output when executing a step in a defined buffer sequence */
    TDD_PULSE_MODE_10,	/**< TBGen TDD Switching Unit Timer generates a 50 % duty cycle pulse on the TX Enable output when executing a step in a defined buffer sequence */
} TddPulseMode_t;

/* RFG Timer Parameters*/
/**
 * \enum FrameSyncSelect_t
 * 10 ms Frame SYNC Select
 */
typedef enum FrameSyncSelect
{
    FRAME_SYNC_SRC_SYSREF = 0,			/**< SYSREF_IN signal used as the source for the 10 ms Frame SYNC signal */
    FRAME_SYNC_SRC_CPRI_RX_RFP = 1,		/**< CPRI_RX_RFP signal used as the source for the 10 ms Frame SYNC signal */
    FRAME_SYNC_SRC_CPRI_RX_RFG_0b10 = 2,/**< RFG output is used as the source for the 10 ms Frame SYNC signal */
    FRAME_SYNC_SRC_CPRI_RX_RFG_0b11 = 3,/**< RFG output is used as the source for the 10 ms Frame SYNC signal */
} FrameSyncSelect_t;

/**
 * \enum RFGRefSyncSel_t
 * 10 ms Reference SYNC Select
 */
typedef enum RFGRefSyncSel
{
    RFG_SYSREF_IN = 0,	/**< The Radio Frame Generator uses the SYSREF_IN signal as the 10 ms reference frame SYNC signal */
    RFG_CPRI_RX,		/**< The Radio Frame Generator uses the CPRI_RX_RFP signal as the 10 ms reference frame SYNC signal */
    RFG_INTERNAL,		/**< Starts (initiates) the Internally generated Radio Frame SYNC signal */
} RFGRefSyncSel_t;

/**
 * \enum RFGSyncOut_t
 * Sysref output select control bit
 */
typedef enum RFGSyncOut
{
    RFG_SYSREF_IN_SYNC_OUT = 0,	/**< Select sysref_in as the sysref_out source */
    RFG_GENERATED_SYNC_OUT,		/**< Select internally generated 10 ms as the sysref_out source */
    RFG_CPRI_RX_SYNC_OUT,		/**< Select cpri_rx_rfp as the sysref_out source */
} RFGSyncOut_t;

/**
 * \enum TimerType_t
 * Enum for Various Timer Types
 */
typedef enum TimerType
{
	AXRF = 0,
	RX_ALIGNMENT,
	SRX_ALIGNMENT,
	SPI_TRIGGER,
	AGC_ENABLE,
	TIMED_INT,
	GPE,
	TDD,
} TimerType_t;

/*
___________________________________________________________________
|TBGEN1 Timer Type|     Signal Name     |  TBGEN1 Event Name       |
|                 |                     |                          |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|     AXRF        |                     |                          |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|RX_ALIGNMENT[0:2]| ipp_ind_ext_int[5:7]| jesd_rx_strobe[0:2]      |
|RX_ALIGNMENT[3]  | ipp_ind_ext_int[4]  | jesd_rx_strobe[3]        |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|SRX_ALIGNMENT[0] |start_rx_max_search  | jesd_srx_strobe[0]       |
|SRX_ALIGNMENT[1] |stop_rx_max_search   | jesd_srx_strobe[1]       |
|SRX_ALIGNMENT[2:3]LS_TIME_GPO[9:10]    | jesd_srx_strobe[2:3]     |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|SPI_TRIGGER[0:4] |    ext_go[8:9]      | spi_trigger[0:4]         |
|SPI_TRIGGER[5:7] | LS_TIME_GPO[27:29]  | spi_trigger[5:7], 2’b0   |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|AGC_ENABLE[0:7]  | LS_TIME_GPO[19:26]  | agc_en[0:7]              |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|TIMED_INT[0]     |                     |                          |
|TIMED_INT[1:11]  |    ext_go[8:9]      | tbgen_ruby_trigger[0:10] |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|    GPE[0:7]     | LS_TIME_GPO[11:18]  | gp_event[0:7]            |
|_________________|_____________________|__________________________|
|                 |                     |                          |
| TDD_SWITCH[0:3] | ls_rx allowed[1:4]  | rx_enable[0:3]           |
| TDD_SWITCH[0:3] | ls_tx allowed[1:4]  | tx_enable[0:3]           |
| TDD_SWITCH[4]   | ls_rxlp1            | rx_enable[4]             |
| TDD_SWITCH[5]   | ls_rxlp2            | rx_enable[5]             |
| TDD_SWITCH[6]   | ls_rxlp3            | rx_enable[6]             |
| TDD_SWITCH[7]   | ls_rxlp4            | rx_enable[7]             |
| TDD_SWITCH[4]   | ls_txlp1            | tx_enable[4]             |
| TDD_SWITCH[5]   | ls_txlp2            | tx_enable[5]             |
| TDD_SWITCH[6]   | ls_txlp3            | tx_enable[6]             |
| TDD_SWITCH[7]   | ls_txlp4            | tx_enable[7]             |
|_________________|_____________________|__________________________|

___________________________________________________________________
|TBGEN2 Timer Type|     Signal Name     |  TBGEN2 Event Name       |
|                 |                     |                          |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|  AXRF[0:7]      |  HS_TIME_GPO[0:7]   | jesd_tx_axrfen[2:9]      |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|RX_ALIGNMENT[0:3]|ipp_ind_ext_int[8:11]| jesd_rx_strobe[0:3]      |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|SRX_ALIGNMENT[0] |start_rx_max_search  | jesd_srx_strobe[0]       |
|SRX_ALIGNMENT[1] |stop_rx_max_search   | jesd_srx_strobe[1]       |
|SRX_ALIGNMENT[2] |HS_TIME_GPO[15]      | jesd_srx_strobe[2]       |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|SPI_TRIGGER[0:4] |   ext_go[10:11]     |   spi_trigger[0:4]       |
|SPI_TRIGGER[5:7] | HS_TIME_GPO[12:14]  | spi_trigger[5:7], 2’b0   |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|AGC_ENABLE[0:7]  | HS_TIME_GPO[24:31]  | agc_en[0:7]              |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|TIMED_INT[0]     |                     |                          |
|TIMED_INT[1:11]  |    ext_go[10:11]    | tbgen_ruby_trigger[0:10] |
|_________________|_____________________|__________________________|
|                 |                     |                          |
|    GPE[0:7]     | HS_TIME_GPO[16:23]  | gp_event[0:7]            |
|_________________|_____________________|__________________________|
|                 |                     |                          |
| TDD_SWITCH[0:1] | hs_rx allowed[1:2]  | rx_enable[0:1]           |
| TDD_SWITCH[0:1] | hs_tx allowed[1:2]  | tx_enable[0:1]           |
| TDD_SWITCH[2]   | hs_rxlp1            | rx_enable[2]             |
| TDD_SWITCH[3]   | hs_rxlp2            | rx_enable[3]             |
| TDD_SWITCH[2]   | hs_txlp1            | tx_enable[2]             |
| TDD_SWITCH[3]   | hs_txlp2            | tx_enable[3]             |
|_________________|_____________________|__________________________|
*/

/**
 * \enum TimerInstance_t
 * Various Timer Types has different Timer Instances count.
 *
 * Use below enum for all kind of Timer Instances.
 *
 * For example, want to use GPE Timer Instance 3 -> Use TIMER_INSTANCE_3.
 * For example, want to use TDD Timer Instance 4 -> Use TIMER_INSTANCE_4.
 * AXRF Timer has 10 instances(0:9).
 * RX Alignment Timer has 4 instances.
 * SRX Alignment Timer has 4 instances.
 * SPI Trigger Timer has 8 instances.
 * AGC Timer has 8 instances.
 * Timed Interrupt Timer has 12 instances.
 * GPE Timer has 8 instances.
 * TDD Timer has 8 instances.
 *
 * Note:
 *
 * Use Timer Instance as per above table.
 * TIMED_INT Timers 1 through 11 go to VSPA platform (They are VSPA Event Triggers).
 * TIMED_INT Timer 0 is reserved for MCU Interrupts.
 */

typedef enum TimerInstance
{
    TIMER_INSTANCE_0 = 0,
    TIMER_INSTANCE_1,
    TIMER_INSTANCE_2,
    TIMER_INSTANCE_3,
    TIMER_INSTANCE_4,
    TIMER_INSTANCE_5,
    TIMER_INSTANCE_6,
    TIMER_INSTANCE_7,
    TIMER_INSTANCE_8,
    TIMER_INSTANCE_9,
    TIMER_INSTANCE_10,
    TIMER_INSTANCE_11,
} TimerInstance_t;

/**
 * \struct TbgenHostTTIConf_t
 * Use to store Tbgen Timer conf that can generate Host TTI 
 */
typedef struct TbgenHostTTIConf
{
	TimerType_t etype;		/**< Timer type */
	TimerInstance_t eInst;	/**< Timer Instance */
} TbgenHostTTIConf_t;

/**
 * Interrupt based Timers Callback is supported by:
 * 1. RX_ALIGNMENT Timer(External Interrupt lines)
 * 2. GPE and TDD Timers(on TBGEN Interrupt line)
 *
 * Interrupt is affined to a core by calling bMpicEnable() from that core.
 * Note: Interrupt can be routed only to a single core at a time.
 *
 * For RX_ALIGNMENT Timer, interrupt will get affined to core from which
 * iTbgenProgramTimer() is called as iTbgenProgramTimer() internally
 * calls bMpicEnable().
 *
 * For GPE and TDD Timers, interrupt will get affined to core from which
 * iTbgenDevOpen() is called at TBGEN initialization time as
 * this function internally calls bMpicEnable().
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 * @param[in] pxTimerParams
 * 	Timer-related parameters
 */
typedef void (* TimerCallbackFn)( uint8_t ucTbgenNo, TimerInstance_t eInstance, void *pxTimerParams );

/**
 * RFG Callback
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 */
typedef void (* RFGCallbackFn)( uint8_t ucTbgenNo );

/**
 * \struct TddDuration_t
 * Tdd Duration Parameters
 */
typedef struct TddDuration
{
    u32 uDur;				/**< configure the duration of TDD Steps in the TDD TX/RX switching unit sequence. Represents number of reference clocks */
    TddDurationMode_t eMode;/**< TDD TX/RX Enable Output Mode (TDD_MODE_00/01/10/11) */
} TddDuration_t;

/**
 * \struct TddTimerParams_t
 * Tdd Timer Parameters. User must fill this structure to program TDD Timers
 *
 * NOTE:
 *
 * uOffset value should not be less than (Current Master Counter value + delta).
 * Current Master Counter value = Master Counter Value, when user is programming the uOffset parameter.
 * delta = time taken by iTbgenProgramTddTimer() API + iTbgenEnableTddTimer() API + time elapsed until user calls these APIs.
 * Time taken by (iTbgenProgramTddTimer() API + iTbgenEnableTddTimer() API) ~ (0.5us - 2us).
 */

typedef struct TddTimerParams
{
	u64 uOffset;			/**< Master Counter Value at next timer action */
	TriggerMode_t eTrigMode;/**< configure the Timer as continuous or one-shot mode */
	TddPulseMode_t ePm;		/**< determine the output values when executing a step in a defined buffer sequence (TDD_PULSE_MODE_00/01/10) */
	u16 uPw;				/**< configure the timer output pulse width in Pulse Mode */
    TddDurationMode_t eRxTxEnManual;/**< configure the default values of the TX and RX enable outputs */
	u8 ucTddSeqSteps;		/**< No of steps in TDD TX/RX sequence. Max Value can be 16 */
    TddDuration_t xDuration[ MAX_TDD_SEQUENCE_STEPS ];/**< Fill Mode and Duration time(in clk cycles) for every ucTddSeqSteps */
	TimerCallbackFn pvCb;	/**< Timer Callback to be registered (for the Interrupt Based Timers) */
} TddTimerParams_t;

/**
 * \struct TimerParams_t
 * Non-Tdd Timer Parameters. User must fill this structure to program Non-TDD Timers (for example, AXRF, RX Alignment, SRX Alignment, SPI Trigger, Timed Interrupt, AGC, and GPE)
 *
 * NOTE:
 *
 * uOffset value should not be less than (Current Master Counter value + delta).
 *
 * Current Master Counter value = Master Counter Value, when user is programming the uOffset parameter.
 *
 * delta = time taken by iTbgenProgramTimer() API + iTbgenEnableTimer() API + time elapsed until user calls these APIs.
 *
 * Time taken by (iTbgenProgramTimer() API + iTbgenEnableTimer() API) ~ 0.5us.
 */
typedef struct TimerParams
{
	u64 uOffset;				/**< Master Counter Value at next timer action */
	u32 uInterval;				/**< Interval after which timer starts again(in continuous mode) */
	TriggerMode_t eTrigMode;	/**< configure the Timer as continuous or one-shot mode */
	StrobeMode_t eSm;			/**< configure the timer's output strobe mode (STROBE_MODE_TOGGLE/PULSE/CYCLE) */
	PulseWidth_t ePw;			/**< configure the timer output pulse width in Pulse Mode */
	StrobePolarity_t ePolarity;	/**< configures timer output polarity */
	TimerCallbackFn pvCb;		/**< Timer Callback to be registered (for the Interrupt Based Timers) */
} TimerParams_t;

/**
 * \struct RFGParams_t
 * RFG Parameters
 */
typedef struct RFGParams
{
	RFGSyncOut_t eSyncOut;			/**< Sysref output select control bit */
	RFGRefSyncSel_t eRefSyncSel;	/**< 10 ms Reference SYNC Select */
	FrameSyncSelect_t eFrameSyncSel;/**< 10 ms Frame SYNC Select */
	RFGCallbackFn pvCb;				/* RFG Callback function to be registered(if applicable) */
} RFGParams_t;

/**
 * This API resets all the timers for passed Tbgen Instance
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO
 */
int iTbgenPerformSwRst( uint8_t ucTbgenNo );

/**
 * This API will Enable interrupts for TDD and GPE Timers
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 *  eTimerType can be TDD or GPE
 * @param[in] eInstance
 *  Timer instance of TDD or GPE(that is, TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 */
int iTbgenTimerInterruptEn( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );

/**
 * This API will Disable interrupts for TDD and GPE Timers
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 *  eTimerType can be TDD or GPE
 * @param[in] eInstance
 *  Timer instance of TDD or GPE(that is, TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 */
int iTbgenTimerInterruptDis( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );

/**
 * This API will Acknowledge interrupts for TDD and GPE Timers
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 *  eTimerType can be TDD or GPE
 * @param[in] eInstance
 *  Timer instance of TDD or GPE(that is, TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 */
int iTbgenTimerInterruptClr( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );

/**
 * This API reads current value of the master counter register of
 * TBGEN instance passed (the Timebase Master Counter Unit (TMCU) of TBGEN).
 * This register is read-only, therefore, no write is allowed on it.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 *
 * @return
 *  - On Success, Returns Master counter value
 */
u64 ullTbgenGetMasterCounter( uint8_t ucTbgenNo );
/**
 * This API reads current value of the master counter register of
 * TBGEN instance passed (the Timebase Master Counter Unit (TMCU) of TBGEN).
 * This register is read-only, therefore, no write is allowed on it.
 * This API is less accurate (that is, lower 32-bit value may be wrong when there is overflow of lower 32 bits)
 * than ullTbgenGetMasterCounter, but takes less time than ullTbgenGetMasterCounter.
 *
 * Max Inaccuracy can be (4,228,250,625) clks.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 *
 * @return
 *  - On Success, Returns Master counter value
 */
u64 ullTbgenGetMasterCounterRaw( uint8_t ucTbgenNo );

/**
 * This API reads current value of the TS10MS counter register of
 * TBGEN instance passed.
 * This register is read-only, therefore, no write is allowed on it.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 *
 * @return
 *  - On Success, Returns TS10MS counter value
 */
u64 ullTbgenGet10MSCounter( uint8_t ucTbgenNo );

/**
 * This API will Program the TDD Timer parameters
 * User must fill all or some pxTddTimerParams.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 * @param[in] pxTddTimerParams
 *  pointer to TddTimerParams_t
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_INSTANCE
 *
 *
 * How to Use API Parameters:
 *
 * For example:
 * ucTbgenNo = TBGEN_2,
 * eInstance = TIMER_INSTANCE_0,
 * pxTddTimerParams->uOffset = Current Master Counter value + 100us(that is, anytime in clocks),
 * pxTddTimerParams->eTrigMode = TM_REPETITIVE,
 * pxTddTimerParams->ePm = TDD_PULSE_MODE_00,
 * pxTddTimerParams->uPw = 200,
 * pxTddTimerParams->eRxTxEnManual = <User can ignore it or TDD_MODE_00/01/10/11>,
 * pxTddTimerParams->ucTddSeqSteps = 2,
 * pxTddTimerParams->xDuration[0].uDur = 5us,
 * pxTddTimerParams->xDuration[0].eMode = TDD_MODE_01,
 * pxTddTimerParams->xDuration[1].uDur = 7us,
 * pxTddTimerParams->xDuration[1].eMode = TDD_MODE_10,
 * pxTddTimerParams->pvCb = NULL or Callback function,
 *
 *
 * NOTE:
 *
 * pxTddTimerParams->uOffset value should not be less than (Current Master Counter value + delta).
 * Current Master Counter value = Master Counter Value, when user is programming the pxTddTimerParams->uOffset parameter.
 * delta = time taken by iTbgenProgramTddTimer() API + iTbgenEnableTddTimer() API + time elapsed until user calls these APIs.
 * Time taken by (iTbgenProgramTddTimer() API + iTbgenEnableTddTimer() API) ~ (0.5us - 2us).
 */
int iTbgenProgramTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddTimerParams_t * pxTddTimerParams );

/**
 * This API will Enable TDD Timer instance.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_INSTANCE
 */
int iTbgenEnableTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance );

/**
 * This API will Disable TDD Timer instance.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_INSTANCE
 */
int iTbgenDisableTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance );

/**
 * This API will Reload TDD Timer instance.
 * It will reload the TDD Timer offset Register value.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 * @param[in] uOffset
 *  Master Counter Value at next timer action
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_INSTANCE
 *
 *
 * NOTE:
 *
 * uOffset value should not be less than (Current Master Counter value + delta).
 * Current Master Counter value = Master Counter Value, when user is programming the uOffset parameter.
 * delta = time taken by iTbgenReloadTddTimer() API + time elapsed until user calls these APIs.
 * Time taken by iTbgenReloadTddTimer() API ~ 0.2us.
 */
int iTbgenReloadTddTimer( uint8_t ucTbgenNo, TimerInstance_t eInstance, u64 uOffset );

/**
 * This API configures TDD TX/RX Enable Default Output Mode
 * It can be used to control manually the TX and RX enable outputs when the TDD Switching Timer is disabled.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_7)
 * @param[in] eRxTxEnManual
 *  (TDD_MODE_00/01/10/11)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_INSTANCE
 */
int iTbgenProgramTddTimerTxRxManual( uint8_t ucTbgenNo, TimerInstance_t eInstance, TddDurationMode_t eRxTxEnManual );

/**
 * This API will Program the Non-TDD Timer(for example AXRF, RX Alignment, SRX Alignment, SPI Trigger, Timed Interrupt, AGC, and GPE) parameters.
 * User must fill all or some pxTimerParams.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 * (AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 * @param[in] pxTimerParams
 *  pointer to TimerParams_t
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 *
 *
 * How to Use API Parameters:
 *
 * For example:
 * ucTbgenNo = TBGEN_2,
 * eTimerType = GPE,
 * eInstance = TIMER_INSTANCE_0,
 * pxTimerParams->uOffset = Current Master Counter value + 100us(that is, anytime in clocks),
 * pxTimerParams->uInterval = 25us (for example, 25 * 245.76),
 * pxTimerParams->eTrigMode = TM_REPETITIVE,
 * pxTimerParams->eSm = STROBE_MODE_PULSE,
 * pxTimerParams->ePw = PULSE_WIDTH_CLK_CYCLE_16,
 * pxTimerParams->ePolarity = STROBE_POL_RISING,
 * pxTimerParams->pvCb = NULL or Callback function,
 *
 *
 * NOTE:
 *
 * pxTimerParams->uOffset value should not be less than (Current Master Counter value + delta).
 * Current Master Counter value = Master Counter Value, when user is programming the pxTimerParams->uOffset parameter.
 * delta = time taken by iTbgenProgramTimer() API + iTbgenEnableTimer() API + time elapsed until user calls these APIs.
 * Time taken by (iTbgenProgramTimer() API + iTbgenEnableTimer() API) ~ 0.5us.
 */
int iTbgenProgramTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, TimerParams_t * pxTimerParams );

/**
 * This API will Program the Non-TDD Timer Interval Register(for example, RX Alignment, SRX Alignment, SPI Trigger, AGC and GPE) parameters.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 * (AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 * @param[in] uInterval
 *  Timer Interval in clocks
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 *
 *
 * How to Use API Parameters:
 *
 * For example:
 * ucTbgenNo = TBGEN_2,
 * eTimerType = GPE,
 * eInstance = TIMER_INSTANCE_0,
 * pxTimerParams->uInterval = 25us (for example, 25 * 245.76)
 */

int iTbgenConfigTimerIntrvl( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, u32 uInterval );

/**
 * This API will Enable the Non-TDD Timer(for example, AXRF, RX Alignment, SRX Alignment, SPI Trigger, Timed Interrupt, AGC and GPE) instance
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 * (AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 */
int iTbgenEnableTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );

/**
 * This API disables the Non-TDD Timer(for example, AXRF, RX Alignment, SRX Alignment, SPI Trigger, Timed Interrupt, AGC and GPE) instance
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 * (AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 */
int iTbgenDisableTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );

/**
 * This API will Reload the Non-TDD Timer(for example, AXRF, RX Alignment, SRX Alignment, SPI Trigger, Timed Interrupt, AGC and GPE) instance
 * It will reload the Non-TDD Timer offset Register value.
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 * (AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 * @param[in] uOffset
 *  Master Counter Value at next timer action
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 *
 *
 * NOTE:
 *
 * uOffset value should not be less than (Current Master Counter value + delta).
 * Current Master Counter value = Master Counter Value, when user is programming the uOffset parameter.
 * delta = time taken by iTbgenReloadTimer() API + time elapsed until user calls these APIs.
 * Time taken by iTbgenReloadTimer() API ~ 0.2us.
 */
int iTbgenReloadTimer( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, u64 uOffset );

/**
 * This API configures the polarity while reloading the timer(for example, AXRF, RX Alignment, SRX Alignment, SPI Trigger, AGC and GPE)
 * It reloads the Non-TDD Timer offset Register value and reprograms Timer Polarity
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 *	(AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 * @param[in] ePolarity
 *	(STROBE_POL_RISING, STROBE_POL_FALLING)
 * @param[in] uOffset
 *	Master Counter Value at next timer action
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO, INVALID_TIMER_TYPE, INVALID_TIMER_INSTANCE
 *
 *
 * NOTE:
 *
 * uOffset value should not be less than (Current Master Counter value + delta).
 * Current Master Counter value = Master Counter Value, when user is programming the uOffset parameter.
 * delta = time taken by iTbgenReloadTimer() API + time elapsed until user calls these APIs.
 * Time taken by iTbgenReloadTimer() API ~ 0.2us
 */
int iTbgenReloadTimerAndPolarity( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance, StrobePolarity_t ePolarity, u64 uOffset );

/**
 * This API will initialize the RFG Counter, which will generate frame after every 10 ms
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] pxRFGParams
 *  pointer to RFGParams_t
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO
 */
int iInitRFG( uint8_t ucTbgenNo, RFGParams_t * pxRFGParams );

/**
 * This API disables the RFG Counter
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns INVALID_TBGEN_NO
 */
int iDisableRFG( uint8_t ucTbgenNo );

/**
 * This API will Configure Mux on pin associated with Tbgen Instance for TDD Timer.
 * It configures pin as Tbgen. Only TBGEN_2 supports this
 *
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE7)
 * @param[in] ucTxRx
 *  1 - Configure pins associated with TX pin
 *  0 - Configure pins associated with RX pin
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns -1
 */
int iConfPMuxModeTbgenTdd( TimerInstance_t eInstance, uint8_t ucTxRx );

/**
 * This API will Configure Mux on pin associated with Tbgen Instance.
 * It configures pin as Tbgen
 *
 * @param[in] ucTbgenNo
 *  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eTimerType
 * (AXRF, RX_ALIGNMENT, SRX_ALIGNMENT, SPI_TRIGGER, AGC_ENABLE, TIMED_INT, GPE)
 * @param[in] eInstance
 *	Tbgen Timer Instance for which we must program parameters (TIMER_INSTANCE_0 - TIMER_INSTANCE_11)
 *
 * @return
 *  - On Success, Returns 0
 *  - On Failure, Returns -1
 */
int iConfPMuxModeTbgen( uint8_t ucTbgenNo, TimerType_t eTimerType, TimerInstance_t eInstance );

/**
 * This API enables TimeStamp trigger on specified Tbgen Instance.
 *
 * @param[in] ucTbgenNo
 *  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *  There are 4 inputs, therefore instance is [0..3]
 * @param[in] eSetReset
 *  Enable or disable
 */

void vConfigTsMcu( uint8_t ucTbgenNo, u8 ucInstance, RegSetReset_t eSetReset );

/**
 * This API configures TimeStamp interrupts on specified Tbgen Instance.
 *
 * @param[in] ucTbgenNo
 *  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] eInstance
 *  There are 4 inputs, therefore instance is [0..3]
 * @param[in] eSetReset
 *  Enable or disable
 */
void vConfigTsMcuInterrupts( uint8_t ucTbgenNo, u8 ucInstance, RegSetReset_t eSetReset );

/**
 * This API should be called when acking the TS interrupt
 *
 * @param[in] ucTbgenNo
 *  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
 * @param[in] ucTsIndex
 *  Index is [0..3]
 */
void vAckTsInstance( uint8_t ucTbgenNo, u8 ucTsIndex );

/**
* This API returns Tbgen Frequency in kHz.
*
* @param[in] ucTbgenNo
*  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
*
* @return
*  - Tbgen1 or Tbgen2 Frequency in kHz
*/
uint32_t uGetTbgenFreq( uint8_t ucTbgenNo );

/**
* This API returns Tbgen Clock Source
*
* @param[in] ucTbgenNo
*  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
*
* @return
*  - Tbgen Clock Source - (NO_SRC, HS_DCS_OUTPUT, LS_DCS_OUTPUT, IPG_CLK)
*/
enum tbgen_clk_source eGetTbgenClkSrc( uint8_t ucTbgenNo );

/**
* This API enables Tbgen Clock Divider
*
* @param[in] ucTbgenNo
*  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
*/
void vEnableTbgen( uint8_t ucTbgenNo );

/**
* This API disables Tbgen Clock Divider
*
* @param[in] ucTbgenNo
*  Tbgen Instance (that is, TBGEN_1 or TBGEN_2)
*/
void vDisableTbgen( uint8_t ucTbgenNo );
/** @} */
#endif /* _TBGEN_NEW_H_ */
