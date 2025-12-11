// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <debug_console.h>
#include <io.h>
#include <platform_def.h>
#include "mpic.h"

#define mainUART_COMMAND_CONSOLE_STACK_SIZE           ( 384 ) /* Stack Size in bytes 384*4=1536 */
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1 )
#define IPI_RX_TASK_STACKSIZE                         ( configMINIMAL_STACK_SIZE * 2 )
#define IPI_RX_TASK_PRIORITY                          ( tskIDLE_PRIORITY + 10 )
#define IDLE_CALIB_INIT_TASK_STACKSIZE                ( configMINIMAL_STACK_SIZE )
#define LATENCY_TASK1_STACKSIZE                       ( configMINIMAL_STACK_SIZE * 2)
#define LATENCY_TASK2_STACKSIZE                       ( configMINIMAL_STACK_SIZE * 2)
#define IDLE_CALIB_INIT_PRIORITY                      ( tskIDLE_PRIORITY + 1 )
#define LATENCY_TASK1_PRIORITY                        ( tskIDLE_PRIORITY + 4 )
#define LATENCY_TASK2_PRIORITY                        ( tskIDLE_PRIORITY + 3 )
#define TEST_FRAMEWORK_TASK_STACKSIZE                 ( configMINIMAL_STACK_SIZE * 2 )
#define TEST_FRAMEWORK_TASK_PRIORITY                  ( tskIDLE_PRIORITY + 2 )
#define RF_LOOPBACK_TASK_STACKSIZE                    ( configMINIMAL_STACK_SIZE * 2 )
#define RF_LOOPBACK_TASK_PRIORITY                     ( tskIDLE_PRIORITY + 2 )
#define HAWK_CORE_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )
#define HAWK_CORE_TASK_STACK_SIZE                     ( configMINIMAL_STACK_SIZE * 2 )
#define TMU_TASK_STACKSIZE                            ( configMINIMAL_STACK_SIZE * 2)
#ifndef WARMUP_ENABLE
#define TMU_TASK_PRIORITY                             ( tskIDLE_PRIORITY + 1 )
#define TMU_TASK_CORE                                 ( 3 )
#else
#define TMU_TASK_PRIORITY                             ( tskIDLE_PRIORITY + 6 )
#define TMU_TASK_CORE                                 ( 0 )
#define WARMUP_TASK_STACKSIZE                         ( configMINIMAL_STACK_SIZE * 2)
#define WARMUP_TASK_PRIORITY                          ( tskIDLE_PRIORITY + 5 )
#endif

#if (defined LA12XX_DRIVER_PCI) || (defined LA12XX_DRIVER_PCI_LAT_FP)
#define PCIE_TASK_STACKSIZE                            ( configMINIMAL_STACK_SIZE * 2)
#define PCIE_TASK_PRIORITY                             ( tskIDLE_PRIORITY + 6 )
#define PCIE_RESET_TASK_STACKSIZE                      ( configMINIMAL_STACK_SIZE)
#define PCIE_RESET_TASK_PRIORITY                       ( tskIDLE_PRIORITY + 5 )
#endif

#define DCSTMON_TASK_CORE                             ( 4 )
#define DCSTMON_TASK_STACKSIZE                        ( configMINIMAL_STACK_SIZE * 2)
#define DCSTMON_TASK_PRIORITY                         ( tskIDLE_PRIORITY + 4 )

#define lower_32_bits(n)                              ((uint32_t)(n))
#ifdef ARCH64
#define upper_32_bits(n)                              ((uint32_t)(((n) >> 16) >> 16))
#else
#define upper_32_bits(n) 0
#endif
#define ALIGN(x, a)                                   (((x) + (a) - 1) & ~((a) - 1));

extern bool_t vPortTickISR( uint32_t, void * );

void vBoardEarlyInit(uint8_t);
void vPrintTbgenClkinfo(void);
void vSCFGInitTbgenClk(volatile struct gul_hif *pxHif, int iLSDCSInitStatus, int * iHsDcsIPClkEn);
void vInitTbgenTTIConnectivity(void);
void vSocInit(uint8_t);
void vBootRelease(uint8_t);

#ifndef GEUL_LA12XX
int32_t iSetupPcie(void);
#endif

void *pvGeulMalloc( size_t xWantedSize );
void vGeulFree( void *pv );

#if ( configUSE_HAWK_FILTER_FACILITY == 1 )
extern void vSetHAWKMarker(bool bMark);
#endif

#ifdef WARMUP_ENABLE
extern int lava_feca_app();
#endif

#define L1C_CLI_TASK_PRIORITY ( tskIDLE_PRIORITY + 2 )
#define HOST_CLI_TASK_PRIORITY ( tskIDLE_PRIORITY + 2 )
#define CLI_TASK_STACK_SIZE ( configMINIMAL_STACK_SIZE * 2)*3

#endif /* _COMMON_H_ */
