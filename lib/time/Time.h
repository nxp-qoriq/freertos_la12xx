// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef __TIME_H__
#define __TIME_H__

#include <platform_def.h>
#include <mpic.h>
#include "ppc.h"
#include "tbgen_new.h"


/**
 * @file        Time.h
 * @brief       TIME-related APIs.
 * @addtogroup  TIME_API
 * @{
 */

#define US_TO_TIMER_COUNT(us)	(us * (MPIC_TIMER_CLOCK / 1000000))
#define TIMER_COUNT_TO_US(count) (count / (MPIC_TIMER_CLOCK / 1000000))

//#define MPIC_TIME

/* Cycles UDELAY takes to execute */
#define UDELAY_CYCLES		5
/* Cycles UDELAY takes to execute with BPEN=1 */
#define UDELAY_BP_CYCLES	2
/* ERROR adjustment factor */
#define UDELAY_ERR_ADJ(n)	( n / 2 )	
/* ERROR adjustment factor with BPEN=1 */
#define UDELAY_BP_ERR_ADJ(n)	( n * 2 )

/* Macro to convert micro second time to no. of loops */
#define USEC2LOOPCOUNT(n, cycles, err_adj) ((n * (PLAT_FREQ / ( 1000000 * cycles ))) - err_adj )

/**
 * @brief Structure to store tick count.
 *
 */
struct Time {
	uint32_t ulCurrentTimerCounter;   /**< Current timer count */
	uint32_t ulCurrentTickCountMpic;  /**< Current tick count from MPIC */
	u64 ulCurrentTickCount;           /**< Current tick count from TBGEN */
	u64 ulGlobalTimerBCount;	/**< Current tick count from Global Timer B */
};


/**
 * @brief This function initializes timing library.
 */
void timeLibInit(uint8_t tbgen_id);

/**
 * @brief This function provides elapsed time within same core.
 *	  TBGEN timer is used to calculate elapsed time.
 *
 * @param[in] pxTime Pointer to the timer structure
 *
 * @return
 *	- On success, returns elapsed time in micro seconds
 *	- On failure, returns highest 32-bit unsigned integer
 */
uint32_t ulGetElapsedTime( struct Time *pxTime );

/**
 * @brief This function provides elapsed time within same core.
 *        MPIC timer is used to calculate elapsed time.
 *
 * @param[in] pxTime Pointer to the timer structure
 *
 * @return
 *	- On success, returns elapsed time in micro seconds
 *	- On failure, returns highest 32-bit unsigned integer
 */
uint32_t ulGetElapsedTimeMpic( struct Time *pxTime );

/**
 * @brief This function provides elapsed time. This API must be used to get elapsed
 *        time in between different core. TBGEN timer is used to calculate elapsed time.
 *
 * @param[in] coreId E200 core Id
 *
 * @return Elapsed time in micro seconds
 */
uint32_t ulGetElapsedTimeMulticore( uint32_t coreId);

/**
 * @brief This function waits until the input time is elapsed.
 *        TBGEN timer is used inside the function.
 *
 * @param[in] ulUS Time in micro second to elapsed
 *
 */
void vBusyWait( uint32_t ulUS );

/**
 *  @brief This function waits until the input time is elapsed.
 *         MPIC timer is used inside the function.
 *
 *  @param[in] ulUS Time in micro second to elapsed
 *
 */
void vBusyWaitMpic( uint32_t ulUS );

/**
 * @brief This function provides current timestamp.
 *        TBGEN timer is used to get the timestamp.
 *
 * @param[out] pxTime Pointer to the timer structure
 *
 */
void vGetCurrentTime( struct Time *pxTime );

/**
 * @brief This function provides current timestamp.
 *        MPIC timer is used to get the timestamp.
 *
 * @param[out] pxTime Pointer to the timer structure
 *
 */
void vGetCurrentTimeMpic( struct Time *pxTime );

/**
 * @brief This function updates current timestamp in SMEM which is
 *        used to calculate elapsed time in between different e200 core.
 */
void vGetCurrentTimeMulticore( void );

/**
 * @brief This function checks if elapsed time is greater
 *        than the input time or not. TBGEN timer is used inside the function.
 *
 * @param[in] pxTime Pointer to the timer structure
 * @param[in] ulTimeToCheckInUS Time in micro second
 *
 * @return
 *	- On success, returns 1
 *	- On failure, returns 0
 */
int32_t iHasTimeElapsed( struct Time *pxTime, uint32_t ulTimeToCheckInUS );

/**
 * @brief This function checks if elapsed time is greater
 *        than the input time or not. MPIC timer is used inside the function.
 *
 * @param[in] pxTime Pointer to the timer structure
 * @param[in] ulTimeToCheckInUS Time in micro second
 *
 * @return
 *	- On success, returns 1
 *	- On failure, returns 0
 */
int32_t iHasTimeElapsedMpic( struct Time *pxTime, uint32_t ulTimeToCheckInUS );

/**
 * @brief This function busy waits until the input time is elapsed
 * 		  Instruction based looping is used inside this function
 *
 * @param[in] usec Time to busy wait in micro seconds
 *
 * @return
 *	- This function returns void
 */
void vUdelay( uint32_t usec );

/**
 * @brief This function provides current count of MPIC Global B
 * Timers 0 and 1.
 *
 * @param[in] pxTime Pointer to the timer structure
 *
 * @return
 *	- This function returns void
 */
void vGetCurrentTimeMPICGB( struct Time *pxTime );

/**
 * @brief This function provides elapsed time of MPIC Global B
 * Cascaded Timers 0 and 1 and is used to calculate elapsed time.
 *
 * @param[in] pxTime Pointer to the timer structure
 *
 * @return
 *	- On success return elapsed time in micro seconds
 *	- On failure return highest 32-bit unsigned integer
 */
uint32_t ulGetElapsedTimeMPICGB( struct Time *pxTime );

/**
 * @brief This function checks if elapsed time is greater
 *        than the input time or not. MPIC global timer B is used
 *        inside the function.
 *
 * @param[in] pxTime Pointer to the timer structure
 * @param[in] ulTimeToCheckInUS Time in micro second
 *
 * @return
 *	- On success return 1
 *	- On failure return 0
 */

int32_t iHasTimeElapsedMPICGB( struct Time *pxTime, uint32_t ulTimeToCheckInUS );

/** @} */
#endif	/* __TIME_H__ */
