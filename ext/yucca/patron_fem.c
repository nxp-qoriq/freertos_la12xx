// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include "FreeRTOS.h"
#include "types.h"
#include "yuc_rfic.h"
#include "patron_fem.h"

extern volatile uint32_t brd_ver;

void xFemSwitchRxTX( RficHandle_t xHandle,
                     bool bIsRx , bool bIsRxDcCalib)
{
    vTaskSuspendAll();

    
    uint32_t uPALnaVal = 1;
    uint32_t uPinVal = 1;
    uint32_t isUniGPIO = 0;
#ifndef GEUL_LA1224
    if (brd_ver != 0)
        isUniGPIO = 1;
#endif
    if (bIsRxDcCalib || !bIsRx)
        uPALnaVal = 0;

    if (!bIsRx)
        uPinVal = 0;
    if (isUniGPIO == 0)
    {
        if ( ( xHandle->eFR1Mode & eFR1Mode2t2r0 ) )
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_TRX_YC1, uPinVal );

        if ( ( xHandle->eFR1Mode & eFR1Mode2t2r1 ) )
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_TRX_YC2, uPinVal );

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r0 ) )
        {    
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX1_PA_LNA_CTRL, uPALnaVal );
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TRX1_SW_CTRL, uPinVal );
        }

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r1 ) )
        {
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX2_PA_LNA_CTRL, uPALnaVal );
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TRX2_SW_CTRL, uPinVal );
        }

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r2 ) )
        {
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TX1_PA_LNA_CTRL, uPALnaVal );
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TRX1_SW_CTRL, uPinVal );
        }

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r3 ) )
        {
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TX2_PA_LNA_CTRL, uPALnaVal );
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TRX2_SW_CTRL, uPinVal );
        }

    }
    else
    {
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_TRX, uPinVal );
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_PA_LNA_CTRL, uPALnaVal );
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_TRX_SW_CTRL, uPinVal );
    }

    uint32_t gpio_data;
    exGpioGetDataRegister(RFIC_GPIO_2, &gpio_data);
    RF_LOGDBG("GPIO Set to 0x%X",gpio_data);

    xTaskResumeAll();
}

void xFemLna2Bypass( RficHandle_t xHandle, bool onoff)
{
    vTaskSuspendAll();
    uint32_t isUniGPIO = 0;
#ifndef GEUL_LA1224
    if (brd_ver != 0)
        isUniGPIO = 1;
#endif

     /* 0 - LNA2 Bypass Mode Disable */
     /* 1 - LNA2 Bypass Mode Enable  */

    if (isUniGPIO == 0)
    {
        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r0 ) )
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX1_LNA_BYPASS, onoff );

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r1 ) )
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX2_LNA_BYPASS, onoff );

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r2 ) )
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX1_LNA_BYPASS, onoff );

        if ( ( xHandle->eFR1Mode & eFR1Mode1t1r3 ) )
            exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX2_LNA_BYPASS, onoff );

    }
#ifndef GEUL_LA1224
    else
    {
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_LNA_BYPASS, onoff );
    }
#endif

    xTaskResumeAll();
    uint32_t gpio_data;
    exGpioGetDataRegister(RFIC_GPIO_2, &gpio_data);
    RF_LOGDBG("GPIO Set to 0x%X",gpio_data);
}

void xFemDpdSwCtrl( RficHandle_t xHandle,
                     bool onoff)
{
    vTaskSuspendAll();

    /* 0 - DPD Path Enable
     * 1 - LNA Rx path Enable */
    if ( ( xHandle->eFR1Mode & eFR1Mode1t1r0 ) )
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX1_SW_CTRL, onoff );

    if ( ( xHandle->eFR1Mode & eFR1Mode1t1r1 ) )
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX2_SW_CTRL, onoff );

    if ( ( xHandle->eFR1Mode & eFR1Mode1t1r2 ) )
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX1_SW_CTRL, onoff );
    
    if ( ( xHandle->eFR1Mode & eFR1Mode1t1r3 ) )
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX2_SW_CTRL, onoff );

    xTaskResumeAll();
    uint32_t gpio_data;
    exGpioGetDataRegister(RFIC_GPIO_2, &gpio_data);
    RF_LOGDBG("GPIO Set to 0x%X",gpio_data);
}

int32_t iFemGpioInit()
{
    uint32_t gpdata;
    uint32_t isUniGPIO = 0;
#ifndef GEUL_LA1224
    if (brd_ver != 0)
        isUniGPIO = 1;
#endif

    RF_LOGDBG("Board version is %d",brd_ver);
    if (isUniGPIO == 0)
    {
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX1_LNA_BYPASS, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX2_LNA_BYPASS, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX1_LNA_BYPASS, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX2_LNA_BYPASS, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX1_PA_LNA_CTRL, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX2_PA_LNA_CTRL, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TRX1_SW_CTRL  , GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TRX2_SW_CTRL  , GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TX1_PA_LNA_CTRL, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX2_PA_LNA_CTRL, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TRX1_SW_CTRL  , GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TRX2_SW_CTRL  , GPIO_OUTPUT ) ) return -1;

        /* Set default - for both Yucca */
        /* RX1_LNA_BYPASS (Bypass LNA2) to 1  */
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX1_LNA_BYPASS, 1 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX1_LNA_BYPASS, 1 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX2_LNA_BYPASS, 1 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX2_LNA_BYPASS, 1 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX1_PA_LNA_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX2_PA_LNA_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TRX1_SW_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TRX2_SW_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TX1_PA_LNA_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_TX2_PA_LNA_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TRX1_SW_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_TRX2_SW_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_TRX_YC1              , 0 );
        exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_TRX_YC2              , 0 );
    }
#ifndef GEUL_LA1224
    else
    {
        if ( exGpioInit( RFIC_GPIO_3, RFIC_GPIO_FEM_LNA_BYPASS, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_3, RFIC_GPIO_FEM_PA_LNA_CTRL, GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_3, RFIC_GPIO_FEM_TRX_SW_CTRL  , GPIO_OUTPUT ) ) return -1;
        if ( exGpioInit( RFIC_GPIO_3, RFIC_GPIO_FEM_TRX , GPIO_OUTPUT ) ) return -1;

        /* Set default - for both Yucca */
        /* RX1_LNA_BYPASS (Bypass LNA2) to 1  */
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_LNA_BYPASS, 1 );
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_PA_LNA_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_TRX_SW_CTRL  , 0 );
        exGpioSetRFData( RFIC_GPIO_3, RFIC_GPIO_FEM_TRX              , 0 );

    }
#endif

    if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX1_SW_CTRL   , GPIO_OUTPUT ) ) return -1;
    if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX2_SW_CTRL   , GPIO_OUTPUT ) ) return -1;
    if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX1_SW_CTRL   , GPIO_OUTPUT ) ) return -1;
    if ( exGpioInit( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX2_SW_CTRL   , GPIO_OUTPUT ) ) return -1;

    /* RX1_DPD_SWITCH_CNTL to 1 */
    exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX1_SW_CTRL   , 1 );
    exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC1_RX2_SW_CTRL   , 1 );
    exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX1_SW_CTRL   , 1 );
    exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_FEM_YC2_RX2_SW_CTRL   , 1 );
    exGpioSetRFData( RFIC_GPIO_2, RFIC_GPIO_AGC_EN               , 0 );

    exGpioGetDataRegister( RFIC_GPIO_2, &gpdata );
    log_dbg(" GPIO2 Reg Value 0x%x \n", gpdata );

    return 0;
}
