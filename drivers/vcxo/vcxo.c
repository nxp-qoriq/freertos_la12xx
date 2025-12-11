// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "vcxo.h"

/* VCXO Device structure */
static VcxoDevice_t xVcxoDev;

int32_t prvVcxoWriteUpdate( uint16_t usData )
{
    int32_t iRet;
    uint8_t ucData[ 4 ] = { 0 };

    /* Write synthesizer register */
    ucData[ 0 ] = 0x0;
    ucData[ 1 ] = ( DAC_CMD_WRITEUPDATE << 4 );
    /* ignore the 4 bits of MSB */
    usData = ( usData << 4 );
    ucData[ 2 ] = ( uint8_t )(( usData & 0xFF00 ) >> 8 );
    ucData[ 3 ] = ( uint8_t )( usData & 0x00FF);

    iRet = lDspiPush( xVcxoDev.xDspiHandle, DSPI_CS0, DSPI_DEV_WRITE,
                      ucData, 4 );
    if( 0 > iRet )
    {
        log_err( "%s: lDspiPush failed, error[%d]\r\n", __func__,
                 iRet );
    }

    return iRet;
}

static portBASE_TYPE xVcxoCorrectCb( char *pcWriteBuf, size_t xSize,
                                     const char *pcCmd )
{
    uint16_t usData;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    usData = strtoul( pcParam, ( char ** ) NULL, 10 );
    if( VCXO_MAX_VAL < usData )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    log_info( "%s: usData[%u]\r\n", __func__, usData );

    if( prvVcxoWriteUpdate( usData ))
    {
        strncpy( pcWriteBuf, "Error: write_r command failed\r\n", xSize );
        return pdFALSE;
    }

    /* Update the current value */
    xVcxoDev.usCurVal = usData;

    return pdFALSE;
}

static const CLI_Command_Definition_t xVcxoCorrect =
{
    "vcxo",
    "\r\nvcxo < 12-bit data > - [ To tune the VCXO output freq ]\r\n",
    xVcxoCorrectCb,
    1
};

int32_t iVcxoInit( void )
{
    int32_t iRet = 0;

    /* Initialize DSPI Handler */
    xVcxoDev.xDspiHandle = pxDspiInit( VCXO_DSPI_BLOCK, VCXO_DSPI_CS_MASK );
    if( NULL == xVcxoDev.xDspiHandle )
    {
        log_err( "%s: VCXO DSPI init failed\r\n", __func__ );
	iRet = -1;
        goto err;
    }

    /* CLI for VCXO correction */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xVcxoCorrect ))
    {
        log_err( "%s: VCXO CLI failed\r\n", __func__ );
	iRet = -1;
        goto err;
    }

    /* Reset value of DAC LTC2621 is half of the max value*/
    xVcxoDev.usCurVal = VCXO_MAX_VAL / 2;

err:
    return iRet;
}
