// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include "common.h"
#include "io.h"
#include "immap.h"
#include "mpic.h"

#ifdef CONFIG_WDOG_LE
    #define watchdog_in32( a )        in_le32( (volatile uint32_t *)a )
    #define watchdog_out32( a, v )    out_le32( (volatile uint32_t *)a, v )
#else
    #define watchdog_in32( a )        in_be32( a )
    #define watchdog_out32( a, v )    out_be32( a, v )
#endif

#define PCTBEN_REG                   ( PCTB_BASE_ADDR + 0x8a0 )

#define SCFG_WDOGTOUT_RES            18
#define SCFG_WDOGTOUT_RES_CORE0      14
#define SCFG_CONFIG_CONTROL0_WDOG    SCFG_BASE_ADDR + 0x000

#define CLEAR_BIT( addr, bit )    watchdog_out32( addr, in_le32( (volatile uint32_t *)addr ) & ( uint32_t ) ( ~( 1 << bit ) ) )

#define WDOG_LOAD_REG         ( WDOG_BASE_ADDR + 0x000 )
#define WDOG_VALUE_REG        ( WDOG_BASE_ADDR + 0x004 )
#define WDOG_CONTROL_REG      ( WDOG_BASE_ADDR + 0x008 )
#define WDOG_INTCLR_REG       ( WDOG_BASE_ADDR + 0x00c )
#define WDOG_RIS_REG          ( WDOG_BASE_ADDR + 0x010 )
#define WDOG_MIS_REG          ( WDOG_BASE_ADDR + 0x014 )
#define WDOG_LOCK_REG         ( WDOG_BASE_ADDR + 0xc00 )
#define WDOG_ITCR_REG         ( WDOG_BASE_ADDR + 0xf00 )
#define WDOG_ITOP_REG         ( WDOG_BASE_ADDR + 0xf04 )

#define WDOG_CONTROL_INTEN    ( 1 << 0 )
#define WDOG_CONTROL_RESEN    ( 1 << 1 )
#define WDOG_UNLOCK           0x1ACCE551
#define WDOG_LOCK             0x00000001

bool xWatchdogIrqHandler( uint32_t ulIrq_No,
                          void * vDev_Data );
#endif /* __WATCHDOG_H__*/
