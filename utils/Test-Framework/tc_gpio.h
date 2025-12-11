// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef _TEST_CASES_GPIO_H_
#define _TEST_CASES_GPIO_H_

#include "test_framework.h"
#include "gpio.h"

#if GEUL_DEMO_GPIO_TEST

void vGpioTest( void );
#endif
int32_t gpioLedTestCase( GpioModule_t ucGpioModule, uint8_t ucPin );

#endif
