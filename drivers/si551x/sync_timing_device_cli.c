/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2022 NXP
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* Standard includes. */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

#include "debug_console.h"
#include "sync_timing_device.h"
#include "sync_timing_device_cli.h"
#include "sync_timing_common.h"
#include <fsl_dspi.h>

SyncTimingDeviceContext_t xSyncTimingDevice;

static portBASE_TYPE prvTimesyncCLI( char * pcWriteBuffer,
                                     size_t xWriteBufferLen,
                                     const char * pcCommandString );

static const CLI_Command_Definition_t xTimesyncCLICommand =
{
    "sync", /* The command string to type. */
    "sync:\r\n"
    "\tsync 1 (version)\r\n"
    "\tsync 2 <div> <steps> (var_dco)\r\n"
    "\tsync 3 <DSPI_controller> (version)\r\n",
    prvTimesyncCLI, /* The function to run. */
    -1              /* The user can enter any number of commands. */
};

void vRegisterTimesyncCLICommands( void )
{
    FreeRTOS_CLIRegisterCommand( &xTimesyncCLICommand );
}

static portBASE_TYPE prvTimesyncCLIShowHelp( void )
{
    log_info( "%s\r\n", xTimesyncCLICommand.pcHelpString );
    return pdFALSE;
}

static portBASE_TYPE prvTimesyncCLI( char * pcWriteBuffer,
                                     size_t xWriteBufferLen,
                                     const char * pcCommandString )
{
    BaseType_t lParameterStringLength;
    const char * pcParam1;
    const char * pcParam2;
    const char * pcParam3;
    uint32_t ulArg1 = 0;
    uint32_t ulArg2 = 0;
    int32_t lArg;

    /* Remove compile time warnings about unused parameters, and check the
     * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
     * write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    ( void ) pcWriteBuffer;

    configASSERT( pcWriteBuffer );

    pcParam1 = FreeRTOS_CLIGetParameter( pcCommandString, 1, &lParameterStringLength );

    if( pcParam1 == NULL )
    {
        return prvTimesyncCLIShowHelp();
    }
    else
    {
        ulArg1 = strtoul( pcParam1, ( char ** ) NULL, 10 );
    }

    SyncTimingDeviceContext_t * pxContext = pxSyncTimingDeviceGetContext();

    if( pxContext == NULL )
    {
        return pdFALSE;
    }

    switch( ulArg1 )
    {
        case eSyncTimingDeviceCommandGetVersion:
            xSyncTimingDeviceGetVersionInfo( pxContext );
            break;
        case eSyncTimingDeviceCommandSetDCO:
            pcParam2 = FreeRTOS_CLIGetParameter( pcCommandString, 2, &lParameterStringLength );

            if( pcParam2 == NULL )
            {
                return prvTimesyncCLIShowHelp();
            }

            ulArg2 = strtoul( pcParam2, ( char ** ) NULL, 10 );
            pcParam3 = FreeRTOS_CLIGetParameter( pcCommandString, 3, &lParameterStringLength );

            if( pcParam3 == NULL )
            {
                return prvTimesyncCLIShowHelp();
            }

            lArg = ( int ) strtol( pcParam3, ( char ** ) NULL, 10 );
            xSyncTimingDeviceFWAPIVariableOffsetDco( pxContext, ulArg2, lArg );
            break;
	    case eSyncTimingDeviceCommandDspiGetVersion:
            pcParam2 = FreeRTOS_CLIGetParameter( pcCommandString, 2, &lParameterStringLength );

            if( pcParam2 == NULL )
            {
                return prvTimesyncCLIShowHelp();
            }

            ulArg2 = (int) strtoul( pcParam2, ( char ** ) NULL, 10 );

            switch ( ulArg2 )
            {
                case 2:
                    xSyncTimingDevice.xDspiHandle = pxDspiInitNormalSPIMode( DSPI_BLOCK2,
                            SYNC_TIMING_DSPI_CS_MASK );
                    break;
                case 3:
                    xSyncTimingDevice.xDspiHandle = pxDspiInitNormalSPIMode( DSPI_BLOCK3,
                            SYNC_TIMING_DSPI_CS_MASK );
                    break;
                case 4:
                    xSyncTimingDevice.xDspiHandle = pxDspiInitNormalSPIMode( DSPI_BLOCK4,
                            SYNC_TIMING_DSPI_CS_MASK );
                    break;
                case 5:
                    xSyncTimingDevice.xDspiHandle = pxDspiInitNormalSPIMode( DSPI_BLOCK5,
                            SYNC_TIMING_DSPI_CS_MASK );
                    break;
                default :
                    log_err( "%s: wrong argument passed : %d \r\n",
                            __func__,ulArg2);
                    return prvTimesyncCLIShowHelp();

            }
            if( xSyncTimingDevice.xDspiHandle == NULL)
            {
                log_err( "%s: Timing DSPI init failed \r\n",
                        __func__);
                break;
            }
            xSyncTimingDeviceGetVersionInfo( pxContext );
            break;
    default:
        return prvTimesyncCLIShowHelp();
    }

    return pdFALSE;
}
