// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef I2CAPI_H
#define I2CAPI_H

/**
 * @file        i2cAPI.h
 * @brief       This file contains the I2C API for e200 core
 *              to read/write data from/to a particular I2C slave device
 * @addtogroup  I2C_API
 * @{
 */

/* I2C Error Codes */
#define I2C_TIMEOUT                  1
#define I2C_RESTART                  2
#define I2C_NODEV                    3
#define I2C_NOT_IDLE                 4
#define I2C_NOT_BUSY                 5
#define I2C_INVALID_OFFSET           6
#define I2C_NO_WAKEUP_INIT           7
#define I2C_NO_WAKEUP_READ           8
#define I2C_NOACK                    9
#define I2C_READ_TIMEOUT             10
#define I2C_SLAVE_ADDR_TIMEOUT       11
#define I2C_MEM_ADDR_TIMEOUT         12

/**
 * MACRO to set device offset 0
 */
#define I2C_DEV_OFFSET_LEN_0_BYTE   0
/**
 * MACRO to set device offset 1
 */
#define I2C_DEV_OFFSET_LEN_1_BYTE   1
/**
 * MACRO to set device offset 2
 */
#define I2C_DEV_OFFSET_LEN_2_BYTE   2
/**
 * MACRO to set device offset 3
 */
#define I2C_DEV_OFFSET_LEN_3_BYTE   3
/**
 * MACRO to set device offset 4
 */
#define I2C_DEV_OFFSET_LEN_4_BYTE   4

/* XXX: Removed the I2C_Disable() which disables the I2C environment
 * for application.
 */

/**
 * @details	Initializes the I2C environment for application.
 *
 * @param[in]	ulI2C_Regs_P address of I2C module.
 * @param[in]	ulSys_Freq frequency at which system works. It is in Hz.
 * @param[in]	ulI2C_Bus_Freq frequency at which I2C Bus works. It is in Hz.
 *
 * @return
 *	- On Success, Returns 1
 *	- On Failure, Returns Error Code
 */

int iI2C_Init( uint32_t ulI2C_Regs_P,
               uint32_t ulSys_Freq,
               uint32_t ulI2C_Bus_Freq );


/**
 * @details	For reading the data from the specific device(for example, EEPROM) based
 * 		on the address received in parameter (ucDev_Addr) and copies data
 *		into the given buffer.
 *
 * @param[in]	ulI2C_Regs_P address of I2C Module.
 * @param[in]	ucDev_Addr address of the device from where data must be read
 *		(slave address)
 * @param[in]	ulDev_Offset offset in the device
 * @param[in]	ucDev_Offset_Len no. of bytes of offset length. It can be any
 *		value which is defined above in macros.
 *		Pass above macros name as a parameter
 * @param[in]	*pDst pointer to the destination where data must be copied
 * @param[in]	ulD_Len length of the data must be read
 *
 * @return
 *	- On Success, Returns No. of bytes read
 *	- On Failure, Returns Error Code
 */

int iI2C_Read( uint32_t ulI2C_Regs_P,
               uint8_t ucDev_Addr,
               uint32_t ulDev_Offset,
               uint8_t ucDev_Offset_Len,
               uint8_t *pDst,
               uint32_t ulD_Len );


/**
 * @details	For writing the data from specific location to a specific device
 *		(for example, EEPROM)
 *
 * @param[in]	ulI2C_Regs_P Address of I2C Module
 * @param[in]	ucDev_Addr Address of the device to where data must be written
 * @param[in]	ulDev_Offset Offset in the device
 * @param[in]	ucDev_Offset_Len No. of bytes of offset length. It can be any
 *		value which is defined above in macros.
 *		Pass above macros name as a parameter.
 * @param[in]	*psrc Pointer to the src from where data must be read
 * @param[in]	ulD_Len length of the data must be written
 *
 * @return
 *	- On Success, Returns No. of bytes written
 *	- On Failure, Returns Error Code
 */

int iI2C_Write( uint32_t ulI2C_Regs_P,
                uint8_t ucDev_Addr,
                uint32_t ulDev_Offset,
                uint8_t ucDev_Offset_Len,
                uint8_t *psrc,
                uint32_t ulD_Len );


/** @} */
#endif /* _I2C_H_ */
