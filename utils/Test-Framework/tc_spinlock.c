// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#include "tc_spinlock.h"

#if GEUL_DEMO_SPINLOCK_TEST
    #include "FreeRTOS.h"
    #include "task.h"
    #include "tbgen_new.h"
    #include "spinlock.h"
    #include "ppc.h"
    #include "spinlock_api.h"
    #define MAX_LOOP        10
    #define MAX_TEST_IDX    1000

    static TaskHandle_t xHandle;
    static TaskHandle_t xHandlesamelock;

    extern struct SpinlockMetadata pxSpinLockMD;

    struct SpinlockTest
    {
        volatile uint32_t ulData; /*More data will be added to struct as per requirment*/
    };

    struct SpinlockTestCore
    {
        volatile uint32_t ulData;
    };
    struct SpinlockTestCore xSpintestCore;

    struct SpinlockTest SpinTestMem __attribute__( ( section( ".smem" ) ) );

    void vTaskCodeCore( void )
    {
        unsigned long ulFlags = NULL, ulCurrFlags = NULL, ulIrqState = NULL;
        uint32_t ulTrylock = 0;
        uint32_t ulIndex = 0;
        uint32_t ulCoreId = 0;
        uint64_t ulStart = 0;
        uint64_t ulTotalTime = 0;
		u8 ucTbgenNo = TBGEN_2;
	/* To remove unused variable warnings */
	(void)ulTotalTime;

        ulCoreId = ulMpicCurrentCore();
        struct SpinlockTest * pxSpinTest = &SpinTestMem; /*Its the location where shared data will be stored*/

        /* Static Spinlock used, which is SPINLOCK_TEST,  0 */
        struct SpinLock * pxSpinLockVar = pxSpinLockGet( xHandlesamelock, pxSpinTest, SPINLOCK_TEST );

        ulTrylock = uiSpinLockCheckStatus( pxSpinLockVar );

        if( ulTrylock )
        {
            log_dbg( "Core %u : The lock status : contended \r\n", ulMpicCurrentCore() );
        }
        else
        {
            log_dbg( "Core %u : The lock status : free \r\n", ulMpicCurrentCore() );
        }

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            vSpinLockAcquire( pxSpinLockVar );
            pxSpinTest->ulData = ulCoreId;
            vSpinLockRelease( pxSpinLockVar );

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
            log_dbg( "%s : total tick spent = %ul \r\n", __func__, ulTotalTime );
        }

        log_dbg( "vTaskCodeCore: Spinlock Acquire/Release completed \r\n" );

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
            ulCurrFlags = mfmsr();
            /* Forcefully making interrupts to Enable and then after acquire/release irq based , checking the interrupt state */
            mtmsr( mfmsr() | MSR_EE );
            ulIrqState = mfmsr();
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            ulFlags = ulSpinLockAcquireIRQlock( pxSpinLockVar, ulFlags );
            pxSpinTest->ulData = ulCoreId;
            vSpinLockReleaseIRQlock( pxSpinLockVar, ulFlags );

            if( ulIrqState != mfmsr() )
            {
                log_dbg( "IRQ Flags are not restored to Enable state \r\n" );
            }

            mtmsr( ulCurrFlags );

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
            log_dbg( "%s : total tick spent = %ul \r\n", __func__, ulTotalTime );
        }

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
            ulCurrFlags = mfmsr();
            /* Forcefully making interrupts Disable and then after acquire/release irq based s, checking the interrupt state */
            mtmsr( mfmsr() & ~MSR_EE );
            ulIrqState = mfmsr();
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            ulFlags = ulSpinLockAcquireIRQlock( pxSpinLockVar, ulFlags );
            pxSpinTest->ulData = ulCoreId;
            vSpinLockReleaseIRQlock( pxSpinLockVar, ulFlags );

            if( ulIrqState != mfmsr() )
            {
                log_dbg( "IRQ Flags are not restored to Disable state \r\n" );
            }

            mtmsr( ulCurrFlags );

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
            log_dbg( "Total tick spent = %ul \r\n", ulTotalTime );
        }

        log_dbg( "vTaskCodeCore: IRQ based Spinlock Acquire/Release completed \r\n" );

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            ulTrylock = uiSpinLockTryAcquire( pxSpinLockVar );

            if( ulTrylock )
            {
                log_dbg( "The lock is contended \r\n:" );
            }
            else
            {
                pxSpinTest->ulData = ulCoreId;
                vSpinLockRelease( pxSpinLockVar );
            }

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
        }

        log_dbg( "vTaskCodeCore: Spinlock Trylock completed \r\n" );

        ulTrylock = uiSpinLockCheckStatus( pxSpinLockVar );

        if( ulTrylock )
        {
            log_dbg( "vTaskCodeCore : Core %u : The lock status : contended \r\n", ulMpicCurrentCore() );
        }
        else
        {
            log_dbg( "vTaskCodeCore : Core %u : The lock status : free \r\n", ulMpicCurrentCore() );
        }

        log_dbg( "\r\nvTaskCodeCore: Spinlock testing ends....!! \r\n" );
    }

    void vTaskCode( void )
    {
        uint32_t ulCurrentCore = ulMpicCurrentCore();
        unsigned long ulFlags = NULL;
        uint32_t ulIndex = 0;
        uint32_t ulTrylock = 0;
        uint32_t ulVdata = 0;
        uint64_t ulStart = 0;
        uint64_t ulTotalTime = 0;
		u8 ucTbgenNo = TBGEN_2;
        struct SpinLock * pxSpinLockCore = NULL;

	/* To remove unused variable warnings */
	(void)ulTotalTime;
        pxSpinLockCore = pxSpinLockAlloc( xHandle, &xSpintestCore );
        log_dbg( "Executing %s...\r\n", __func__ );

        /* 4 is added to the ulMpicCurrentCore(),so that the data value from the vTaskCodeCore task in the same core is different. The critical section data value in  vTaskCodeCore is set using ulMpicCurrentCore() */
        ulVdata = ulMpicCurrentCore() + 4;

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            vSpinLockAcquire( pxSpinLockCore );
            xSpintestCore.ulData = ulVdata;
            vSpinLockRelease( pxSpinLockCore );

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
            log_dbg( "%s : total tick spent = %ul \r\n", __func__, ulTotalTime );

            if( ulMpicCurrentCore() != pxSpinLockCore->ucCoreId )
            {
                log_info( "%s : Core ID Mismatch xPortGetCoreID = %u and SpinLock->core_id = %d \r\n", __func__, ulMpicCurrentCore(), pxSpinLockCore->ucCoreId );
            }

            if( ( pxSpinLockCore->pvParentDataStruct ) != ( &xSpintestCore ) )
            {
                log_info( "%s : Parent_data_struct Mismatch \r\n", __func__ );
            }
        }

        log_dbg( "vTaskCode: Spinlock acquire/release completed \r\n" );

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            ulFlags = ulSpinLockAcquireIRQlock( pxSpinLockCore, ulFlags );
            xSpintestCore.ulData = ulVdata;
            vSpinLockReleaseIRQlock( pxSpinLockCore, ulFlags );

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
            log_dbg( "%s : Total tick spent = %ul \r\n", __func__, ulTotalTime );

            if( ulMpicCurrentCore() != pxSpinLockCore->ucCoreId )
            {
                log_dbg( "%s : Core ID Mismatch xPortGetCoreID = %u and SpinLock->core_id = %d \r\n", __func__, ulMpicCurrentCore(), pxSpinLockCore->ucCoreId );
            }

            if( ( pxSpinLockCore->pvParentDataStruct ) != ( &xSpintestCore ) )
            {
                log_dbg( "%s : Parent_data_struct Mismatch \r\n", __func__ );
            }
        }

        log_dbg( "vTaskCode: IRQ based Spinlock acquire/release completed \r\n" );

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
			ulStart = ullTbgenGetMasterCounter( ucTbgenNo );
            ulTrylock = uiSpinLockTryAcquire( pxSpinLockCore );

            if( ulTrylock )
            {
                log_dbg( "%s : The lock is contended \r\n", __func__ );
            }
            else
            {
                xSpintestCore.ulData = ulVdata;
                vSpinLockRelease( pxSpinLockCore );
                /*		log_dbg("%s : Core %u : data = %d \r\n",__func__,ulMpicCurrentCore(),spin_test_t->data); */
            }

			ulTotalTime = ullTbgenGetMasterCounter( ucTbgenNo ) - ulStart;
            log_dbg( "%s : Total tick spent = %ul \r\n", __func__, ulTotalTime );

            if( ulMpicCurrentCore() != pxSpinLockCore->ucCoreId )
            {
                log_dbg( " %s : Core ID Mismatch xPortGetCoreID = %u and SpinLock->core_id = %d \r\n", __func__, ulMpicCurrentCore(), pxSpinLockCore->ucCoreId );
            }

            if( ( pxSpinLockCore->pvParentDataStruct ) != ( &xSpintestCore ) )
            {
                log_dbg( "%s : Parent_data_struct Mismatch \r\n", __func__ );
            }
        }

        log_dbg( "vTaskCode: Spinlock Trylock completed \r\n" );

        for( ulIndex = 0; ulIndex < MAX_LOOP; ulIndex++ )
        {
            ulTrylock = uiSpinLockCheckStatus( pxSpinLockCore );

            if( ulTrylock )
            {
                log_dbg( "%s : Core %u : The lock status : contended \r\n", __func__, ulMpicCurrentCore() );
            }
            else
            {
                log_dbg( "%s : Core %u : The lock status : free \r\n", __func__, ulMpicCurrentCore() );
            }
        }

        if( ulFreeIndx >= MAX_TEST_IDX )
        {
            for( ulIndex = SPINLOCK_MAX; ulIndex < LOCK_COUNT; ulIndex++ )
            {
                ClearBit( pxSpinLockMD.iLockFreeBitmap, ulIndex );
            }
        }

        log_dbg( "vTaskCode: Spinlock test Completed...!! \r\n" );
        SET_TEST_STATUS( ulCurrentCore, GEUL_DEMO_SPINLOCK_TEST_STATUS );
    }

    void vGeulDemoSpinlockTest( void )
    {
        log_info( "Starting Spinlock Testing.. \r\n" );
        /* Run Same  on all cores */
        vTaskCodeCore();

        /* Run separate  on each core */
        vTaskCode();
        log_info( "Spinlock Test Done \r\n" );
    }

#endif /* GEUL_DEMO_SPINLOCK_TEST*/
