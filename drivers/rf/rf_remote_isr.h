// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __RF_REMOTE_ISR_H__
#define __RF_REMOTE_ISR_H__

#include "rf_dev.h"

int32_t iRemoteISRSetup( RficHandle_t xHandle );
bool_t bRFRemoteISR( uint32_t ulIrqNo,
                     void * pvDevData );

#endif /* __RF_REMOTE_ISR_H__ */
