/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "tbgen_new.h"
#include "la12xx_tbgen.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "l1c_defs.h"
#include "l1c_rf_ctrl.h"

// #define LS10465GRU  // Specific for S. board

extern volatile uint32_t brd_ver;
TimerParams_t ctrl_sig_params;

#define IN_USE  1

#if defined(GEUL_LA1238RDB)
/*
 *   GPIO info     Timer    Description
 *  _____________________________________________________
 * | GPIO2_17 | GP_EVENT[6] | TRX_YC1 tx/rx switch       |
 * | GPIO2_22 | AGC_EN[3]   | TX1_PA_EN                  |
 * | GPIO2_23 | AGC_EN[4]   | RX1_LNA_BYPASS             |
 * | GPIO2_24 | AGC_EN[5]   | TRX1_SW1_CTRL              |
 * | GPIO2_25 | AGC_EN[6]   | RX1_SW_CTRL                |
 * | GPIO2_26 | AGC_EN[7]   | TX2_PA_EN                  |
 * | GPIO2_27 | SPI_TRIG[5] | RX2_LNA_BYPASS             |
 * | GPIO2_28 | SPI_TRIG[6] | TRX2_SW1_CTRL              |
 * | GPIO2_29 | SPI_TRIG[7] | RX2_SW_CTRL                |
 * |          |             |                            |
 * | GPIO2_18 | GP_EVENT[7] | TRX_YC2 tx/rx switch       |
 * | GPIO2_9  | JESD_SRX[2] | TX1_PA_EN                  |
 * | GPIO2_10 | JESD_SRS[3] | RX1_LNA_BYPASS             |
 * | GPIO2_11 | GP_EVENT[0] | TRX1_SW1_CTRL              |
 * | GPIO2_12 | GP_EVENT[1] | RX1_SW_CTRL                |
 * | GPIO2_13 | GP_EVENT[2] | TX2_PA_EN                  |
 * | GPIO2_14 | GP_EVENT[3] | RX2_LNA_BYPASS             |
 * | GPIO2_15 | GP_EVENT[4] | TRX2_SW1_CTRL              |
 * | GPIO2_16 | GP_EVENT[5] | RX2_SW_CTRL                |
 * |_____________________________________________________|
 *
 *                   ________________
 *  TRX_YC1                          |_____ _ _ _
 *                   ______________
 *  YC1_TX1_PA_LNA                 |______ _ _ _
 *                   _______________
 *  YC1_TRX1                        |_____ _ _ _

 */

#ifndef MW_GPIO_TEST
static rf_ctrl_tbgen_signal_t rf_fem_controls[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC1        [17]", TBGEN_1, GPE,           TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_LNA [22]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_3, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX1_SW    [24]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_5, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
	},
	[LS_DCS0_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC1        [17]", TBGEN_1, GPE,           TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TX2_PA_LNA [26]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX2_SW    [28]", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
	},
	[LS_DCS1_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC2        [18]", TBGEN_1, GPE,           TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TX1_PA_LNA [ 9]", TBGEN_1, SRX_ALIGNMENT, TIMER_INSTANCE_2, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX1_SW    [11]", TBGEN_1, GPE,           TIMER_INSTANCE_0, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
	},
	[LS_DCS1_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC2        [18]", TBGEN_1, GPE,           TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TX2_PA_LNA [13]", TBGEN_1, GPE,           TIMER_INSTANCE_2, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX2_SW    [15]", TBGEN_1, GPE,           TIMER_INSTANCE_4, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
	},
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};

static rf_ctrl_tbgen_signal_t rf_fem_controls_tdd[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC       [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TX_PA_LNA [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TRX_SW    [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		//RF_CTRL_SIGNAL_ENTRY("YC_DPD       [GPIO_3_11]", TBGEN_2, TDD, TX_INSTANCE_7, STROBE_POL_RISING,  0,   0 * TBGEN_100NS),
	},
	[LS_DCS0_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC       [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TX_PA_LNA [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TRX_SW    [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		//RF_CTRL_SIGNAL_ENTRY("YC_DPD       [GPIO_3_11]", TBGEN_2, TDD, TX_INSTANCE_7, STROBE_POL_RISING,  0,   0 * TBGEN_100NS),
	},
	[LS_DCS1_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC       [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TX_PA_LNA [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TRX_SW    [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		//RF_CTRL_SIGNAL_ENTRY("YC_DPD       [GPIO_3_11]", TBGEN_2, TDD, TX_INSTANCE_7, STROBE_POL_RISING,  0,   0 * TBGEN_100NS),
	},
	[LS_DCS1_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("TRX_YC       [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TX_PA_LNA [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC_TRX_SW    [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		//RF_CTRL_SIGNAL_ENTRY("YC_DPD       [GPIO_3_11]", TBGEN_2, TDD, TX_INSTANCE_7, STROBE_POL_RISING,  0,   0 * TBGEN_100NS),
	},
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};
#else
static rf_ctrl_tbgen_signal_t rf_fem_controls[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = { },
	[LS_DCS0_IF1] = { },
	[LS_DCS1_IF0] = { },
	[LS_DCS1_IF1] = { },
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};

static rf_ctrl_tbgen_signal_t rf_fem_controls_tdd[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = { },
	[LS_DCS0_IF1] = { },
	[LS_DCS1_IF0] = { },
	[LS_DCS1_IF1] = { },
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};
#endif

rf_ctrl_gpio_signal_t rf_fem_controls_gpio[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
#ifdef MW_GPIO_TEST
#ifdef MW_REVA
		GPIO_CTRL_SIGNAL_ENTRY("TRX_YC1        [GPIO_2_17]", GPIO_2, 17, STROBE_POL_RISING, 0, -10 * TBGEN_100NS),
		GPIO_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_LNA [GPIO_2_22]", GPIO_2, 22, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
		GPIO_CTRL_SIGNAL_ENTRY("YC1_TRX1_SW    [GPIO_2_24]", GPIO_2, 24, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
#else
		GPIO_CTRL_SIGNAL_ENTRY("TRX_YC1        [GPIO_3_8]",  GPIO_3,  8, STROBE_POL_RISING, 0, -10 * TBGEN_100NS),
		GPIO_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_LNA [GPIO_3_9]",  GPIO_3,  9, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
		GPIO_CTRL_SIGNAL_ENTRY("YC1_TRX1_SW    [GPIO_3_10]", GPIO_3, 10, STROBE_POL_RISING, 0, -11 * TBGEN_100NS),
#endif
#endif
	},
	[LS_DCS0_IF1] = {
	},
	[LS_DCS1_IF0] = {
	},
	[LS_DCS1_IF1] = {
	},
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};
#endif

#if defined (DIORA_RF)
/* 
 * support of Diora board - 
 * polarity of ls0-trx1 and ls1-trx2 set to STROBE_POL_RISING for Diora boards v#1 and v#2
 * polarity of ls0-tx1  and ls1-tx2  MUST set to STROBE_POL_RISING for Diora boards v#1 (!! not the same polarity 
 * on Diora boards v#1 and v#2 !!)
 * 
 */
#define DIORA_V1 0
#if DIORA_V1
#define TX_POLARITY STROBE_POL_RISING
#else
#define TX_POLARITY STROBE_POL_FALLING
#endif

#endif

#if defined(GEUL_LA1224)
/*
 *      Signal      Description      GPIO info      GP timer    RF pin      GPIO info LS10465GRU
 *  __________________________________________________________________
 * |TX1_PA_EN     | PA1 enable   | LS_GPIO_2[8]  |     n/a     |      |		LS_GPIO_2[22]
 * |RX1_LNA_EN    | LNA1 enable  | LS_GPIO_2[9]  |srx_strobe[2]|      |		LS_GPIO_2[23]
 * |TRX1_SW       | TRX1 Switch  | LS_GPIO_2[12] | gp_event[1] |      |		LS_GPIO_2[24]
 * |              |              |               |             |      |
 * |TX2_PA_EN     | PA2 enable   | LS_GPIO_2[16] | gp_event[5] |  76  |		LS_GPIO_2[26]
 * |RX2_LNA_EN    | LNA2 enable  | LS_GPIO_2[15] | gp_event[4] |  74  |		LS_GPIO_2[27]
 * |TRX2_SW       | TRX2 Switch  | LS_GPIO_2[13] | gp_event[2] |  70  |		LS_GPIO_2[17]
 * |______________|______________|_______________|_____________|______|
 * |              |              |               |             |      |
 * |TX2_PA_EN     | PA2 enable   | LS_GPIO_2[23] |    agc[4]   |      |		LS_GPIO_2[9]
 * |RX2_LNA_EN    | LNA2 enable  | LS_GPIO_2[24] |    agc[5]   |      |		LS_GPIO_2[10]
 * |TRX2_SW       | TRX2 Switch  | LS_GPIO_2[27] |    spi[5]   |      |		LS_GPIO_2[11]
 * |              |              |               |             |      |
 * |TX1_PA_EN     | PA1 enable   | LS_GPIO_2[31] |     n/a     |      |		LS_GPIO_2[13]
 * |RX1_LNA_EN    | LNA1 enable  | LS_GPIO_2[30] |     n/a     |      |		LS_GPIO_2[14]
 * |TRX1_SW       | TRX1 Switch  | LS_GPIO_2[28] |    spi[6]   |      |		LS_GPIO_2[18]
 * |______________|______________|_______________|_____________|______|
 *
 *                   ______________
 *  TX2_PA_EN     __|              |________ _ _ _
 *                __                ________ _ _ _
 *  RX2_LNA_EN      |______________|
 *                   ______________
 *  TRX2_SW       __|              |________ _ _ _
 *
 */

/* DIORA- --
 * On GEUL_LA1224 rev A/B boards, lsx_tx1 (lsx_rx1) and lsx_tx2 (lsx_rx2) are welded together 
 * Hardware workaround 
 */
static rf_ctrl_tbgen_signal_t rf_fem_controls_revAB[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = { /* some GPIOs not controlled by TBGEN */
		RF_CTRL_SIGNAL_ENTRY("ls0-trx1", TBGEN_1, GPE,           TIMER_INSTANCE_1, STROBE_POL_FALLING, 0 * TBGEN_100NS,  -2 * TBGEN_100NS),
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("ls0-tx1",  TBGEN_1, GPE,           TIMER_INSTANCE_5, TX_POLARITY, 0 * TBGEN_100NS, -11 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("ls0-rx1",  TBGEN_1, SRX_ALIGNMENT, TIMER_INSTANCE_2, STROBE_POL_RISING,  0 * TBGEN_100NS,   0 * TBGEN_100NS),
	},
	[LS_DCS0_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("ls0-trx2", TBGEN_1, GPE,           TIMER_INSTANCE_2, STROBE_POL_FALLING, 0 * TBGEN_100NS,  -2 * TBGEN_100NS),
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("ls0-tx2",  TBGEN_1, GPE,           TIMER_INSTANCE_5, TX_POLARITY, 0 * TBGEN_100NS, -11 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("ls0-tx2",  TBGEN_1, GPE,           TIMER_INSTANCE_5, STROBE_POL_FALLING, 0 * TBGEN_100NS, -11 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("ls0-rx2",  TBGEN_1, GPE,           TIMER_INSTANCE_4, STROBE_POL_RISING,  0 * TBGEN_100NS,   0 * TBGEN_100NS),
	},
	[LS_DCS1_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("ls1-trx1", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_5, STROBE_POL_FALLING, 0 * TBGEN_100NS,  -2 * TBGEN_100NS),
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("ls1-tx1",  TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_4, TX_POLARITY, 0 * TBGEN_100NS, -11 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("ls1-tx1",  TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_4, STROBE_POL_FALLING, 0 * TBGEN_100NS, -11 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("ls1-rx1",  TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_5, STROBE_POL_RISING,  0 * TBGEN_100NS,   0 * TBGEN_100NS),
	},
	[LS_DCS1_IF1] = { /* some GPIOs not controlled by TBGEN */
		RF_CTRL_SIGNAL_ENTRY("ls1-trx2", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_6, STROBE_POL_FALLING, 0 * TBGEN_100NS,  -2 * TBGEN_100NS),
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("ls1-tx2",  TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_4, TX_POLARITY, 0 * TBGEN_100NS, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("ls1-rx2",  TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_5, STROBE_POL_RISING,  0 * TBGEN_100NS,   0 * TBGEN_100NS),
#endif
	},

	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};

rf_ctrl_gpio_signal_t rf_fem_controls_gpio_revAB[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
#if !defined (DIORA_RF)
		GPIO_CTRL_SIGNAL_ENTRY("ls0-tx1", GPIO_2,  8, STROBE_POL_FALLING, 0, -11 * TBGEN_100NS),
#endif
	},
	[LS_DCS0_IF1] = {
	},
	[LS_DCS1_IF0] = {
	},
	[LS_DCS1_IF1] = {
#if !defined (DIORA_RF)
		GPIO_CTRL_SIGNAL_ENTRY("ls1-tx2", GPIO_2, 31, STROBE_POL_FALLING, 0, -11 * TBGEN_100NS),
		GPIO_CTRL_SIGNAL_ENTRY("ls1-rx2", GPIO_2, 30, STROBE_POL_RISING,  0,   0 * TBGEN_100NS),
#endif
	},
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};

/*
 *        Signal          GPIO info     TBGEN timer
 *  ________________________________________________
 * |YC1_TX1_PA_EN     | LS_GPIO_2[22] |    agc[3]   |
 * |YC1_TRX1_SW       | LS_GPIO_2[25] |    agc[6]   |
 * |YC1_RX1_SW        | LS_GPIO_2[24] |    agc[5]   |
 * |                  |               |             |
 * |YC1_TX2_PA_EN     | LS_GPIO_2[26] |    agc[7]   |
 * |YC1_TRX2_SW       | LS_GPIO_2[28] |    spi[6]   |
 * |YC1_RX2_SW        | LS_GPIO_2[29] |    spi[7]   |
 * |__________________|_______________|_____________|
 * |                  |               |             |
 * |YC2_TX1_PA_EN     | LS_GPIO_2[13] | gp_event[2] |
 * |YC2_TRX1_SW       | LS_GPIO_2[15] | gp_event[4] |
 * |YC2_RX1_SW        | LS_GPIO_2[16] | gp_event[5] |
 * |                  |               |             |
 * |YC2_TX2_PA_EN     | LS_GPIO_2[9]  |srx_strobe[2]|
 * |YC2_TRX2_SW       | LS_GPIO_2[11] | gp_event[0] |
 * |YC2_RX2_SW        | LS_GPIO_2[12] | gp_event[1] |
 * |__________________|_______________|_____________|
 *
 */

static rf_ctrl_tbgen_signal_t rf_fem_controls_revC[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [22]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_3, TX_POLARITY, 0, 0 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [22]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_3, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX1_SW1_CTRL [24]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_5, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX           [17]", TBGEN_1, GPE,           TIMER_INSTANCE_6, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
	},
	[LS_DCS0_IF1] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC1_TX2_PA_EN     [26]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_7, TX_POLARITY, 0, 0 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [26]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_7, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX2_SW1_CTRL [28]", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_6, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX           [17]", TBGEN_1, GPE,           TIMER_INSTANCE_6, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
	},
	[LS_DCS1_IF0] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC2_TX1_PA_LNA    [13]", TBGEN_1, GPE,           TIMER_INSTANCE_2, TX_POLARITY, 0, 0 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX1_SW1_CTRL [15]", TBGEN_1, GPE,           TIMER_INSTANCE_4, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),		
#else
		RF_CTRL_SIGNAL_ENTRY("YC2_TX1_PA_LNA    [ 9]", TBGEN_1, SRX_ALIGNMENT, TIMER_INSTANCE_2, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX1_SW1_CTRL [11]", TBGEN_1, GPE,           TIMER_INSTANCE_0, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX           [18]", TBGEN_1, GPE,           TIMER_INSTANCE_7, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
	},
	[LS_DCS1_IF1] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC2_TX2_PA_LNA    [ 9]", TBGEN_1, SRX_ALIGNMENT, TIMER_INSTANCE_2, TX_POLARITY, 0, 0 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX2_SW1_CTRL [11]", TBGEN_1, GPE,           TIMER_INSTANCE_0, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),		
#else
		RF_CTRL_SIGNAL_ENTRY("YC2_TX2_PA_LNA    [13]", TBGEN_1, GPE,           TIMER_INSTANCE_2, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX2_SW1_CTRL [15]", TBGEN_1, GPE,           TIMER_INSTANCE_4, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX           [18]", TBGEN_1, GPE,           TIMER_INSTANCE_7, STROBE_POL_RISING, 0, 0 * TBGEN_100NS),
	},
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};

rf_ctrl_gpio_signal_t rf_fem_controls_gpio_revC[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = { },
	[LS_DCS0_IF1] = { },
	[LS_DCS1_IF0] = { },
	[LS_DCS1_IF1] = { },
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};

static rf_ctrl_tbgen_signal_t rf_fem_controls_tdd[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("TRX     [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("PA_LNA  [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("TRX_SW  [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
	},
	[LS_DCS0_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("TRX     [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("PA_LNA  [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("TRX_SW  [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
	},
	[LS_DCS1_IF0] = {
		RF_CTRL_SIGNAL_ENTRY("TRX     [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("PA_LNA  [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("TRX_SW  [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
	},
	[LS_DCS1_IF1] = {
		RF_CTRL_SIGNAL_ENTRY("TRX     [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_RISING,  0, -10 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("PA_LNA  [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("TRX_SW  [GPIO_3_10]", TBGEN_2, TDD, TX_INSTANCE_6, STROBE_POL_RISING,  0, -11 * TBGEN_100NS),
	},
	[HS_DCS_IF0]  = { },
	[HS_DCS_IF1]  = { },
};
#endif

#define RF_NUM_SIGNALS	ARRAY_SIZE

rf_ctrl_tbgen_signal_t * l1c_get_rf_fem_controls(uint8_t interface)
{
#if defined(GEUL_LA1224)
	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			return rf_fem_controls_revC[interface];
		case GEUL_HOST_REVA_VAL:
		case GEUL_HOST_REVB_VAL:
		default:
			return rf_fem_controls_revAB[interface];
	}
#else
	return rf_fem_controls[interface];
#endif
}

rf_ctrl_tbgen_signal_t * l1c_get_rf_tdd_fem_controls(uint8_t interface)
{
	return rf_fem_controls_tdd[interface];
}

rf_ctrl_gpio_signal_t * l1c_get_rf_gpio_fem_controls(uint8_t interface)
{
#if defined(GEUL_LA1224)
	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			return rf_fem_controls_gpio_revC[interface];
		case GEUL_HOST_REVA_VAL:
		case GEUL_HOST_REVB_VAL:
		default:
			return rf_fem_controls_gpio_revAB[interface];
	}
#else
	return rf_fem_controls_gpio[interface];
#endif
}

#if defined(GEUL_LA1224)
void l1c_gpio_pmux_setup()
{
	GpioStatusCode_t ret = GPIO_SUCCESS;
	rf_ctrl_tbgen_signal_t *fem_ctrl = NULL;

	for (uint8_t dcs = 0; dcs < MAX_DCS_INTERFACES; dcs++)
	{
		/* non-TDD timer PMUX settings */
		fem_ctrl = l1c_get_rf_fem_controls(dcs);

		for (uint8_t iter = 0; iter < MAX_RF_CTRL_SIGNALS; iter++)
		{
			if (fem_ctrl[iter].in_use)
				iConfPMuxModeTbgen(fem_ctrl[iter].tbgen, fem_ctrl[iter].timer_type, fem_ctrl[iter].timer_instance);
		}
	}

	/* configure pmux to allow tbgen tick on L1_CORE_3 as well */
	switchPMuxMode(PMUX_7, PMUX7_IRQ_4, ALT_MODE1);

	/* specific init here */
	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			break;
		case GEUL_HOST_REVA_VAL:
		case GEUL_HOST_REVB_VAL:
		default:
			/* LS-DCS0 - antenna #0 */
			ret |= exGpioInit(GPIO_2, 8, GPIO_OUTPUT);
			/* LS-DCS1 - antenna #1 */
			ret |= exGpioInit(GPIO_2, 30, GPIO_OUTPUT);
			ret |= exGpioInit(GPIO_2, 31, GPIO_OUTPUT);

			if (ret != GPIO_SUCCESS)
				PRINTF("GPIO init failed \r\n");

			break;
	}

#if 0
	/* Future GPIO3[8..11] TDD timer control */
	switchPMuxMode(PMUX_5, PMUX5_GPIO_3_8, ALT_MODE2);
	switchPMuxMode(PMUX_5, PMUX5_GPIO_3_9, ALT_MODE2);
	switchPMuxMode(PMUX_5, PMUX5_GPIO_3_10, ALT_MODE2);
	//switchPMuxMode(PMUX_5, PMUX5_GPIO_3_11, ALT_MODE2);
#endif
}
#endif /* GEUL_LA1224 */

#if defined(GEUL_LA1238RDB)
void l1c_gpio_pmux_setup()
{
#ifdef MW_GPIO_TEST
	GpioStatusCode_t ret = GPIO_SUCCESS;
#ifdef MW_REVA
	ret |= exGpioInit(GPIO_2, 17, GPIO_OUTPUT);
	ret |= exGpioInit(GPIO_2, 22, GPIO_OUTPUT);
	ret |= exGpioInit(GPIO_2, 24, GPIO_OUTPUT);
#else /* MW_REVB*/
	ret |= exGpioInit(GPIO_3,  8, GPIO_OUTPUT);
	ret |= exGpioInit(GPIO_3,  9, GPIO_OUTPUT);
	ret |= exGpioInit(GPIO_3, 10, GPIO_OUTPUT);
#endif
	if (ret != GPIO_SUCCESS)
		PRINTF("GPIO init failed \r\n");
#else
	rf_ctrl_tbgen_signal_t *fem_ctrl = NULL;

	/* PMUX settings */
	for (uint8_t dcs = 0; dcs < MAX_DCS_INTERFACES; dcs++)
	{
		/* non-TDD timer PMUX settings */
		fem_ctrl = l1c_get_rf_fem_controls(dcs);

		for (uint8_t iter = 0; iter < MAX_RF_CTRL_SIGNALS; iter++)
		{
			if (fem_ctrl[iter].in_use)
				iConfPMuxModeTbgen(fem_ctrl[iter].tbgen, fem_ctrl[iter].timer_type, fem_ctrl[iter].timer_instance);
		}

		/* TDD timer PMUX settings */
		fem_ctrl = l1c_get_rf_tdd_fem_controls(dcs);

		for (uint8_t iter = 0; iter < MAX_RF_CTRL_SIGNALS; iter++)
		{
			if (fem_ctrl[iter].in_use)
				iConfPMuxModeTbgenTdd(fem_ctrl[iter].timer_type, fem_ctrl[iter].timer_instance);
		}
	}
#endif /* MW_GPIO_TEST */

	/* configure pmux to allow tbgen tick on L1_CORE_3 as well */
	switchPMuxMode(PMUX_7, PMUX7_IRQ_4, ALT_MODE1);
}
#endif /* GEUL_LA1238RDB */

void l1c_rf_ctrl_sig_setup(rf_ctrl_tbgen_signal_t *controls, u64 transition_time)
{
	uint8_t i;

	memset(&ctrl_sig_params, 0, sizeof(ctrl_sig_params));

	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++)
	{
		if (unlikely(!controls[i].desc))
			break;

		/* we are doing non-TDD operations here */
		if (controls[i].timer_type == TDD)
			continue;

#ifdef L1C_REFAPP_DEBUG
		PRINTF("rf_fem_control[%d]: %s set to %d\r\n", i, controls[i].desc, controls[i].polarity);
#endif
		ctrl_sig_params.eTrigMode = TM_ONE_SHOT;
		ctrl_sig_params.eSm = STROBE_MODE_TOGGLE;
		ctrl_sig_params.ePolarity = controls[i].polarity;
		ctrl_sig_params.uOffset = transition_time;

		iTbgenProgramTimer(controls[i].tbgen, controls[i].timer_type, controls[i].timer_instance, &ctrl_sig_params);
		iTbgenEnableTimer(controls[i].tbgen, controls[i].timer_type, controls[i].timer_instance);
	}
}

void l1c_rf_ctrl_sig_transition(rf_ctrl_tbgen_signal_t *controls, u64 transition_time, bool_t tx_to_rx)
{
	u32 i;

	/* check if rf_fem ctrl list is empty and exit early */
	if (!controls[0].in_use)
		return;

	for (i = 0; i < MAX_RF_CTRL_SIGNALS; i++)
	{
		/* no more signals to control */
		if (!controls[i].in_use)
			break;

		/* we are doing non-TDD operations here */
		if (controls[i].timer_type == TDD)
			continue;

		/* avoid glitches in TBGEN signals when programming transitions */
		if ((tx_to_rx && !controls[i].tx_on) || (!tx_to_rx && controls[i].tx_on))
			continue;

		controls[i].tx_on = !tx_to_rx;

		iTbgenReloadTimerAndPolarity(controls[i].tbgen,
									 controls[i].timer_type,
									 controls[i].timer_instance,
									 ((tx_to_rx) ? controls[i].polarity : !controls[i].polarity),
									 transition_time + ((tx_to_rx) ? controls[i].tx_rx_delta : controls[i].rx_tx_delta));
	}

	return;
}
