// SPDX-License-Identifier: BSD-3-Clause
/**
 *  Copyright 2021 NXP
 */

/**
 * @Flash info
 *
 */
#ifndef __FLASH_INFO_H_
#define __FLASH_INFO_H_

#define SZ_16M_BYTES                    0x1000000U

#if defined(CONFIG_MT25QU512A)
#define F_SECTOR_256K 			0x40000U
#define F_SECTOR_64K 			0x10000U
#define F_PAGE_256 			0x100U
#define F_USE_4K_ERASE 			0x1U
#define F_FLASH_SIZE_BYTES   		0x4000000U
#define F_SECTOR_ERASE_SZ               F_SECTOR_64K
#elif defined(CONFIG_MT25QU256A)
#define F_SECTOR_256K 			0x40000U
#define F_SECTOR_64K 			0x10000U
#define F_PAGE_256 			0x100U
#define F_USE_4K_ERASE 			0x1U
#define F_FLASH_SIZE_BYTES   		0x2000000U
#define F_SECTOR_ERASE_SZ               F_SECTOR_64K
#endif

#endif /* __FLASH_INFO_H_ */
