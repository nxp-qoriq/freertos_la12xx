// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __VCXO_H
#define __VCXO_H
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "fsl_dspi.h"

/*
 * Macro Definition
 */
/* LTC2621 is 12-bit DAC */
#define VCXO_MAX_VAL		( 0x0FFF )

/* Supported commands */
#define DAC_CMD_WRITE		( 0x0 )
#define DAC_CMD_UPDATE		( 0x1 )
#define DAC_CMD_WRITEUPDATE	( 0x3 )
#define DAC_CMD_POWERDOWN	( 0x4 )

/* DSPI BLOCK */
#define VCXO_DSPI_BLOCK		( DSPI_BLOCK6 )
#define VCXO_DSPI_CS_MASK	( 1 << DSPI_CS0 )

/*
 * Structure Definition
 */
/* VCXO device structure */
typedef struct VcxoDev
{
    struct LA12xxDspiInstance * xDspiHandle;
    uint16_t usCurVal;
}VcxoDevice_t;

/*
 * Function Declaration
 */
int32_t iVcxoInit( void );

#endif //__VCXO_H
