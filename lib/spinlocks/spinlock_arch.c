// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#include "spinlock_api.h"
#include "spinlock.h"
#include "ppc.h"

#define LOCK_TOKEN    1

/*
 * SpinLock arch level implementation.
 */

static inline uint32_t prvSpinArchtryToAcquire( struct SpinLock * pxLock )
{
    uint32_t ulTmp, ulToken;

    ulToken = LOCK_TOKEN;

    __asm__ volatile (
        "1:  lbarx     %0,  0, %2\n\
		e_cmpi    0,  %0, 0\n\
		e_bne     2f\n\
		stbcx.    %1,  0, %2\n\
		e_bne     1b\n"
        "se_isync\n"
        "2:"
        : "=&r" ( ulTmp )
        : "r" ( ulToken ), "r" ( pxLock )
        : "cr0", "memory"
        );

    return ulTmp;
}

void vSpinArchAcquireLock( struct SpinLock * pxLock )
{
    volatile uint32_t uiLockStatus;

    do
    {
        uiLockStatus = ( !( prvSpinArchtryToAcquire( pxLock ) ) == 0 ) ? 1 : 0;
    } while( uiLockStatus );
}

uint32_t uiSpinArchtryToAcquire( struct SpinLock * pxLock )
{
    return( prvSpinArchtryToAcquire( pxLock ) );
}

static inline unsigned long prvSaveIRQFlags( void )
{
    return mfmsr();
}

unsigned long ulSaveIRQFlags( unsigned long ulFlags )
{
    ulFlags = prvSaveIRQFlags();
    mtmsr( ulFlags & ~MSR_EE );

    return ulFlags;
}

void vRestoreIRQFlags( unsigned long ulFlags )
{
    mtmsr( ulFlags );
}

void vSpinArchReleaseLock( struct SpinLock * pxLock )
{
    __asm__ __volatile__ ( "# vSpin_Arch_ReleaseLock\n\t"
                           "se_isync" : : : "memory" );
    pxLock->ucLock = 0;
}
