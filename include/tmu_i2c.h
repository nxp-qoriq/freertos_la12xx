// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#ifndef _TMU_I2C_DEVICES_H_
#define _TMU_I2C_DEVICES_H_

#define MAX_RETRY_ATTEMPT 5
#define MAX_I2C_TEMP_DELAY 1000 /*In u-sec*/

#define ENABLE_I2C_MUXER 0xe0
#define ENABLE_I2C_CHANNEL_BIT (1<<3)

/*Registers related to SA56004ED Device*/
#define REG_LTLB  0x22
#define REG_LTHB  0x00
#define REG_SR    0x02
#define REG_CONW  0x09
#define REG_CRW   0x0A
#define REG_SHOT  0x0F
#define REG_AM    0xBF

#define TMU_I2C_FAILED MTD_TEMP_INVALID

#if MTD_I2C_DIODE_TEMP
int16_t sa56xxxx_get_temp_c( void );
#endif

#endif /* _PCAL__DEVICES_API_H_ */
