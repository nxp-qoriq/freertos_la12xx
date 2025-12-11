// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef _GPIO_REGS_H_
#define _GPIO_REGS_H_

#define GPIO_BASE_ADDR( x )    ( ( uint32_t ) CCSR_BASE_ADDR + ( x ) )
#define GPIO_PIN_MAX    ( 31 )
#define GPIO_PIN( x )          ( ( uint8_t ) ( x ) )
#define BITS( x )              ( ( uint32_t ) ( 1 << ( GPIO_PIN_MAX - x ) ) )

typedef enum GpioAddr
{
    /* 16kb offset */
    GPIO1_ADDR = 0x1130000,
    GPIO2_ADDR = 0x1134000,
    GPIO3_ADDR = 0x1138000,
    GPIO4_ADDR = 0x113C000,
} GpioAddr_t;

typedef struct GpioPort
{
    uint32_t ulGpDir;
    uint32_t ulGpOdr;
    uint32_t ulGpDat;
    uint32_t ulGpIer;
    uint32_t ulGpImr;
    uint32_t ulGpIcr;
    uint32_t ulGpIbe;
} __attribute__( ( packed ) ) GpioPort_t;

#endif /* ifndef _GPIO_REGS_H_ */
