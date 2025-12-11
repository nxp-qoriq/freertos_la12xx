// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

/*
 * This file contains LA1224RDB tbgen signal connectivity on board
 */
#include "FreeRTOS.h"
#include "tbgen_new.h"
#include "gul_host_if.h"

extern volatile uint32_t brd_ver;

/* Tbgen1 and Tbgen2 total instances that can generate Host TTI */
int TbgenTotalInstGenHostTTI[ TBGEN_MAX ];

TbgenHostTTIConf_t xTbgen1HostTTIConf[2];
TbgenHostTTIConf_t xTbgen2HostTTIConf[2];

void vInitTbgenTTIConnectivity()
{
	TbgenTotalInstGenHostTTI[0] = 0;
	TbgenTotalInstGenHostTTI[1] = 2;

	xTbgen2HostTTIConf[0].etype = GPE;
	xTbgen2HostTTIConf[0].eInst = TIMER_INSTANCE_2;

	xTbgen2HostTTIConf[1].etype = GPE;
	xTbgen2HostTTIConf[1].eInst = TIMER_INSTANCE_3;

	if( brd_ver == GEUL_HOST_REVC_VAL )
	{
		TbgenTotalInstGenHostTTI[0] = 1;
		xTbgen1HostTTIConf[0].etype = AGC_ENABLE;
		xTbgen1HostTTIConf[0].eInst = TIMER_INSTANCE_2;
	}
}
