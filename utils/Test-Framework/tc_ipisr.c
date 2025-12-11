// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#include "tc_ipi.h"
#include "soc.h"

#if GEUL_DEMO_IPI_ISR_TEST
    #include "FreeRTOS.h"
    #include "ipiQueue.h"
	#include "mpic_regs.h"
	#include "Time.h"

	#define SOC_MSIIR_SRS5		0xA0000000
	#define SOC_MSIIR_SRS5_IBS	0x1B000000

	#define IPI_DEMO_EVENT_NUM    5
/* #define IPI_FUNCTION_CALLBACK 1 */
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
        IPI_CHAIN_SINGLE_SRC_SINGLE_DST,
        IPI_ISR_SINGLE_SRC_MULTI_DST
    };


	static bool_t ipi_msi_test_interrupt_handler( uint32_t ulIrq_No, void *vDev_Data )
	{
		u32 uiCurrentCore = ulMpicCurrentCore();
		u32 sent = 0, dstCore = 1;
		int ret = -1;

		(void)vDev_Data;
		(void)ulIrq_No;

		while( sent < ( IPI_TEST_COUNT_NUM * get_soc_numcores() - 1 ) )
		{
			for( u32 j = 0; j < IPI_DEMO_EVENT_NUM; j++ )
			{
				if( sent < ( IPI_TEST_COUNT_NUM * get_soc_numcores() - 1 ) )
				{
					ret = vIPISendDatafromISR( dstCore, g_eventTestID[ j ], ( void * ) uiCurrentCore );
	
					if( ret == pdTRUE )
					{
						sent++;
						log_dbg( "send from isr of core %d to %d, with event %d successfully, sent:%d\r\n", uiCurrentCore,
								 dstCore, g_eventTestID[ j ], sent );
					}
					else
					{
						log_info( "send from isr of core %d to %d, with event %d failed, sent:%d\r\n", uiCurrentCore,
								  dstCore, g_eventTestID[ j ], sent );
					}
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

	//		vBusyWait(1);

			/* receive msg from interrupt, read will also clear interrupt */
			in_be32(MPIC_REGS_MSIR5);
		}

		return true;
	}

    static u32 gReceived = 0;
    #ifdef IPI_FUNCTION_CALLBACK
        void vIPIEventCallback( enum IPIEventID eventID,
                                void * userData,
                                void * cookie )
        {
            u32 current_core = ulMpicCurrentCore();

            /*To Avoid compilation warnings */
            ( void ) cookie;

		if	( current_core != 0 )
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
    void vGeulIPIFromISRDemoEntry()
    {
        u32 current_core = ulMpicCurrentCore();
        u8 i;
        BaseType_t ret;

        gReceived = 0;
        #ifndef IPI_FUNCTION_CALLBACK
            void * rxData;
        #endif

        for( i = 0; i < IPI_DEMO_EVENT_NUM; i++ )
        {
            #ifdef IPI_FUNCTION_CALLBACK
                vIPIEventRegister( IPIGlobalEventID[ i ], NULL, vIPIEventCallback, NULL );
            #else
                vIPIEventRegister( IPIGlobalEventID[ i ], &pxRxQueue[ i ], NULL, NULL );
            #endif

            g_eventTestID[ i ] = IPIGlobalEventID[ i ];
        }

        syncUnSync();

        while( 1 )
        {
				if (current_core == 0)
				{
					log_info("Register and enable MSI\n\r");
					lRegisterIrq(145 + INTERNAL_IRQ_OFFSET, ipi_msi_test_interrupt_handler, NULL);

					bMpicEnable(DEVICE_SHARE_MESSAGE, 5);
					log_info("Trigger MSI\n\r");
					out_be32(MPIC_REGS_MSIIR, SOC_MSIIR_SRS5 | SOC_MSIIR_SRS5_IBS);
					goto TEST_COMPLETE;
				}

            #ifndef IPI_FUNCTION_CALLBACK
                   if ( current_core != 0 )               	{
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
                }
            #endif /* ifndef IPI_FUNCTION_CALLBACK */

            if( gReceived >= IPI_TEST_COUNT_NUM )
            {
                goto TEST_COMPLETE;
            }
        }

TEST_COMPLETE:
        SET_TEST_STATUS( current_core, GEUL_DEMO_IPI_ISR_TEST_STATUS );
	vUnregisterIrq(145 + INTERNAL_IRQ_OFFSET);
	    log_info("LA12XX IPI FROM ISR Test Completed\r\n");

        for( i = 0; i < IPI_DEMO_EVENT_NUM; i++ )
        {
            vIPIEventUnRegister( i );
        }
        return;
    }

#endif /* GEUL_DEMO_IPI_ISR_TEST */
