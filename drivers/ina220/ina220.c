// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2023 NXP
 */

#include "types.h"
#include "immap.h"
#include "common.h"
#include "pcal.h"
#include "ina220_api.h"
#include "FreeRTOS.h"
#include "i2cAPI.h"
#include "platform_def.h"
#include "Time.h"
#include "gul_host_if.h"

uint16_t ina220_get_calibration(void) {
	uint16_t cal_value;
	int ret = iI2C_Read( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_CALIBRATION,
                         I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&cal_value, 2 );

	if (ret < 0) {
		log_err("\n Error %d Calibration Register 0x%X Read \n\r", ret, INA220_CALIBRATION);
		return ret;
	}

	log_dbg("\nCalibration Value = %d\n\r",cal_value);

	return cal_value;

}
uint16_t ina220_get_current_ma(void) {
	uint16_t current;
	int ret = iI2C_Read( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_CURRENT,
                 I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&current, 2 );
	if( ret < 0 )
	{
		log_info( "\nPM_READ_CURRENT: i2c_read failed 0x%X address\n\r", INA220_CURRENT );
		return ret;
	}

	log_dbg( "\ncurrent == 0x%X read from 0x%X offset of 0x%X device   \n\r",
			current, INA220_CURRENT, INA220_BASE_ADDRESS);

	if(current & 0x8000) {
		current = ~current + 1;
	}

	current = current * INA220_CURRENT_LSB_mA;

	log_dbg( "\nCurrent == [%d.%dA]\n\r",
			(current/1000),(current%1000));
	return current;

}
uint16_t ina220_get_shunt_voltage_uv(void) {
	uint16_t shunt_v;
	int ret = iI2C_Read( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_SHUNT_VOLTAGE,
                         I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&shunt_v, 2 );

	if( ret < 0 )
	{
		log_info( "\nPM_READ_SHUNT_REG: i2c_read failed 0x%X address\n\r", INA220_SHUNT_VOLTAGE );
		return -1;
	}

	log_dbg( "\nshunt_v == 0x%X read from 0x%X offset of 0x%X device   \n\r",
			shunt_v, INA220_SHUNT_VOLTAGE, INA220_BASE_ADDRESS);

	if(shunt_v & 0x8000) {
		shunt_v = ~shunt_v + 1;
	}

	log_dbg( "\nShunt Voltage == [%d.%dmV]\n\r",
			(shunt_v/INA220_SHUNT_V_DIV_FACTOR),(shunt_v%INA220_SHUNT_V_DIV_FACTOR));
	return shunt_v * 10;
}

uint16_t ina220_get_bus_voltage_mv(void) {
	uint16_t  bus_v;
	int ret = iI2C_Read( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_BUS_VOLTAGE,
                 I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&bus_v, 2 );

	if(ret < 0) {
		log_err("ERROR %d READ BUS VOLTAGE REGISTER 0x%X\n\r", ret, INA220_BUS_VOLTAGE);
		return -1;
	}

	log_dbg( "\nBus voltage register ==  0x%X read from 0x%X offset of 0x%X device\n\r",
					bus_v, INA220_BUS_VOLTAGE, INA220_BASE_ADDRESS);

	bus_v = bus_v >> INA220_BUS_V_RSHIFT;
	bus_v = bus_v * INA220_BUS_V_LSB_MV;

	log_dbg( "\nBus voltage  ==  [%d.%dV]\n\r", (bus_v/1000),(bus_v%1000));
	return bus_v;
}

uint16_t ina220_get_power_mw(void) {
	uint16_t power;
	int ret = iI2C_Read( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_POWER,
                 I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&power, 2 );

	if( ret < 0 )
	{
		log_info( "\nPM_READ_Power: i2c_read failed 0x%X address\n\r", INA220_POWER );
		return false;
	}

	log_dbg( "\nPower == 0x%X read from 0x%X offset of 0x%X device   \n\r",
			power, INA220_POWER, INA220_BASE_ADDRESS);

	power = power * INA220_POWER_LSB_MW;
	log_dbg( "\nPower == [%d.%dW]  \n\r", (power/1000),(power%1000));
	return power;
}

static bool ina220_calibrate ( void )
{
	uint16_t calibration_reg = ( 40960.0 / ((double)INA220_RSHUNT_DEFAULT_mohm * (double)INA220_CURRENT_LSB_mA));

	if (calibration_reg == MAX_16_INT) {
		log_err("Overflow occurs. Calibration register value exceeds 16 bits.\n\r");
		log_err("Please Carefully choose Macros INA220_RSHUNT_DEFAULT_mohm and INA220_CURRENT_LSB_mA \r\n");
		return false;
	}
	log_dbg("Writing Calibration Register == 0x%X \n\r", calibration_reg);
	log_dbg("CALIBRATION : curr_lsb_mA %d , power_lsb_mW = %d, cal_reg == %d \n\r",(int)INA220_CURRENT_LSB_mA, INA220_POWER_LSB_MW, calibration_reg);
	int ret = iI2C_Write( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_CALIBRATION,
                         I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&calibration_reg, 2 );
	if (ret < 0) {
		log_err("\nCANNOT WRITE CALIBRATION_FAILED, ret = %d\n\r", ret);
		return false;
	}
	return true;

}

bool ina220_init(void)
{
	uint16_t def_config = INA220_CONFIG_DEFAULT;
	int ret = 0;

	ret = select_i2c_channel(SELECT_INA220_I2C_CHANNEL);
	if (ret < 0) {
		log_err("\nChannel %d Selection failed %d\n\r",
				SELECT_INA220_I2C_CHANNEL, ret);
		return false;
	}

	ret = iI2C_Write( I2C1_BASE_ADDR, INA220_BASE_ADDRESS, INA220_CONFIG,
                         I2C_DEV_OFFSET_LEN_1_BYTE, (void *)&def_config, 2 );
	if (ret < 0){
		log_err("\nINA220_CONFIG_WRITE ERROR , ret = %d \n\r", ret);
		return false;
	}

	if ( ina220_calibrate() && ina220_get_calibration() == 0) {
			log_err( "I2C : INA220 Get calibration failed\n\r");
			log_info("Retrying...\n\r");
			vUdelay(INA220_MAX_DELAY);
			return ina220_calibrate();
	}
	return true;

}

int8_t get_sensor_info(union mtdpowerInfo *pmtd_power_info)
{

	if (ina220_init() == false) {
		log_err( "I2C : INA220 initialization failed \n\r" );
		pmtd_power_info->power_info_fh = 0;
		pmtd_power_info->power_info_sh = 0;
		return -1;
	}

	pmtd_power_info->current_val = ina220_get_current_ma();
	if (pmtd_power_info->current_val < 0) {
		log_err( "I2C : INA220 Current read failed %d\n\r",
				pmtd_power_info->current_val );
		pmtd_power_info->current_val = 0;
	}

	pmtd_power_info->shunt_volt = ina220_get_shunt_voltage_uv();
	if (pmtd_power_info->shunt_volt < 0) {
		log_err( "I2C : INA220 shunt_volt read failed %d\n\r",
				pmtd_power_info->shunt_volt );
		pmtd_power_info->shunt_volt = 0;
	}

	pmtd_power_info->bus_volt = ina220_get_bus_voltage_mv();
	if (pmtd_power_info->bus_volt < 0) {
		log_err( "I2C : INA220 bus_volt read failed %d\n\r",
				pmtd_power_info->bus_volt  );
		pmtd_power_info->bus_volt = 0;
	}

	pmtd_power_info->power_val = ina220_get_power_mw();
	if (pmtd_power_info->power_val < 0) {
		log_err( "I2C : INA220 power read failed %d\n\r",
				pmtd_power_info->power_val );
		pmtd_power_info->power_val = 0;
	}

	return 0;
}
