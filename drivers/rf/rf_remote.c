// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "rf_config.h"
#include "rf_dev.h"
#include "rf_core.h"
#include "types.h"
#include "rf_hif.h"

int32_t iRaiseRemoteRFEvent( RficHandle_t xHandle,
                             uint32_t ulEventId,
                             uint32_t ulDestCoreId )
{
	( void ) xHandle;
	( void ) ulDestCoreId;

    mpic_out32( MPIC_REGS_MSIIRB, ( ( ulEventId << 24 ) | ( ulEventId << 29 ) ) );

	return 0;
}
