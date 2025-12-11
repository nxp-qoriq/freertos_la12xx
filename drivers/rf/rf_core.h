// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __RF_CORE_H__
#define __RF_CORE_H__

#include "rf_config.h"

void vRFCoreTask( void * pvParameters );

#if RF_ENABLE_STATS_COLLECTION
    #define RF_STATS_ADD( var )               ( var += 1 )
    #define RF_STATS_SET( var )               ( var = 1 )
    #define RF_STATS_SET_VALUE( var, val )    ( var = val )
#else
    #define RF_STATS_ADD( var )
    #define RF_STATS_SET( var )
    #define RF_STATS_SET_VALUE( var, val )
#endif

#define RF_HOST_CORE_ID              ( 6 ) /* 0 to 5 are for e200 cores */

/* Below values are used as array index, So don't change */
#define RF_REMOTE_CORE0_EINDEX       ( 0 )
#define RF_REMOTE_CORE1_EINDEX       ( 1 )
#define RF_REMOTE_CORE2_EINDEX       ( 2 )
#define RF_REMOTE_CORE3_EINDEX       ( 3 )
#define RF_REMOTE_CORE4_EINDEX       ( 4 )
#define RF_REMOTE_CORE5_EINDEX       ( 5 )
#define RF_REMOTE_HOST_CMD_EINDEX    ( RF_HOST_CORE_ID )

/* Events */
#define RF_REMOTE_HOST_CMD_EVENT     ( 1 << RF_REMOTE_HOST_CMD_EINDEX )
#define RF_REMOTE_CORE0_EVENT        ( 1 << RF_REMOTE_CORE0_EINDEX )
#define RF_REMOTE_CORE1_EVENT        ( 1 << RF_REMOTE_CORE1_EINDEX )
#define RF_REMOTE_CORE2_EVENT        ( 1 << RF_REMOTE_CORE2_EINDEX )
#define RF_REMOTE_CORE3_EVENT        ( 1 << RF_REMOTE_CORE3_EINDEX )
#define RF_REMOTE_CORE4_EVENT        ( 1 << RF_REMOTE_CORE4_EINDEX )
#define RF_REMOTE_CORE5_EVENT        ( 1 << RF_REMOTE_CORE5_EINDEX )
#define RF_LOCAL_CMD_EVENT           ( 1 << 7 )

#define RF_CORE_TASK_EVENT_MASK  \
    ( RF_LOCAL_CMD_EVENT |       \
      RF_REMOTE_HOST_CMD_EVENT | \
      RF_REMOTE_CORE0_EVENT |    \
      RF_REMOTE_CORE1_EVENT |    \
      RF_REMOTE_CORE2_EVENT |    \
      RF_REMOTE_CORE3_EVENT |    \
      RF_REMOTE_CORE4_EVENT |    \
      RF_REMOTE_CORE5_EVENT )

#define RF_CORE_REMOTE_EVENT_MASK \
    ( RF_REMOTE_HOST_CMD_EVENT |  \
      RF_REMOTE_CORE0_EVENT |     \
      RF_REMOTE_CORE1_EVENT |     \
      RF_REMOTE_CORE2_EVENT |     \
      RF_REMOTE_CORE3_EVENT |     \
      RF_REMOTE_CORE4_EVENT |     \
      RF_REMOTE_CORE5_EVENT )

/* Message to BG Task */
#define TRIGGER_SQ_PROGRAM         ( 1 )
#define PPF_RECAL_FAILED           ( 2 )
#define PPF_RECAL_SUCCESS          ( 3 )
#define CONTINUE_SQ_READING_TX     ( 4 )
#define CONTINUE_SQ_READING_RX     ( 5 )
#define RF_CARD_ISR_BOTTOM_HALF    ( 6 )

#endif /* __RF_CORE_H__ */
