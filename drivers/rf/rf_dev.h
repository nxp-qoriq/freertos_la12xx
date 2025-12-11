// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __RF_DEV_H__
#define __RF_DEV_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "rf_common.h"

#define RF_DEV_INIT_DEATH_LOOP( ulCoreId )                                               \
    do {                                                                                 \
        RF_LOGERR( "RF init error..going in death loop: %u", ulCoreId); \
        while( 1 ) \
        { \
            ; \
        }                                                                     \
    }                                                                                     \
    while( 0 )

typedef enum
{
    eRFDeviceFailed = -1,
    eRFDeviceUnInitialized = 0,
    eRFCoreTaskCreated = 1,
    eRFLocalQueueCreated = 2,
    eRFRemoteQueueCreated = 3,
    eRFLocalRespQueueCreated = 4,
    eRFCmdDescSemaphoreCreated = 5,
    eRFCoreTaskEventGrpCreated = 7,
    eRFDeviceInitialized = 9
} eRFDeviceState;

typedef struct RFDevice
{
    volatile struct rf_common_mdata * pxRFMData; //4
    volatile struct rf_priv_mdata * pxPrvRFMData; //8
    /** TODO handle RF_SW_CMD_MAX_COUNT  properly in below stats */
    volatile struct rf_ep_stats * pxStats; //12
    TaskHandle_t xRFCoreTask; //16
    QueueHandle_t xRFLocalQueue; //20
    QueueHandle_t xRFRemoteQueue[ RF_TOTAL_USERS ]; //48
    QueueHandle_t xRFLocalQueueResp; //52
    SemaphoreHandle_t xRFSWCmdDescSema[ E200_CORE_COUNT ]; //76
    EventGroupHandle_t xRFCoreTaskEventGrp; //80
    eRFDeviceState eState[ E200_CORE_COUNT ]; //104
    eRficFR1Mode eFR1Mode; //108
    eRficFR1DupMode eFR1DupMode; //112
    eRficDevType eDevtype;
    /**
     * Common functionality of all rfic cards can be achieved by updating this
     * structure.
     */

    /**
     * This will be used for handling different rfic specific
     * operations.
     */
    volatile uint8_t *rfic_priv;
} RFDevice_t;

typedef volatile RFDevice_t * RficHandle_t;

/**
 * @brief Common type for yucca firmware command handler.
 *
 * @param xHandle RF Device handle.
 * @param pCmdDesc Command descriptor.
 *
 * @return Pass or fail.
 */

typedef int32_t (* iRFCmdApi)( RficHandle_t xHandle,
                                  rf_sw_cmd_desc_t * pCmdDesc );
typedef struct cmdHandler {
    iRFCmdApi func;
    uint8_t state;
} cmdHandler_t;

RficHandle_t xRFDeviceInit( void );
void xRFDevicePreInit( void );
RficHandle_t xRFGetDeviceHandle( void );
int32_t iHandleRFCmds( RficHandle_t xHandle,
                       rf_sw_cmd_desc_t * pxRFSWCmdDesc );

#endif /* ifndef __RF_DEV_H__ */
