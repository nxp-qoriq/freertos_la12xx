/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2022 NXP
 */

#ifndef __SYNC_TIMING_DEVICE_H
#define __SYNC_TIMING_DEVICE_H

#include <immap.h>
#include <sync_timing_common.h>

#define SYNC_TIMING_DEVICE_MAX_CMD_DATA_SIZE      ( 255 )
#define SYNC_TIMING_DEVICE_MCU_PORTAL_ADDR        ( 0xF00F )
#define SYNC_TIMING_DEVICE_MCU_PORTAL_ADDR_LEN    ( 0x2 )
#define SYNC_TIMING_DEVICE_RETRY_COUNT            ( 100 )
#define SYNC_TIMING_DEVICE_CMD_REPLY_READY        ( 0x80 )
#define SYNC_TIMING_DEVICE_CLEAR_TO_SEND          ( 0x80 )
#define SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE    ( 255 )
#define SYNC_TIMING_MAX_SPI_HDR_SIZE              ( 4 )
#define SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE    ( 32 )
#define SYNC_TIMING_CTS_REPLY_DATA_TRANSFER_SIZE  ( 2 )
#define SYNC_TIMING_AFTER_CTS_REPLY_DATA_TRANSFER_SIZE   ( 31 )
#define SYNC_TIMING_MUX_NOT_SET                 ( 0xff )
#define SYNC_TIMING_READ_OPR      ( 1 << 7 )
/* DSPI BLOCK */
#define SYNC_TIMING_DSPI_BLOCK     ( DSPI_BLOCK1 )
#define SYNC_TIMING_DSPI_CS_MASK   ( 1 << DSPI_CS0 )

typedef enum
{
    eSyncStatusSuccess = 0,
    eSyncStatusFailure,
    eSyncStatusTimeout,
    eSyncStatusMax
} SyncStatus_t;

typedef enum
{
    eSyncWriteTransaction = 0,
    eSyncReadTransaction
} SyncTransactionType_t;

typedef enum
{
    eSyncTimingDeviceModeAppln = 0,
    eSyncTimingDeviceModeBootloader = 1,
    eSyncTimingDeviceModeInvalid = 2,
} SyncTimingDeviceMode_t;

typedef enum
{
    eSyncTimingDeviceStatusUnInitialized = 0,
    eSyncTimingDeviceStatusInitialized = 1
} SyncTimingDeviceStatus_t;

typedef struct
{
    uint32_t dspiHandle;
    uint32_t portalAddr;
    uint8_t dspiAddr;
    uint8_t portalAddrLen;
} SyncTimingDeviceDSPIInfo_t;

typedef struct
{
    uint32_t fwVersionMajor;
    uint32_t fwVersionMinor;
    uint32_t fwVersionBuildNum;
    uint8_t deviceRevision;
    uint8_t deviceSubRevision;
    uint8_t deviceStatus;
    uint8_t reserved;
    SyncTimingDeviceMode_t deviceMode;
    struct LA12xxDspiInstance * xDspiHandle;
} SyncTimingDeviceContext_t;


/**
 * @brief       Function is used to get the sync timing device context handle.
 * @return  context handle on success, NULL otherwise.
 */
SyncTimingDeviceContext_t * pxSyncTimingDeviceGetContext( void );

/**
 * @brief	Function is used to initialize the sync driver and get the context handle.
 * @return  context handle on success, NULL otherwise.
 */
SyncTimingDeviceContext_t * pxSyncTimingDeviceInit( void );

/**
 * @brief	Function is used to get the sync timing device version info.
 * @param[in]   pxContext Sync timing device context handle
 * @return      eSyncStatusSuccess on success, eSyncStatusFailure otherwise.
 */
SyncStatus_t xSyncTimingDeviceGetVersionInfo( SyncTimingDeviceContext_t * pxContext );

/**
 * @brief       Function is used to read register.
 * @param[in]   pxContext Sync timing device context handle
 * @param[in]   usAddr register address
 * @param[in]   pucBuff buffer to hold read data
 * @param[in]   ulLen length of the data
 * @return      eSyncStatusSuccess on success, eSyncStatusFailure otherwise.
 */

SyncStatus_t xSyncTimingDeviceMemReadDirect( SyncTimingDeviceContext_t * pxContext,
                                             uint16_t usAddr,
                                             uint8_t * pucBuff,
                                             uint32_t ulLen );
/**
 * @brief	Function is used to get chip mode.
 * @param[in]   pxContext Sync timing device context handle
 * @param[in]   pxMode buffer to hold chip mode
 * @return      eSyncStatusSuccess on success, eSyncStatusFailure otherwise.
 */
SyncStatus_t xSyncTimingDeviceGetChipsetMode( SyncTimingDeviceContext_t * pxContext,
                                              SyncTimingDeviceMode_t * pxMode );

/**
 * @brief	Function is used to set variable offset dco.
 * @param[in]   pxContext Sync timing device context handle
 * @param[in]   ucDivider Divider which needs to be 1,2,4,8 or 16
 * @param[in]   lSteps steps
 * @return      eSyncStatusSuccess on success, eSyncStatusFailure otherwise.
 */
SyncStatus_t xSyncTimingDeviceFWAPIVariableOffsetDco( SyncTimingDeviceContext_t * pxContext,
                                                      uint8_t ucDivider,
                                                      int32_t lSteps );

#endif /* __SYNC_TIMING_DEVICE_H */
