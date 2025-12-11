// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __SPINLOCK__H
#define __SPINLOCK__H

#include "config.h"
#include "types.h"

/** @file spinlock.h
 *  @brief Spinlock internal structure support.
 *
 *  This contains the structure and macro definitions for the spinlocks
 *
 */

#define LOCK_COUNT                128
#define LOCK_COUNT_STATIC		  LOCK_COUNT / 2

#define DEBUG_SPINLOCK            1

#define SPINLOCK_LOCK_UNLOCKED    0

#define SetBit( A, k )      ( A[ ( k / 32 ) ] |= ( 1 << ( k % 32 ) ) )
#define TestBit( A, k )     ( A[ ( k / 32 ) ] & ( 1 << ( k % 32 ) ) )
#define ClearBit( A, k )    ( A[ ( k / 32 ) ] &= ~( 1 << ( k % 32 ) ) )

/**
 *  Spinlocks should store globally in the shared SRAM region.
 */

/**
 * Structure for spinlock
 * @struct SpinLock:
 *  ucLock: spinlock
 *  ucCoreId: core number
 *  pvParentDataStruct: pointer to the data structure which acquires the lock, this data is passed by the application
 *  xTask: Task which is owning the spinlock
 */
struct SpinLock
{
    volatile uint8_t ucLock;

    #ifdef DEBUG_SPINLOCK
        uint8_t ucCoreId;
        uint8_t ucFlag;
        void * pvParentDataStruct;
        TaskHandle_t xTask;
    #endif
};

/**
 * Internal MetaData Structure for spinlock
 * @struct SpinlockMetadata:
 *  iLockFreeBitmap: bitmap of locks used to figure out next free lock
 *  xSpinLockInfo: Array of struct SpinLock's
 */
struct SpinlockMetadata
{
    int iLockFreeBitmap[ LOCK_COUNT / 32 ];
    struct SpinLock xSpinLockInfo[ LOCK_COUNT ];
};


/**
 * SpinLock Arch Function ProtoTypes*
 */
void vSpinArchAcquireLock( struct SpinLock * pxLock );

uint32_t uiSpinArchtryToAcquire( struct SpinLock * pxLock );

void vSpinArchReleaseLock( struct SpinLock * pxLock );

unsigned long ulSaveIRQFlags( unsigned long ulFlags );

void vRestoreIRQFlags( unsigned long ulFlags );

#endif /* ifndef __SPINLOCK__H */
