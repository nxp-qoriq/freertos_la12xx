// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __AGAVE_CLI_H
#define __AGAVE_CLI_H
#include "FreeRTOS_CLI.h"

/*
 * Macro Declaration
 */
#define AGV_SYNTH_MIN_REG_ADDR  0x0
#define AGV_SYNTH_MAX_REG_ADDR  0x40
#define AGV_DEMOD_MIN_REG_ADDR  0x0
#define AGV_DEMOD_MAX_REG_ADDR  0x17

/*
 * Function Declaration
 */
BaseType_t vRegisterRFCli( void );
#endif //__AGAVE_CLI_H

