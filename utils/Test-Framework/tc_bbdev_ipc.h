// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#ifndef __GEUL_BBDEV_IPC_TEST_H__
#define __GEUL_BBDEV_IPC_TEST_H__

#include "FreeRTOS.h"
#include "test_framework.h"
#include "timers.h"

#define TEST_IPC_MAX_CHANNELS	8

/*Define the channel IDs*/
/*Channels from MAC towards L1C*/
#define L2_TO_L1_MSG_CH_1		0x00
#define L2_TO_L1_MSG_CH_2		0x01
#define L2_TO_L1_MSG_CH_3		0x02

/*Channels from L1C towards MAC*/
#define L1_TO_L2_MSG_CH_4		0x03
#define L1_TO_L2_MSG_CH_5		0x04

/*Pointer channels*/
#define L1_TO_L2_PRT_CH_1		0x05
#define L1_TO_L2_PRT_CH_2		0x06
void vBbdevIpcTest(void);
void vBbdevIpcTest_du(void);
#endif /* __IPC_TEST_H__ */
