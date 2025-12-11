// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#include "types.h"
#include "debug_console.h"
#include "gpio.h"
#include "gpio_regs.h"

static inline uint32_t ulGpioBaseAddr( GpioModule_t eGpioModule )
{
    uint32_t ulAddr = 0;

    switch( eGpioModule )
    {
        case GPIO_1:
            ulAddr = GPIO_BASE_ADDR( GPIO1_ADDR );
            break;

        case GPIO_2:
            ulAddr = GPIO_BASE_ADDR( GPIO2_ADDR );
            break;

        case GPIO_3:
            ulAddr = GPIO_BASE_ADDR( GPIO3_ADDR );
            break;

        case GPIO_4:
            ulAddr = GPIO_BASE_ADDR( GPIO4_ADDR );
            break;

        default:
            ulAddr = 0;
            log_err( "%s : eGpioModule (%d) is not supported\n\r", __func__, eGpioModule );
            break;
    }

    return ulAddr;
}

static inline bool bIsPinSupported( GpioModule_t eGpioModule,
                                    uint8_t ucPin )
{
    ( void ) eGpioModule;

    if( ucPin <= GPIO_PIN_MAX )
    {
        return true;
    }

    log_err( "%s : eGpioModule/ucPin (%d,%d) is not supported\n\r", __func__, eGpioModule, ucPin );
    return false;
}

static GpioStatusCode_t exGpioInputBufferEnable( GpioModule_t eGpioModule,
                                                 uint8_t ucPin )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );

    if( !pxGpioPort )
    {
        log_err( "%s : eGpioModule(%d) is not supported\n\r", __func__, eGpioModule );
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    ucPin = GPIO_PIN( ucPin );
    setbits_le32( &pxGpioPort->ulGpIbe, BITS( ucPin ) );

    return GPIO_SUCCESS;
}

GpioStatusCode_t exGpioInit( GpioModule_t eGpioModule,
                             uint8_t ucPin,
                             GpioType_t eGpioType )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );

    if( !pxGpioPort )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    if( !bIsPinSupported( eGpioModule, ucPin ) )
    {
        return GPIO_PIN_NOT_SUPPORTED;
    }

    ucPin = GPIO_PIN( ucPin );

    if( eGpioType == GPIO_INPUT )
    {
        clrbits_le32( &pxGpioPort->ulGpDir, BITS( ucPin ) );
        setbits_le32( &pxGpioPort->ulGpIbe, BITS( ucPin ) );
    }
    else if( eGpioType == GPIO_OUTPUT )
    {
        setbits_le32( &pxGpioPort->ulGpDir, BITS( ucPin ) );
        setbits_le32( &pxGpioPort->ulGpIbe, BITS( ucPin ) );
    }
    else
    {
        setbits_le32( &pxGpioPort->ulGpOdr, BITS( ucPin ) );
    }

    return GPIO_SUCCESS;
}

GpioStatusCode_t exGpioSetData( GpioModule_t eGpioModule,
                                uint8_t ucPin,
                                uint32_t ulVal )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );
    uint32_t ulDir;

    if( !pxGpioPort )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    if( !bIsPinSupported( eGpioModule, ucPin ) )
    {
        return GPIO_PIN_NOT_SUPPORTED;
    }

    if( exGpioInputBufferEnable( eGpioModule, ucPin ) )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    ucPin = GPIO_PIN( ucPin );

    ulDir = in_le32( &pxGpioPort->ulGpDir );

    if( ulDir & BITS( ucPin ) )
    {
        if( ulVal )
        {
            setbits_le32( &pxGpioPort->ulGpDat, BITS( ucPin ) );
        }
        else
        {
            clrbits_le32( &pxGpioPort->ulGpDat, BITS( ucPin ) );
        }
    }
    else
    {
        log_err( "%s: ucPin is input mode\n\r", __func__ );
        return GPIO_TYPE_NOT_SET;
    }

    return GPIO_SUCCESS;
}
GpioStatusCode_t exGpioSetInputData( GpioModule_t eGpioModule,
                                     uint8_t ucPin,
                                     uint32_t ulVal )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );
    uint32_t ulDir;

    if( !pxGpioPort )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    if( !bIsPinSupported( eGpioModule, ucPin ) )
    {
        return GPIO_PIN_NOT_SUPPORTED;
    }

    if( exGpioInputBufferEnable( eGpioModule, ucPin ) )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    ucPin = GPIO_PIN( ucPin );

    ulDir = in_le32( &pxGpioPort->ulGpDir );

    if( !( ulDir & BITS( ucPin ) ) )
    {
        if( ulVal )
        {
            setbits_le32( &pxGpioPort->ulGpDat, BITS( ucPin ) );
        }
        else
        {
            clrbits_le32( &pxGpioPort->ulGpDat, BITS( ucPin ) );
        }
    }
    else
    {
        log_err( "%s: ucPin is Output mode\n\r", __func__ );
        return GPIO_TYPE_NOT_SET;
    }

    return GPIO_SUCCESS;
}
GpioStatusCode_t exGpioGetData( GpioModule_t eGpioModule,
                                uint8_t ucPin )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );
    uint32_t ulGpDat;

    if( !pxGpioPort )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    if( !bIsPinSupported( eGpioModule, ucPin ) )
    {
        return GPIO_PIN_NOT_SUPPORTED;
    }

    if( exGpioInputBufferEnable( eGpioModule, ucPin ) )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    ucPin = GPIO_PIN( ucPin );

    ulGpDat = in_le32( &pxGpioPort->ulGpDat );
    log_dbg( "%s: ulGpDat : 0x%x\n\r", __func__, ulGpDat );

    if( ulGpDat & BITS( ucPin ) )
    {
        log_dbg( "%s: ucPin :%u is input is : 1\n\r", __func__, ucPin );
    }
    else
    {
        log_dbg( "%s: ucPin :%u is input is : 0\n\r", __func__, ucPin );
    }

    return GPIO_SUCCESS;
}
GpioStatusCode_t exGpioGetDataRegister( GpioModule_t eGpioModule,
                                        uint32_t * ulGpdata )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );

    if( !pxGpioPort )
    {
        return GPIO_CTRL_NOT_SUPPORTED;
    }

    *ulGpdata = in_le32( &pxGpioPort->ulGpDat );

    return GPIO_SUCCESS;
}


GpioStatusCode_t exGpioSetRFData( GpioModule_t eGpioModule,
                                  uint8_t ucPin,
                                  uint32_t ulVal )
{
    GpioPort_t * pxGpioPort = ( GpioPort_t * ) ( ulGpioBaseAddr( eGpioModule ) );

/*	if( !pxGpioPort )
 *  {
 *      return GPIO_CTRL_NOT_SUPPORTED;
 *  }
 *
 *  if( !bIsPinSupported( eGpioModule, ucPin ) )
 *  {
 *      return GPIO_PIN_NOT_SUPPORTED;
 *  }
 *
 *  if( exGpioInputBufferEnable( eGpioModule, ucPin ) )
 *  {
 *      return GPIO_CTRL_NOT_SUPPORTED;
 *  }
 *
 *  ucPin = GPIO_PIN( ucPin );
 */
    if( ulVal )
    {
        setbits_le32( &pxGpioPort->ulGpDat, BITS( ucPin ) );
    }
    else
    {
        clrbits_le32( &pxGpioPort->ulGpDat, BITS( ucPin ) );
    }

    return GPIO_SUCCESS;
}

GpioStatusCode_t exGpioPinControl( GpioModule_t eGpioModule,
                                   uint8_t ucPin,
                                   bool bIsFlag )
{
    log_dbg( "%s - In func\n\n\r", __func__ );

    if( !( bIsPinSupported( eGpioModule, ucPin ) ) )
    {
        log_err( "%s:GPIO PIN (%d) is not supported\n\n\r",
                 __func__, ucPin );
        return -GPIO_PIN_NOT_SUPPORTED;
    }

    uint32_t * p_reg = ( uint32_t * ) ( ulGpioBaseAddr( eGpioModule ) );

    if( bIsFlag == 1 )
    {
        setbits_le32( p_reg, ( uint32_t ) BITS( ucPin ) );
    }
    else if( bIsFlag == 0 )
    {
        clrbits_le32( p_reg, ( uint32_t ) BITS( ucPin ) );
    }
    else
    {
        log_err( "%s:Not supported flag\n\n\r", __func__, ucPin );
    }

    return GPIO_SUCCESS;
}
