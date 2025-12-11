// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "agave_init.h"
#include "agave_cli.h"
#include "agave_demod.h"

int32_t iCustomStrtoul( const char *pcStr, uint32_t ulStrLen,
                        char **pcEndptr, int32_t iBase)
{
    uint32_t i;

    if( 10 == iBase )
    {
        for( i = 0; i < ulStrLen; i++ )
            if( !( pcStr[i] >= '0' && pcStr[i] <= '9' ))
                return -1;
    }
    else if( 16 == iBase )
    {
        if( !( '0' == pcStr[0] && ( 'x' == pcStr[1] || 'X' ==  pcStr[1] )))
            return -1;
        for( i = 2; i < ulStrLen; i++ )
            if( !(( pcStr[i] >= '0' && pcStr[i] <= '9' ) ||
		  ( pcStr[i] >= 'a' && pcStr[i] <= 'f' ) ||
		  ( pcStr[i] >= 'A' && pcStr[i] <= 'F' )))
                return -1;
    }

    return ( strtoul( pcStr, pcEndptr, iBase ));
}

static portBASE_TYPE xAgvFeCtrlCb( char *pcWriteBuf, size_t xSize,
                                   const char *pcCmd )
{
    int32_t iDevId, iPath, iState;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    iDevId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DEV_0 != iDevId && AGV_DEV_1 != iDevId )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the second parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 2, &lParamStrLen );
    iPath = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_PATH0 != iPath && AGV_PATH1 != iPath )
    {
        strncpy( pcWriteBuf, "Error: Invalid second parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the third parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 3, &lParamStrLen );
    iState = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_PATH_DISABLE > iState || AGV_PATH_RX < iState )
    {
        strncpy( pcWriteBuf, "Error: Invalid third parameter\r\n", xSize );
        return pdFALSE;
    }

    log_info( "%s: DevId[%d], Path[%d], State[%d]\r\n", __func__,
              iDevId, iPath, iState );

    if( pdFALSE == xAgvCtrlFeState( iDevId, iPath, iState ))
    {
        strncpy( pcWriteBuf, "Error: tx_en command failed\r\n", xSize );
        return pdFALSE;
    }

    return pdFALSE;
}

static portBASE_TYPE xAgvRegReadCb( char *pcWriteBuf, size_t xSize,
                                    const char *pcCmd )
{
    int32_t iDevId, iComId, iAddr;
    uint16_t usData = 0;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    iDevId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DEV_0 != iDevId && AGV_DEV_1 != iDevId )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the second parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 2, &lParamStrLen );
    iComId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DSPI_SLV_SYNTH > iComId || AGV_DSPI_SLV_DEMOD_1 < iComId )
    {
        strncpy( pcWriteBuf, "Error: Invalid second parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the third parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 3, &lParamStrLen );
    iAddr = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 16 );
    if((( AGV_DSPI_SLV_SYNTH == iComId) &&
       ( AGV_SYNTH_MIN_REG_ADDR > iAddr || AGV_SYNTH_MAX_REG_ADDR < iAddr )) ||
       (( AGV_DSPI_SLV_SYNTH < iComId) &&
       ( AGV_DEMOD_MIN_REG_ADDR > iAddr || AGV_DEMOD_MAX_REG_ADDR < iAddr )))
    {
        strncpy( pcWriteBuf, "Error: Invalid third parameter\r\n", xSize );
        return pdFALSE;
    }

    log_info( "%s: iDevId[%d], iCompId[%d], iAddr[0x%x]\r\n", __func__,
              iDevId, iComId, iAddr );

    if( pdFALSE == xAgvReadReg( iDevId, iComId, iAddr, &usData ))
        strncpy( pcWriteBuf, "Error: read_r command failed\r\n", xSize );
    else
        snprintf( pcWriteBuf, xSize, "Read Reg Addr[0x%x] = 0x%x\r\n",
                  ( unsigned int )iAddr, usData);

    return pdFALSE;
}

static portBASE_TYPE xAgvRegWriteCb( char *pcWriteBuf, size_t xSize,
                                     const char *pcCmd )
{
    int32_t iDevId, iComId, iAddr, iVal;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    iDevId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DEV_0 != iDevId && AGV_DEV_1 != iDevId )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the second parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 2, &lParamStrLen );
    iComId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DSPI_SLV_SYNTH > iComId || AGV_DSPI_SLV_DEMOD_1 < iComId )
    {
        strncpy( pcWriteBuf, "Error: Invalid second parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the third parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 3, &lParamStrLen );
    iAddr = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 16 );
    if((( AGV_DSPI_SLV_SYNTH == iComId ) &&
       ( AGV_SYNTH_MIN_REG_ADDR > iAddr || AGV_SYNTH_MAX_REG_ADDR < iAddr )) ||
       (( AGV_DSPI_SLV_SYNTH < iComId ) &&
       ( AGV_DEMOD_MIN_REG_ADDR > iAddr || AGV_DEMOD_MAX_REG_ADDR < iAddr )))
    {
        strncpy( pcWriteBuf, "Error: Invalid third parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the fourth parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 4, &lParamStrLen );
    iVal = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 16 );
    if(( 0 > iVal ) || (( AGV_DSPI_SLV_SYNTH < iComId ) && ( 0xFF < iVal )) ||
       (( AGV_DSPI_SLV_SYNTH == iComId ) && ( 0xFFFF < iVal )))
    {
        strncpy( pcWriteBuf, "Error: Invalid fourth parameter\r\n", xSize );
        return pdFALSE;
    }

    log_info( "%s: iDevId[%d], iCompId[%d], iAddr[0x%x], iVal[0x%x]\r\n", __func__,
              iDevId, iComId, iAddr, iVal );

    if( pdFALSE == xAgvWriteReg( iDevId, iComId, iAddr, iVal ))
    {
        strncpy( pcWriteBuf, "Error: write_r command failed\r\n", xSize );
        return pdFALSE;
    }

    return pdFALSE;
}

static portBASE_TYPE xAgvLoopbackCb( char *pcWriteBuf, size_t xSize,
                                     const char *pcCmd )
{
    int32_t iDevId, iPath, iState;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    iDevId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DEV_0 != iDevId && AGV_DEV_1 != iDevId )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the second parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 2, &lParamStrLen );
    iPath = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_PATH0 != iPath && AGV_PATH1 != iPath )
    {
        strncpy( pcWriteBuf, "Error: Invalid second parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the third parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 3, &lParamStrLen );
    iState = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( DISABLE != iState && ENABLE != iState )
    {
        strncpy( pcWriteBuf, "Error: Invalid third parameter\r\n", xSize );
        return pdFALSE;
    }

    log_info( "%s: DevId[%d], Path[%d], State[%d]\r\n", __func__,
              iDevId, iPath, iState );

    if( pdFALSE == xAgvCtrlLoopback( iDevId, iPath, iState ))
    {
        strncpy( pcWriteBuf, "Error: lb_en command failed\r\n", xSize );
        return pdFALSE;
    }


    return pdFALSE;
}

static portBASE_TYPE xAgvRxGainCb( char *pcWriteBuf, size_t xSize,
                                   const char *pcCmd )
{
    int32_t iDevId, iRxId, iAttn, iGain;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    iDevId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DEV_0 != iDevId && AGV_DEV_1 != iDevId )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the second parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 2, &lParamStrLen );
    iRxId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_RX0 != iRxId && AGV_RX1 != iRxId )
    {
        strncpy( pcWriteBuf, "Error: Invalid second parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the third parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 3, &lParamStrLen );
    iAttn = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );

    /* Parse the fourth parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 4, &lParamStrLen );
    iGain = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );

    log_info( "%s: DevId[%d], RxId[%d], Attn[%d] Gain[%d]\r\n", __func__,
              iDevId, iRxId, iAttn, iGain );

    if( pdFALSE == xAgvRxGainCtrl( iDevId, iRxId, iAttn, iGain ))
    {
        strncpy( pcWriteBuf, "Error: rx_attn command failed\r\n", xSize );
        return pdFALSE;
    }

    return pdFALSE;
}

static portBASE_TYPE xAgvAdjFreq( char *pcWriteBuf, size_t xSize,
                                   const char *pcCmd )
{
    int32_t iDevId, iFreq;
    const char *pcParam;
    BaseType_t lParamStrLen;

    /* Initialize buffer with null */
    pcWriteBuf[ 0 ] = '\0';

    /* Parse the first parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 1, &lParamStrLen );
    iDevId = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( AGV_DEV_0 != iDevId && AGV_DEV_1 != iDevId )
    {
        strncpy( pcWriteBuf, "Error: Invalid first parameter\r\n", xSize );
        return pdFALSE;
    }

    /* Parse the second parameters */
    pcParam = FreeRTOS_CLIGetParameter( pcCmd, 2, &lParamStrLen );
    iFreq = iCustomStrtoul( pcParam, lParamStrLen, ( char ** ) NULL, 10 );
    if( iFreq < 20000 || iFreq > 5500000 )
    {
        strncpy( pcWriteBuf, "Error: Invalid Frequency\r\n", xSize );
        return pdFALSE;
    }

    if( iFreq < 3400000 || iFreq > 3600000 )
    {
        strncpy( pcWriteBuf, "WARNING: Agave doesn't support this frequency.Performance could be non-deterministic.\r\n", xSize );
    }

    if( pdFALSE == xAgvAdjustPllFreq( iDevId, iFreq))
    {
        strncpy( pcWriteBuf, "Error: Adjust Frequency command failed\r\n", xSize );
        return pdFALSE;
    }

    return pdFALSE;
}


static const CLI_Command_Definition_t xAgvFeCtrl =
{
    "fe_ctrl",
    "\r\nfe_ctrl <0-AgvDev0 | 1-AgvDev1> <0-path1 | 1-path2> <0-RxLowGain | 1-Tx | 2-RxHighGain>\r\n",
    xAgvFeCtrlCb,
    3
};

static const CLI_Command_Definition_t xAgvRegRead =
{
    "reg_r",
    "reg_r <0-AgvDev0 | 1-AgvDev1> <0-synth | 1-demod0 | 2-demod1> <addr>\r\n",
    xAgvRegReadCb,
    3
};

static const CLI_Command_Definition_t xAgvRegWrite =
{
    "reg_w",
    "reg_w <0-AgvDev0 | 1-AgvDev1> <0-synth | 1-demod0 | 2-demod1> <addr> <value>\r\n",
    xAgvRegWriteCb,
    4
};

static const CLI_Command_Definition_t xAgvLoopback =
{
    "lb_en",
    "lb_en <0-AgvDev0 | 1-AgvDev1> <0-path1, 1-path2> <0-disable | 1-enable>\r\n",
    xAgvLoopbackCb,
    3
};

static const CLI_Command_Definition_t xAgvRxGain =
{
    "rx_gn",
    "rx_gn <0-AgvDev0 | 1-AgvDev1> <0-rx1, 1-rx2> <RF Attn - range:0-31> <BB Gain - range:0-7>\r\n",
    xAgvRxGainCb,
    4
};

static const CLI_Command_Definition_t xAgvFreqAdj =
{
    "set_freq",
    "set_freq <0-AgvDev0 | 1-AgvDev1> <Freq in khz> \r\n",
    xAgvAdjFreq,
    2
};

BaseType_t vRegisterRFCli( void )
{
    /* CLI for TX enable/disable */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xAgvFeCtrl ))
        goto err;

    /* CLI for Register read */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xAgvRegRead ))
        goto err;

    /* CLI for Register write */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xAgvRegWrite ))
        goto err;

    /* CLI for loopback mode */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xAgvLoopback ))
        goto err;

    /* CLI for Rx Gain ctrl */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xAgvRxGain ))
        goto err;

    /* CLI for Synth Freq Adjustment */
    if( pdFAIL == FreeRTOS_CLIRegisterCommand( &xAgvFreqAdj ))
        goto err;

    return pdPASS;
err:
    log_err("%s: CLI register failed\r\n", __func__);
    return pdFAIL;
}
