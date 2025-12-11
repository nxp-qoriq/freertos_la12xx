/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2024 NXP
 *
 */
#include "FreeRTOS.h"
#include <types.h>
#include <bit.h>
#include <task.h>
#include "tc_dspi.h"
#include <fsl_dspi.h>

#define MAX_TX_FRAME    20
#define MAX_DATA_ARRAY_SIZE  35
void vGeulDspiTest(void)
{
	uint16_t data_frame[MAX_DATA_ARRAY_SIZE] =
						{
							0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x8888,
							0x9999,0xAAAA,0xBBBB,0xCCCC,0xDDDD,0xEEEE,0xFFFF,0x1111,
							0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x8888,0x9999,
							0xAAAA,0xBBBB,0xCCCC,0xDDDD,0xEEEE,0xFFFF,0x1111,0x2222,
							0x3333,0x4444,0x5555
						};
	/*Close DSPI handle having old configuartion if any before reloading new configuration*/
	vDspiExit(DSPI_BLOCK1);
	log_info("\r\n====>Starting DSPI Stream Test===> :\r\n");
	dspi_write_stream(DSPI_BLOCK1, DSPI_CS0, data_frame, MAX_TX_FRAME);
}
