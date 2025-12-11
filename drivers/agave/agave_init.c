// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "agave_init.h"
#include "agave_synth.h"
#include "agave_demod.h"
#include "agave_cli.h"

static AgvDevice_t *pxAgvDev0 = NULL;
static AgvDevice_t *pxAgvDev1 = NULL;

/* [0 : 1] = [Pin Number : Default Value] */
static const uint32_t AGV_DEV0_GPIO[ AGV_GPIO_MAX ][ 2 ] = {
	{ 2, 1 }, { 8, 0 }, { 9, 0 }, { 12, 0 }, { 11, 1 }, { 16, 0 },
	{ 15, 0 }, { 13, 0 }, { 14, 1 }};
static const uint32_t AGV_DEV1_GPIO[ AGV_GPIO_MAX ][ 2 ] = {
        { 17, 1 }, { 23, 0}, { 24, 0 }, { 27, 0 }, { 26, 1 }, { 31, 0 },
        { 30, 0 }, { 28, 0}, { 29, 1 }};

static inline AgvDevice_t * xGetAgvDev( AgvInstance_t eAgvInstance )
{
    if ( eAgvInstance == AGV_DEV_0 )
        return pxAgvDev0;
    else if ( eAgvInstance == AGV_DEV_1 )
        return pxAgvDev1;
    else
        return NULL;
}

BaseType_t xAgvCtrlLoopback( AgvInstance_t eAgvInstance, AgvPath_t ePath,
                             State_t eState )
{
    AgvDevice_t *pxAgvDev = xGetAgvDev( eAgvInstance );
    AgvGpioFunc_t eLb;

    eLb = ( AGV_PATH0 == ePath ) ? AGV_GPIO_TRX0_LB_EN : AGV_GPIO_TRX1_LB_EN;

    if( ENABLE == eState )
    {
        /* Enable Loopback */
        if( exGpioSetData( pxAgvDev->eGpio[ eLb ].eBlock,
                           pxAgvDev->eGpio[ eLb ].ulPin, 0 ))
        {
            log_err( "%s: LB GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
    }
    else
    {
        /* Disable Loopback */
        if( exGpioSetData( pxAgvDev->eGpio[ eLb ].eBlock,
                           pxAgvDev->eGpio[ eLb ].ulPin, 1 ))
        {
            log_err( "%s: ANT GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
    }

    return pdPASS;
}
BaseType_t xAgvRxGainCtrl( AgvInstance_t eAgvInstance, AgvRx_t eRxId,
			   int32_t iAttn, int32_t iGain )
{
    AgvDevice_t *pxAgvDev = xGetAgvDev( eAgvInstance );

    /* Check for valid range for attenuation value */
    if(( AGV_DEMOD_ATTN_MIN > iAttn ) || ( AGV_DEMOD_ATTN_MAX < iAttn ))
    {
        log_err( "%s: Invalid attenuation value\r\n", __func__ );
        return pdFALSE;
    }

    /* Check for valid range for gain value */
    if(( AGV_DEMOD_GAIN_MIN > iGain ) || ( AGV_DEMOD_GAIN_MAX < iGain ))
    {
        log_err( "%s: Invalid gain value\r\n", __func__ );
        return pdFALSE;
    }

    /* Select demodulator */
    if( iAgvSelectDspiSlave( pxAgvDev, AGV_DSPI_SLV_DEMOD_0 ))
    {
        log_err( "%s: Select DSPI Slave device failed\r\n", __func__);
        return pdFAIL;
    }

    if( xAgvDemodGainCtrl( pxAgvDev, eRxId, iAttn, iGain ))
    {
        log_err( "%s: failed\r\n", __func__ );
	return pdFALSE;
    }

    return pdPASS;
}
BaseType_t xAgvAdjustPllFreq( AgvInstance_t eAgvInstance,
			   int32_t ifreq )
{
    AgvDevice_t *pxAgvDev = xGetAgvDev( eAgvInstance );
     /* Select DSPI slave device */
    if( iAgvSelectDspiSlave( pxAgvDev,  AGV_DSPI_SLV_SYNTH))
    {
        log_err( "%s: Select DSPI Slave device failed\r\n", __func__);
        return pdFAIL;
    }
    AgvAdjustPllFreq( pxAgvDev, ifreq );
    return pdPASS;
}

BaseType_t xAgvCtrlFeState( AgvInstance_t eAgvInstance, AgvPath_t ePath,
                            AgvPathState_t eState )
{
    AgvDevice_t *pxAgvDev = xGetAgvDev( eAgvInstance );
    AgvGpioFunc_t ePa, eLna, eAnt;

    ePa  = ( AGV_PATH0 == ePath ) ? AGV_GPIO_TRX0_PA_EN : AGV_GPIO_TRX1_PA_EN;
    eLna = ( AGV_PATH0 == ePath ) ? AGV_GPIO_TRX0_LNA_EN : AGV_GPIO_TRX1_LNA_EN;
    eAnt = ( AGV_PATH0 == ePath ) ? AGV_GPIO_TRX0_ANT_SEL : AGV_GPIO_TRX1_ANT_SEL;

    if( AGV_PATH_TX == eState )
    {
        /* Disable LNA */
        if( exGpioSetData( pxAgvDev->eGpio[ eLna ].eBlock,
                           pxAgvDev->eGpio[ eLna ].ulPin, 0 ))
        {
            log_err( "%s: LNA GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
        /* Switch antenna to Tx */
        if( exGpioSetData( pxAgvDev->eGpio[ eAnt ].eBlock,
                           pxAgvDev->eGpio[ eAnt ].ulPin, 1 ))
        {
            log_err( "%s: ANT GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
        /* Enable PA */
        if( exGpioSetData( pxAgvDev->eGpio[ ePa ].eBlock,
                           pxAgvDev->eGpio[ ePa ].ulPin, 1 ))
        {
            log_err( "%s: PA GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
    }
    else if( AGV_PATH_RX == eState )
    {
        /* Disable PA */
        if( exGpioSetData( pxAgvDev->eGpio[ ePa ].eBlock,
                           pxAgvDev->eGpio[ ePa ].ulPin, 0 ))
        {
            log_err( "%s: PA GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
        /* Switch antenna to Rx */
        if( exGpioSetData( pxAgvDev->eGpio[ eAnt ].eBlock,
                           pxAgvDev->eGpio[ eAnt ].ulPin, 0 ))
        {
            log_err( "%s: ANT GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
        /* Enable LNA */
        if( exGpioSetData( pxAgvDev->eGpio[ eLna ].eBlock,
                           pxAgvDev->eGpio[ eLna ].ulPin, 1 ))
        {
            log_err( "%s: LNA GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
    }
    else
    {
        /* Disable PA */
        if( exGpioSetData( pxAgvDev->eGpio[ ePa ].eBlock,
                           pxAgvDev->eGpio[ ePa ].ulPin, 0 ))
        {
            log_err( "%s: PA GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
        /* Disable LNA */
        if( exGpioSetData( pxAgvDev->eGpio[ eLna ].eBlock,
                           pxAgvDev->eGpio[ eLna ].ulPin, 0 ))
        {
            log_err( "%s: LNA GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
        /* Switch antenna to Rx */
        if( exGpioSetData( pxAgvDev->eGpio[ eAnt ].eBlock,
                           pxAgvDev->eGpio[ eAnt ].ulPin, 0 ))
        {
            log_err( "%s: ANT GPIO set failed\r\n", __func__ );
            return pdFAIL;
        }
    }

    return pdPASS;
}

BaseType_t xAgvReadReg( AgvInstance_t eAgvInstance, AgvDspiSlv_t eSlaveDev,
                        uint8_t ucAddr, uint16_t *usData )
{
    AgvDevice_t *pxAgvDev = xGetAgvDev( eAgvInstance );
    AgvRx_t eRxId;

    /* Select DSPI slave device */
    if( iAgvSelectDspiSlave( pxAgvDev, eSlaveDev ))
    {
        log_err( "%s: Select DSPI Slave device failed\r\n", __func__);
        return pdFAIL;
    }

    if( AGV_DSPI_SLV_SYNTH == eSlaveDev )
    {
        if( prvAgvSynthReadReg( pxAgvDev, ucAddr, usData ))
        {
            log_err( "%s: Synth read reg failed\r\n", __func__);
            return pdFAIL;
        }
    }
    else
    {
        eRxId = (( AGV_DSPI_SLV_DEMOD_0 == eSlaveDev) ? AGV_RX0 : AGV_RX1 );
        if( prvAgvDemodReadReg( pxAgvDev, eRxId, ucAddr, ( uint8_t * )usData ))
        {
            log_err( "%s: Demod read reg failed\r\n", __func__);
            return pdFAIL;
        }
	*usData = ( *usData >> 8 );
    }

    return pdPASS;
}

BaseType_t xAgvWriteReg( AgvInstance_t eAgvInstance, AgvDspiSlv_t eSlaveDev,
                         uint8_t ucAddr, uint16_t usData )
{
    AgvDevice_t *pxAgvDev = xGetAgvDev( eAgvInstance );
    AgvRx_t eRxId;

    /* Select DSPI slave device */
    if( iAgvSelectDspiSlave( pxAgvDev, eSlaveDev ))
    {
        log_err( "%s: Select DSPI Slave device failed\r\n", __func__);
        return pdFAIL;
    }

    if( AGV_DSPI_SLV_SYNTH == eSlaveDev )
    {
        if( prvAgvSynthWriteReg(pxAgvDev, ucAddr, usData ))
        {
            log_err( "%s: Synth write reg failed\r\n", __func__);
            return pdFAIL;
        }
    }
    else
    {
        eRxId = (( AGV_DSPI_SLV_DEMOD_0 == eSlaveDev) ? AGV_RX0 : AGV_RX1 );
        if( prvAgvDemodWriteReg( pxAgvDev, eRxId, ucAddr, ( uint8_t )usData ))
        {
            log_err( "%s: Demod write reg failed\r\n", __func__);
            return pdFAIL;
        }
    }

    log_info( "Write Reg Addr[0x%x] = 0x%x\r\n", ucAddr, usData);
    return pdPASS;


}

int32_t iAgvSelectDspiSlave( AgvDevice_t *pxAgvDev, AgvDspiSlv_t eSlaveDev )
{
    int32_t iRet;

    if( AGV_DSPI_SLV_DEMOD_0 == eSlaveDev || AGV_DSPI_SLV_DEMOD_1 == eSlaveDev )
    {
        /* Set SPI_SEL0 GPIO to 0 */
        iRet = exGpioSetData( pxAgvDev->eGpio[ AGV_GPIO_DSPI_SEL ].eBlock,
                              pxAgvDev->eGpio[ AGV_GPIO_DSPI_SEL ].ulPin, 0 );
    }
    /* AGV_DSPI_SLV_SYNTH */
    else
    {
        /* Set SPI_SEL0 GPIO to 1 */
        iRet = exGpioSetData( pxAgvDev->eGpio[ AGV_GPIO_DSPI_SEL ].eBlock,
                              pxAgvDev->eGpio[ AGV_GPIO_DSPI_SEL ].ulPin, 1 );
    }

    if( 0 > iRet )
    {
        log_err( "%s: Set GPIO[%d] fail, error[%d]\r\n", __func__,
                 pxAgvDev->eGpio[AGV_GPIO_DSPI_SEL].ulPin, iRet );
    }

    return iRet;
}

static int32_t prvAgvPreGpioInit( void )
{
    /* RF Card 1 detect */
    if( exGpioInit( AGV_GPIO_BLK_RF_DET, AGV_GPIO_DEV0_DET_BIT,
                    GPIO_INPUT ) )
    {
        log_err( "%s: GPIO-%d init failed\r\n", __func__, AGV_GPIO_DEV0_DET_BIT );
        return -1;
    }

    /* RF Card 2 detect */
    if( exGpioInit( AGV_GPIO_BLK_RF_DET, AGV_GPIO_DEV1_DET_BIT,
                    GPIO_INPUT ) )
    {
        log_err( "%s: GPIO-%d init failed\r\n", __func__, AGV_GPIO_DEV1_DET_BIT );
        return -1;
    }

    return 0;
}

static int32_t prvAgvPostGpioInit( AgvDevice_t *pxAgvDev )
{
    uint8_t index;

    for( index = 0; index < AGV_GPIO_MAX; index++ )
    {
        if( exGpioInit( pxAgvDev->eGpio[ index ].eBlock,
                        pxAgvDev->eGpio[ index ].ulPin,
                        pxAgvDev->eGpio[ index ].etype ))
        {
            log_err( "%s: GPIO-%d init failed\r\n", __func__,
                     pxAgvDev->eGpio[ index ].ulPin );
            return -1;
        }

	log_dbg( "%s: GPIO[%d], Default Val[%d]\r\n", __func__,
                  pxAgvDev->eGpio[ index ].ulPin,
                  pxAgvDev->eGpio[ index ].ucDefaultVal );

        if( exGpioSetData( pxAgvDev->eGpio[ index ].eBlock,
                           pxAgvDev->eGpio[ index ].ulPin,
                           pxAgvDev->eGpio[ index ].ucDefaultVal ))
        {
            log_err( "%s: GPIO-%d set failed\r\n", __func__,
                     pxAgvDev->eGpio[ index ].ulPin );
            return -1;
        }
    }

    return 0;
}

int32_t iAgvDeviceParamInit( AgvDevice_t *pxAgvDev, AgvInstance_t eAgvInstance )
{
    uint8_t index;

    if( AGV_DEV_0 == eAgvInstance )
    {
        /* Initialize GPIO Details */
        for( index = 0; index < AGV_GPIO_MAX; index++ )
        {
            pxAgvDev->eGpio[ index ].eBlock = AGV_GPIO_BLK_RF_CTRL;
            pxAgvDev->eGpio[ index ].etype = GPIO_OUTPUT;
            pxAgvDev->eGpio[ index ].ulPin = AGV_DEV0_GPIO[ index ][ 0 ];
            pxAgvDev->eGpio[ index ].ucDefaultVal = AGV_DEV0_GPIO[ index ][ 1 ];
        }

        /*Initialize DPSI block number */
         pxAgvDev->eDspiBlock = DSPI_BLOCK2;
    }
    else if ( AGV_DEV_1 == eAgvInstance )
    {
        /* Initialize GPIO Details */
        for( index = 0; index < AGV_GPIO_MAX; index++ )
        {
            pxAgvDev->eGpio[ index ].eBlock = AGV_GPIO_BLK_RF_CTRL;
            pxAgvDev->eGpio[ index ].etype = GPIO_OUTPUT;
            pxAgvDev->eGpio[ index ].ulPin = AGV_DEV1_GPIO[ index ][ 0 ];
            pxAgvDev->eGpio[ index ].ucDefaultVal = AGV_DEV1_GPIO[ index ][ 1 ];
        }

        /*Initialize DPSI block number */
        pxAgvDev->eDspiBlock = DSPI_BLOCK3;
    }
    else
    {
        log_err( "%s: Invalid Agave Instance - %d\r\n", __func__,
                 eAgvInstance );
        return -1;
    }

    /* Initialize DSPI chip select mask for CS0 and CS1 */
    pxAgvDev->ucCsMask = (( 1 << DSPI_CS0 ) | ( 1 << DSPI_CS1 ));

    return 0;
}

static AgvDevice_t *prvAgvDevInit( AgvInstance_t eAgvInstance )
{
    AgvDevice_t *pxAgvDev = NULL;
    int32_t iRet;

    /* Allocate Agave Device structure */
    pxAgvDev = ( AgvDevice_t * )pvGeulMalloc( sizeof( AgvDevice_t ));
    if( NULL == pxAgvDev )
    {
        log_err( "%s: Agave Dev alloc failed [instance - %d]\r\n",
                 __func__, eAgvInstance );
        goto out;
    }

    /* Initialize Device parameters */
    iRet = iAgvDeviceParamInit( pxAgvDev, eAgvInstance );
    if( 0 != iRet )
    {
        log_err( "%s: Agave Dev Param init failed [instance - %d]\r\n",
                 __func__, eAgvInstance );
        goto out;
    }

    /* Initialize Agave RF Control GPIO */
    iRet = prvAgvPostGpioInit( pxAgvDev );
    if( 0 != iRet )
    {
        log_err( "%s: Agave RF CTRL GPIO init failed [instance - %d]\r\n",
                 __func__, eAgvInstance );
        goto out;
    }

    /* Initialize DSPI Handler */
    pxAgvDev->xDspiHandle = pxDspiInit( pxAgvDev->eDspiBlock,
                                        pxAgvDev->ucCsMask );
    if( NULL == pxAgvDev->xDspiHandle )
    {
        log_err( "%s: Agave DSPI init failed [instance - %d]\r\n",
                  __func__, eAgvInstance );
        goto out;
    }

    /* Select Slave device - synthesizer */
    iRet = iAgvSelectDspiSlave( pxAgvDev, AGV_DSPI_SLV_SYNTH );
    if( 0 != iRet )
    {
        log_err( "%s: Agave select DSPI slave dev failed [instance - %d]\r\n",
                  __func__, eAgvInstance );
        goto out;
    }

    /* Sythesizer Initialization */
    iRet = AgvSynthInit( pxAgvDev );
    if( 0 != iRet )
    {
        log_err( "%s: Agave Synth init failed [instance - %d]\r\n",
                 __func__, eAgvInstance );
        goto out;
    }

    /* Select Slave device - demodulator */
    iRet = iAgvSelectDspiSlave( pxAgvDev, AGV_DSPI_SLV_DEMOD_0 );
    if( 0 != iRet )
    {
        log_err( "%s: Agave select DSPI slave dev failed [instance - %d]\r\n",
                  __func__, eAgvInstance );
        goto out;
    }

    /* Demodulator Initialization */
    iRet = AgvDemodInit( pxAgvDev );
    if( 0 != iRet )
    {
        log_err( "%s: Agave Demod init failed [instance - %d]\r\n",
                 __func__, eAgvInstance );
        goto out;
    }

    return pxAgvDev;
out:
    if( NULL != pxAgvDev )
    {
        vGeulFree( pxAgvDev );
        pxAgvDev = NULL;
    }
    return pxAgvDev;
}

int32_t iAgaveInit( void )
{
    uint32_t ulGpioData;
    int32_t iRet = 0;

    /* Initialize only those GPIO which are required to detect agave card */
    iRet = prvAgvPreGpioInit();
    if( 0 != iRet )
    {
        log_err( "%s: Pre GPIO init failed\r\n", __func__ );
        goto out;
    }

    /* Read GPIO data register to detect agave card */
    iRet = exGpioGetDataRegister( AGV_GPIO_BLK_RF_DET, &ulGpioData );
    if( 0 != iRet )
    {
        log_err( "%s: GPIO read failed\r\n", __func__ );
        goto out;
    }

    /* Check instance1 detect bit */
    if ( 0 == ( ulGpioData & BITS( AGV_GPIO_DEV0_DET_BIT )))
    {
        log_info( "%s: Agave Dev0 detected\r\n", __func__ );
        pxAgvDev0 = prvAgvDevInit( AGV_DEV_0 );
        if( NULL == pxAgvDev0 )
        {
            log_err( "%s: Agave Dev0 init failed\r\n", __func__ );
            iRet = -1;
            goto out;
        }
    }
    else
    {
        log_info( "%s: Agave Dev0 is not detected\r\n", __func__ );
    }

    /* Check instance2 detect bit */
    if ( 0 == ( ulGpioData & BITS( AGV_GPIO_DEV1_DET_BIT )))
    {
        log_info( "%s: Agave Dev1 detected\r\n", __func__ );
        pxAgvDev1 = prvAgvDevInit( AGV_DEV_1 );
        if( NULL == pxAgvDev1 )
        {
            log_err( "%s: Agave Dev1 init failed\r\n", __func__ );
            iRet = -1;
            goto out;
        }
    }
    else
    {
        log_info( "%s: Agave Dev1 is not detected\r\n", __func__ );
    }

    /* RF CLI Register */
    if( pdFAIL == vRegisterRFCli() )
    {
        log_err( "%s: Agave CLI register failed\r\n", __func__ );
        iRet = -1;
        goto out;
    }
out:
    return iRet;
}
