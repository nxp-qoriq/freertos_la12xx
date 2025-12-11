/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2022 NXP
 */

#ifndef __SYNC_TIMING_DEVICE_CLI_H
#define __SYNC_TIMING_DEVICE_CLI_H

typedef enum
{
    eSyncTimingDeviceCommandGetVersion = 1,
    eSyncTimingDeviceCommandSetDCO = 2,
    eSyncTimingDeviceCommandDspiGetVersion = 3,
} SyncTimingDeviceCommand_t;

void vRegisterTimesyncCLICommands( void );

#endif /* __SYNC_TIMING_DEVICE_CLI_H */
