// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

/*!
 * @file	fspi.h
 * @brief 	This file contains the FlexSPI/FSPI API for e200 core
 * 		to communicate to attached Slave device.
 * @addtogroup	FSPI_API
 * @{
 */

#ifndef __FSPI_API_H_
#define __FSPI_API_H_

#include "types.h"

/*!
 * @details AHB read/IP Read, decision to be internal to API
 * Minimum Read size = 4Byte
 * @param[in] src_off source offset from where data to read from flash,
 * needs to be word aligned
 * @param[out] des Destination location where data needs to be copied
 * @param[in] len length in Bytes,where 1-word=4-bytes/32-bits
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiRead(u32 src_off, u32 *des, u32 len);
/*!
 * @details Sector erase, Minimum size 256KB(0x40000)/128KB(0x20000)
 * depending upon flash, Calls vFspiWren() internally
 * @param[out] erase_offset Destination erase location on flash which has
 * 	       to be erased, needs to be multiple of 0x40000/0x20000/0x10000
 * @param[in] erase_len length in bytes in Hex like 0x100000 for 1MB, minumum
 * erase size is 1 sector(0x40000/0x20000/0x10000)
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiSecErase(u32 erase_offset, u32 erase_len);
/*!
 * @details IP write, For writing data to flash, calls vFspiWren() internally.
 * Multiple page write cannot begin at offset 0x1, 0x2, 0x3, inother words
 * start address should be 4 aligned.
 * Single page write can start @any offset, but performance will be low
 * due to ERRATA
 * @param[out] dst_off Destination location on flash where data needs to be written
 * @param[in] src source offset from where data to be read
 * @param[in] len length in bytes,where 1-word=4-bytes/32-bits
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiIpWrite(u32 dst_off, u32 *src, u32 len);
/*!
 * @details vFspiInit, Init function.
 * @param[in] void
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiInit(void);
/*!
 * @details vFlashIsBusy, Check if any erase or write or lock is pending on flash/slave
 * @param[in] void
 *
 * @return TRUE/FLASE
*/
bool bFlashIsBusy (void);
/*!
 * @details Write enable, to be used by advance users only.
 * Step 1 for sending write commands to flash.
 * @param[in] dst_off destination offset where data will be written
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiWren(u32 dst_off);
/*!
 * @details AHB read, meaning direct memory mapped access to flash,
 * Minimum Read size = 4Byte
 * @param[in] src_off source offset from where data to read from flash,
 * needs to be word aligned
 * @param[out] des Destination location where data needs to be copied
 * @param[in] len length in Bytes,where 1-word=4-bytes/32-bits
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiAhbRead32(u32 src_off, u32 *des, u32 len);
/*!
 * @details IP read, READ via RX buffer from flash, minimum READ size = 1Byte
 * @param[in] src_off source offset from where data to be read from flash
 * @param[out] des Destination location where data needs to be copied
 * @param[in] len length in Bytes,where 1-word=4-bytes/32-bits
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiIpRead(u32 src_off, u32 *des, u32 len);
/*!
 * @details CHIP erase, Erase complete chip in one go
 *
 * @return FSPI_SUCCESS or error code
*/
int iFspiErase(void);

				 

#endif /* __FSPI_API_H_ */
