/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2023-2024 NXP */

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
#include "l1c_axiq.h"
#include "l1c_rf_ctrl.h"


extern volatile uint32_t brd_ver;


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

#endif /* DIORA_RF */


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
 * |YC1_RX1_DPD_SW    | LS_GPIO_2[25] |    agc[6]   |
 * |YC1_RX1_SW        | LS_GPIO_2[24] |    agc[5]   |
 * |                  |               |             |
 * |YC1_TX2_PA_EN     | LS_GPIO_2[26] |    agc[7]   |
 * |YC1_RX2_DPD_SW    | LS_GPIO_2[29] |    spi[6]   |
 * |YC1_RX2_SW        | LS_GPIO_2[28] |    spi[7]   |
 * |__________________|_______________|_____________|
 * |                  |               |             |
 * |YC2_TX1_PA_EN     | LS_GPIO_2[13] | gp_event[2] |
 * |YC2_TRX1_SW       | LS_GPIO_2[15] | gp_event[4] |
 * |YC2_RX1_DPD_SW    | LS_GPIO_2[16] | gp_event[5] |
 * |                  |               |             |
 * |YC2_TX2_PA_EN     | LS_GPIO_2[9]  |srx_strobe[2]|
 * |YC2_TRX2_SW       | LS_GPIO_2[11] | gp_event[0] |
 * |YC2_RX2_DPD_SW    | LS_GPIO_2[12] | gp_event[1] |
 * |__________________|_______________|_____________|
 *
 */

static rf_ctrl_tbgen_signal_t rf_fem_controls_revC[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [22]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_3, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_RX1_DPD_SW    [25]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_6, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [22]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_3, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX           [17]", TBGEN_1, GPE,           TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_RX1_SW_CTRL   [25]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX1_SW1_CTRL [24]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_5, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
	},
	[LS_DCS0_IF1] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [26]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_7, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_RX2_DPD_SW    [29]", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_7, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("YC1_TX1_PA_EN     [26]", TBGEN_1, AGC_ENABLE,    TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX           [17]", TBGEN_1, GPE,           TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC1_RX2_SW_CTRL   [29]", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
#endif
		RF_CTRL_SIGNAL_ENTRY("YC1_TRX2_SW1_CTRL [28]", TBGEN_1, SPI_TRIGGER,   TIMER_INSTANCE_6, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
	},
	[LS_DCS1_IF0] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC2_TX1_PA_LNA    [13]", TBGEN_1, GPE,           TIMER_INSTANCE_2, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX1_SW1_CTRL [15]", TBGEN_1, GPE,           TIMER_INSTANCE_4, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_RX1_DPD_SW    [16]", TBGEN_1, GPE,           TIMER_INSTANCE_5, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("YC2_TX1_PA_LNA    [ 9]", TBGEN_1, SRX_ALIGNMENT, TIMER_INSTANCE_2, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX1_SW1_CTRL [11]", TBGEN_1, GPE,           TIMER_INSTANCE_0, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX           [18]", TBGEN_1, GPE,           TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_RX1_SW_CTRL   [16]", TBGEN_1, GPE,           TIMER_INSTANCE_5, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
#endif
	},
	[LS_DCS1_IF1] = {
#if defined (DIORA_RF)
		RF_CTRL_SIGNAL_ENTRY("YC2_TX2_PA_LNA    [ 9]", TBGEN_1, SRX_ALIGNMENT, TIMER_INSTANCE_2, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX2_SW1_CTRL [11]", TBGEN_1, GPE,           TIMER_INSTANCE_0, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_RX2_DPD_SW    [12]", TBGEN_1, GPE,           TIMER_INSTANCE_1, STROBE_POL_FALLING, 0, -2 * TBGEN_100NS),
#else
		RF_CTRL_SIGNAL_ENTRY("YC2_TX2_PA_LNA    [13]", TBGEN_1, GPE,           TIMER_INSTANCE_2, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX2_SW1_CTRL [15]", TBGEN_1, GPE,           TIMER_INSTANCE_4, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_TRX           [18]", TBGEN_1, GPE,           TIMER_INSTANCE_7, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("YC2_RX2_SW_CTRL   [12]", TBGEN_1, GPE,           TIMER_INSTANCE_1, STROBE_POL_RISING, 0, -2 * TBGEN_100NS),
#endif
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

static rf_ctrl_tbgen_signal_t rf_fem_controls_tdd_revC[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data"))) = {
	[LS_DCS0_IF0] = { },
	[LS_DCS0_IF1] = { },
	[LS_DCS1_IF0] = { },
	[LS_DCS1_IF1] = { },

	[HS_DCS_IF0]  = { 
		RF_CTRL_SIGNAL_ENTRY("TX     [GPIO_3_8]",  TBGEN_2, TDD, TX_INSTANCE_4, STROBE_POL_FALLING,  0, -2 * TBGEN_100NS),
		RF_CTRL_SIGNAL_ENTRY("RX     [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5, STROBE_POL_RISING,  0, -2 * TBGEN_100NS),
	},
	[HS_DCS_IF1]  = { 
	},
};

static l1c_trig_out_signal_t trig_out_revC[4] __attribute__ ((section (".shared.data"))) = {
	[TRIG_OUT_1]  = L1C_TRIG_OUT_SIGNAL_ENTRY("TRIG_OUT_1  [GPIO_2_20]", TBGEN_1, AGC_ENABLE, TIMER_INSTANCE_1),
	[TRIG_OUT_2]  = L1C_TRIG_OUT_SIGNAL_ENTRY("TRIG_OUT_2  [GPIO_3_3]",  TBGEN_2, AXRF, TIMER_INSTANCE_5),
	//[TRIG_OUT_2]  = L1C_TRIG_OUT_SIGNAL_ENTRY("TRIG_OUT_2     [GPIO_3_9]",  TBGEN_2, TDD, TX_INSTANCE_5),

#ifdef LA12XX_DRIVER_PCI_LAT_FP
	[TX_ENABLE]  = L1C_TRIG_OUT_SIGNAL_ENTRY("TX_ENABLE    [GPIO_2_17]",  TBGEN_1, GPE, TIMER_INSTANCE_6),
	[RX_ENABLE]  = L1C_TRIG_OUT_SIGNAL_ENTRY("RX_ENABLE    [GPIO_2_22]",  TBGEN_1, AGC_ENABLE, TIMER_INSTANCE_3),
#endif
};

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

		/* TDD timer PMUX settings */
		fem_ctrl = l1c_get_rf_tdd_fem_controls(dcs);

		for (uint8_t iter = 0; iter < MAX_RF_CTRL_SIGNALS; iter++)
		{
			if (fem_ctrl[iter].in_use) {
				TimerInstance_t timer_instance = fem_ctrl[iter].timer_instance & TDD_INSTANCE_MASK;
				uint8_t ucTxRx = (fem_ctrl[iter].timer_instance & TDD_TX_RX_MASK) >> 4;
				ucTxRx = ucTxRx/2;

				iConfPMuxModeTbgenTdd(timer_instance, ucTxRx);
			}
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
rf_ctrl_tbgen_signal_t * l1c_get_rf_fem_controls(uint8_t interface)
{
	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			return rf_fem_controls_revC[interface];
		case GEUL_HOST_REVA_VAL:
		case GEUL_HOST_REVB_VAL:
		default:
			return rf_fem_controls_revAB[interface];
	}
}

rf_ctrl_tbgen_signal_t * l1c_get_rf_tdd_fem_controls(uint8_t interface)
{
	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			return rf_fem_controls_tdd_revC[interface];
		default:
			return NULL;
	}
}


rf_ctrl_gpio_signal_t * l1c_get_rf_gpio_fem_controls(uint8_t interface)
{
	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			return rf_fem_controls_gpio_revC[interface];
		case GEUL_HOST_REVA_VAL:
		case GEUL_HOST_REVB_VAL:
		default:
			return rf_fem_controls_gpio_revAB[interface];
	}
}

l1c_trig_out_signal_t * l1c_get_trig_out_controls(int trig_num)
{
	if (trig_num > NUM_TRIG_OUT - 1)
		return NULL;

	switch (brd_ver)
	{
		case GEUL_HOST_REVC_VAL:
			return &trig_out_revC[trig_num];
		default:
			return NULL;
	}
}

#endif /* GEUL_LA1224 */
