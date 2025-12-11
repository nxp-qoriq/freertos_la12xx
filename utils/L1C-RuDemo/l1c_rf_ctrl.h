/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#ifndef __L1C_RF_CTRL_H
#define __L1C_RF_CTRL_H

#include "tbgen_new.h"
#include "la12xx_tbgen.h"
#include "gpio.h"

#define TDD_RX              0x10
#define TDD_TX              0x20
#define TDD_TX_RX_MASK      0x30
#define TDD_INSTANCE_MASK   0x07 /* 0..7 */

typedef enum {
	/* use these defines to distinguish between GPIOs associated with either tx_enable or rx_enable signals */
	TX_INSTANCE_0 = (TIMER_INSTANCE_0 | TDD_TX),
	TX_INSTANCE_1 = (TIMER_INSTANCE_1 | TDD_TX),
	TX_INSTANCE_2 = (TIMER_INSTANCE_2 | TDD_TX),
	TX_INSTANCE_3 = (TIMER_INSTANCE_3 | TDD_TX),
	TX_INSTANCE_4 = (TIMER_INSTANCE_4 | TDD_TX),
	TX_INSTANCE_5 = (TIMER_INSTANCE_5 | TDD_TX),
	TX_INSTANCE_6 = (TIMER_INSTANCE_6 | TDD_TX),
	TX_INSTANCE_7 = (TIMER_INSTANCE_7 | TDD_TX),
	/* rx */
	RX_INSTANCE_0 = (TIMER_INSTANCE_0 | TDD_RX),
	RX_INSTANCE_1 = (TIMER_INSTANCE_1 | TDD_RX),
	RX_INSTANCE_2 = (TIMER_INSTANCE_2 | TDD_RX),
	RX_INSTANCE_3 = (TIMER_INSTANCE_3 | TDD_RX),
	RX_INSTANCE_4 = (TIMER_INSTANCE_4 | TDD_RX),
	RX_INSTANCE_5 = (TIMER_INSTANCE_5 | TDD_RX),
	RX_INSTANCE_6 = (TIMER_INSTANCE_6 | TDD_RX),
	RX_INSTANCE_7 = (TIMER_INSTANCE_7 | TDD_RX),
} tbgen_tx_rx_instance;

typedef enum {
	LS_DCS0_IF0 = 0,
	LS_DCS0_IF1,
	LS_DCS1_IF0,
	LS_DCS1_IF1,
	HS_DCS_IF0,
	HS_DCS_IF1,
	MAX_DCS_INTERFACES
} dcs_interfaces_t;

#define MAX_RF_CTRL_SIGNALS   6

typedef struct {
	const char *desc;
	uint8_t tbgen;
	TimerType_t timer_type;
	uint8_t timer_instance;
	uint8_t polarity; /* required polarity in TX operation */
	int tx_rx_delta;  /* [ +/- tbgen counts] */
	int rx_tx_delta;  /* [ +/- tbgen counts] */
	bool_t in_use;
	bool_t tx_on;
} rf_ctrl_tbgen_signal_t;

#define IN_USE	1
#define RF_CTRL_SIGNAL_ENTRY(desc, tbgen, timer_type, timer_inst, polarity, tx_rx_delta, rx_tx_delta) \
				{ (desc), (tbgen), (timer_type), (timer_inst), (polarity), (tx_rx_delta), (rx_tx_delta), IN_USE, 1 }

typedef struct {
	const char *desc;
	GpioModule_t gpio;
	uint8_t pin;
	uint8_t polarity; /* required polarity in TX operation */
	int tx_rx_delta;  /* [ +/- tbgen counts] */
	int rx_tx_delta;  /* [ +/- tbgen counts] */
	bool_t in_use;
	bool_t tx_on;
} rf_ctrl_gpio_signal_t;

enum {
	TRIG_OUT_1 = 0,
	TRIG_OUT_2 = 1,
	TX_ENABLE = 2,
	RX_ENABLE = 3,
	NUM_TRIG_OUT = 4
};

typedef struct {
	const char *desc;
	uint8_t tbgen;
	TimerType_t timer_type;
	uint8_t timer_instance;
} l1c_trig_out_signal_t;

#define L1C_TRIG_OUT_SIGNAL_ENTRY(desc, tbgen, timer_type, timer_inst) \
				{ (desc), (tbgen), (timer_type), (timer_inst) }

#define GPIO_CTRL_SIGNAL_ENTRY(desc, gpio, pin, polarity, tx_rx_delta, rx_tx_delta) \
				{ (desc), (gpio), (pin), (polarity), (tx_rx_delta), (rx_tx_delta), IN_USE, 1 }

void l1c_gpio_pmux_setup();

void l1c_rf_ctrl_sig_setup(rf_ctrl_tbgen_signal_t *ctrl_sig, u64 start_offset);
void l1c_rf_ctrl_sig_transition(rf_ctrl_tbgen_signal_t *ctrl_sig, u64 start_offset, bool_t tx_to_rx);

rf_ctrl_tbgen_signal_t* l1c_get_rf_fem_controls(uint8_t interface);
rf_ctrl_gpio_signal_t* l1c_get_rf_gpio_fem_controls(uint8_t interface);
rf_ctrl_tbgen_signal_t* l1c_get_rf_tdd_fem_controls(uint8_t interface);
int l1c_trig_out_setup(int trig_num, uint64_t start_time);
#endif
