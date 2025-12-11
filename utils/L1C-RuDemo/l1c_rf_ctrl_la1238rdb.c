/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2023 NXP */

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
static rf_ctrl_tbgen_signal_t rf_fem_controls[MAX_DCS_INTERFACES][MAX_RF_CTRL_SIGNALS] __attribute__ ((section (".shared.data")))= {
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
#endif /*MW_GPIO_TEST */

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
#endif  /* MW_REVA */
#endif /* MW_GPIO_TEST*/
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

rf_ctrl_tbgen_signal_t * l1c_get_rf_fem_controls(uint8_t interface)
{
	return rf_fem_controls[interface];
}

rf_ctrl_tbgen_signal_t * l1c_get_rf_tdd_fem_controls(uint8_t interface)
{
	return rf_fem_controls_tdd[interface];
}

rf_ctrl_gpio_signal_t * l1c_get_rf_gpio_fem_controls(uint8_t interface)
{
	return rf_fem_controls_gpio[interface];
}

l1c_trig_out_signal_t * l1c_get_trig_out_controls(int trig_num)
{
	return NULL;
}

#endif /* GEUL_LA1238RDB */
