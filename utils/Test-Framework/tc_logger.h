// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "mem_log.h"
#include "test_framework.h"

#define BUF_MAX		0x100

#ifndef GEUL_BOOT_MODE_PCI
void vLaunchPEBMLogger( void );
#endif
