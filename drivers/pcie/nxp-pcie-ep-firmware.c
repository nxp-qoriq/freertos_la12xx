// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#include "FreeRTOS.h"
#include "nxp-pcie.h"
#include "task.h"

/*!
 * @file    nxp-pcie.h
 * @brief   This file contains APIs and code for PCIe EP-FW driver.
 * @defgroup   PCIE_API
 * @{
 */

/*!
 * \fn static void prvPciePopulateCtrlEP (uint8_t eId, void * pvDevData)
 * @brief Function to populate PCIe EP controller
 *
 * @param[in]   ucId        PCIe controller ID
 * @param[in]   pvDevData   Represents PCIe task
 *
 * @return
 *   - Nothing
 */
static void prvPciePopulateCtrlEP (uint8_t eId, void *pvDevData)
{
	xModemPciDev_t *pxModemPciDev = NULL;

	pxModemPciDev = (xModemPciDev_t *)&xModemPciDev[eId];
	memset(pxModemPciDev, 0, (sizeof(xModemPciDev_t)));

	pxModemPciDev->eId = eId;
	pxModemPciDev->eHdrType = 0;
	pxModemPciDev->ucEnable = 1;
	pxModemPciDev->pvDevData = pvDevData;
}

/*!
 * \fn static void prvSetupPcieInboundAtuEP (uint8_t ucId)
 * @brief Function to setup EP iATU
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
static void prvSetupPcieInboundAtuEP (uint8_t ucId)
{
	SetupPcieInboundAtu (ucId, 0x0, GEUL_PCIE_ATU_TYPE_MEM, 0x0,
							GEUL_PCIE_ATU_REGION_INBOUND, GEUL_PCIE_ATU_BAR0_TARGET);
	SetupPcieInboundAtu (ucId, 0x1, GEUL_PCIE_ATU_TYPE_MEM, 0x1,
							GEUL_PCIE_ATU_REGION_INBOUND, GEUL_PCIE_ATU_BAR1_TARGET);
	SetupPcieInboundAtu (ucId, 0x2, GEUL_PCIE_ATU_TYPE_MEM, 0x2,
							GEUL_PCIE_ATU_REGION_INBOUND, GEUL_PCIE_ATU_BAR2_TARGET);
}

/*!
 * \fn uint32_t PcieSetupEP (uint8_t ucId, uint8_t ucMode, void *pvDevData)
 * @brief Function to setup PCIe EP controller
 *
 * @param[in]	ucId        PCIe controller ID
 * @param[in]	ucMode      PCIe controler mode (RC/EP)
 * @param[in]	pvDevData   Represents PCIe task
 *
 * @return
 *   - pdTRUE      PcieSetupEP success
 *   - pdFALSE     PcieSetupEP failed
 */
uint32_t PcieSetupEP (uint8_t ucId, uint8_t ucMode, void *pvDevData)
{
	prvPciePopulateCtrlEP (ucId, pvDevData);
	prvSetupPcieInboundAtuEP (ucId);

	/* To differentiate multiple NXP EP devices and to make them work with same RC driver we are
	 * using config space to hold PCIe controller address.
	 * It is not required for other EP devices.
	 */
	out_le32((uint32_t *)GEUL_PCIE_RO_WR_EN(ucId), 0x1);
	out_le32((uint32_t *)GEUL_PCIE_SUBS(ucId), PCIE_CTRL_OFFSET(ucId));
	out_le32((uint32_t *)GEUL_PCIE_RO_WR_EN(ucId), 0x0);
	out_le32((uint32_t *)GEUL_PCIE_DCTRL(ucId), GEUL_PCIE_DCTRL_VAL);
	out_le16((uint32_t *)GEUL_PCIE_CMD(ucId), (uint16_t)GEUL_PCIE_CMD_EP);

	PcieEnableLink(ucId, ucMode);
	vTaskDelay(1000);

	log_info ("%s : PCIE%d : (EP) Init Completed \n\r", __func__,GEUL_PCIE_ID(ucId));
	return pdTRUE;
}
/** @} */
