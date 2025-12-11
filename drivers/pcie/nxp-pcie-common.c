// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#include "FreeRTOS.h"
#include "nxp-pcie.h"
#include "task.h"

/*!
 * @file    nxp-pcie.h
 * @brief   This file contains common code and APIs to be used by PCIe drivers
 * @defgroup   PCIE_API
 * @{
 */

extern xModemPciDev_t xModemPciDev[GEUL_PCIE_NUM_CTRL] __attribute__ ((section (".smem")));

/*!
 * \fn void SetupPcieInboundAtu (uint8_t ucId, uint8_t ucIndex, uint8_t ucType,
									uint8_t ucBarNum, uint32_t ucRegion,
									uint32_t ulTaddr)
 * @brief Function to create inbound window on NXP platforms
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ucIndex   Index number
 * @param[in]	ucType	  Type
 * @param[in]	ucBarNum  Bar Number
 * @param[in]	ucRegion  Region
 * @param[in]	ulTaddr   Target address
 *
 * @return
 *   - Returns Nothing
 */
void SetupPcieInboundAtu (uint8_t ucId,
							uint8_t ucIndex,
							uint8_t ucType,
							uint8_t ucBarNum,
							uint32_t ucRegion,
							uint32_t ulTaddr)
{
	out_le32((uint32_t *)GEUL_PCIE_ATU_VIEWPORT_E(ucId), ((uint32_t)ucRegion | ucIndex));
	out_le32((uint32_t *)GEUL_PCIE_ATU_LOWER_TARGET_E(ucId), ulTaddr);
	out_le32((uint32_t *)GEUL_PCIE_ATU_UPPER_TARGET_E(ucId), (uint32_t)0x0);
	out_le32((uint32_t *)GEUL_PCIE_ATU_CR1_E(ucId), ucType);
	out_le32((uint32_t *)GEUL_PCIE_ATU_CR2_E(ucId), (uint32_t)(GEUL_PCIE_ATU_ENABLE |
				GEUL_PCIE_ATU_BAR_MODE_ENABLE | GEUL_PCIE_ATU_BAR_NUM(ucBarNum)));
}

/*!
 * \fn void SetupPcieOutboundAtu (uint8_t ucId, uint8_t ucIndex, uint8_t ucType,
									uint32_t ucRegion, uint32_t ulSaddr,
									uint32_t ulTaddr, uint32_t ulSize)
 * @brief Function to create outbound window on NXP platforms
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ucIndex   Index number
 * @param[in]	ucType	  Type
 * @param[in]	ucRegion  Region
 * @param[in]	ulSaddr   Source address
 * @param[in]	ulTaddr   Target address
 * @param[in]	ulSize    Size
 *
 * @return
 *   - Returns Nothing
 */
void SetupPcieOutboundAtu (uint8_t ucId,
							uint8_t ucIndex,
							uint8_t ucType,
							uint32_t ucRegion,
							uint32_t ulSaddr,
							uint32_t ulTaddr,
							uint32_t ulSize)
{
	out_le32((uint32_t *)GEUL_PCIE_ATU_VIEWPORT_E(ucId), ((uint32_t)ucRegion | ucIndex));
	out_le32((uint32_t *)GEUL_PCIE_ATU_LOWER_BASE(ucId), ulSaddr);
	out_le32((uint32_t *)GEUL_PCIE_ATU_UPPER_BASE(ucId), (uint32_t)0x0);
	out_le32((uint32_t *)GEUL_PCIE_ATU_UPPER_LIMIT_ADDRESS(ucId), (ulSaddr + ulSize - 1));
	out_le32((uint32_t *)GEUL_PCIE_ATU_LOWER_TARGET_E(ucId), ulTaddr);
	out_le32((uint32_t *)GEUL_PCIE_ATU_UPPER_TARGET_E(ucId), (uint32_t)0x0);
	out_le32((uint32_t *)GEUL_PCIE_ATU_CR1_E(ucId), ucType);
	out_le32((uint32_t *)GEUL_PCIE_ATU_CR2_E(ucId), (uint32_t)(GEUL_PCIE_ATU_ENABLE));
}

/*!
 * \fn void PcieSetBusdev (uint8_t ucId, uint32_t ulBusDev)
 * @brief Function to set BDF
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ulBusDev  BDF
 *
 * @return
 *   - Returns Nothing
 */
void PcieSetBusdev (uint8_t ucId,
					uint32_t ulBusDev)
{
	out_le32((uint32_t *)GEUL_PCIE_ATU_VIEWPORT_E(ucId),
			((uint32_t)GEUL_PCIE_ATU_REGION_OUTBOUND |
			 (uint32_t)GEUL_PCIE_ATU_REGION_INDEX0));
	out_le32((uint32_t *)GEUL_PCIE_ATU_LOWER_TARGET_E(ucId), ulBusDev);
}

/*!
 * \fn bool_t bPcieIsLinkUp (uint8_t ucId)
 * @brief Function to check of link is up
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - 1     Link is up
 *   - 0     Link is not up
 */
bool_t bPcieIsLinkUp (uint8_t ucId)
{
	uint32_t retries = 0;

	log_info ("%s : PCIE%d : (RC) Waiting for link-up \n\r", __func__,GEUL_PCIE_ID(ucId));

	while (retries <= GEUL_PCIE_LINK_RETRY) {
		if ((in_le32((volatile uint32_t *) PEX_PF0_DBG(ucId)) & 0xff) == 0x11)
			return pdTRUE;
		retries++;
		vTaskDelay(200);
	}

	log_err ("%s : PCIE%d : (RC) Linkup-up failed \n\r", __func__,GEUL_PCIE_ID(ucId));
	return pdFALSE;
}

/*!
 * \fn void  PcieEnableLink (uint8_t id, uint8_t ucMode);
 * @brief Function to enable pcie link
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ucMode    PCIe controler mode (RC/EP)
 *
 * @return
 *   - Returns Nothing
 */
void PcieEnableLink (uint8_t id, uint8_t ucMode)
{
	uint32_t ulValue;

	ulValue = in_le32((uint32_t *)PEX_PF0_CONFIG(id)) | GEUL_PCIE_LTSSM_EN;
	if (!ucMode)
		ulValue |= GEUL_PCIE_CFG_READY;
	out_le32((uint32_t *)PEX_PF0_CONFIG(id), ulValue);
}

/*!
 * \fn void vGeulPCIeResetTask( void *pvParameters )
 * @brief Function to handle reset requests
 * @param[in]	pvParameters   Pointer to xModemPciDev_t structure
 *
 * @return
 *   - Returns Nothing
 */
void vGeulPCIeResetTask( void *pvParameters )
{
	xModemPciDev_t *pxModemPciDev = NULL;
	pxModemPciDev = (xModemPciDev_t *)pvParameters;

	if (pxModemPciDev->ucResetMode == PCIE_SRESET) {
		out_le32((uint32_t *)GEUL_PCIE_PEX_PF_DBG_BASE(pxModemPciDev->eId),
					GEUL_PCIE_SRESET_DASSERT);
		out_le32((uint32_t *)GEUL_PCIE_PEX_PF_DBG_BASE(pxModemPciDev->eId),
					GEUL_PCIE_SRESET_ASSERT);
		vTaskDelay(200);
		out_le32((uint32_t *)GEUL_PCIE_PEX_PF_DBG_BASE(pxModemPciDev->eId),
					GEUL_PCIE_SRESET_DASSERT);
	} else if (pxModemPciDev->ucResetMode == PCIE_HRESET) {
		/* Implementation dependent based on board connections */
	}

	vTaskDelete(NULL);
}

/*!
 * \fn void  PcieControllerReset (uint8_t ucId, uint8_t ucResetMode)
 * @brief Function to reset PCIe controller
 * @param[in]	ucId          PCIe controller ID
 * @param[in]	ucResetMode   Reset Mode (PCIE_SRESET/PCIE_HRESET)
 *
 * @return
 *   - Returns Nothing
 */
void PcieControllerReset (uint8_t ucId, uint8_t ucResetMode)
{
	int iRc;
	xModemPciDev_t *pxModemPciDev = NULL;

	pxModemPciDev = (xModemPciDev_t *)&xModemPciDev[ucId];
	pxModemPciDev->ucResetMode = ucResetMode;

	iRc = xTaskCreate (vGeulPCIeResetTask, "Geul PCIe Reset Task", PCIE_RESET_TASK_STACKSIZE,
						( void * ) pxModemPciDev, PCIE_RESET_TASK_PRIORITY, NULL);

	if (iRc != pdPASS )
	{
		log_err("Failed to create PCIe task\n\r");
	}
}

/*!
 * \fn void  PcieHandleInterrupt (uint8_t ucId)
 * @brief Function to handle PCIe interrupt
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Returns Nothing
 */
void PcieHandleInterrupt (uint8_t ucId)
{
	xModemPciDev_t *pxModemPciDev = NULL;

	pxModemPciDev = (xModemPciDev_t *)&xModemPciDev[ucId];

	log_dbg ("%s : PCIE%d : Handle Interrupt (0x%x) \n\r", __func__,GEUL_PCIE_ID(ucId),
			pxModemPciDev->ulPmisValue);

	if ((pxModemPciDev->ulPmisValue & GEUL_PCIE_PMI_LUDIE) == GEUL_PCIE_PMI_LUDIE)
		iPcieCheckAndInitCtrl (pxModemPciDev->eId, pxModemPciDev->pvDevData);

}

/*!
 * \fn void  PcieGetMode (uint8_t id);
 * @brief Function to get mode (EP/RC) of pcie controller
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - For RC returns 1
 *   - For EP returns 0
 */
uint32_t PcieGetMode (uint8_t ucId)
{
	return xModemPciDev[ucId].eHdrType;
}
/** @} */
