// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#ifndef _AVI_H_
#define _AVI_H_
#include "FreeRTOS.h"
#include "task.h"

/**
 * @file        geul_avi.h
 * @brief       This file contains the VSPA AVI APIs for managing VSPA.
 * @addtogroup  VSPA_API
 * @{
 */

/*******************************************************
*			Defines
*******************************************************/
#define VSPA_MAX_CORES		(  8  )

struct AviHandle;
extern uint32_t ulVspaResp;
extern int32_t ulOverlay_resp;

/** Structure for 64-bit mailbox */
typedef struct AviMboxData {
	uint32_t ulMsb;
	uint32_t ulLsb;
} AviMboxData_t;

/** Enum To define Mailbox Index */
typedef enum VspaMboxIndex {
	VSPA_MBOX_0 = 0,
	VSPA_MBOX_1 = 1,
	VSPA_MBOX_MAX
} VspaMboxIndex_t;

#define VSPA_MBOX_R 0x1
#define VSPA_MBOX_W 0x2
#define VSPA_MBOX_RW (VSPA_MBOX_R | VSPA_MBOX_W)

/** Enum to define available VSPA Cores */
typedef enum VspaCore {
	VSPA_CORE_0 = 0,
	VSPA_CORE_1,
	VSPA_CORE_2,
	VSPA_CORE_3,
	VSPA_CORE_4,
	VSPA_CORE_5,
	VSPA_CORE_6,
	VSPA_CORE_7,
	VSPA_CORE_MAX = 8
} VspaCore_t;

/** Enum to define various VSPA interrupt groups */
typedef enum {
	VSPA_GROUP_A = 0,
	VSPA_GROUP_B = 1,
	VSPA_GROUP_MAX = 2
} VspaIsrGroup_t;

/** Enum to define VSPA AVI status codes */
typedef enum {
	AVI_SUCCESS			= 0,
	AVI_INVALID_MBOX_INDEX		= -1,
	AVI_MBOX_NOT_AVAILABLE		= -2,
	AVI_MBOX_RCV_FAIL		= -3,
	AVI_INIT_NOT_DONE		= -4,
	AVI_IRQ_ALREADY_REGISTERED	= -5,
	AVI_IRQ_REG_FAILURE		= -6,
	AVI_NO_MESSAGE_IN_MBOX0		= -7,
	AVI_NO_MESSAGE_IN_MBOX1		= -8,
} AviStatusCodes_t;

typedef struct AviHandle AviHandle_t;

typedef bIsrFunc VspaCallbackFn;
typedef void *VspaCallbackData;

/**
 * @brief Register interrupt for VSPA mailbox
 * @param[in] pxAviHandle AVI library handler
 * @param[in] eVspaCore VSPA core
 * @param[in] eVspaGroup VSPA interrupt group
 * @param[in] pvIsr function pointer to ISR
 * @param[in] pvData ISR data
 * @param[in] dir Select interrupt Enable direction: Recieve message from VSPA (R), Send message to VSPA (W) and RW
 * @param[in] QueueInit Enable queue to recieve message or recieve it directly in interrupt
 *
 * @return AviStatusCodes_t Returns AVI status Codes
*/
AviStatusCodes_t exGeulRegisterVspaInterrupt( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore, VspaMboxIndex_t eVspaGroup, VspaCallbackFn pvIsr, VspaCallbackData pvData, int dir, bool QueueInit);

/**
 * @brief Unregister interrupt for VSPA mailbox
 * @param[in] pxAviHandle AVI library handler
 * @param[in] eVspaCore VSPA core
 * @param[in] eVspaGroup VSPA interrupt group
 * @param[in] dir Select interrupt Enable direction: Recieve message from VSPA (R), Send message to VSPA (W) and RW
 *
 * @return AviStatusCodes_t Returns AVI status Codes
*/
AviStatusCodes_t exGeulUnRegisterVspaInterrupt( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore, VspaMboxIndex_t eVspaGroup, int dir );

/**
 * @brief Sends MBOX to VSPA using MBOX_0.
 *      Receiver's responsibility is to make sure that multiple e200 cores are not accessing the same mailbox
 *
 * @param[in] pxAviHandle AVI library handle
 * @param[in] eVspaCore VSPA instance
 * @param[in] eVspaMboxIndex VSPA mailbox index to be writen
 * @param[in] xAviMboxData Mailbox structure to be sent
 *
 * @return AviStatusCodes_t Returns AVI status Codes
*/
AviStatusCodes_t exGeulAviHostSendFastMboxToVspa( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore, VspaMboxIndex_t eVspaMboxIndex, AviMboxData_t xAviMboxData );

/**
 * @brief Sends MBOX to VSPA using MBOX_1
 * 	Synchronization is achieved using spinlock
 * @param[in] pxAviHandle AVI library handle
 * @param[in] eVspaCore VSPA instance
 * @param[in] eVspaMboxIndex VSPA mailbox index to be written
 * @param[in] xAviMboxData Mailbox structure to be sent
 *
 * @return AviStatusCodes_t Returns AVI status Codes
*/
AviStatusCodes_t exGeulAviHostSendSlowMboxToVspa( AviHandle_t * pxAviHandle, VspaCore_t eVspaCore, VspaMboxIndex_t eVspaMboxIndex, AviMboxData_t xAviMboxData );

/**
 * @brief Checks if VSPA out mailbox is valid. If valid, reads the VSPA
 *               mailbox and sends the mailbox to the relevant Queue
 *               (if enabled) or else returns error code
 *
 * @param[in]  pxAviHandle AVI library handle
 * @param[in]  eVspaCore VSPA core number
 * @param[in]  eVspaMboxIndex VSPA mailbox index to be read
 * @param[out] pxAviMboxData Mailbox structure to be populated after reading
 *
 * @return AviStatusCodes_t Returns AVI status Codes
 */
AviStatusCodes_t exGeulAviHostHandleMboxIrq( AviHandle_t *pxAviHandle, VspaCore_t eVspaCore, VspaMboxIndex_t eVspaMboxIndex, AviMboxData_t *pxAviMboxData );

/**
 * @brief Recieves the VSPA mailbox in the queue and
 * 		does further processing if needed.
 * @param[in] eVspaCore VSPA core number
 * @param[in] eVspaMboxIndex VSPA mailbox index to be recieved and processed.
 * @param[in] mbox VSPA mailbox to recieve the data
 *
 * @return AviStatusCodes_t Returns AVI status Codes
 */
AviStatusCodes_t exGeulAviHostRecvMboxFromVspa( VspaCore_t eVspaCore, VspaMboxIndex_t eVspaMboxIndex, AviMboxData_t *mbox );

/**
 * @brief Initializes the AVI for use
 * @param[in] xHandle Task handler of the task invoking pxGeulAviInit API, it is required to create spinlock
 *
 * @return SUCCESS initializes AVI, FAILURE returns NULL
 */
AviHandle_t *pxGeulAviInit( TaskHandle_t xHandle );

/**
 * @brief Returns AVI handler pointer
 *
 * @return Returns the pointer to AVI handler; if initialization is not done, returns NULL
 */
AviHandle_t *pxGeulAviGetHandle( void );
/**
 * @brief To check VSPA core is booted
 * @param[in] eVspaCore VSPA core id
 *
 * @return If VSPA is booted successfully, returns SW version else returns 0
 */
uint32_t uiCheckVSPABoot( VspaCore_t eVspaCore );

/**@*/
#endif
