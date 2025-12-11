// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#ifndef __RF_LOCAL_H__
#define __RF_LOCAL_H__

#include "rf_sw_cmds.h"
#include "rf_dev.h"
#include "Time.h"

int32_t xRFPostLocalSWCmd( RficHandle_t xHandle,
                           volatile rf_sw_cmd_desc_t * pxRFSWCmdDesc,
                           struct Time * pxEntryTime );

#endif /* __RF_LOCAL_H__ */
