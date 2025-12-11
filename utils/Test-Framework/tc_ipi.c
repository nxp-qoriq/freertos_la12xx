// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_ipi.h"
#include "soc.h"

#if GEUL_DEMO_IPI_QUEUE_TEST
    #include "FreeRTOS.h"
    #include "ipiQueue.h"

    #define IPI_DEMO_EVENT_NUM    5
/*#define IPI_FUNCTION_CALLBACK 1 */
    static enum IPIEventID g_eventTestID[ IPI_DEMO_EVENT_NUM ] =
    {
        IPI_EVT_ID1,
        IPI_EVT_ID3,
        IPI_EVT_ID5,
        IPI_EVT_ID7,
        IPI_EVT_ID9
    };

    #define IPI_TEST_COUNT_NUM    10
    enum IPITestMode
    {
        IPI_MULTI_SRC_SINGLE_DST,
        IPI_SINGLE_SRC_MULTI_DST,
        IPI_CHAIN_SINGLE_SRC_SINGLE_DST
    };

    static enum IPITestMode gIPITestMode = IPI_CHAIN_SINGLE_SRC_SINGLE_DST;

    static u32 gReceived = 0;
    #ifdef IPI_FUNCTION_CALLBACK
        void vIPIEventCallback( enum IPIEventID eventID,
                                void * userData,
                                void * cookie )
        {
            u32 current_core = ulMpicCurrentCore();

            /*To Avoid compilation warnings */
            ( void ) cookie;

            if( ( ( gIPITestMode == IPI_MULTI_SRC_SINGLE_DST ) &&
                  ( current_core == ( get_soc_numcores() - 1 ) ) ) ||
                ( ( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST ) &&
                  ( current_core != 0 ) ) ||
                ( gIPITestMode == IPI_CHAIN_SINGLE_SRC_SINGLE_DST ) )
            {
                if( pxRxQueue[ eventID ] != NULL )
                {
                    void * rxData;
                    BaseType_t ret;

                    ret = xQueueReceive( pxRxQueue[ eventID ], &rxData, 0 );

                    if( ret == pdTRUE )
                    {
                        gReceived++;
                        log_info( "core %d receive from core: %d with event:%d, ret:%d, received:%d\r\n",
                                  current_core, ( int ) rxData, ( int ) eventID, ret, gReceived );
                    }
                }
                else
                {
                    log_info( "core %d receive from core: %d with event:%d, received:%d\r\n",
                              current_core, ( int ) userData, ( int ) eventID, gReceived );
                    gReceived++;
                }
            }

            #ifdef IPI_GLOBAL_Q_DBG
                vIPIGlobalQStatusCheck( ( int ) current_core, -1 );
            #endif
        }
    #endif /* ifdef IPI_FUNCTION_CALLBACK */
    void vGeulIPIDemoEntry()
    {
        u32 current_core = ulMpicCurrentCore();
        u8 i, j;
        BaseType_t ret;
        u32 dstCore;
        u32 sent = 0;

        gReceived = 0;
        #ifndef IPI_FUNCTION_CALLBACK
            void * rxData;
        #endif

        if( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST )
        {
            dstCore = 1;
        }

        for( i = 0; i < IPI_DEMO_EVENT_NUM; i++ )
        {
            #ifdef IPI_FUNCTION_CALLBACK
                vIPIEventRegister( IPIGlobalEventID[ i ], &pxRxQueue[ i ], vIPIEventCallback, NULL );
            #else
                vIPIEventRegister( IPIGlobalEventID[ i ], &pxRxQueue[ i ], NULL, NULL );
            #endif

            g_eventTestID[ i ] = IPIGlobalEventID[ i ];
        }

        syncUnSync();

        while( 1 )
        {
            if( gIPITestMode == IPI_MULTI_SRC_SINGLE_DST )
            {
                if( current_core < ( get_soc_numcores() - 1 ) )
                {
                    dstCore = ( get_soc_numcores() - 1 );

                    for( j = 0; j < IPI_DEMO_EVENT_NUM; j++ )
                    {
                        if( sent < IPI_TEST_COUNT_NUM )
                        {
                            ret = vIPISendData( dstCore, g_eventTestID[ j ], ( void * ) current_core );

                            if( ret == pdTRUE )
                            {
                                sent++;
                                log_dbg( "send from core %d to %d, with event %d successfully, sent:%d\r\n", current_core,
                                         dstCore, g_eventTestID[ j ], sent );
                            }
                            else
                            {
                                log_info( "send from core %d to %d, with event %d failed, sent:%d\r\n", current_core,
                                          dstCore, g_eventTestID[ j ], sent );
                            }
                        }
                        else
                        {
                            goto TEST_COMPLETE;
                        }
                    }
                }

                vTaskDelay( 1 );
            }
            else if( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST )
            {
                if( current_core == 0 )
                {
                    for( j = 0; j < IPI_DEMO_EVENT_NUM; j++ )
                    {
                        if( sent < ( IPI_TEST_COUNT_NUM * get_soc_numcores() - 1 ) )
                        {
                            ret = vIPISendData( dstCore, g_eventTestID[ j ], ( void * ) current_core );

                            if( ret == pdTRUE )
                            {
                                sent++;
                                log_dbg( "send from core %d to %d, with event %d successfully, sent:%d\r\n", current_core,
                                         dstCore, g_eventTestID[ j ], sent );
                            }
                            else
                            {
                                log_info( "send from core %d to %d, with event %d failed, sent:%d\r\n", current_core,
                                          dstCore, g_eventTestID[ j ], sent );
                            }
                        }
                        else
                        {
                            goto TEST_COMPLETE;
                        }
                    }

                    if( dstCore == ( get_soc_numcores() - 1 ) )
                    {
                        dstCore = 1;
                    }
                    else
                    {
                        dstCore++;
                    }

                    vTaskDelay( 1 );
                }
            }
            else if( gIPITestMode == IPI_CHAIN_SINGLE_SRC_SINGLE_DST )
            {
                dstCore = current_core == ( get_soc_numcores() - 1 ) ?
                          0 : ( current_core + 1 );

                for( j = 0; j < IPI_DEMO_EVENT_NUM; j++ )
                {
                    if( sent < IPI_TEST_COUNT_NUM )
                    {
                        ret = vIPISendData( dstCore, g_eventTestID[ j ], ( void * ) current_core );

                        if( ret == pdTRUE )
                        {
                            sent++;
                            log_dbg( "send from core %d to %d, with event %d successfully, sent:%d\r\n", current_core,
                                     dstCore, g_eventTestID[ j ], sent );
                        }
                        else
                        {
                            log_info( "send from core %d to %d, with event %d failed, sent:%d\r\n", current_core,
                                      dstCore, g_eventTestID[ j ], sent );
                        }
                    }
                }

                vTaskDelay( 1 );
            }

            #if 0
                if( ( ( gIPITestMode == IPI_MULTI_SRC_SINGLE_DST ) &&
                      ( current_core == ( get_soc_numcores() - 1 ) ) ) ||
                    ( ( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST ) &&
                      ( current_core != 0 ) ) ||
                    ( gIPITestMode == IPI_CHAIN_SINGLE_SRC_SINGLE_DST ) )
                {
                    if( ( gIPITestMode == IPI_MULTI_SRC_SINGLE_DST ) &&
                        ( gReceived >= ( IPI_TEST_COUNT_NUM * get_soc_numcores() - 1 ) ) )
                    {
                        goto TEST_COMPLETE;
                    }

                    if( ( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST ) &&
                        ( gReceived >= IPI_TEST_COUNT_NUM ) )
                    {
                        goto TEST_COMPLETE;
                    }

                    if( ( gIPITestMode == IPI_CHAIN_SINGLE_SRC_SINGLE_DST ) &&
                        ( gReceived >= IPI_TEST_COUNT_NUM ) &&
                        ( sent >= IPI_TEST_COUNT_NUM ) )
                    {
                        goto TEST_COMPLETE;
                    }
                }
            #endif /* if 0 */
            #ifndef IPI_FUNCTION_CALLBACK
                if( ( ( gIPITestMode == IPI_MULTI_SRC_SINGLE_DST ) &&
                      ( current_core == ( get_soc_numcores() - 1 ) ) ) ||
                    ( ( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST ) &&
                      ( current_core != 0 ) ) ||
                    ( gIPITestMode == IPI_CHAIN_SINGLE_SRC_SINGLE_DST ) )
                {
                    for( i = 0; i < IPI_DEMO_EVENT_NUM; i++ )
                    {
                        ret = xQueueReceive( pxRxQueue[ i ], &rxData, 0 );

                        if( ret == pdTRUE )
                        {
                            gReceived++;
                            log_info( "core %d receive from core: %d with event:%d, ret:%d, gReceived:%d\r\n",
                                      current_core, ( int ) rxData, i, ret, gReceived );
                        }
                    }

                    #ifdef IPI_GLOBAL_Q_DBG
                        vIPIGlobalQStatusCheck( ( int ) current_core, -1 );
                    #endif
                }
            #endif /* ifndef IPI_FUNCTION_CALLBACK */

            if( ( gIPITestMode == IPI_MULTI_SRC_SINGLE_DST ) &&
                ( gReceived >= ( IPI_TEST_COUNT_NUM * get_soc_numcores() - 1 ) ) )
            {
                goto TEST_COMPLETE;
            }

            if( ( gIPITestMode == IPI_SINGLE_SRC_MULTI_DST ) &&
                ( gReceived >= IPI_TEST_COUNT_NUM ) )
            {
                goto TEST_COMPLETE;
            }

            if( ( gIPITestMode == IPI_CHAIN_SINGLE_SRC_SINGLE_DST ) &&
                ( gReceived >= IPI_TEST_COUNT_NUM ) &&
                ( sent >= IPI_TEST_COUNT_NUM ) )
            {
                goto TEST_COMPLETE;
            }
        }

TEST_COMPLETE:
        SET_TEST_STATUS( current_core, GEUL_DEMO_IPI_QUEUE_TEST_STATUS );

        for( i = 0; i < IPI_DEMO_EVENT_NUM; i++ )
        {
            vIPIEventUnRegister( i );
        }

        return;
    }

    #if GEUL_IPI_STATS_TEST
        void vGeulIPIStats( void )
        {
            IPIStatsData_t statsData;
            u8 i;

            vIPIGetStats( ( void * ) &statsData );

            /* Show IPI Sent and Recv Stats */
            log_info( "\n\rIPI Stats Data for core: %d:\n\r", statsData.current_core );
            log_info( "------------------------------------------------------------------------------\n\r" );
            log_info( "Core\t\tSent\t\tReceived\tGEnqueue\tGDeque\n\r" );
            log_info( "------------------------------------------------------------------------------\n\r" );

            for( i = 0; i < get_soc_numcores(); i++ )
            {
                if( i == statsData.current_core )
                {
                    log_info( "SELF\t\t%d\t\t%d\t\t%d\t\t%d\n\r",
                              statsData.IPISentStats[ i ], statsData.IPIRecvStats[ i ],
                              statsData.IPIGlobalEnq[ i ], statsData.IPIGlobalDeq[ i ] );
                }
                else
                {
                    log_info( "%d\t\t%d\t\t%d\t\t%d\t\t%d\n\r",
                              i, statsData.IPISentStats[ i ], statsData.IPIRecvStats[ i ],
                              statsData.IPIGlobalEnq[ i ], statsData.IPIGlobalDeq[ i ] );
                }
            }
        }
    #endif /* GEUL_IPI_STATS_TEST */

#endif /* GEUL_DEMO_IPI_QUEUE_TEST */
