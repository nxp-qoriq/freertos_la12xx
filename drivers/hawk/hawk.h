// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef _HAWK_H_
#define _HAWK_H_

#include "gul_host_if.h"

#define HAWK_MAX_EVENTS			(4)
#define HAWK_EVENT_MAX_STRING_LEN	(64)

extern uint8_t crt_core_id;

typedef struct HawkDevice
{
    volatile struct hawk_common_mdata * pxHAWKMData;
    volatile struct hawk_priv_mdata * pxPrvHAWKMData;
    TaskHandle_t xHawkCoreTask;
    QueueHandle_t xHawkRemoteQueue[ E200_CORE_COUNT ];
} HawkDevice_t;

typedef volatile HawkDevice_t* HawkHandle_t;

void xHawkInit( void );
void vRegisterHAWKCommands(void);

#endif /* ifndef _HAWK_H_ */
