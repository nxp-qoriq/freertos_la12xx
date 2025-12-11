// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2023 NXP
 */

#ifndef _PCAL__DEVICES_API_H_
#define _PCAL__DEVICES_API_H_

#define SELECT_INA220_I2C_CHANNEL     (1<<1)

/* INA220 related Registers*/
#define INA220_BASE_ADDRESS 0x40
#define INA220_CONFIG                   0x00
#define INA220_SHUNT_VOLTAGE    0x01 /*  readonly */
#define INA220_BUS_VOLTAGE              0x02 /*  readonly */
#define INA220_POWER                    0x03 /*  readonly */
#define INA220_CURRENT                  0x04 /*  readonly */
#define INA220_CALIBRATION              0x05

#define INA220_CONFIG_DEFAULT           0x399F
#define INA220_MAX_DELAY                        69 /*  worst case delay in ms */


#define INA220_RSHUNT_DEFAULT_mohm              1
#define INA220_CALIBRATION_DEFAULT              4096
#define INA220_SHUNT_V_DIV_FACTOR               100 /* For default config the shunt divisbility fact
or is 100 */

#define INA220_CURRENT_LSB_mA                   1
#define INA220_POWER_LSB_MW                     20*INA220_CURRENT_LSB_mA
#define INA220_BUS_V_LSB_MV                             4
#define INA220_BUS_V_RSHIFT                             3
#define MAX_16_INT                                              (1 << 16) - 1


#if MTD_I2C_PWR_INFO
int8_t get_sensor_info( union mtdpowerInfo * );
uint16_t ina220_get_calibration( void );
uint16_t ina220_get_current_ma( void );
uint16_t ina220_get_shunt_voltage_uv( void );
uint16_t ina220_get_bus_voltage_mv( void );
uint16_t ina220_get_power_mw( void );
bool ina220_init( void );
#endif
#endif /* _PCAL__DEVICES_API_H_ */
