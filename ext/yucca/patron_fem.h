// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include "rf_dev.h"
#include "types.h"
#include "gpio.h"
#include "gpio_regs.h"

#define RFIC_GPIO_2                        ( GPIO_2 )
#define RFIC_GPIO_3                        ( GPIO_3 )

/* MW-A specific PINs */
/* GPIO PIN's */
#define RFIC_GPIO_RESET_YC1                ( 0 )
#define RFIC_GPIO_RESET_YC2                ( 1 )
#define RFIC_GPIO_TRX_YC1                  ( 17 )
#define RFIC_GPIO_TRX_YC2                  ( 18 )


#define RFIC_GPIO_FEM_YC1_TX1_PA_LNA_CTRL   ( 22 )
#define RFIC_GPIO_FEM_YC1_RX1_LNA_BYPASS    ( 23 )
#define RFIC_GPIO_FEM_YC1_TRX1_SW_CTRL     ( 24 )

#define RFIC_GPIO_FEM_YC1_TX2_PA_LNA_CTRL   ( 26 )
#define RFIC_GPIO_FEM_YC1_RX2_LNA_BYPASS    ( 27 )
#define RFIC_GPIO_FEM_YC1_TRX2_SW_CTRL     ( 28 )

#define RFIC_GPIO_FEM_YC2_TX1_PA_LNA_CTRL   ( 13 )
#define RFIC_GPIO_FEM_YC2_RX1_LNA_BYPASS    ( 14 )
#define RFIC_GPIO_FEM_YC2_TRX1_SW_CTRL     ( 15 )

#define RFIC_GPIO_FEM_YC2_TX2_PA_LNA_CTRL   ( 9 )
#define RFIC_GPIO_FEM_YC2_RX2_LNA_BYPASS    ( 10 )
#define RFIC_GPIO_FEM_YC2_TRX2_SW_CTRL     ( 11 )


/* Common PINs for MW-A and MW-B */
#define RFIC_GPIO_FEM_YC1_RX1_SW_CTRL       ( 25 )
#define RFIC_GPIO_FEM_YC1_RX2_SW_CTRL       ( 29 )
#define RFIC_GPIO_FEM_YC2_RX1_SW_CTRL       ( 16 )
#define RFIC_GPIO_FEM_YC2_RX2_SW_CTRL       ( 12 )

#define RFIC_GPIO_AGC_EN                   ( 21 )

/* MW-B specific PINs */
#define RFIC_GPIO_FEM_TRX             ( 8 )
#define RFIC_GPIO_FEM_PA_LNA_CTRL ( 9 )
#define RFIC_GPIO_FEM_TRX_SW_CTRL     ( 10 )
#define RFIC_GPIO_FEM_LNA_BYPASS  ( 11 )

void xFemSwitchRxTX( RficHandle_t xHandle,
                     bool bIsRx , bool bIsRxDcCalib);
int32_t iFemGpioInit();
void xFemDpdSwCtrl( RficHandle_t xHandle,
                     bool onoff);
void xFemLna2Bypass( RficHandle_t xHandle,
                     bool onoff);
