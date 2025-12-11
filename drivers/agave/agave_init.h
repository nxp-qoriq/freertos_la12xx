// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __AGAVE_INIT_H
#define __AGAVE_INIT_H
#include "FreeRTOS.h"
#include "gpio.h"
#include "gpio_regs.h"
#include "fsl_dspi.h"
/*
 * Macro Definition
 */
#define AGV_GPIO_BLK_RF_DET    ( GPIO_1 )
#define AGV_GPIO_DEV0_DET_BIT  ( 3 )
#define AGV_GPIO_DEV1_DET_BIT  ( 4 )

#define AGV_GPIO_BLK_RF_CTRL   ( GPIO_2 )
/*
 * Enum Definition
 */
typedef enum AgaveInstance
{
    AGV_DEV_0,
    AGV_DEV_1,
    AGV_DEV_MAX
}AgvInstance_t;

typedef enum AgaveGpioFunc
{
    AGV_GPIO_DSPI_SEL,
    AGV_GPIO_TRX0_PA_EN,
    AGV_GPIO_TRX0_LNA_EN,
    AGV_GPIO_TRX0_ANT_SEL,
    AGV_GPIO_TRX0_LB_EN,
    AGV_GPIO_TRX1_PA_EN,
    AGV_GPIO_TRX1_LNA_EN,
    AGV_GPIO_TRX1_ANT_SEL,
    AGV_GPIO_TRX1_LB_EN,
    AGV_GPIO_MAX
} AgvGpioFunc_t;

typedef enum AgaveDspiSlave
{
    AGV_DSPI_SLV_SYNTH,
    AGV_DSPI_SLV_DEMOD_0,
    AGV_DSPI_SLV_DEMOD_1
} AgvDspiSlv_t;

typedef enum AgaveTx
{
    AGV_TX0,
    AGV_TX1
} AgvTx_t;

typedef enum AgaveRx
{
    AGV_RX0,
    AGV_RX1
} AgvRx_t;

typedef enum AgavePath
{
    AGV_PATH0,
    AGV_PATH1
} AgvPath_t;

typedef enum AgavePathState
{
    AGV_PATH_DISABLE,
    AGV_PATH_TX,
    AGV_PATH_RX
} AgvPathState_t;

typedef enum State
{
    DISABLE,
    ENABLE
} State_t;
/*
 * Structure Definition
 */
/* GPIO details */
typedef struct AgaveGpio
{
    GpioModule_t    eBlock;
    GpioType_t      etype;
    uint32_t        ulPin;
    bool            ucDefaultVal;
} AgvGpio_t;

/* Agave device structure */
typedef struct AgaveDev
{
    /* Agave Card Id */
    AgvInstance_t eDeviceId;

    /* GPIO details */
    AgvGpio_t eGpio[AGV_GPIO_MAX];

    /* TODO: Add read/write function pointers*/
    /* TODO: Calibration table */
    /* TODO: Current status */
    /* TODO: misc info/ metadata */
    /* TODO: RFCmd structure, queues, semaphores */
    DspiBlock_t eDspiBlock;
    uint8_t ucCsMask;
    struct LA12xxDspiInstance * xDspiHandle;
}AgvDevice_t, *AgvHandle_t;
/*
 * Function Declaration
 */
int32_t iAgaveInit( void );
int32_t iAgvSelectDspiSlave( AgvDevice_t *pxAgvDev, AgvDspiSlv_t eSlaveDev );
BaseType_t xAgvCtrlLoopback( AgvInstance_t eAgvInstance, AgvPath_t ePath,
                             State_t eState );
BaseType_t xAgvRxGainCtrl( AgvInstance_t eAgvInstance, AgvRx_t eRxId,
                           int32_t iAttn, int32_t iGain );
BaseType_t xAgvCtrlFeState( AgvInstance_t eAgvInstance, AgvPath_t eTxId,
                            AgvPathState_t eState );
BaseType_t xAgvReadReg( AgvInstance_t eAgvInstance, AgvDspiSlv_t eSlaveDev,
                        uint8_t ucAddr, uint16_t *usData );
BaseType_t xAgvWriteReg( AgvInstance_t eAgvInstance, AgvDspiSlv_t eSlaveDev,
                         uint8_t ucAddr, uint16_t usData );
BaseType_t xAgvAdjustPllFreq( AgvInstance_t eAgvInstance,
					   int32_t ifreq );
#endif //__AGAVE_INIT_H

