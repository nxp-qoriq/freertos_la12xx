// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2022 NXP
 */

#ifndef I2C_H
#define I2C_H

#include <common.h>

#define IS_CURRENT_MASTER             1
#define ISNOT_CURRENT_MASTER          0

/* Maximum FDR Value allowed is 0xBF (191)
 * 8 bit Value with b11xxxxxxxx as reserved
 */
#define I2C_MAX_FDR                   191

#define I2C_CR_MDIS                   ( 1 << 7 )
#define I2C_CR_MENA                   ( 0 << 7 )
#define I2C_CR_MSSL                   ( 1 << 5 )
#define I2C_CR_TXRX                   ( 1 << 4 )
#define I2C_CR_NOACK                  ( 1 << 3 )
#define I2C_CR_RSTA                   ( 1 << 2 )

#define I2C_RESERVED_ID               0x03

#define I2C_MAX_SRC_OFFSET_0_BYTES    0x00
#define I2C_MAX_READ_SIZE_0_BYTES     0x00
#define I2C_MAX_SRC_OFFSET_1_BYTES    0xFF
#define I2C_MAX_READ_SIZE_1_BYTES     0xFF
#define I2C_MAX_SRC_OFFSET_2_BYTES    0xFFFF
#define I2C_MAX_READ_SIZE_2_BYTES     0xFFFF
#define I2C_MAX_SRC_OFFSET_3_BYTES    0xFFFFFF
#define I2C_MAX_READ_SIZE_3_BYTES     0xFFFFFF
#define I2C_MAX_SRC_OFFSET_4_BYTES    0xFFFFFFFF
#define I2C_MAX_READ_SIZE_4_BYTES     0xFFFFFFFF

#define I2C_SR_TCF                    ( 1 << 7 )
#define I2C_SR_IBIF_CLEAR             ( 1 << 1 )
#define I2C_SR_IBIF                   ( 1 << 1 )
#define I2C_SR_IBB_IDLE               ( 0 << 5 )
#define I2C_SR_IBB_BUSY               ( 1 << 5 )
#define I2C_SR_IBAL                   ( 1 << 4 )
#define I2C_SR_NO_RXAK                ( 1 << 0 )
#define I2C_DBG_GLITCH_EN             ( 1 << 3 )
#define I2C_TIMEOUT_DELAY	          50
#define I2C_TIMEOUT_COUNT             20

typedef struct i2c_regs
{
    #ifdef CONFIG_I2C_REGS_32BIT
        uint32_t ucI2C_Ibad;
        uint32_t ucI2C_Ibfd;
        uint32_t ucI2C_Ibcr;
        uint32_t ucI2C_Ibsr;
        uint32_t ucI2C_Ibdr;
        uint32_t ucI2C_Ibic;
        uint32_t ucI2C_Ibdbg;
    #else
        uint8_t ucI2C_Ibad;
        uint8_t ucI2C_Ibfd;
        uint8_t ucI2C_Ibcr;
        uint8_t ucI2C_Ibsr;
        uint8_t ucI2C_Ibdr;
        uint8_t ucI2C_Ibic;
        uint8_t ucI2C_Ibdbg;
    #endif /* ifdef CONFIG_I2C_REGS_32BIT */
} i2c_regs_t;

#endif /* I2C_H_ */
