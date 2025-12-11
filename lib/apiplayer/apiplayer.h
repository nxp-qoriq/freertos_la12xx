// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#ifndef __APIPLAYER_H__
#define __APIPLAYER_H__

#include <platform_def.h>
#include "FreeRTOS.h"
#include <stdint.h>
#include "gul_api_player.h"

#define APIPLAYER_CORE_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )
#define APIPLAYER_CORE_TASK_STACK_SIZE                     ( configMINIMAL_STACK_SIZE * 2 )

typedef struct APIPlayerDev {
    uint32_t temp;
} APIPlayerDev_t;

typedef void (* xAPIGroupHandler)(apiplayer_api_t * api);

void xStartAPIPlayer(uint32_t ulAPIListAddr);
void xApiPlayerInit( void );

#endif /* __APIPLAYER_H__ */
