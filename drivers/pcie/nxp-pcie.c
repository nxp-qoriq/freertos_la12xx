// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#include "FreeRTOS.h"
#include "nxp-pcie.h"
#include "task.h"

xModemPciDev_t xModemPciDev[GEUL_PCIE_NUM_CTRL] __attribute__ ((section (".smem")));

/*!
 * \fn static uint32_t prvPcieInit (uint8_t eId, uint8_t ucMode, void *pvDevData)
 * @brief Function to initialize PCIe controller
 *
 * @param[in]	ucId        PCIe controller ID
 * @param[in]	ucMode      PCie controller mode
 * @param[in]	pvDevData   Represents PCIe task
 *
 * @return
 *   - 1     Controller init pass
 *   - 0     Controller init fail
 */
static uint32_t prvPcieInit (uint8_t eId, uint8_t ucMode, void *pvDevData)
{
	if (ucMode)
		return PcieSetupRC(eId, ucMode, pvDevData);
	else {
		return PcieSetupEP(eId, ucMode, pvDevData);
	}
}

/*!
 * \fn void PcieInterruptHandler (uint32_t Irq __attribute__((unused)), void *dev_data)
 * @brief PCIe interrupt handler
 *
 * @param[in]	Irq       Interrupt number
 * @param[in]	dev_data  Interrupt handler data
 *
 * @return
 *		Nothing
 */
void PcieInterruptHandler (uint32_t Irq __attribute__((unused)),
					  void *dev_data)
{
	uint32_t ulPmisValue;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xModemPciDev_t *pxModemPciDev = (xModemPciDev_t *)dev_data;

	ulPmisValue = in_le32((uint32_t *)GEUL_PCIE_PMIS(pxModemPciDev->eId));
	out_le32((uint32_t *)GEUL_PCIE_PMIS(pxModemPciDev->eId), ulPmisValue);
	pxModemPciDev->ulPmisValue = ulPmisValue;

	xTaskNotifyFromISR( pxModemPciDev->pvDevData, pxModemPciDev->eId, eSetValueWithOverwrite,
						&xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

	log_dbg ( "%s : PCIE%d (%s): Irq=0x%x \r\n", __func__, GEUL_PCIE_ID(pxModemPciDev->eId),
			pxModemPciDev->eHdrType ? "RC" : "EP", (Irq - INTERNAL_IRQ_OFFSET));
}

/*!
 * \fn int32_t iPcieCheckAndInitCtrl(eModemPciCtrlId_t ucId, void *pvDevData)
 * @brief Function to check and Initialize PCIe controller.
 *
 * @param[in]	ucId        PCIe controller ID
 * @param[in]	pvDevData   PCIe task handle
 *
 * @return
 *   - On Success, pdTRUE
 *   - On Failure, pdFALSE
 */
int32_t iPcieCheckAndInitCtrl (eModemPciCtrlId_t eId, void *pvDevData)
{
	BaseType_t xResult = pdFALSE;
	uint32_t ulPorSr2;
	uint8_t ucMode = 0;

	if (eId > PCIE_2) {
		log_err ("%s : Invalid PCIe controller (%d)  \n\r", __func__, GEUL_PCIE_ID(eId));
		return xResult;
	}

	if (eId == PCIE_1) {
		return prvPcieInit (eId, 0x0, pvDevData);
	} else {
		ulPorSr2 = in_le32((int32_t *)(CCSR_DCFG_BASE_ADDR + 0x4));

		if(((ulPorSr2 >> 19) & 0x7) == 1) {
			log_info ("%s : PCIE%d :  NOT ENABLED (ulPorSr2=0x%x) \n\r",
					__func__, GEUL_PCIE_ID(eId), ulPorSr2);
			return xResult;
		}

		ucMode = (uint8_t)((ulPorSr2 >> 9) & 0x1);
		xResult = prvPcieInit(eId, ucMode, pvDevData);

		if (xResult == pdTRUE) {
			out_le32((uint32_t *)GEUL_PCIE_PMIS(eId), in_le32((uint32_t *)GEUL_PCIE_PMIS(eId)));
			out_le32((uint32_t *)GEUL_PCIE_PMIE(eId), (uint32_t)GEUL_PCIE_PMI_ENABLE);

			if( !( lRegisterIrq( GEUL_PCIE_IRQ(eId) + INTERNAL_IRQ_OFFSET,
							( bIsrFunc ) PcieInterruptHandler,
							( void * ) &xModemPciDev[eId] ) == 1 ) )
            {
                log_err ( "%s : PCIE%d : Failed register error irq \r\n", __func__,
						  GEUL_PCIE_ID(eId));
                return pdFAIL;
            }
			bMpicEnable(DEVICE_INTERNAL, GEUL_PCIE_IRQ(eId));
		}
	}

	if (ucMode == 1)
		PcieInvokeHostDriver (eId);

	return xResult;
}
