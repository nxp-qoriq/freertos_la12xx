// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#ifndef __RF_CONFIG_H__
#define __RF_CONFIG_H__

#include "FreeRTOSConfig.h"

#define RF_DIRECT_EXECUTE_GPIO_COMMANDS     ( 1 )
#define RF_ENABLE_LATENCY_CALC              ( 1 )
#define RF_ENABLE_STATS_COLLECTION          ( 1 )
#define RF_CORE_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define RF_CORE_TASK_STACK_SIZE             ( configMINIMAL_STACK_SIZE * 2 )
#define RF_LOCAL_QUEUE_LENGTH               ( 1 )
#define RF_REMOTE_QUEUE_LENGTH              ( 1 )
#define RF_LOCAL_RESP_QUEUE_LENGTH          ( 1 )

#define RF_LOCAL_RESP_QUEUE_RECV_TIMEOUT    ( 1 )
#define RF_LOCAL_QUEUE_SEND_TIMEOUT         ( 0 )

#define RF_SW_CMD_DEFAULT_PRIORITY          ( 10 )

/* TIMEOUTS in microseconds */
#if RF_DEBUG
#define RF_SW_CMD_TIMEOUT                   ( 6144 )
#else
#define RF_SW_CMD_TIMEOUT                   ( 20000 )
#endif /* RF_DEBUG */
#endif /* __RF_REMOTE_H__ */
