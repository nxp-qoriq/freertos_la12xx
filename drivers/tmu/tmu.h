// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

 /**
 * @file        tmu.h
 * @brief       Thermal monitor unit APIs.
 * @addtogroup  TMU_API
 * @{
 */
#ifndef _TMU_REGS_H_
#define _TMU_REGS_H_

#include "gul_host_if.h"

#include "geul_tmu.h"

#define NO_PWR 0 
#define MAX_ATTEMPT 5
#define MAX_DELAY_ITR 100000

enum mtd_temp_sites {
    VSPA_TEMP  = 0,            /**<MTD VSPA Temperature site at 0th position. >**/
    FECA_TEMP  = 1,            /**<MTD FECA Temperature site at 1st position. >**/
    PCI_TEMP   = 2,            /**<MTD PCI Temperature site at 2nd position. >**/
    DIODE_TEMP = 3,            /**<MTD Diode Temperature site via i2c read.  >**/
};

typedef void (* mtdThermalHandler_t) ( struct mtd_thermalEvent * );
typedef uint32_t (* rfTempFn_t) ( void );
typedef uint32_t (* rfTempIrqEnableFn_t) ( int32_t threshold );

/**
 * @brief Function to initialize TMU module
 *
 * @return Void
 */
void tmuInit( void );

/**
 * @brief Function to get the temp of LA12xx
 *
 * @return Current temperature
 */

int32_t mtdGetTemp( void );

/**
 * @brief Function to get the temp of RF Card
 *
 * @return Current temperature
 */

int32_t rtdGetTemp( void );

/**
 * @brief Interrupt Handler to be called by of RF Card Driver
 *
 * @return Success or Failure 
 */
bool_t rtd_tempIrqHandler();
	
/**
 * @brief Function to register function to check RF card temperature
 * @param[in] rtd_temp_cbk Pointer to RF temp function function
 * @param[in] rtd_irq_cbk Pointer to RF temp Irq Enable function
 *
 * @return valid Id on success, RTD_REGISTER_FAILED on failure
 */

int8_t rtd_temp_fn_register( rfTempFn_t rtd_temp_cbk,
			rfTempIrqEnableFn_t rtd_irq_cbk);

/**
 * @brief Function to deregister function to check RF card temperature
 *
 * @return Void
 */

void rtd_temp_fun_deregister( void );
/**
 * @brief Function to register callback to get notification for thermal event
 * @param[in] mtd_cbk Pointer to callback function
 *
 * @return valid Id on success, MTD_REGISTER_FAILED on failure
 */

int8_t mtd_register( mtdThermalHandler_t mtd_cbk );

/**
 * @brief Function to deregister callback to get notification for thermal event
 * @param[in] id valid id passed at the time of callback registration
 *
 * @return Void
 */

void mtd_deregister( int8_t id );

/**
 * @brief Function to register MSI interrupt from HOST
 *
 * @return Void
 */
void tmuRegisterHostinterrupt( void * pvDevData );

/**
 * @brief : Function to enable TMU interrupts
 *
 * @return : Void
 */
void tmuEnableInterrupt( void );
#endif /* ifndef _TMU_REGS_H_ */
