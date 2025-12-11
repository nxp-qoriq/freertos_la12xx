// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef SRC_INIQUEUE_H_
#define SRC_INIQUEUE_H_

#include "platform_def.h"
#include "queue.h"
#include "event_groups.h"

/**
 * @file        ipiQueue.h
 * @brief       IPI-related APIs.
 * @addtogroup  IPI_API
 * @{
 */

#define IPI_GQUEUE_SIZE 8

/**
 * \enum IPIEventID_t
 * @brief Enum to represent event IDs for IPI events. Range: [0 - 15]
 */
typedef enum IPIEventID
{
    IPI_EVT_ID_NULL = -1,
    IPI_EVT_ID0 = 0,
    IPI_EVT_ID1 = 1,
    IPI_EVT_ID2 = 2,
    IPI_EVT_ID3 = 3,
    IPI_EVT_ID4 = 4,
    IPI_EVT_ID5 = 5,
    IPI_EVT_ID6 = 6,
    IPI_EVT_ID7 = 7,
    IPI_EVT_ID8 = 8,
    IPI_EVT_ID9 = 9,
    IPI_EVT_ID10 = 10,
    IPI_EVT_ID11 = 11,
    IPI_EVT_ID12 = 12,
    IPI_EVT_ID13 = 13,
    IPI_EVT_ID14 = 14,
    IPI_EVT_ID15 = 15,
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
    IPI_EVT_WDOG,
    IPI_EVT_ID_MAX
} IPIEventID_t;

/**
 * \struct IPIStatsData_t
 * @brief Structure to store IPI stats
 */
typedef struct IPIStatsData {
	u32 current_core;								/**< Current Core */
	u32 IPISentStats[GEUL_E200_CORE_GLOBAL_NUM];	/**< Sent IPI Stats wrt all cores */
	u32 IPIRecvStats[GEUL_E200_CORE_GLOBAL_NUM];	/**< Received IPI Stats wrt all cores */
	int IPIGlobalEnq[GEUL_E200_CORE_GLOBAL_NUM];	/**< IPI Global Enqueue count wrt all cores */
	int IPIGlobalDeq[GEUL_E200_CORE_GLOBAL_NUM];	/**< IPI Global Dequeue Count wrt all cores */
} IPIStatsData_t;

/**
 * \var IPIGlobalEventID
 * @brief Global Enum type array to store registered IPI Events. Range: [0 - 15]
 */
enum IPIEventID IPIGlobalEventID[ IPI_EVT_ID_MAX ];

/**
 * \typedef pxEventCb(enum IPIEventID eventID, void *userData, void *cookie)
 * @brief   Function Pointer for callback mechanism when an IPI Event occurs
 *
 * @param[in]	eventID			IPI eventID of the event. Range: [0 - 15]
 * @param[in]	userdata		A void pointer pointing to the data received
 * @param[in]	cookie			A void pointer pointing to the stored cookie
 *
 * @return
 *	- Returns Nothing
 */
typedef void (*pxEventCb)(enum IPIEventID eventID, void *userData, void *cookie);

/**
 * \fn enum IPIEventID vIPIEventRegister(enum IPIEventID eventID, QueueHandle_t *Queue, pxEventCb cb, void *cookie)
 * @brief Function to register an IPI event
 *
 * @param[in]	eventID			IPI eventID of the event to register. Range: [0 - 15]
 * @param[in]	Queue			Pointer to the queue to store the created Queue Handle if Queue is needed. Can be NULL if not needed.
 * @param[in]	cb				The callback function wrt the eventID. Can be NULL if not needed.
 * @param[in]	cookie			Pointer to stored data to keep as cookie and can be get back through the callback
 *
 * @return
 *   - On Success, Returns registered eventID
 *   - On Failure, Returns EVENT_ID_NULL, -1
 */
enum IPIEventID vIPIEventRegister(enum IPIEventID eventID, QueueHandle_t *Queue, pxEventCb cb, void *cookie);

/**
 * \fn void vIPIEventUnRegister(enum IPIEventID eventID)
 * @brief Function to Unregister a registered event
 *
 * @param[in]	eventID		IPI eventID of the event. Range: [0 - 15]
 *
 * @return
 *	- Returns Nothing
 */
void vIPIEventUnRegister(enum IPIEventID eventID);

/**
 * \fn BaseType_t vIPICoreInit(uint8_t core_id)
 * @brief Function to Initialize IPI Functionality
 *
 * @param[in]	core_id		Core ID. Range: [0 - 3]
 *
 * @return
 *   - On Success, pdPASS
 *   - On Failure, pdFAIL
 */
BaseType_t vIPICoreInit(uint8_t core_id);

/**
 * \fn BaseType_t vIPISendData(u32 dstCore, enum IPIEventID eventID, void *useData)
 * @brief Function to Send an IPI to a core
 *
 * @param[in]  dstCore                 Destination Core ID. Range: [0 - 3]
 * @param[in]  eventID              IPI eventID of the event. Range: [0 - 15]
 * @param[in]  useData                A void pointer pointing to the data received
 *
 * @return
 *   -On Success, pdPASS.
 *    On Failure, pdFAIL
 */
BaseType_t vIPISendData( u32 dstCore,
                         enum IPIEventID eventID,
                         void * useData );

/**
 * \fn BaseType_t vIPISendDatafromISR(u32 dstCore, enum IPIEventID eventID, void *useData)
 * @brief Function to Send an IPI to a core from ISR
 *
 * @param[in]	dstCore		Destination Core ID. Range: [0 - 3]
 * @param[in]	eventID		IPI eventID of the event. Range: [0 - 15]
 * @param[in]	useData		A void pointer pointing to the data received
 *
 * @return
 *   - On Success, pdPASS
 *   - On Failure, pdFAIL
 */
BaseType_t vIPISendDatafromISR( u32 dstCore,
                         enum IPIEventID eventID,
                         void * useData );

/**
 * \var pxRxQueue
 * @brief Global array to store QueueHandles of Registered IPI events
 */
extern QueueHandle_t pxRxQueue[ IPI_EVT_ID_MAX ];

/**
 * \fn void vIPIGetStats(void *statsData)
 * @brief Function Get stats data
 *
 * @param[in]	statsData		Pointer to store stats data
 *
 * @return
 *	- Returns Nothing
 */
void vIPIGetStats(void *statsData);

#ifdef IPI_GLOBAL_Q_DBG
/**
 * \fn void vIPIGlobalQStatusCheck(int dstCore, int srcCore)
 * @brief Function to check and display status of global Queue. Used only for debugging.
 *
 * @param[in]	dstCore		Destination Core ID. Range: [0 - 3]
 * @param[in]	srcCore		Source Core ID. Range: [0 - 3]
 *
 * @return
 *	- Returns Nothing
 */
void vIPIGlobalQStatusCheck(int dstCore, int srcCore);
#endif

/**
 * \fn void syncUnSync()
 * @brief Function to synchronize the cores
 *
 * @return
 *	- Returns Nothing
 */
void syncUnSync( void );

/** @} */
#endif /* SRC_INIQUEUE_H_ */
