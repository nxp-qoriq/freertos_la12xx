// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#ifndef __WATCHDOG_API_H__
#define __WATCHDOG_API_H__

/**
 * @file        watchdog_api.h
 * @brief       Watchdog related APIs.
 * @addtogroup  WATCHDOG_API
 * @{
 */


/**
 * Application can use this API to Start/Enable Watchdog Timer
 * 	Start Watchdog Timer with a given counter value.
 * 	For LA12xx, Watchdog clock input is 32.76kHz and
 * 	Watchdog timer resolution is 30.5 us.
 *
 *
 * @param[in]  wdog_load_val
 *	Watchdog load value
 *
 * @param[in]  L1C_callback_func
 *	Callback function
 *
 * @return  -
 *       	void
 *
 * NOTE: This API can be called only once.
 *
 */
void vWatchdogStart(uint32_t wdog_load_val, int (*L1C_callback_func)() );

/**
 * Application can use this API to Stop/Disable Watchdog.
 *
 * @return -
 *   void
 *
 * NOTE: Use vWdogStart() function to start WDOG again if vWdogStop() function was used.
 *
 */
void vWatchdogStop( void );

/**
 * Application can use this API to Reload Watchdog counter.
 *
 *  @param[in]  new_wdog_load_val
 *	Watchdog load value
 *
 *  @param[in]  new_L1C_callback_func
 *	Callback function; this callback function can be same which was passed to vWdogStart function.
 *
 * @return -
 *               void
 *
 */
void vWatchdogReload( uint32_t new_wdog_load_val, int ( *new_L1C_callback_func )( void ) );

/**
 *
 * API for demo of Watchdog functionality
 */
void vWatchdogTest( void );


/**
 *
 * API to check Watchdog is already enabled?
 */
uint8_t wdog_status(void);
/** @} */
#endif /* __WATCHDOG_API_H__ */
