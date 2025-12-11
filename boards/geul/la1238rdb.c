// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

/*
 * This file contains la1238rdb tbgen signal connectivity on board
 */
#include "FreeRTOS.h"
#include "tbgen_new.h"

/* Tbgen1 and Tbgen2 total instances that can generate Host TTI */
int TbgenTotalInstGenHostTTI[ TBGEN_MAX ];

TbgenHostTTIConf_t xTbgen1HostTTIConf[2];

TbgenHostTTIConf_t xTbgen2HostTTIConf[2];

void vInitTbgenTTIConnectivity()
{
	TbgenTotalInstGenHostTTI[0] = 1;
	TbgenTotalInstGenHostTTI[1] = 1;

	xTbgen2HostTTIConf[0].etype = GPE;
	xTbgen2HostTTIConf[0].eInst = TIMER_INSTANCE_2;

	xTbgen1HostTTIConf[0].etype = AGC_ENABLE;
	xTbgen1HostTTIConf[0].eInst = TIMER_INSTANCE_2;
}
