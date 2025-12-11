// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#ifndef _GEUL_PCI_EP_HOST_DEF_H
#define _GEUL_PCI_EP_HOST_DEF_H

/*!
 * @file    nxp-pcie-ep-host-driver.h
 * @brief   This file contains PCIe EP host driver APIs and MACROS.
 * @defgroup   PCIE_API
 * @{
 */

#include <stdint.h>
#include "config.h"
#include "platform_def.h"
#include "types.h"
#include "bit.h"
#include "nxp-pcie.h"

/**
 * \enum ePciEpMemRegionLa12xx_t
 * @brief enum for EP memory regions
 */
typedef enum {
    PCIE_EP_REGION_CCSR, /**< EP Memory Region CCSR */
    PCIE_EP_REGION_DCSR, /**< EP Memory Region DCSR */
    PCIE_EP_REGION_PEBM, /**< EP Memory Region PEBM */
    PCIE_EP_REGION_MAX,  /**< EP Memory Region MAX */
}ePciEpMemRegionLa12xx_t;

/**
 * @struct xPciMemRegionInfo_t
 * @brief Structure to hold memory region info start and end address
 */
typedef struct {
    uint32_t ulAddrS; /**< Start Address */
    uint32_t ulAddrE; /**< End Address */
}xPciMemRegionInfo_t;

/**
 * @struct xPciEpDev_t
 * @brief Structure for holding EP device details
 */
typedef struct {
    eModemPciCtrlId_t eId;                                /**< Controller ID */
	xPciMemRegionInfo_t xMemRegionEP[PCIE_EP_REGION_MAX]; /**< Memory regions EP */
    xPciMemRegionInfo_t xMemRegionRC[PCIE_RC_REGION_MAX]; /**< Memory regions RC */
    void    * pvDevData;                                  /**< PCIe task handler */
}xPciEpDev_t;

extern xPciEpDev_t xPciEpDev __attribute__ ((section (".smem")));

#define PCIE_EP_REGION_CCSR_OFFSET    0x8000000
#define PCIE_EP_REGION_CCSR_SIZE      0x4000000

#define PCIE_EP_REGION_DCSR_OFFSET    0xc000000
#define PCIE_EP_REGION_DCSR_SIZE      0x100000

#define PCIE_EP_REGION_PEBM_OFFSET    0x200000
#define PCIE_EP_REGION_PEBM_SIZE      0x200000

#define GEUL_PCIE_RATU_VIEWPORT_E(n)          (n + 0x900) /**<  PCIe viewport */
#define GEUL_PCIE_RATU_CR1_E(n)               (n + 0x904) /**<  PCIe CR1 */
#define GEUL_PCIE_RATU_CR2_E(n)               (n + 0x908) /**<  PCIe CR2 */
#define GEUL_PCIE_RATU_LOWER_TARGET_E(n)      (n + 0x918) /**<  PCIe target lower */
#define GEUL_PCIE_RATU_UPPER_TARGET_E(n)      (n + 0x91C) /**<  PCIe target upper */
#define GEUL_PCIE_RATU_UPPER_LIMIT_ADDRESS(n) (n + 0x914) /**<  PCIe ATU limit register */
#define GEUL_PCIE_RATU_LOWER_BASE(n)          (n + 0x90C) /**<  PCIe base lower */
#define GEUL_PCIE_RATU_UPPER_BASE(n)          (n + 0x910) /**<  PCIe base upper */

#define GEUL_PCIE_RC_PEB_REGION_CPU_ADDRESS(n)   (uint32_t)PCIE_CPU_ADDR(n)
#define GEUL_PCIE_RC_HRAM_REGION_CPU_ADDRESS(n)  (uint32_t)GEUL_PCIE_RC_PEB_REGION_CPU_ADDRESS(n) \
												  + GEUL_PCIE_RC_REGION_PEB_SIZE
												 

/*!
 * \fn uint32_t ulPcieGetEpMemregion (eModemPciCtrlId_t ucId, ePciEpMemRegionLa12xx_t eRegion,
										uint32_t *pSaddr, uint32_t *pEaddr)
 * @brief Function to get memory regions of NXP's EP device
 *
 * @param[in]	ucId     PCIe controller ID
 * @param[in]	eRegion  Memory region
 * @param[in]	pSaddr   Pointer to Start address of region
 * @param[in]	pEaddr   Pointer to End address of region
 *
 * @return
 *   - Returns pdTRUE
 */
uint32_t
ulPcieGetEpMemregion (eModemPciCtrlId_t ucId, ePciEpMemRegionLa12xx_t eRegion, uint32_t *pSaddr,
						uint32_t *pEaddr);

/** @} */
#endif // _GEUL_PCI_EP_HOST_DEF_H
