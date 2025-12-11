// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef __GEUL_H__
#define __GEUL_H__

#include "immap.h"
#include "config.h"

#define SYS_CLK_MULTIPLIER 5
#define SYS_CLK_FREQ         122880000  // 122.88 MHz
#define PLAT_FREQ       (SYS_CLK_FREQ * SYS_CLK_MULTIPLIER) // 614.4 MHz
#define TIMER_COUNTER			0x00001000
#define TICK_FREQ			1000 // In Hz

#define UART_BAUDRATE			115200
#define UART_CLOCK_FREQUENCY		PLAT_FREQ

#define I2C_CLK_FREQ_DIV             8
#define I2C_FREQ			         100000
#define I2C_CLK_FREQ                 (PLAT_FREQ / I2C_CLK_FREQ_DIV)

#define STATS_INTERRUPT_RAISED		0
#define GEUL_E200_CORE_GLOBAL_NUM GUL_EP_CORE_MAX

#define MW_REVA_VERSION			0x00
#define MW_REVB_VERSION			0x01

#endif /* __GEUL_H__ */
