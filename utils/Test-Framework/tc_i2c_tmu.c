// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#include "FreeRTOS.h"
#include "immap.h"
#include "i2cAPI.h"
#include "platform_def.h"
#include "test_framework.h"
#include "ina220_api.h"
#include "tmu_i2c.h"
#include "Time.h"

#if GEUL_DEMO_I2C_TEST

    void vI2CtmuReadTest( void )
    {

        u32 current_core = ulMpicCurrentCore();
#if MTD_I2C_DIODE_TEMP
    	 int16_t temp;
        /* Read values from TMU sensor */
        temp = sa56xxxx_get_temp_c();
		log_info( "\nTemperature = %d deg C \n\r",temp);

        if( temp < 0 )
        {
            log_err( "I2C : Temperature Read  failed %d\n\r", temp );
            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            return;
        }
#else
	log_err( "I2C : Diode Temp sensor not available/disabled!\n\r");
#endif
#if MTD_I2C_PWR_INFO
		int16_t current, power, bus_volt, shunt_volt;
		if( ina220_init() == false )
        {
            log_err( "I2C : INA220 initialization failed \n\r");
            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            return;
        }


		current = ina220_get_current_ma();

		if( current < 0 )
        {
            log_err( "I2C : INA220 Current read failed %d\n\r", current );
            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            return;
        }

		log_info( "Current = [%d.%dA] \n\r", (current/1000), (current % 1000));

		shunt_volt = ina220_get_shunt_voltage_uv();

		if( shunt_volt < 0 )
        {
            log_err( "I2C : INA220 shunt_volt read failed %d\n\r", shunt_volt );
            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            return;
        }
		log_info( "Shunt Voltage = [%d.%dmV] \n\r", (shunt_volt / 1000), (shunt_volt % 1000));


		bus_volt = ina220_get_bus_voltage_mv();

		if( bus_volt < 0 )
        {
            log_err( "I2C : INA220 bus_volt read failed %d\n\r", bus_volt );
            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            return;
        }

		log_info( "Bus Voltage = [%d.%dV] \n\r", (bus_volt / 1000), (bus_volt % 1000));

		power = ina220_get_power_mw();

		if( power < 0 )
        {
            log_err( "I2C : INA220 power read failed %d\n\r", power );
            RESET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
            return;
        }

		log_info( "Power = [%d.%dW] \n\r", (power / 1000), (power % 1000));
#else
		log_err( "I2C : Power Sensor not available/disabled !\n\r");
#endif
        SET_TEST_STATUS( current_core, GEUL_DEMO_I2C_TEST_STATUS );
    }
#endif /* GEUL_DEMO_I2C_TEST */
