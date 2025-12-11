// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "agave_init.h"
#include "agave_demod.h"

int32_t prvAgvDemodWriteReg( AgvDevice_t *pxAgvDev, AgvRx_t eRxId, uint8_t addr,
                             uint8_t data )
{
    int32_t iRet;
    uint8_t ucData[ 4 ] = { 0 };
    DspiChipSel_t eChipSelect = ( AGV_RX0 == eRxId ) ? DSPI_CS0 : DSPI_CS1;

    log_dbg("%s: WriteReg[%x : %x]\r\n", __func__, addr, data);
    /* Read demod register before writing */
    ucData[ 0 ] = addr | AGV_DEMOD_READ_OPR;
    iRet = lDspiPush( pxAgvDev->xDspiHandle, eChipSelect, DSPI_DEV_READ,
                      ucData, 4 );
    if( 0 > iRet )
    {
        log_err( "%s: lDspiPush failed, error[%d]\r\n", __func__,
                 iRet );
        return iRet;
    }

    iRet = lDspiPop( pxAgvDev->xDspiHandle, ucData, 4 );
    if( 0 > iRet )
    {
        log_err( "%s: lDspiPop failed, error[%d]\r\n", __func__,
                 iRet );
        return iRet;
    }
    /* Write demodulator register */
    ucData[ 0 ] = addr;
    ucData[ 1 ] = data;
    iRet = lDspiPush( pxAgvDev->xDspiHandle, eChipSelect, DSPI_DEV_WRITE,
                      ucData, 4 );
    if( 0 > iRet )
    {
        log_err( "%s: lDspiPush failed, error[%d]\r\n", __func__,
                 iRet );
    }

    return iRet;
}

int32_t prvAgvDemodReadReg( AgvDevice_t *pxAgvDev, AgvRx_t eRxId, uint8_t addr,
                            uint8_t *data )
{
    int32_t iRet;
    uint8_t ucData[ 4 ] = { 0 };
    DspiChipSel_t eChipSelect = ( AGV_RX0 == eRxId ) ? DSPI_CS0 : DSPI_CS1;

    /* Push address to be read, Set MSB bit
     * which indicate read operation */
    ucData[ 0 ] = addr | AGV_DEMOD_READ_OPR;
    iRet = lDspiPush( pxAgvDev->xDspiHandle, eChipSelect, DSPI_DEV_READ,
                      ucData, 4 );
    if( 0 > iRet )
    {
        log_err( "%s: lDspiPush failed, error[%d]\r\n", __func__,
                 iRet );
        return iRet;
    }

    iRet = lDspiPop( pxAgvDev->xDspiHandle, ucData, 4 );
    if( 0 > iRet )
    {
        log_err( "%s: lDspiPop failed, error[%d]\r\n", __func__,
                 iRet );
        return iRet;
    }

    /* index 0 should be ignored, this is outcome
     * of above push operation */
    *data = ucData[ 1 ];

    return iRet;
}

BaseType_t xAgvDemodGainCtrl( AgvDevice_t *pxAgvDev, AgvRx_t eRxId,
                              int32_t iAttn, int32_t iGain )
{
    uint8_t ucData;
    int32_t iRet;

    /* Read and update rf attenuation */
    iRet = prvAgvDemodReadReg( pxAgvDev, eRxId, AGV_DEMOD_REG16, &ucData );
    if( 0 > iRet )
    {
        log_err( "%s: Attn read failed, error[%d]\r\n", __func__, iRet );
        return iRet;
    }

    ucData = (( ucData & AGV_DEMOD_ATTN_MASK ) |
              (( uint8_t )( iAttn << AGV_DEMOD_ATTN_SHIFT )));

    iRet = prvAgvDemodWriteReg( pxAgvDev, eRxId, AGV_DEMOD_REG16, ucData );
    if( 0 > iRet )
    {
        log_err( "%s: Attn write failed, error[%d]\r\n", __func__, iRet );
        return iRet;
    }

    /* Read and update BB gain */
    iRet = prvAgvDemodReadReg( pxAgvDev, eRxId, AGV_DEMOD_REG21, &ucData );
    if( 0 > iRet )
    {
        log_err( "%s: Gain read failed, error[%d]\r\n", __func__, iRet );
        return iRet;
    }

    ucData = (( ucData & AGV_DEMOD_GAIN_MASK ) |
              (( uint8_t )( iGain << AGV_DEMOD_GAIN_SHIFT )));

    iRet = prvAgvDemodWriteReg( pxAgvDev, eRxId, AGV_DEMOD_REG21, ucData );
    if( 0 > iRet )
    {
        log_err( "%s: Gain write failed, error[%d]\r\n", __func__, iRet );
        return iRet;
    }

    return iRet;
}

int32_t AgvDemodLoMatching( AgvDevice_t *pxAgvDev, AgvRx_t eRxId )
{
    int32_t iRet;

    /* Configure LVCM and CF1 */
    iRet = prvAgvDemodWriteReg( pxAgvDev, eRxId, AGV_DEMOD_REG18, AGV_DEMOD_LO_MATCH1 );
    if( iRet < 0 )
    {
        log_err( "%s: demod%d write reg[%x] failed\r\n", __func__, eRxId,
                 AGV_DEMOD_REG18 );
        return -1;
    }

    /* Configure Band, LF1 and CF2 */
    iRet = prvAgvDemodWriteReg( pxAgvDev, eRxId, AGV_DEMOD_REG19, AGV_DEMOD_LO_MATCH2 );
    if( iRet < 0 )
    {
        log_err( "%s: demod%d write reg[%x] failed\r\n", __func__, eRxId,
                 AGV_DEMOD_REG19 );
        return -1;
    }

    return 0;
}

int32_t AgvDemodInit( AgvDevice_t *pxAgvDev )
{
    int32_t iRet;
    uint16_t usData;

    /* Demodulator-1 soft reset */
    usData = AGV_DEMOD_RESET;
    iRet = prvAgvDemodWriteReg( pxAgvDev, AGV_RX0, AGV_DEMOD_REG22, usData );
    if( iRet < 0 )
    {
        log_err( "%s: demod%d write reg[%x] failed\r\n", __func__, AGV_RX0,
		 AGV_DEMOD_REG22 );
        return -1;
    }

    /* LO matching for 3500-6000MHz */
    iRet = AgvDemodLoMatching( pxAgvDev, AGV_RX0 );
    if( iRet < 0 )
    {
        log_err( "%s: demod%d LO matching failed\r\n", __func__, AGV_RX0);
        return -1;
    }

    /* Demodulator-2 soft reset */
    usData = AGV_DEMOD_RESET;
    iRet = prvAgvDemodWriteReg( pxAgvDev, AGV_RX1, AGV_DEMOD_REG22, usData );
    if( iRet < 0 )
    {
        log_err( "%s: demod%d write reg[%x] failed\r\n", __func__, AGV_RX1,
                 AGV_DEMOD_REG22 );
        return -1;
    }

    /* LO matching for 3500-6000MHz */
    iRet = AgvDemodLoMatching( pxAgvDev, AGV_RX1 );
    if( iRet < 0 )
    {
        log_err( "%s: demod%d LO matching failed\r\n", __func__, AGV_RX0);
        return -1;
    }


    return 0;
}
