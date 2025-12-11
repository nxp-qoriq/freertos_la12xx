// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef __SPINLOCK_API__H
#define __SPINLOCK_API__H

#include "FreeRTOS.h"
#include "task.h"
#include "config.h"
#include "spinlock.h"

/**
 * @file spinlock_api.h
 * @brief Spinlock API support.
 *
 * This contains the function definitions for the spinlock
 * functionality to acquire and release of the spinlocks.
 * Spinlocks allows only a single CPU anywhere.
 * Note: In LA12xx, spinlocks are stored in SRAM.
 *
 * @addtogroup	SPINLOCK_API
 * @{
 */

/**
 * Enum for different Spinlock types
 * In LA12xx, We have total of 1024 spinlock support.
 *
 */
typedef enum
{
    SPINLOCK_MIN = -1,		/**< Spinlock for spinlock Test framework*/
    SPINLOCK_TEST = 0,          /**< Spinlock for spinlock Test framework*/
    SPINLOCK_VSPA = 1,          /**< Spinlock for VSPA */
    SPINLOCK_TBGEN_COUNTER = 2, /**< Spinlock for TBGEN Counter */
    SPINLOCK_MPIC = 3, /**< Spinlock for MPIC */
#ifdef TESTFRAMEWORK_ENABLE
    SPINLOCK_MSI_TEST	   = 4, 	/**< Spinlock for MSI Test */
#endif /* TESTFRAMEWORK_ENABLE */
    SPINLOCK_LAST,
    SPINLOCK_MAX = LOCK_COUNT_STATIC	/**< Max Static Spinlock value */
} Spinlock_t;

/**
 *  This function allocates the memory to spinlock from shared SRAM region and
 *  returns spinlock to the caller. Each time calling xSpinLockAlloc, returns
 *  new spinlock.
 *  This function returns the lock, which is used to pass as a parameter to all
 *  other spinlock functions.
 *  Note: A total of 1024 spinlock support is provided in LA12xx, once all spinlocks
 *  are used, it will return NULL if no free locks available.
 *
 * @param[in]  xTask
 *  Task which will own the lock
 * @param[in]  pvParentDataStruct
 *  pointer to data(it can be a structure) which is protected by this lock
 * @return
 *  - On success, returns the lock, which is used to pass as a parameter to all
 *  	other spinlock functions.
 *  - On faiulre, NULL  means that all 1024 locks are in use.
 */
struct SpinLock* pxSpinLockAlloc( TaskHandle_t xTask, void *pvParentDataStruct);

/**
 * Allocates the memory to spinlock from shared SRAM region and returns spinlock to the caller.
 * This function returns the lock(static spinlock, which is defined in enum Spinlock_t),
 * which is used to pass as a parameter to all other spinlock functions.
 * @param[in]  xTask
 * Task which will own the lock
 * @param[in]  pvParentDataStruct
 *  pointer to data(it can be a structure) which is protected by this lock
 * @param[in]  xSpinlockIndx
 *  Enum value defined in Spinlock_t
 * @return
 *  - On success, returns the lock, which is used to pass as a parameter to all
 *  	other spinlock functions.
 *  - On failure, NULL means that all 1024 locks are in use.
 */
struct SpinLock * pxSpinLockGet( TaskHandle_t xTask,
                                 void * pvParentDataStruct,
                                 Spinlock_t xSpinlockIndx );

/**
 * This function helps to protect the critical section between normal tasks.
 * If the lock is free, acquires it; otherwise spins until lock become free.
 * You must pass the lock as an argument to this function;
 * You can get the lock from calling pxSpinLockAlloc function or pxSpinLockGet function.
 * @param[in]  pxLock
 *  spinlock received in pxSpinLockGet or pxSpinLockAlloc function
 */
void vSpinLockAcquire( struct SpinLock* pxLock );

/**
 * Releases the given lock.
 * This function should call along with vSpinLockAcquire/uiSpinLockTryAcquire
 * functions.
 * This function releases the lock which was previously acquired by
 * vSpinLockAcquire/uiSpinLockTryAcquire.
 * @param[in]  pxLock
 *  spinlock
 */
void vSpinLockRelease( struct SpinLock* pxLock );

/**
 * Checks whether the lock is free or not.
 * @param[in]  pxLock
 *  spinlock
 * @return:
 * 	Returns nonzero if the given lock is acquired,
 * 	otherwise it returns zero
 */
uint32_t uiSpinLockCheckStatus( struct SpinLock* pxLock );

/**
 * Tries to acquire the given lock; if the lock is free, acquires it, otherwise,
 * exits without acquiring the lock. Therefore, this function does not spin for the lock
 * as the xSpinLockAcquire function does.
 * @param[in]  pxLock
 *  spinlock received in pxSpinLockGet or pxSpinLockAlloc function
 * @return
 * 	If success, returns zero, else returns nonzero value.
 */
uint32_t uiSpinLockTryAcquire( struct SpinLock* pxLock );

/**
 * If the critical section is shared between process and interrupt handler, you
 * should use ulSpinLockAcquireIRQlock() function instead of vSpinLockAcquire()function.
 * This function saves the current state of local interrupts, disables the local
 * interrupts and then acquires the lock.
 * @param[in]  pxLock
 *  spinlock
 * @param[out]  ulFlags
 *  Interrupt state should be saved in to this flag
 * @return
 *  Returns interrupt flags that are used to restore the interrupt state by
 *  calling vSpinLockReleaseIRQlock() function
 */
unsigned long ulSpinLockAcquireIRQlock( struct SpinLock * pxLock,
                                        unsigned long ulFlags );

/**
 * Releases the lock and restores local interrupts to given previous state(prior
 * to ulSpinLockAcquireIRQlock())
 * The interrupt state Prior to calling ulSpinLockAcquireIRQlock() function is saved
 * into flags argument by ulSpinLockAcquireIRQlock() function and the same flags are
 * passed to vSpinLockReleaseIRQlock() function and therefore vSpinLockReleaseIRQlock() function
 * restores the previous interrupt state and also releases the lock which was
 * acquired by ulSpinLockAcquireIRQlock() function.
 * @param[in]  pxLock
 *  spinlock
 * @param[in]  ulFlags
 *  Interrupt state should be saved in to this flag
 */
void vSpinLockReleaseIRQlock( struct SpinLock * pxLock,
                              unsigned long ulFlags );

/** @} */
#endif /* ifndef __SPINLOCK_API__H */
