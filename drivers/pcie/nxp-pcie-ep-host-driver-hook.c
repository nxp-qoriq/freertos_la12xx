// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023-2024 NXP
 */

#include "FreeRTOS.h"
#include "nxp-pcie.h"
#include "nxp-pcie-ids.h"

/*!
 * @file    nxp-pcie.h
 * @brief   This file provide API to hook specific PCIe EP host drivers.
 * @defgroup   PCIE_API
 * @{
 */

/*!
 * \fn void  PcieInvokeHostDriver (uint8_t ucId)
 * @brief Function to invoke PCIe EP driver
 *
 * @param[in]	ucId    PCIe controller ID
 *
 * @return
 *   - Returns Nothing
 */
void PcieInvokeHostDriver (uint8_t ucId)
{
	uint32_t ulBusDev;
	uint32_t ulVidDid;

	ulBusDev = GEUL_PCIE_ATU_BUS(1) | GEUL_PCIE_ATU_DEV(0) | GEUL_PCIE_ATU_FUNC(0);
	ulVidDid = ulPcieReadConfig (PCIE_2, ulBusDev, 0);

	log_dbg ("%s : PCIE%d ulVidDid=0x%x \n\r", __func__,GEUL_PCIE_ID(ucId), ulVidDid);

	/* Hook EP host driver registration here */
  switch ((uint16_t) ulVidDid) {
    case PCI_VENDOR_ID_NXP:
    case PCI_VENDOR_ID_LATTICE_FPGA:
      PcieEpHostDriverNxp (ulVidDid);
      break;
    default : 
      log_err ("%s : PCIE%d : Matching driver not found \n\r", __func__,GEUL_PCIE_ID(ucId));
	}

}
/** @} */
