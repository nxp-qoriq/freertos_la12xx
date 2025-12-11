/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2022-2023 NXP
 */

#include <stdlib.h>
#include <string.h>
#include <sync_timing_device.h>
#include <sync_timing_aruba_cmd_map.h>
#include <sync_timing_core_aruba_interface.h>
#include <sync_timing_common.h>
#include <sync_timing_aruba_cmd.h>
#include <fsl_dspi.h>
#include <FreeRTOS.h>
#include "task.h"
#include "Time.h"

SyncTimingDeviceContext_t xSyncTimingDevice;

bool fillDspi(struct DspiCustom *xDspiInit) {
	uint32_t ulMcrCfgVal;
	uint32_t ulCtarCfgVal;
	uint32_t ulSrCfgVal;
	DspiBlock_t eBlock = SYNC_TIMING_DSPI_BLOCK;
	uint8_t ucCsMask = SYNC_TIMING_DSPI_CS_MASK;
	/*
	 * uint32_t ulIrsrCfgVal;
	 * uint32_t ulCtarXVal;
	 */
	struct LA12xxDspiInstance * readDspi = NULL;
	DspiBlock_t eDspiBlock = eBlock - 1;

	if( eBlock > DSPI_BLOCK6 )
	{
		log_err( "DSPI Err :  Invalid DSPI block number \r\n" );
		return false;
	}

	readDspi = ( struct LA12xxDspiInstance * ) pvGeulMalloc( sizeof( struct LA12xxDspiInstance));
	if( readDspi == NULL )
	{
		log_err( "DSPI read Handle allocation failed!! \r\n" );
		return false;
	}

	readDspi->DspiRegs = ( struct DspiReg * ) ( DSPI_REG_BASE_ADDRESS
			+ ( ( eDspiBlock ) * DSPI_REG_BASE_OFFSET ) );

	xDspiInit->eBlockNumber = eBlock;
	xDspiInit->ucCsMask = ucCsMask;
	xDspiInit->ulBusClk = DSPI_DEFAULT_FREQUENCY;

	vDspiClkSet(readDspi, DSPI_DEFAULT_FREQUENCY);

	ulMcrCfgVal = ( DSPI_MCR_MSTR | DSPI_MCR_PCSIS( ucCsMask ) |
			DSPI_MCR_HALT );
	/*  Setting Default frequency 4MHz*/
	ulCtarCfgVal= in_dspile32( &readDspi->DspiRegs->ulCtar[ 0 ] );
	ulCtarCfgVal &= DSPI_CTAR_TRSZ_MASK;
	ulCtarCfgVal |= ( uint32_t ) DSPI_CTAR_TRSZ( 0x7 );
	ulSrCfgVal = ( DSPI_SR_TCF | DSPI_SR_EOQF | DSPI_SR_TFFF |
			DSPI_SR_RFOF | DSPI_SR_RFDF | DSPI_SR_TFIWF |
			DSPI_SR_SPEF | DSPI_SR_CTCF );

	/*
	 * If you want to set Ctar[1] and CtarX[1] then set it in DspiInit
	 * and also update xDSpiInit->ulCtar1 = 1 and xDSpiInit->ulCtarX1 = 1
	 * respectively.
	 *
	 * ulCtarXVal = DSPI_CTAR_X_TRSZ | DSPI_CTAR_X_DTCP( 0x1 );
	 * ulIrsrCfgVal = DSPI_IRSR_DISABLE;
	 */

	xDspiInit->DspiRegs.ulMcr = ulMcrCfgVal;
	xDspiInit->DspiRegs.ulCtar[0] = ulCtarCfgVal;
	xDspiInit->DspiRegs.ulSr = ulSrCfgVal;
	vGeulFree( readDspi );

	return true;
}

SyncTimingDeviceContext_t * pxSyncTimingDeviceGetContext()
{
    if( xSyncTimingDevice.deviceStatus != eSyncTimingDeviceStatusInitialized )
    {
        log_err( "%s: uninitalized\r\n", __func__ );
        return NULL;
    }
    else
    {
        return &xSyncTimingDevice;
    }
}

SyncTimingDeviceContext_t * pxSyncTimingDeviceInit()
{
    if( xSyncTimingDevice.deviceStatus == eSyncTimingDeviceStatusInitialized )
    {
        return &xSyncTimingDevice;
    }
    /* Initialize DSPI Handler */
#if 0
    xSyncTimingDevice.xDspiHandle = pxDspiInitNormalSPIMode( SYNC_TIMING_DSPI_BLOCK,
            SYNC_TIMING_DSPI_CS_MASK );
#endif
    struct DspiCustom xDspiInit;
    if (!fillDspi(&xDspiInit)) {
	log_err( "%s: Timing DSPI init failed \r\n",
			__func__);
	return NULL;
    }
    xSyncTimingDevice.xDspiHandle = pxDspiInitCustom(&xDspiInit);
    if( xSyncTimingDevice.xDspiHandle == NULL)
    {
        log_err( "%s: Timing DSPI init failed \r\n",
                __func__);
        return NULL;
    }

    xSyncTimingDevice.deviceStatus = eSyncTimingDeviceStatusInitialized;

    return &xSyncTimingDevice;
}

static SyncStatus_t prvSyncTimingDeviceVersion( SyncTimingDeviceContext_t * pxContext )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    uint32_t ulLen = sizeof( reply_DEVICE_INFO_map_t );
    uint8_t ucBuff[ulLen];
    uint32_t count = 0, i = 0;
    reply_DEVICE_INFO_map_t xReplyDeviceInfo;
    cmd_DEVICE_INFO_map_t xCmdDeviceInfo;
    int ret = 0;

    xCmdDeviceInfo.OPCODE = SYNC_TIMING_OP_API_CMD;
    xCmdDeviceInfo.CMD = cmd_ID_DEVICE_INFO;

    ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, sizeof(cmd_DEVICE_INFO_map_t), (void *)&xCmdDeviceInfo, (void *)&ucBuff[0] );
    if( !ret ) {
        returnStatus = eSyncStatusSuccess;
    } else {
        returnStatus = eSyncStatusFailure;
    }
    if( returnStatus != eSyncStatusSuccess )
    {
        log_err("%s: DSPI write error at %d\r\n", __func__, __LINE__);
        return returnStatus;
    }

    for( i = 0; i < ulLen; i++ ) {
        ucBuff[i] = 0;
    }
    while( 1 )
    {
        ucBuff[0] = SYNC_TIMING_OP_API_REPLY;

        ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, ulLen, (void *)&ucBuff[0], (void *)&xReplyDeviceInfo );
        if( !ret ) {
            returnStatus = eSyncStatusSuccess;
        } else {
            returnStatus = eSyncStatusFailure;
        }
        if( returnStatus != eSyncStatusSuccess )
        {
            log_err("%s: DSPI read error at %d\r\n", __func__, __LINE__);
            return returnStatus;
        }

        if( xReplyDeviceInfo.STATUS & SYNC_TIMING_DEVICE_CLEAR_TO_SEND )
        {
            break;
        }

        if( ++count == SYNC_TIMING_DEVICE_RETRY_COUNT )
        {
            log_err( "%s: waited too long %d\r\n", __func__, __LINE__ );
            return eSyncStatusTimeout;
        }
    }
    log_info( "\r\nsync_timing_device blversion: %u.%u.%u.%u\r\n",
            ( uint32_t ) xReplyDeviceInfo.ROM,
            ( uint32_t ) xReplyDeviceInfo.MROM,
            ( uint32_t ) xReplyDeviceInfo.BROM,
            ( uint32_t ) in_le32(&(xReplyDeviceInfo.SVN )));
    pxContext->deviceRevision = ( xReplyDeviceInfo.M0 >> 0x4 ) & 0x0F;
    pxContext->deviceSubRevision = ( xReplyDeviceInfo.M0 ) & 0x0F;

    log_info( "sync_timing_device device revision: %u, subrevision: %u\r\n",
            pxContext->deviceRevision,
            pxContext->deviceSubRevision );

    return returnStatus;
}

static SyncStatus_t prvSyncTimingDeviceAppInfo( SyncTimingDeviceContext_t * pxContext )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    uint32_t ulLen = sizeof( reply_APP_INFO_map_t );
    uint8_t ucBuff[ulLen];
    uint32_t count = 0, i = 0;
    reply_APP_INFO_map_t xReplyAppInfo;
    cmd_APP_INFO_map_t xCmdAppInfo;
    int ret = 0;

    xCmdAppInfo.OPCODE = SYNC_TIMING_OP_API_CMD;
    xCmdAppInfo.CMD = cmd_ID_APP_INFO;

    ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, sizeof(cmd_APP_INFO_map_t), (void *)&xCmdAppInfo, (void *)&ucBuff[0] );
    if( !ret ) {
        returnStatus = eSyncStatusSuccess;
    } else {
        returnStatus = eSyncStatusFailure;
    }
    if( returnStatus != eSyncStatusSuccess )
    {
        log_err("%s: DSPI write error at %d\r\n", __func__, __LINE__);
        return returnStatus;
    }

    for( i = 0; i < ulLen; i++ ) {
        ucBuff[i] = 0;
    }
    while( 1 )
    {
        ucBuff[0] = SYNC_TIMING_OP_API_REPLY;

        ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, ulLen, (void *)&ucBuff[0], (void *)&xReplyAppInfo );
        if( !ret ) {
            returnStatus = eSyncStatusSuccess;
        } else {
            returnStatus = eSyncStatusFailure;
        }
        if( returnStatus != eSyncStatusSuccess )
        {
            log_err("%s: DSPI read error at %d\r\n", __func__, __LINE__);
            return returnStatus;
        }

        if( xReplyAppInfo.STATUS & SYNC_TIMING_DEVICE_CLEAR_TO_SEND )
        {
            break;
        }

        if( ++count == SYNC_TIMING_DEVICE_RETRY_COUNT )
        {
            log_err( "%s: waited too long %d\r\n", __func__, __LINE__ );
            return eSyncStatusTimeout;
        }
    }
    log_info( "sync_timing_device App Version = %u.%u.%u_svn_%u\r\n",
            ( uint32_t ) xReplyAppInfo.A_MAJOR,
            ( uint32_t ) xReplyAppInfo.A_MINOR,
            ( uint32_t ) xReplyAppInfo.A_BRANCH,
            ( uint32_t ) in_le16(&(xReplyAppInfo.A_BUILD )));
    log_info( "sync_timing_device Planner Version = %u.%u.%u_svn_%u\r\n",
            ( uint32_t ) xReplyAppInfo.P_MAJOR,
            ( uint32_t ) xReplyAppInfo.P_MINOR,
            ( uint32_t ) xReplyAppInfo.P_BRANCH,
            ( uint32_t ) in_le16(&(xReplyAppInfo.P_BUILD )));

    log_info( "sync_timing_device Fplan Design ID = %c%c%c%c%c%c%c%c\r\n",
            xReplyAppInfo.DESIGN_ID[ 0 ],
            xReplyAppInfo.DESIGN_ID[ 1 ],
            xReplyAppInfo.DESIGN_ID[ 2 ],
            xReplyAppInfo.DESIGN_ID[ 3 ],
            xReplyAppInfo.DESIGN_ID[ 4 ],
            xReplyAppInfo.DESIGN_ID[ 5 ],
            xReplyAppInfo.DESIGN_ID[ 6 ],
            xReplyAppInfo.DESIGN_ID[ 7 ] );

    pxContext->fwVersionMajor = ( uint32_t ) xReplyAppInfo.A_MAJOR;
    pxContext->fwVersionMinor = ( uint32_t ) xReplyAppInfo.A_MINOR;
    pxContext->fwVersionBuildNum = ( uint32_t ) xReplyAppInfo.A_BUILD;

    return returnStatus;
}

SyncStatus_t xSyncTimingDeviceMemReadDirect( SyncTimingDeviceContext_t * pxContext,
        uint16_t usAddr,
        uint8_t * pucBuff,
        uint32_t ulLen )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    uint8_t ucTxBuff[SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    uint8_t ucRxBuff[SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE + SYNC_TIMING_MAX_SPI_HDR_SIZE];
    int ret = 0;

    if( ulLen > SYNC_TIMING_MAX_SPI_DATA_TRANSFER_SIZE )
    {
        log_err("data length error in %s\r\n", __func__);
        return eSyncStatusFailure;
    }

    ucTxBuff[0] = SYNC_TIMING_OP_SET_ADDR;
    ucTxBuff[1] = usAddr & 0xff;
    ucTxBuff[2] = ( usAddr >> 8 ) & 0xff;
    ucTxBuff[3] = SYNC_TIMING_OP_RD_RBURSTU;
    ucTxBuff[4] = 0x0;

    ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, ulLen, (void *)&ucTxBuff[0], (void *)&ucRxBuff[0] );
    if( !ret ) {
        returnStatus = eSyncStatusSuccess;
    } else {
        returnStatus = eSyncStatusFailure;
    }
    *pucBuff = ucRxBuff[4];

    return returnStatus;
}

static SyncStatus_t prvSyncTimingDeviceWaitForCTS( SyncTimingDeviceContext_t * pxContext )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    uint32_t count = 0, i =0;
    uint8_t ucTxBuff[SYNC_TIMING_CTS_REPLY_DATA_TRANSFER_SIZE + SYNC_TIMING_AFTER_CTS_REPLY_DATA_TRANSFER_SIZE];
    uint8_t ucRxBuff[SYNC_TIMING_CTS_REPLY_DATA_TRANSFER_SIZE + SYNC_TIMING_AFTER_CTS_REPLY_DATA_TRANSFER_SIZE];
    int ret = 0;

    ucTxBuff[0] = SYNC_TIMING_OP_API_REPLY;
    for( i = 1; i < SYNC_TIMING_CTS_REPLY_DATA_TRANSFER_SIZE + SYNC_TIMING_AFTER_CTS_REPLY_DATA_TRANSFER_SIZE - 1; i++) {
        ucTxBuff[i] = 0x0;
    }
    while( 1 )
    {
        ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, SYNC_TIMING_CTS_REPLY_DATA_TRANSFER_SIZE +
                SYNC_TIMING_AFTER_CTS_REPLY_DATA_TRANSFER_SIZE, (void *)&ucTxBuff[0],
                (void *)&ucRxBuff[0] );
        if( !ret ) {
            returnStatus = eSyncStatusSuccess;
        } else {
            returnStatus = eSyncStatusFailure;
        }

        if( returnStatus != eSyncStatusSuccess )
        {
            log_err("%s: DSPI: Sync driver: 0xD0 read failed\r\n", __func__);
            return returnStatus;
        }

        if(ucRxBuff[1] == SYNC_TIMING_MUX_NOT_SET )
        {
            log_err("\r\nSync driver: Mux not Set\r\nTry setting Mux from Host side\r\n");
            return eSyncStatusFailure;
        }

        if( ucRxBuff[1] & SYNC_TIMING_DEVICE_CLEAR_TO_SEND )

        {
            return returnStatus;
        }

        if( ++count == SYNC_TIMING_DEVICE_RETRY_COUNT )
        {
            log_err( "%s: waited too long\r\n", __func__ );
            return eSyncStatusTimeout;
        }
    }

    return returnStatus;
}

SyncStatus_t xSyncTimingDeviceGetChipsetMode( SyncTimingDeviceContext_t * pxContext,
        SyncTimingDeviceMode_t * pxMode )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    uint8_t ucMCUBootState = 0;

    if( pxMode == NULL )
    {
        return returnStatus;
    }
    returnStatus = xSyncTimingDeviceMemReadDirect( pxContext,
            SYNC_TIMING_MCU_BOOTSTATE,
            &ucMCUBootState,
            5 );

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "%s: SYNC_TIMING_MCU_BOOTSTATE read error\r\n", __func__ );
        return returnStatus;
    }

    if( ( ucMCUBootState > 0 ) && ( ucMCUBootState <= BOOTLOADER_READY_STATE ) )
    {
        *pxMode = eSyncTimingDeviceModeBootloader;
    }
    else if( ucMCUBootState >= APPLICATION_READY_STATE )
    {
        *pxMode = eSyncTimingDeviceModeAppln;
    }
    else
    {
        *pxMode = eSyncTimingDeviceModeInvalid;
    }

    pxContext->deviceMode = *pxMode;

    return returnStatus;
}

static SyncStatus_t prvSyncTimingDeviceGetDeviceInfo( SyncTimingDeviceContext_t * pxContext )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;

    returnStatus = prvSyncTimingDeviceWaitForCTS( pxContext );

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "%s: cmd error\r\n", __func__ );
        return returnStatus;
    }

    returnStatus = prvSyncTimingDeviceVersion( pxContext );

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "%s: cmd error\r\n", __func__);
        return returnStatus;
    }

    return returnStatus;
}

static SyncStatus_t prvSyncTimingDeviceGetAppInfo( SyncTimingDeviceContext_t * pxContext )

{
    SyncStatus_t returnStatus = eSyncStatusFailure;

    returnStatus = prvSyncTimingDeviceWaitForCTS( pxContext );

    if( returnStatus != eSyncStatusSuccess )
    {
        return returnStatus;
    }

    returnStatus = prvSyncTimingDeviceAppInfo( pxContext );

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "%s: command write error\r\n", __func__, __LINE__ );
        return returnStatus;
    }

    return returnStatus;
}

SyncStatus_t xSyncTimingDeviceGetVersionInfo( SyncTimingDeviceContext_t * pxContext )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    SyncTimingDeviceMode_t xMode = eSyncTimingDeviceModeInvalid;

    returnStatus = prvSyncTimingDeviceGetDeviceInfo( pxContext );

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "sync timing device GetDeviceInfo failed\r\n" );
        return returnStatus;
    }

    returnStatus = xSyncTimingDeviceGetChipsetMode( pxContext, &xMode );

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "sync timing device GetChipsetMode failed\r\n" );
        return returnStatus;
    }

    if( xMode == eSyncTimingDeviceModeAppln )
    {
        returnStatus = prvSyncTimingDeviceGetAppInfo( pxContext );

        if( returnStatus != eSyncStatusSuccess )
        {
            log_err( "sync timing device GetAppInfo failed\r\n" );
            return returnStatus;
        }
    }
    else
    {
        log_info( "Device mode is %d so not trying to get fw info\r\n", xMode );
    }

    return returnStatus;
}

SyncStatus_t xSyncTimingDeviceFWAPIVariableOffsetDco( SyncTimingDeviceContext_t * pxContext, uint8_t ucDivider, int32_t lSteps )
{
    SyncStatus_t returnStatus = eSyncStatusFailure;
    uint32_t count = 0, i = 0;
    reply_VARIABLE_OFFSET_DCO_map_t xReplyDcoVo;
    cmd_VARIABLE_OFFSET_DCO_map_t xCmdDcoVo;
    uint8_t ucBuff[ sizeof( cmd_VARIABLE_OFFSET_DCO_map_t ) ];
    int ret = 0;

    if( ( ucDivider != 0x1 ) &&
    ( ucDivider != 0x2 ) &&
    ( ucDivider != 0x4 ) &&
    ( ucDivider != 0x8 ) &&
    ( ucDivider != 0x10 ) )
    {
        return returnStatus;
    }

    returnStatus = prvSyncTimingDeviceWaitForCTS( pxContext );

    if( returnStatus != eSyncStatusSuccess )
    {
        return returnStatus;
    }

    xCmdDcoVo.OPCODE = SYNC_TIMING_OP_API_CMD;
    xCmdDcoVo.CMD = cmd_ID_VARIABLE_OFFSET_DCO;
    xCmdDcoVo.DIVIDER_SELECT = ucDivider;
    xCmdDcoVo.OFFSET = in_le32(&lSteps);

    ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, sizeof( cmd_VARIABLE_OFFSET_DCO_map_t ), (void *)&xCmdDcoVo, (void *)&ucBuff[0] );
    if( !ret ) {
        returnStatus = eSyncStatusSuccess;
    } else {
        returnStatus = eSyncStatusFailure;
    }

    if( returnStatus != eSyncStatusSuccess )
    {
        log_err( "%s: data write error\r\n", __func__ );
        return returnStatus;
    }

    for( i = 0; i < sizeof( reply_VARIABLE_OFFSET_DCO_map_t ); i++ ) {
        ucBuff[i] = 0;
    }
    while( 1 )
    {
        ucBuff[0] = SYNC_TIMING_OP_API_REPLY;
        ret = iTransferMsgNormalSPIMode( pxContext->xDspiHandle, DSPI_CS0, sizeof( reply_VARIABLE_OFFSET_DCO_map_t ), (void *)&ucBuff[0], (void *)&xReplyDcoVo );
        if( !ret ) {
            returnStatus = eSyncStatusSuccess;
        } else {
            returnStatus = eSyncStatusFailure;
        }

        if( returnStatus != eSyncStatusSuccess )
        {
            log_err( "%s: data read error\r\n", __func__ );
            return returnStatus;
        }

        if( xReplyDcoVo.STATUS & SYNC_TIMING_DEVICE_CMD_REPLY_READY )
        {
            break;
        }

        if( ++count == SYNC_TIMING_DEVICE_RETRY_COUNT )
        {
            log_err( "%s: waited too long\r\n", __func__ );
            return eSyncStatusTimeout;
        }
        vUdelay(10);
    }

    log_info( "DCO_STATUS: %u\r\n", xReplyDcoVo.DCO_STATUS );

    return returnStatus;
}
