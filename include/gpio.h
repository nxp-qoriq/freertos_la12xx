// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef _GPIO_H_
#define _GPIO_H_

/**
 * @file        gpio.h
 * @brief       This file contains the GPIO-related APIs
 * @addtogroup  GPIO_API
 * @{
 */

/* Local Header files */
#include "io.h"
#include "config.h"

/**
 * @brief Enum for different GPIO Modules
 *
 */
typedef enum GpioModule
{
    GPIO_1, /**< GPIO Module 1*/
    GPIO_2, /**< GPIO Module 2 */
    GPIO_3, /**< GPIO Module 3 */
    GPIO_4, /**< GPIO Module 4 */
    GPIO_MAX,
} GpioModule_t;

/**
 * @brief Error code for Different GPIO API
 *
 */
typedef enum GpioStatusCode
{
    GPIO_SUCCESS = 0,             /**< Return code for GPIO success */
    GPIO_PIN_NOT_SUPPORTED = -1,  /**< Error value if GPIO pin is not in range (0 to 31) */
    GPIO_TYPE_NOT_SET = -2,       /**< Error value if type of GPIO pin is not set */
    GPIO_CTRL_NOT_SUPPORTED = -3, /**< Error value if GPIO module is not in range (0 to 3) */
} GpioStatusCode_t;

/**
 * @brief Enum for GPIO pin type
 *
 */
typedef enum GpioType
{
    GPIO_INPUT,    /**< GPIO pin type input  */
    GPIO_OUTPUT,   /**< GPIO pin type output */
    GPIO_OPENDRAIN /**< GPIO pin type open-drain  */
} GpioType_t;

/**
 * @brief Enum for Interrupt no for GPIO Modules
 *
 */
typedef enum GpioIntr
{
    GPIO1_INTR = 2, /**< IRQ line for GPIO 1 */
    GPIO2_INTR = 3, /**< IRQ line for GPIO 2 */
    GPIO3_INTR = 4, /**< IRQ line for GPIO 3 */
    GPIO4_INTR = 5  /**< IRQ line for GPIO 4 */
} GpioIntr_t;


/**
 * @brief This function initializes a pin of a GPIO Module
 * @param[in]  eGpioModule GPIO module number
 * @param[in]  ucPin GPIO pin number
 * @param[in]  eGpioType GPIO type( Input, Output, OpenDrain )
 *
 * @return
 *	- On success, returns 0
 *	- On failure, returns error code
*/
GpioStatusCode_t exGpioInit( GpioModule_t eGpioModule,
                             uint8_t ucPin,
                             GpioType_t eGpioType );

/**
 * @brief This function updates the GPIO pin if the pin is set as of type output
 * @param[in] eGpioModule GPIO Module number
 * @param[in] ucPin GPIO pin number
 * @param[in] ulVal value to be updated
 *
 * @return
 *	- On success, returns 0
 *	- On failure, returns error code
*/
GpioStatusCode_t exGpioSetData( GpioModule_t eGpioModule,
                                uint8_t ucPin,
                                uint32_t ulVal );

/**
 * @brief This function updates the GPIO pin if the pin is set as of type input
 * @param[in] eGpioModule GPIO Module number
 * @param[in] ucPin GPIO pin number
 * @param[in] ulVal value to be updated
 *
 * @return
 *	- On success, returns 0
 *	- On failure, returns error code
*/
GpioStatusCode_t exGpioSetInputData( GpioModule_t eGpioModule,
                                     uint8_t ucPin,
                                     uint32_t ulVal );

/**
 * @brief This function reads the value of a GPIO pin and prints the value in console
 * @param[in] eGpioModule GPIO Module number
 * @param[in] ucPin GPIO pin number
 *
 * @return
 *	- On success, returns 0
 *	- On failure, returns error code
*/
GpioStatusCode_t exGpioGetData( GpioModule_t eGpioModule,
                                uint8_t ucPin );

/**
 * @brief This function reads the data register of a GPIO module
 * @param[in] eGpioModule GPIO Module number
 * @param[out] ulGpdata Pointer to an output buffer
 *
 * @return
 *	- On success, returns 0
 *	- On failure, returns error code
*/
GpioStatusCode_t exGpioGetDataRegister( GpioModule_t eGpioModule,
                                        uint32_t * ulGpdata );

/**
 * @brief This function sets or resets GPIO pin. This API is only used by RF.
 *	 This API is having minimum latency to update GPIO pin
 * @param[in] eGpioModule GPIO Module number
 * @param[in] ucPin GPIO pin number
 * @param[in] ulVal Value to be updated
 *
 * @return
 *	- On success return 0
 *	- On failure return error code
*/
GpioStatusCode_t exGpioSetRFData( GpioModule_t eGpioModule,
                                  uint8_t ucPin,
                                  uint32_t ulVal );

/**
 * @brief This function sets GPIO MUX mode - it could be
 *          either GPIO mode or non-GPIO mode
 *
 * @param[in] eGpioModule GPIO Module number
 * @param[in] ucPin GPIO pin number
 * @param[in] bIsFlag it is flag to enable or disable pin as GPIO. (1 to
 *		enable and 0 to disable).
 *
 * @return
 *	- On success, returns 0
 *	- On failure, returns error code
*/
GpioStatusCode_t exGpioPinControl( GpioModule_t eGpioModule,
                                   uint8_t ucPin,
                                   bool bIsFlag );

/** @} */

#endif /* ifndef _GPIO_H_ */
