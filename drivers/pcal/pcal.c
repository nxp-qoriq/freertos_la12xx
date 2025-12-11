// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2023 NXP
 */

#include "types.h"
#include "immap.h"
#include "common.h"
#include "pcal.h"
#include "FreeRTOS.h"
#include "i2cAPI.h"
#include "platform_def.h"
#include "Time.h"
#include "gul_host_if.h"

int select_i2c_channel( uint8_t channel_num )
{
	uint8_t channel_info = ENABLE_I2C_MUXER;
	int ret;

	/* Enable MUX INA220*/
	ret = iI2C_Write( I2C1_BASE_ADDR, PCA9547PWMUX_BASE_ADDRESS, 0x00,
				I2C_DEV_OFFSET_LEN_1_BYTE, &channel_info, 1 );
	if (ret < 0) {
		log_err( "\nMUX Enable: i2c_write failed for MUX enable, ret %d\n\r", ret );
		return false;
	}

	/* Enable Channel 2 for INA220*/
	channel_num = (channel_num)|(ENABLE_I2C_CHANNEL_BIT);
	ret = iI2C_Write( I2C1_BASE_ADDR, PCA9547PWMUX_BASE_ADDRESS, 0x00,
			I2C_DEV_OFFSET_LEN_1_BYTE, &channel_num, 1 );
	if (ret < 0) {
		log_err( "\nMUX Enable: i2c_write failed for slave enable %d\n\r", ret );
		return false;
	}
	log_dbg( "\nMUX Enable: i2c_write:: No of bytes = %d\n\r", ret );

	return 0;
}
