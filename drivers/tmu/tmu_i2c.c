// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#include "FreeRTOS.h"
#include "i2cAPI.h"
#include "Time.h"
#include "tmu_i2c.h"
#include "pcal.h"

int16_t sa56xxxx_get_temp_c( void )
{
    int ret;
	uint8_t tempVal, hi, lo, retry = 0;
	int16_t temp;

	ret = select_i2c_channel( TMU_CHANNEL_EN );
	if (ret < 0) {
        log_err( "\nTMU Channel %d Selection failed %d\n\r", TMU_CHANNEL_EN, ret );
        return TMU_I2C_FAILED;
    }

    // Configure a single shot reading
	tempVal = 0xD5;
	ret = iI2C_Write( I2C1_BASE_ADDR, TMU_BASE_ADDRESS, REG_CONW,
                     I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1 );
    if (ret < 0) {
        log_err( "\nTMU REG_CONW: i2c_write failed %d\n\r", ret );
        return TMU_I2C_FAILED;
    }

    // Set conversion rate to 1Hz
	tempVal = 0x04;
    ret = iI2C_Write( I2C1_BASE_ADDR, TMU_BASE_ADDRESS, REG_CRW,
                     I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1 );
    if (ret < 0) {
        log_err( "\nTMU REG_CRW: i2c_write failed %d\n\r", ret );
        return TMU_I2C_FAILED;
    }

    // Ask the sensor to perform a one-shot reading
	tempVal = 0x00;
    ret = iI2C_Write( I2C1_BASE_ADDR, TMU_BASE_ADDRESS, REG_SHOT,
                     I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1 );
    if (ret < 0) {
        log_err( "\nTMU REG_SHOT: i2c_write failed %d\n\r", ret );
        return TMU_I2C_FAILED;
    }

	// Wait for the sensor to finish the reading
	while ( retry++ < MAX_RETRY_ATTEMPT)
	{
		ret = iI2C_Read( I2C1_BASE_ADDR, TMU_BASE_ADDRESS, REG_SR,
						I2C_DEV_OFFSET_LEN_1_BYTE, &tempVal, 1 );
		if (ret < 0) {
			log_err("\nTMU REG_SR: i2c_read failed %d\n\r", ret);
			return TMU_I2C_FAILED;
		} else {
			if ((tempVal & 0x80) != 0x80)
				break;
		}
		vUdelay(MAX_I2C_TEMP_DELAY);
	}
	
	// Get 11-bit signed temperature value in 0.125C steps
	ret = iI2C_Read( I2C1_BASE_ADDR, TMU_BASE_ADDRESS, REG_LTHB,
					I2C_DEV_OFFSET_LEN_1_BYTE, &hi, 1 );
	if (ret < 0) {
		log_err("\nTMU REG_LTHB: i2c_read failed %d\n\r", ret);
        return TMU_I2C_FAILED;
	}

	ret = iI2C_Read( I2C1_BASE_ADDR, TMU_BASE_ADDRESS, REG_LTLB,
					I2C_DEV_OFFSET_LEN_1_BYTE, &lo, 1 );
	if (ret < 0) {
		log_err("\nTMU REG_LTLB: i2c_read failed %d\n\r", ret);
		return TMU_I2C_FAILED;
	}

	log_dbg( "After one shot training, the Hi Byte register value"
					"[0x%X] read from 0x%X offset of 0x%X device\n\r", hi, REG_LTHB, TMU_BASE_ADDRESS);
	log_dbg( "After one shot training, the lo Byte register value"
					"[0x%X] read from 0x%X offset of 0x%X device\n\r", lo, REG_LTLB, TMU_BASE_ADDRESS);

	temp = (hi << 3) | (lo >> 5);
	// Convert from a 11-bit integer to a Celsius temperature

	if (temp & 0x400) {
		// Negative two's complement value
		temp = -((~temp & 0x7FF) + 1);
	}

	// Positive value
	log_dbg("TEMP == [%d.%d] deg C  read from 0x%X address\n\r", temp/8, temp%8, TMU_BASE_ADDRESS);

	temp = temp * 0.125;
	return temp;
}
