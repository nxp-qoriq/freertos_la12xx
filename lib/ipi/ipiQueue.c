// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include <common.h>
#include "soc.h"
#include "config.h"
#include "immap.h"
#include <mpic.h>
#include <platform_def.h>
#include "mpic_regs.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include "ppc.h"
#include "ipiQueue.h"

#ifdef TESTFRAMEWORK_ENABLE
#include "test_framework_config.h"
#if GEUL_DEMO_LATENCY_TEST
#include "tbgen_new.h"
u64 ipiStart;
#endif
#endif

QueueHandle_t pxRxQueue[IPI_EVT_ID_MAX];

struct IPIQElem
{
    u32 srcCore;
    u32 dstCore;
    enum IPIEventID eventID;
    void * useData;
};

#define IPI_NOTIFY_QUEUE_SIZE       8

#define CONFIG_NUM_IPI_TX_CHAN  4 /* limited by number of IPI TX channels */

enum IPIEventID IPIGlobalEventID[ IPI_EVT_ID_MAX ] =
{
    IPI_EVT_ID0,
    IPI_EVT_ID1,
    IPI_EVT_ID2,
    IPI_EVT_ID3,
    IPI_EVT_ID4,
    IPI_EVT_ID5,
    IPI_EVT_ID6,
    IPI_EVT_ID7,
    IPI_EVT_ID8,
    IPI_EVT_ID9,
    IPI_EVT_ID10,
    IPI_EVT_ID11,
    IPI_EVT_ID12,
    IPI_EVT_ID13,
    IPI_EVT_ID14,
    IPI_EVT_ID15,
    #ifdef TESTFRAMEWORK_ENABLE
        IPI_EVT_TESTFRAMEWORK,
    #endif
    #ifdef HAWK_ENABLED
        IPI_EVT_HAWK,
    #endif
    #ifdef APIPLAYER_ENABLED
        IPI_EVT_APIPLAYER,
	#endif
    #if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
        IPI_EVT_L1C_REFAPP_CLI,
        IPI_EVT_L1C_REFAPP_IPI,
    #endif

};



struct IPIGlobalQueue
{
    struct IPIQElem elements[ GEUL_E200_CORE_GLOBAL_NUM ][ IPI_NOTIFY_QUEUE_SIZE ];
    int head[ GEUL_E200_CORE_GLOBAL_NUM ];
    int tail[ GEUL_E200_CORE_GLOBAL_NUM ];
    int eqCnt[ GEUL_E200_CORE_GLOBAL_NUM ];
    int dqCnt[ GEUL_E200_CORE_GLOBAL_NUM ];
};

enum IPIRxISREvent
{
    IPI_RX_IPI0ISR_EVENT = 0x01,
    IPI_RX_IPI1ISR_EVENT = 0x02,
    IPI_RX_IPI2ISR_EVENT = 0x04,
    IPI_RX_IPI3ISR_EVENT = 0x8,
    IPI_EVENT_ALL = ( IPI_RX_IPI0ISR_EVENT |
                      IPI_RX_IPI1ISR_EVENT |
                      IPI_RX_IPI2ISR_EVENT |
                      IPI_RX_IPI3ISR_EVENT
                      )
};

struct IPIQueue
{
    u32 currentCore;
    u32 IPIRXIrqNum[ GEUL_E200_CORE_GLOBAL_NUM ];
    void * IPITxChannel;
    int * IPISync;
    struct IPIGlobalQueue * IPIGlobalQ;
    EventGroupHandle_t IPIISREventGrp;
    QueueHandle_t pxIPIISREventQueue;
    pxEventCb IPIEventCBList[ IPI_EVT_ID_MAX ];
    void * IPIEventCookieList[ IPI_EVT_ID_MAX ];
    u32 IPIEventMux;
    SemaphoreHandle_t IPIEveRegSem;
    SemaphoreHandle_t IPISendSem;
    TaskHandle_t IPIRXHandler;
};

static const char gIPIRXTaskName[ 32 ] = "IPIRXTask";

static struct IPIQueue gIPIQueuePriv;

struct IPIGlobalQueue IPIGlobalQueueMem[ GEUL_E200_CORE_GLOBAL_NUM ] __attribute__( ( section( ".smem" ) ) );
int IPISyncMem[ GEUL_E200_CORE_GLOBAL_NUM ] __attribute__( ( section( ".smem" ) ) );

u32 gIPISentStats[ GEUL_E200_CORE_GLOBAL_NUM ];
u32 gIPIRecvStats[ GEUL_E200_CORE_GLOBAL_NUM ];

static inline int vIPIGlobalQEmpty( int head,
                                    int tail );

/* Update sent stats count */
static inline void vIPIUpdateSentStats( u32 dstCore )
{
    gIPISentStats[ dstCore ] += 1;
}

/* Update recv stats count */
static inline void vIPIUpdateRecvStats( u32 srcCore )
{
    gIPIRecvStats[ srcCore ] += 1;
}

/* Get IPI Stats Data*/
void vIPIGetStats( void * statsData )
{
    u8 i;
    IPIStatsData_t * localStatsData = ( IPIStatsData_t * ) statsData;

    localStatsData->current_core = ulMpicCurrentCore();
    struct IPIGlobalQueue * IPIQ;

    for( i = 0; i < get_soc_numcores(); i++ )
    {
        localStatsData->IPISentStats[ i ] = gIPISentStats[ i ];
        localStatsData->IPIRecvStats[ i ] = gIPIRecvStats[ i ];

        IPIQ = &gIPIQueuePriv.IPIGlobalQ[ i ];
        localStatsData->IPIGlobalEnq[ i ] = IPIQ->eqCnt[ localStatsData->current_core ];

        IPIQ = &gIPIQueuePriv.IPIGlobalQ[ localStatsData->current_core ];
        localStatsData->IPIGlobalDeq[ i ] = IPIQ->dqCnt[ i ];
    }
}

static inline int vIPIGlobalQFull( int head,
                                   int tail,
                                   int total )
{
    if( ( tail == ( total - 1 ) ) && ( head == 0 ) )
    {
        return pdTRUE;
    }

    if( ( tail + 1 ) == head )
    {
        return pdTRUE;
    }

    return pdFALSE;
}

static inline int vIPIGlobalQEmpty( int head,
                                    int tail )
{
    if( head == tail )
    {
        return pdTRUE;
    }

    return pdFALSE;
}

static inline int vIPIGlobalQSpace( int head,
                                    int tail,
                                    int total )
{
    return tail >= head ? ( total - ( tail - head ) ) : ( head - tail - 1 );
}

static int vIPIGlobalQEnqueue( struct IPIQElem * elem )
{
    u32 dstCore = elem->dstCore;
    struct IPIGlobalQueue * IPIQ = &gIPIQueuePriv.IPIGlobalQ[ dstCore ];
    int * head = &IPIQ->head[ elem->srcCore ];
    int * tail = &IPIQ->tail[ elem->srcCore ];
    int * eqCnt = &IPIQ->eqCnt[ elem->srcCore ];

    configASSERT( elem->srcCore == gIPIQueuePriv.currentCore );

    if( !vIPIGlobalQFull( *head, *tail, IPI_NOTIFY_QUEUE_SIZE ) )
    {
        IPIQ->elements[ elem->srcCore ][ *tail ].srcCore = elem->srcCore;
        IPIQ->elements[ elem->srcCore ][ *tail ].dstCore = elem->dstCore;
        IPIQ->elements[ elem->srcCore ][ *tail ].eventID = elem->eventID;
        IPIQ->elements[ elem->srcCore ][ *tail ].useData = elem->useData;

        if( *tail < ( IPI_NOTIFY_QUEUE_SIZE - 1 ) )
        {
            ( *tail )++;
        }
        else
        {
            ( *tail ) = 0;
        }

        ( *eqCnt )++;
        return pdPASS;
    }

    return pdFAIL;
}

static int vIPIGlobalQDequeue( u8 srcCore,
                               void ** useData,
                               enum IPIEventID * eventID )
{
    u32 dstCore = gIPIQueuePriv.currentCore;
    struct IPIGlobalQueue * IPIQ = &gIPIQueuePriv.IPIGlobalQ[ dstCore ];
    int * head = &IPIQ->head[ srcCore ];
    int * tail = &IPIQ->tail[ srcCore ];
    int * dqCnt = &IPIQ->dqCnt[ srcCore ];

    if( !vIPIGlobalQEmpty( *head, *tail ) )
    {
        *useData = IPIQ->elements[ srcCore ][ *head ].useData;
        *eventID = IPIQ->elements[ srcCore ][ *head ].eventID;

        if( *head < ( IPI_NOTIFY_QUEUE_SIZE - 1 ) )
        {
            ( *head )++;
        }
        else
        {
            ( *head ) = 0;
        }

        ( *dqCnt )++;
        return pdPASS;
    }

    return pdFAIL;
}

static void vIPIIrqEnable( u8 ipi_channel,
                           bool enable )
{
    u32 ipivpr;

    ipivpr = mpic_in32( MPIC_REGS_IPIVPR0 + ( u32 ) ( 0x10 * ipi_channel ) );

    log_dbg( "vIPIIrqEnable ipi_channel:%d, ipivpr:0x%08x\r\n", ipi_channel, ipivpr );

    if( enable )
    {
        mpic_out32( MPIC_REGS_IPIVPR0 + ( u32 ) ( 0x10 * ipi_channel ),
                    ipivpr & ~( MASK_DISABLE << MSK ) );
    }
    else
    {
        mpic_out32( MPIC_REGS_IPIVPR0 + ( u32 ) ( 0x10 * ipi_channel ),
                    ipivpr | ( MASK_DISABLE << MSK ) );
    }
}

static inline void vIPIHWTriger( u32 dstCore )
{
    configASSERT( dstCore < get_soc_numcores() );
    mpic_out32( gIPIQueuePriv.IPITxChannel, ( u32 ) ( 1 << dstCore ) );
}

enum IPIEventID vIPIEventRegister( enum IPIEventID eventID,
                                   QueueHandle_t * Queue,
                                   pxEventCb cb,
                                   void * cookie )
{
    log_dbg( "vIPIEventRegister eventID:%d start\r\n", eventID );
    xSemaphoreTake( gIPIQueuePriv.IPIEveRegSem, portMAX_DELAY );
    log_dbg( "vIPIEventRegister eventID:%d, gIPITargetID:0x%08x\r\n", eventID,
             gIPIQueuePriv.IPIEventMux );

    if( ( eventID < IPI_EVT_ID0 ) || ( eventID >= IPI_EVT_ID_MAX ) )
    {
        log_err( "Invalid EventID error with event ID %d\r\n", eventID );
        xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );
        return IPI_EVT_ID_NULL;
    }

    if( gIPIQueuePriv.IPIEventMux & ( u32 ) ( 1 << ( ( int ) eventID ) ) )
    {
        log_err( "Event ID %d is already registered\n\r" );
        xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );
        return IPI_EVT_ID_NULL;
    }

    gIPIQueuePriv.IPIEventMux |= ( u32 ) ( 1 << ( ( int ) eventID ) );

    if( ( Queue == NULL ) && ( cb == NULL ) )
    {
        log_err( "Both Queue and Callback cannot be NULL\n\r" );
        return IPI_EVT_ID_NULL;
    }

    if( ( Queue == NULL ) && ( cb != NULL ) )
    {
        gIPIQueuePriv.IPIEventCBList[ eventID ] = cb;
        gIPIQueuePriv.IPIEventCookieList[ eventID ] = cookie;
    }
    else if( ( Queue != NULL ) && ( cb != NULL ) )
    {
        *Queue = xQueueCreate( IPI_NOTIFY_QUEUE_SIZE, sizeof( void * ) );
        gIPIQueuePriv.IPIEventCBList[ eventID ] = cb;
        gIPIQueuePriv.IPIEventCookieList[ eventID ] = cookie;
    }
    else if( ( Queue != NULL ) && ( cb == NULL ) )
    {
        *Queue = xQueueCreate( IPI_NOTIFY_QUEUE_SIZE, sizeof( void * ) );
        gIPIQueuePriv.IPIEventCBList[ eventID ] = NULL;
        gIPIQueuePriv.IPIEventCookieList[ eventID ] = NULL;
    }

    xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );

    return eventID;
}

void vIPIEventUnRegister( enum IPIEventID eventID )
{
    xSemaphoreTake( gIPIQueuePriv.IPIEveRegSem, portMAX_DELAY );

    if( ( eventID < IPI_EVT_ID0 ) || ( eventID >= IPI_EVT_ID_MAX ) )
    {
        log_err( "Invalid Event error with event ID %d\r\n", eventID );
        xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );
        return;
    }

    if( !( gIPIQueuePriv.IPIEventMux & ( u32 ) ( 1 << ( ( int ) eventID ) ) ) )
    {
        log_err( "Event ID %d not registered\r\n", eventID );
        xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );
        return;
    }

    gIPIQueuePriv.IPIEventMux &= ~( ( u32 ) ( 1 << ( ( int ) eventID ) ) );

    if( pxRxQueue[ eventID ] != NULL )
    {
        vQueueDelete( pxRxQueue[ eventID ] );
        pxRxQueue[ eventID ] = NULL;
    }

    gIPIQueuePriv.IPIEventCBList[ eventID ] = NULL;
    gIPIQueuePriv.IPIEventCookieList[ eventID ] = NULL;
    xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );
}
static BaseType_t vIPIReceiveTaskEntry( uint32_t ulirq )
{
	void *usrData;
	enum IPIEventID eventID;
	BaseType_t wakeup = pdFALSE;
#if defined(TBGEN_ENABLED) && GEUL_DEMO_LATENCY_TEST
	ipiStart = ullTbgenGetMasterCounterRaw ( TBGEN_2 );
#endif
	if( ulirq == gIPIQueuePriv.IPIRXIrqNum[ 0 ] ) {
		while ((vIPIGlobalQDequeue(0, &usrData, &eventID) == pdPASS) 
#if GEUL_E200_CORE_GLOBAL_NUM > GEUL_E200_CORE_REVA_NUM
				|| (vIPIGlobalQDequeue(4, &usrData, &eventID) == pdPASS)
#endif
				) {
			if (eventID < IPI_EVT_ID0 || eventID >= IPI_EVT_ID_MAX) {
				log_err("IPI0 Event error with event ID %d\r\n", eventID);
				break;
			}
			if (!(gIPIQueuePriv.IPIEventMux & (u32)(1 << eventID))) {
				log_err("IPI0 Event %d not registered\r\n", eventID);
				break;
			}
			if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, NULL, gIPIQueuePriv.IPIEventCookieList[eventID]);
			} else if (pxRxQueue[eventID] == NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, usrData, gIPIQueuePriv.IPIEventCookieList[eventID]);
			}
			else if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]==NULL)
			{
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
			}
			vIPIUpdateRecvStats(0);
		}
	}

	if( ulirq == gIPIQueuePriv.IPIRXIrqNum[ 1 ] ){
		while ((vIPIGlobalQDequeue(1, &usrData, &eventID) == pdPASS)
#if GEUL_E200_CORE_GLOBAL_NUM > GEUL_E200_CORE_REVA_NUM
				|| (vIPIGlobalQDequeue(5, &usrData, &eventID) == pdPASS)
#endif
				) {
			if (eventID < IPI_EVT_ID0 || eventID >= IPI_EVT_ID_MAX) {
				log_err("IPI1 Event error with event ID %d\r\n", eventID);
				break;
			}
			if (!(gIPIQueuePriv.IPIEventMux & (u32)(1 << eventID))) {
				log_err("IPI1 Event %d not registered\r\n", eventID);
				break;
			}

			if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, NULL, gIPIQueuePriv.IPIEventCookieList[eventID]);
			} else if (pxRxQueue[eventID] == NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, usrData, gIPIQueuePriv.IPIEventCookieList[eventID]);
			}
			else if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]==NULL)
			{
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
			}

			vIPIUpdateRecvStats(1);
		}
	}

#if defined(GEUL_E200_CORE_GLOBAL_NUM)
	if( ulirq == gIPIQueuePriv.IPIRXIrqNum[ 2 ] ) {
		while (vIPIGlobalQDequeue(2, &usrData, &eventID) == pdPASS) {
			if (eventID < IPI_EVT_ID0 || eventID >= IPI_EVT_ID_MAX) {
				log_err("IPI2 Event error with event ID %d\r\n", eventID);
				break;
			}
			if (!(gIPIQueuePriv.IPIEventMux & (u32)(1 << eventID))) {
				log_err("IPI2 Event %d not registered\r\n", eventID);
				break;
			}

			if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, NULL, gIPIQueuePriv.IPIEventCookieList[eventID]);
			} else if (pxRxQueue[eventID] == NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, usrData, gIPIQueuePriv.IPIEventCookieList[eventID]);
			}
			else if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]==NULL)
			{
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
			}

			vIPIUpdateRecvStats(2);
		}
	}

	if( ulirq == gIPIQueuePriv.IPIRXIrqNum[ 3 ] ) {
		while (vIPIGlobalQDequeue(3, &usrData, &eventID) == pdPASS) {
			if (eventID < IPI_EVT_ID0 || eventID >= IPI_EVT_ID_MAX) {
				log_err("IPI3 Event error with event ID %d\r\n", eventID);
				break;
			}
			if (!(gIPIQueuePriv.IPIEventMux & (u32)(1 << eventID))) {
				log_err("IPI3 Event %d not registered\r\n", eventID);
				break;
			}

			if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, NULL, gIPIQueuePriv.IPIEventCookieList[eventID]);
			} else if (pxRxQueue[eventID] == NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
				gIPIQueuePriv.IPIEventCBList[eventID](eventID, usrData, gIPIQueuePriv.IPIEventCookieList[eventID]);
			}
			else if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]==NULL)
			{
				while (xQueueSendFromISR(pxRxQueue[eventID], &usrData, &wakeup) != pdTRUE) {
					log_dbg("IPI0 RX send error with eventID %d\t\n", eventID);
				}
			}

			vIPIUpdateRecvStats(3);
		}
	}
#endif
	return wakeup;
}

#ifdef IPI_GLOBAL_Q_DBG
    void vIPIGlobalQStatusCheck( int dstCore,
                                 int srcCore )
    {
        struct IPIGlobalQueue * IPIQ;

        if( ( dstCore >= 0 ) && ( dstCore < get_soc_numcores() ) )
        {
            IPIQ = &gIPIQueuePriv.IPIGlobalQ[ dstCore ];

            log_info( "GlobalQ[%d]: %d %d, %d %d, %d %d, %d %d\r\n",
                      dstCore, IPIQ->eqCnt[ 0 ], IPIQ->dqCnt[ 0 ], IPIQ->eqCnt[ 1 ], IPIQ->dqCnt[ 1 ],
                      IPIQ->eqCnt[ 2 ], IPIQ->dqCnt[ 2 ], IPIQ->eqCnt[ 3 ], IPIQ->dqCnt[ 3 ] );
        }

        if( ( srcCore >= 0 ) && ( srcCore < get_soc_numcores() ) )
        {
            IPIQ = gIPIQueuePriv.IPIGlobalQ;

            log_info( "GlobalQ from %d: %d %d, %d %d, %d %d, %d %d\r\n",
                      srcCore, IPIQ[ 0 ].eqCnt[ srcCore ], IPIQ[ 0 ].dqCnt[ srcCore ],
                      IPIQ[ 1 ].eqCnt[ srcCore ], IPIQ[ 1 ].dqCnt[ srcCore ],
                      IPIQ[ 2 ].eqCnt[ srcCore ], IPIQ[ 2 ].dqCnt[ srcCore ],
                      IPIQ[ 3 ].eqCnt[ srcCore ], IPIQ[ 3 ].dqCnt[ srcCore ] );
        }
    }
#endif /* ifdef IPI_GLOBAL_Q_DBG */

static bool_t vIPIHandle(uint32_t ulirq_no, void *dev_data)
{
	BaseType_t wakeup = pdFALSE, ret = pdTRUE;
	(void)dev_data;
	wakeup = vIPIReceiveTaskEntry(ulirq_no);
	portYIELD_FROM_ISR( wakeup );

	return ret;
}

static int vIPILowLevelInit( void )
{
    u8 i = 0, j = 0;
    u32 ulReg;
    int ret = -1;

    gIPIQueuePriv.currentCore = ulMpicCurrentCore();
    gIPIQueuePriv.IPISync = IPISyncMem;
    gIPIQueuePriv.IPIGlobalQ = IPIGlobalQueueMem;

    if( gIPIQueuePriv.currentCore == GEUL_E200_MASTER_CORE )
    {
        for( i = 0; i < get_soc_numcores(); i++ )
        {
            memset( &gIPIQueuePriv.IPIGlobalQ[ i ], 0, sizeof( struct IPIGlobalQueue ) );
            gIPIQueuePriv.IPISync[ i ] = 0;
        }
    }

    i = 0;

    ulReg = MPIC_REGS_IPIVPR0;

    for( j = 0; j < CONFIG_NUM_IPI_TX_CHAN; j++ )
    {
        u32 ipivpr = mpic_in32( ulReg );

        gIPIQueuePriv.IPIRXIrqNum[ i ] = ipivpr & MPIC_IPIVPR_VECTOR_MASK;
        ret = lRegisterIrq( gIPIQueuePriv.IPIRXIrqNum[ i ], vIPIHandle, NULL );

        if( ret != 1 )
        {
            return ret;
        }

        vIPIIrqEnable( i, TRUE );
        log_dbg( "IrqNum[%d]:%d on core:%d\r\n", i, gIPIQueuePriv.IPIRXIrqNum[ i ], gIPIQueuePriv.currentCore );
        ulReg += 0x10;
        i++;
    }

    if (gIPIQueuePriv.currentCore < CONFIG_NUM_IPI_TX_CHAN)
	    gIPIQueuePriv.IPITxChannel = ( void * ) ( MPIC_REGS_IPIDR0 + ( u32 ) ( 0x10 * gIPIQueuePriv.currentCore ) );
    else
	    gIPIQueuePriv.IPITxChannel = ( void * ) ( MPIC_REGS_IPIDR0 + ( u32 ) ( 0x10 * (gIPIQueuePriv.currentCore - CONFIG_NUM_IPI_TX_CHAN) ) );

    return ret;
}

BaseType_t vIPICoreInit( uint8_t core_id  __attribute__((unused)))
{
    int ret;

    memset( &gIPIQueuePriv, 0, sizeof( struct IPIQueue ) );
    ret = vIPILowLevelInit();

    if( ret != 1 )
    {
        return pdFAIL;
    }

    gIPIQueuePriv.IPIISREventGrp = xEventGroupCreate();
    log_dbg( "xIPI2PeerTskEvent:%p Core: %u\r\n", gIPIQueuePriv.IPIISREventGrp, core_id );
    configASSERT( gIPIQueuePriv.IPIISREventGrp );

    if( !gIPIQueuePriv.IPIISREventGrp )
    {
        return pdFAIL;
    }

    gIPIQueuePriv.pxIPIISREventQueue = xQueueCreate( get_soc_numcores(), sizeof( EventBits_t ) );
    if( !gIPIQueuePriv.pxIPIISREventQueue )
    {
	    return pdFAIL;
    }

    gIPIQueuePriv.IPIEveRegSem = xSemaphoreCreateBinary();
    log_dbg( "IPIEveRegSem:%p Core: %u\r\n", gIPIQueuePriv.IPIEveRegSem, core_id );

    if( !gIPIQueuePriv.IPIEveRegSem )
    {
        return pdFAIL;
    }

    xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );

    gIPIQueuePriv.IPISendSem = xSemaphoreCreateBinary();
    log_dbg( "gIPISendSem:%p Core: %u\r\n", gIPIQueuePriv.IPISendSem, core_id );

    if( !gIPIQueuePriv.IPISendSem )
    {
        return pdFAIL;
    }

    xSemaphoreGive( gIPIQueuePriv.IPISendSem );

    log_dbg( "Create IPI RX Task ret:%d Core: %u\r\n", ret, core_id );

    if( ret != pdPASS )
    {
        log_info( "Failed to create task: %s\r\n", gIPIRXTaskName );
    }

    return ret;
}

BaseType_t vIPISendData( u32 dstCore,
                         enum IPIEventID eventID,
                         void * useData )
{
    struct IPIQElem elem;

    if( ( eventID < IPI_EVT_ID0 ) || ( eventID >= IPI_EVT_ID_MAX ) )
    {
        log_err( "Invalid Event error with event ID %d\r\n", eventID );
        xSemaphoreGive( gIPIQueuePriv.IPISendSem );
        return pdFALSE;
    }

    taskENTER_CRITICAL();

    elem.srcCore = gIPIQueuePriv.currentCore;
    elem.dstCore = dstCore;
    elem.eventID = eventID;
    elem.useData = useData;

    xSemaphoreTake( gIPIQueuePriv.IPISendSem, portMAX_DELAY );

    if( dstCore == gIPIQueuePriv.currentCore )
    {
        BaseType_t ret = pdFALSE;

        if( !( gIPIQueuePriv.IPIEventMux & ( u32 ) ( 1 << eventID ) ) )
        {
            log_err( "Event ID %d not registered\r\n", eventID );
            xSemaphoreGive( gIPIQueuePriv.IPISendSem );
            taskEXIT_CRITICAL();
            return pdFALSE;
        }

        if( ( pxRxQueue[ eventID ] != NULL ) && ( gIPIQueuePriv.IPIEventCBList[ eventID ] != NULL ) )
        {
            ret = xQueueSend( pxRxQueue[ eventID ], &useData, 0 );
            gIPIQueuePriv.IPIEventCBList[ eventID ]( eventID, NULL, gIPIQueuePriv.IPIEventCookieList[ eventID ] );
        }
        else if( ( pxRxQueue[ eventID ] == NULL ) && ( gIPIQueuePriv.IPIEventCBList[ eventID ] != NULL ) )
        {
            ret = pdTRUE;
            gIPIQueuePriv.IPIEventCBList[ eventID ]( eventID, useData, gIPIQueuePriv.IPIEventCookieList[ eventID ] );
        }
        else if( ( pxRxQueue[ eventID ] != NULL ) && ( gIPIQueuePriv.IPIEventCBList[ eventID ] == NULL ) )
        {
            ret = xQueueSend( pxRxQueue[ eventID ], &useData, 0 );
        }

        xSemaphoreGive( gIPIQueuePriv.IPISendSem );

        if( ret == pdTRUE )
        {
            vIPIUpdateSentStats( dstCore );
            vIPIUpdateRecvStats( gIPIQueuePriv.currentCore );
        }

        taskEXIT_CRITICAL();
        return ret;
    }

    if( vIPIGlobalQEnqueue( &elem ) == pdFAIL )
    {
        log_err( "Failed to enqueue data to global queue with event ID: %d\r\n", eventID );
        xSemaphoreGive( gIPIQueuePriv.IPISendSem );
        taskEXIT_CRITICAL();
        return pdFALSE;
    }

    vIPIUpdateSentStats( dstCore );

    isync();
    msync();
    vIPIHWTriger( elem.dstCore );
    xSemaphoreGive( gIPIQueuePriv.IPISendSem );

    taskEXIT_CRITICAL();
    return pdTRUE;
}

BaseType_t vIPISendDatafromISR(u32 dstCore, enum IPIEventID eventID, void *useData)
{
	struct IPIQElem elem;

	if (eventID < IPI_EVT_ID0 || eventID >= IPI_EVT_ID_MAX) {
		log_err("Invalid Event error with event ID %d\r\n", eventID);
		xSemaphoreGiveFromISR(gIPIQueuePriv.IPISendSem, NULL);
		return pdFALSE;
	}

	elem.srcCore = gIPIQueuePriv.currentCore;
	elem.dstCore = dstCore;
	elem.eventID = eventID;
	elem.useData = useData;

	if (xSemaphoreTakeFromISR(gIPIQueuePriv.IPISendSem, NULL) != pdPASS) {
		log_err("Failed to take semaphore with event ID: %d\r\n", eventID);
		return pdFALSE;
	}

	if (dstCore == gIPIQueuePriv.currentCore) {
		BaseType_t ret = pdFALSE;
		if (!(gIPIQueuePriv.IPIEventMux & (u32)(1 << eventID))) {
			log_err("Event ID %d not registered\r\n", eventID);
			xSemaphoreGiveFromISR(gIPIQueuePriv.IPISendSem, NULL);
			return pdFALSE;
		}

		if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL) {
			ret = xQueueSendFromISR(pxRxQueue[eventID], &useData, NULL);
			gIPIQueuePriv.IPIEventCBList[eventID](eventID, NULL, gIPIQueuePriv.IPIEventCookieList[eventID]);
		} else if (pxRxQueue[eventID] == NULL && gIPIQueuePriv.IPIEventCBList[eventID]!=NULL)
			{
			ret = pdTRUE;
			gIPIQueuePriv.IPIEventCBList[eventID](eventID, useData, gIPIQueuePriv.IPIEventCookieList[eventID]);
		}
		else if (pxRxQueue[eventID] != NULL && gIPIQueuePriv.IPIEventCBList[eventID]==NULL)
		{
			ret = xQueueSendFromISR(pxRxQueue[eventID], &useData, NULL);
		}

		xSemaphoreGiveFromISR(gIPIQueuePriv.IPISendSem, NULL);

		if ( ret == pdTRUE ) {
			vIPIUpdateSentStats(dstCore);
			vIPIUpdateRecvStats(gIPIQueuePriv.currentCore);
		}

		return ret;
	}

	if (vIPIGlobalQEnqueue(&elem) == pdFAIL) {
		log_err("Failed to enqueue data to global queue with event ID: %d\r\n", eventID);
		xSemaphoreGiveFromISR(gIPIQueuePriv.IPISendSem, NULL);
		return pdFALSE;
	}

	vIPIUpdateSentStats(dstCore);

	isync();
	msync();
	vIPIHWTriger(elem.dstCore);
	xSemaphoreGiveFromISR(gIPIQueuePriv.IPISendSem, NULL );

	return pdTRUE;
}

static void vIPISyncSet( void )
{
    gIPIQueuePriv.IPISync[ gIPIQueuePriv.currentCore ] = 1;
}

static int vIPISyncCheck( void )
{
    uint32_t i;

    for( i = 0; i < get_soc_numcores(); i++ )
    {
        if( !gIPIQueuePriv.IPISync[ i ] )
        {
            return -1;
        }
    }

    return 1;
}
static void vIPIUnSyncSet( void )
{
    gIPIQueuePriv.IPISync[ gIPIQueuePriv.currentCore ] = 0;
}


static int vIPIUnSyncCheck( void )
{
    uint32_t i;

    for( i = 0; i < get_soc_numcores(); i++ )
    {
        log_dbg( "gIPIQueuePriv.IPISync[%d]=%d\r\n", i, gIPIQueuePriv.IPISync[ i ] );

        if( gIPIQueuePriv.IPISync[ i ] )
        {
            return -1;
        }
    }

    return 1;
}

void syncUnSync( void )
{
    xSemaphoreTake( gIPIQueuePriv.IPIEveRegSem, portMAX_DELAY );

    log_dbg( "[%s] Enter Cores Sync\r\n", __func__ );
    vIPISyncSet();

    while( 1 )
    {
        if( vIPISyncCheck() > 0 )
        {
            break;
        }

        vTaskDelay( 1 );
    }

    log_dbg( "Cores Syncing:- Wait for 5 clock ticks\r\n" );
    vTaskDelay( 5 );
    log_dbg( "Now do Cores unSync \r\n" );
    vIPIUnSyncSet();

    while( 1 )
    {
        if( vIPIUnSyncCheck() > 0 )
        {
            break;
        }

        vTaskDelay( 1 );
    }

    log_dbg( "[%s] Cores Synced.. Exiting\r\n", __func__ );

    xSemaphoreGive( gIPIQueuePriv.IPIEveRegSem );
}
