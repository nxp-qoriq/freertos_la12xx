// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021,2023 NXP
 */

#ifndef  _TEST_CASES_LA12XX_TBGEN_H_
#define _TEST_CASES_LA12XX_TBGEN_H_

#include "test_framework.h"

#define TBGEN_TTI_INTERVAL_125		125
#define TBGEN_TTI_INTERVAL_250		250
#define TBGEN_TTI_INTERVAL_500		500
#define TBGEN_TTI_INTERVAL_1000	1000

#define TBGEN_TTI_INTERVAL_125_DIV		8
#define TBGEN_TTI_INTERVAL_250_DIV		4
#define TBGEN_TTI_INTERVAL_500_DIV		2
#define TBGEN_TTI_INTERVAL_1000_DIV		1
/**
 * Test API will Program Non TDD Timers of TBGEN1
 * GPE Instance 0 and 1 programmed in repetitive mode and expire after 125us
 * SPI_TRIGGER Instance 5,6,7 programmed in repetitive mode and expire after 125us
 * SRX_ALIGNMENT Instance 2 and 3 programmed in repetitive mode and expire after 125us
 * AGC_ENABLE Instance 0,1,2,3,4 programmed in repetitive mode and expire after 125us
 * AXRF Instance 2 and 3 programmed in ONE SHOT
 * TIMED_INT Instance 2 and 3 programmed in repetitive mode and expire after 125us
 */
void vTbgen1NonTddTimerTest();
/**
 * Test API will Program Non TDD Timers of TBGEN2
 * GPE Instance 0 and 1 programmed in repetitive mode and expire after 125us
 * SPI_TRIGGER Instance 5,6,7 programmed in repetitive mode and expire after 125us
 * SRX_ALIGNMENT Instance 2  programmed in repetitive mode and expire after 125us
 * AGC_ENABLE Instance 0,1,2,3,4 programmed in repetitive mode and expire after 125us
 * AXRF Instance 2 and 3 programmed in ONE SHOT
 * TIMED_INT Instance 2 and 3 programmed in repetitive mode and expire after 125us
 */
void vTbgen2NonTddTimerTest();
/**
 * Test API will generate Host TTI using Tbgen1 timer.
 * Which Tbgen1 Timer Type generate Host TTI events is platform specific
 * On ISC, AGC_ENABLE Timer Instance 2  will generate Host TTI.
 * On LA1224RDB, No Tbgen1 Timer generate Host TTI
 */
void vTbgen1HostTTIEventTest();
/**
 * Test API will generate Host TTI using Tbgen2 timer.
 * Which Tbgen2 Timer Type generate Host TTI events is platform specific
 * On ISC, GPE Timer Instance 2  will generate Host TTI.
 * On LA1224RDB, GPE Timer Instance 2 and 3 both generate Host TTI
 */
void vTbgen2HostTTIEventTest();
/*
 * Test API to configure the interrupt timing interval and
 * generate host tti
 */
void vConfigurable_host_tbgentti_interval_test(uint8_t, uint8_t);
/**
 * Test API will enable Tbgen1 RFG that generate pulse after every 10ms
 */
void vTbgen1RFGTest();
/**
 * Disable Tbgen1 RFG
 */
void vTbgen1DisableRFGTest();
/**
 * Test API will enable Tbgen2 RFG that generate pulse after every 10ms
 */
void vTbgen2RFGTest();
/**
 * Disable Tbgen2 RFG
 */
void vTbgen2DisableRFGTest();
/**
 * Test API will generate external interrupt using Tbgen1 Timer
 * Tbgen1 RX_ALIGNMENT Instance 0 generate interrupt after every 500us interval
 */
void vTbgen1RxAlignTimerTest();
/**
 * Test API will generate external interrupt using Tbgen2 Timer
 * Tbgen2 RX_ALIGNMENT Instance 0 generate interrupt after every 125us interval
 */
void vTbgen2RxAlignTimerTest();
/**
 * Test API will program Tbgen2 Timer Instance 4 with 16 TDD steps.
 */
void vTestTbgen2TddTimer();
/**
 * Test API will program Tbgen2 Timer all 8 instances in Manual mode
 */
void vTestTbgen2TddTimerManual();
#endif	/* _TEST_CASES_LA12XX_TBGEN_H_ */
