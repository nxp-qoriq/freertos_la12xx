// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2023 NXP
 */

#ifndef _PCAL_H_
#define _PCAL_H_

#define ENABLE_I2C_MUXER 0xe0
#define ENABLE_I2C_CHANNEL_BIT (1<<3)

int select_i2c_channel( uint8_t channel_num );

#endif
