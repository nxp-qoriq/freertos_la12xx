// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#include "spinlock_api.h"
#include "spinlock.h"
#include "mpic.h"

/* Stats Control */
#define GUL_SPINLOCK_STATS_ENABLE    0

struct SpinlockMetadata pxSpinLockMD __attribute__( ( section( ".smem" ) ) );
volatile uint32_t ulFreeIndx;

volatile struct gul_hif * pHif;
volatile struct gul_stats * pGulStats;

static uint32_t prvNextFreeLockPos( int uiLockArray[] )
{
    uint32_t ulIndex;

    for( ulIndex = SPINLOCK_MAX; ulIndex < LOCK_COUNT; ulIndex++ )
    {
        if( TestBit( uiLockArray, ulIndex ) )
        {
            continue;
        }
        else
        {
            break;
        }
    }

    return ulIndex;
}

struct SpinLock * pxSpinLockGet( TaskHandle_t xTask,
                                 void * pvParentDataStruct,
                                 Spinlock_t xSpinlockIndx )
{
#if GUL_SPINLOCK_STATS_ENABLE
    pHif = pGulModPriv->pHif;
    pGulStats = &pHif->stats;
#endif

    if( ( xSpinlockIndx <= SPINLOCK_MIN ) || ( xSpinlockIndx >= SPINLOCK_MAX ) )
    {
        PRINTF( "%s : Invalid spinlock index: %d.\r\nMin: %d\r\nMax: %d\r\n", __func__, xSpinlockIndx, SPINLOCK_MIN, SPINLOCK_MAX );
        return NULL;
    }

    if( !( TestBit( pxSpinLockMD.iLockFreeBitmap, xSpinlockIndx ) ) )
    {
        SetBit( pxSpinLockMD.iLockFreeBitmap, xSpinlockIndx );
        pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ].ucCoreId = ( u8 ) ulMpicCurrentCore();
        pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ].pvParentDataStruct = pvParentDataStruct;
        pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ].xTask = xTask;

        pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ].ucLock = SPINLOCK_LOCK_UNLOCKED;
    }

    if( pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ].ucFlag != 1 )
    {
        pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ].ucFlag = 1;
#if GUL_SPINLOCK_STATS_ENABLE
        out_le32( &pGulStats->spinlock_stats.spinlock_count,
                  ( in_le32( &pGulStats->spinlock_stats.spinlock_count ) + 1 ) );
#endif
    }

    return &( pxSpinLockMD.xSpinLockInfo[ xSpinlockIndx ] );
}

struct SpinLock * pxSpinLockAlloc( TaskHandle_t xTask,
                                   void * pvParentDataStruct )
{
#if GUL_SPINLOCK_STATS_ENABLE
    pHif = pGulModPriv->pHif;
    pGulStats = &pHif->stats;
#endif

    ulFreeIndx = prvNextFreeLockPos( pxSpinLockMD.iLockFreeBitmap );

    if( ulFreeIndx == 0 )
    {
        SetBit( pxSpinLockMD.iLockFreeBitmap, ulFreeIndx );
    }
    else if( ulFreeIndx >= LOCK_COUNT )
    {
        PRINTF( "%s: Spinlock Max count %d is reached, No free locks are available\r\n", __func__, ulFreeIndx );
        return NULL;
    }
    else
    {
        SetBit( pxSpinLockMD.iLockFreeBitmap, ulFreeIndx );
    }

    pxSpinLockMD.xSpinLockInfo[ ulFreeIndx ].ucCoreId = ( u8 ) ulMpicCurrentCore();
    pxSpinLockMD.xSpinLockInfo[ ulFreeIndx ].pvParentDataStruct = pvParentDataStruct;
    pxSpinLockMD.xSpinLockInfo[ ulFreeIndx ].xTask = xTask;

    pxSpinLockMD.xSpinLockInfo[ ulFreeIndx ].ucLock = SPINLOCK_LOCK_UNLOCKED;

#if GUL_SPINLOCK_STATS_ENABLE
    out_le32( &pGulStats->spinlock_stats.spinlock_count,
              ( in_le32( &pGulStats->spinlock_stats.spinlock_count ) + 1 ) );
#endif

    return &( pxSpinLockMD.xSpinLockInfo[ ulFreeIndx ] );
}

void vSpinLockAcquire( struct SpinLock * pxLock )
{
    vTaskSuspendAll();
    vSpinArchAcquireLock( pxLock );
}

void vSpinLockRelease( struct SpinLock * pxLock )
{
    vSpinArchReleaseLock( pxLock );
    xTaskResumeAll();
}

uint32_t uiSpinLockTryAcquire( struct SpinLock * pxLock )
{
    vTaskSuspendAll();

    if( !uiSpinArchtryToAcquire( pxLock ) )
    {
        return 0;
    }

    xTaskResumeAll();
    return 1;
}

uint32_t uiSpinLockCheckStatus( struct SpinLock * pxLock )
{
    return( pxLock->ucLock != 0 );
}

unsigned long ulSpinLockAcquireIRQlock( struct SpinLock * pxLock,
                                        unsigned long ulFlags )
{
    ulFlags = ulSaveIRQFlags( ulFlags );
    vTaskSuspendAll();
    vSpinArchAcquireLock( pxLock );

    return ulFlags;
}

void vSpinLockReleaseIRQlock( struct SpinLock * pxLock,
                              unsigned long ulFlags )
{
    vSpinArchReleaseLock( pxLock );
    vRestoreIRQFlags( ulFlags );
    xTaskResumeAll();
}
