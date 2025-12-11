// SPDX-License-Identifier: BSD-3-Clause

/*
 * Copyright 2023-2024 NXP
 */

#include "FreeRTOS.h"
#include "nxp-pcie.h"
#ifdef LA12XX_DRIVER_PCI_LAT_FP
#include "lattice-pcie-ep-fpga.h"
#endif

/*!
 * @file    nxp-pcie.h
 * @brief   This file contains APIs and code for PCIe RC-FW driver.
 * @defgroup   PCIE_API
 * @{
 */

/*!
 * \fn static void prvPcieConfigBarsRC (uint8_t ucId)
 * @brief Function to configure RC Bars
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
static void prvPcieConfigBarsRC (uint8_t ucId)
{
	out_le32((uint32_t *)GEUL_PCIE_BAR0_BASE_ADDR(ucId), 0x0);
	out_le32((uint32_t *)GEUL_PCIE_BAR1_BASE_ADDR(ucId), 0x0);
	out_le32((uint32_t *)GEUL_PCIE_ROM_BASE_ADDR(ucId), 0xfffffffe);
}

/*!
 * \fn static void prvPcieSetupAtuRC (uint8_t ucId)
 * @brief Function to configure RC iATU
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
static void prvPcieSetupAtuRC (uint8_t ucId)
{
	SetupPcieOutboundAtu (ucId, 0x0, GEUL_PCIE_ATU_TYPE_CFG0,
							GEUL_PCIE_ATU_REGION_OUTBOUND,
							GEUL_PCIE_CFG_BASE(ucId),
							0,
							GEUL_PCIE_CFG_SIZE(ucId));
	SetupPcieOutboundAtu (ucId, 0x1, GEUL_PCIE_ATU_TYPE_MEM,
							GEUL_PCIE_ATU_REGION_OUTBOUND,
							PCIE_CPU_ADDR(ucId),
							PCIE_CPU_ADDR(ucId),
							GEUL_PCIE_CPU_ADDRESS_AVAI_SIZE(ucId));
}

/*!
 * \fn static void prvPcieErrataA008851 (uint8_t ucId)
 * @brief Function to implement RC Errata
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
static void prvPcieErrataA008851 (uint8_t ucId)
{
	out_le32((uint32_t *)(PCIE_BASE_ADDR(ucId) + 0x8bc), 0x00000001);
	out_le32((uint32_t *)(PCIE_BASE_ADDR(ucId) + 0x154), 0x47474747);
	out_le32((uint32_t *)(PCIE_BASE_ADDR(ucId) + 0x158), 0x47474747);
}

/*!
 * \fn bool_t ulPcieCtrlReady (eModemPciCtrlId_t ucId)
 * @brief Function to check if controller is ready
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
bool_t ulPcieCtrlReady (eModemPciCtrlId_t ucId)
{
	return xModemPciDev[ucId].ucEnable && bPcieIsLinkUp(ucId);
}

/*!
 * \fn static void prvPcieErrataRC (uint8_t ucId)
 * @brief Function to implement RC Errata
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
static void prvPcieErrataRC (uint8_t ucId)
{
	prvPcieErrataA008851(ucId);
}

#ifdef LA12XX_DRIVER_PCI_LAT_FP
static void prvPcieConfigLatFp(xModemPciDev_t *pxModemPciDev)
{
  uint32_t ulBusDev, data;
  uint32_t ulAddress;
  ulBusDev = GEUL_PCIE_ATU_BUS(1) | GEUL_PCIE_ATU_DEV(0) |
              GEUL_PCIE_ATU_FUNC(0);

  data = ulPcieReadConfig (pxModemPciDev->eId, ulBusDev, PCIE_CMD_REG_OFFSET);
  data |= (3<<1);
  vPcieWriteConfig (pxModemPciDev->eId, ulBusDev, PCIE_CMD_REG_OFFSET, data);

  PcieSetBusdev(pxModemPciDev->eId, ulBusDev);
  ulAddress = (uint32_t)xModemPciDev[pxModemPciDev->eId].ulPcieCfgRegion + PCIE_EP_LAT_DEV_CFG;
  out_le16((uint32_t *)ulAddress, PCIE_EP_DEV_CFG_VAL);

}
#endif
/*!
 * \fn static int32_t iPcieConfigBars (xModemPciDev_t *pxModemPciDev,
										uint32_t ulPos)
 * @brief Function to configure EP Bars
 *
 * @param[in]	pxModemPciDev    PCIe device structure
 * @param[in]	ulPos            BAR position  
 *
 * @return
 *   - Next bar number
 */
static int32_t iPcieConfigBars (xModemPciDev_t *pxModemPciDev, uint32_t ulPos)
{
	uint32_t l = 0;
	uint32_t sz = 0;
	uint32_t ulBarNum;
	uint32_t mask = 0xffffffff;
	uint32_t ulBusDev;

	ulBarNum = (ulPos - GEUL_PCIE_BAR0_OFFSET) >> 2;
	pxModemPciDev->xBarRegion[ulBarNum].ulValid = 0;
	ulBusDev = GEUL_PCIE_ATU_BUS(1) | GEUL_PCIE_ATU_DEV(0) |
				GEUL_PCIE_ATU_FUNC(0);
	vPcieWriteConfig (pxModemPciDev->eId, ulBusDev, ulPos, mask);
	sz = ulPcieReadConfig (pxModemPciDev->eId, ulBusDev, ulPos);

	/* Represents malformed EP device */
	if (sz == 0xffffffff) {
		sz = 0;
	}

	if (!sz)
		goto fail;

	/* Only Memory resources are supported */
	if ((sz & 0x1) == 0x0) {
		if ((sz & GEUL_PCIE_RESOURCE_MEM_64) == GEUL_PCIE_RESOURCE_MEM_64)
			pxModemPciDev->xBarRegion[ulBarNum].ulFlags |=
													GEUL_PCIE_RESOURCE_MEM_64;

		sz = sz & GEUL_PCI_BASE_ADDRESS_MEM_MASK;
		sz = ~sz + 1;

		if (sz > 0x40000000) {
			log_err ("%s : PCIE%d : BAR%d size not supported\n\r", __func__,
					 GEUL_PCIE_ID(pxModemPciDev->eId), ulBarNum);
			goto fail;
		}

		if ((pxModemPciDev->ulCurrentMemPos + sz) < pxModemPciDev->ulTopMemPos)
		{
			l = pxModemPciDev->ulCurrentMemPos;
			pxModemPciDev->ulCurrentMemPos += sz;
			pxModemPciDev->ulCurrentMemPos =
					ALIGN(pxModemPciDev->ulCurrentMemPos, GEUL_PCIE_BAR_ALIGN);
		} else {
			log_err ("%s : PCIE%d : BAR%d Insufficient memory\n\r", __func__,
					 GEUL_PCIE_ID(pxModemPciDev->eId), ulBarNum);
			goto fail;
		}

		vPcieWriteConfig (pxModemPciDev->eId, ulBusDev, ulPos, l);
		if (pxModemPciDev->xBarRegion[ulBarNum].ulFlags &
			GEUL_PCIE_RESOURCE_MEM_64)
			vPcieWriteConfig (pxModemPciDev->eId, ulBusDev, ulPos + 4, 0x0);

		pxModemPciDev->xBarRegion[ulBarNum].ulAddrS = l;
		pxModemPciDev->xBarRegion[ulBarNum].ulAddrE = l + (sz - 1);

		log_info ("%s : PCIE%d : BAR%d size=0x%x (%s) Start=0x%x End=0x%x \n\r",
				__func__, GEUL_PCIE_ID(pxModemPciDev->eId), ulBarNum, sz,
				(pxModemPciDev->xBarRegion[ulBarNum].ulFlags
				& GEUL_PCIE_RESOURCE_MEM_64) ? "64 Bit" : "32 Bit",
				pxModemPciDev->xBarRegion[ulBarNum].ulAddrS,
				pxModemPciDev->xBarRegion[ulBarNum].ulAddrE);

		pxModemPciDev->xBarRegion[ulBarNum].ulValid = 1;
		return (pxModemPciDev->xBarRegion[ulBarNum].ulFlags &
				GEUL_PCIE_RESOURCE_MEM_64) ? 1 : 0;
	}

fail:
	pxModemPciDev->xBarRegion[ulBarNum].ulFlags = 0;
	return 0;
}

/*!
 * \fn static void prvPcieConfigCtrlRC (uint8_t ucId)
 * @brief Function to configure PCIe RC controller
 *
 * @param[in]	ucId      PCIe controller ID
 *
 * @return
 *   - Nothing
 */
static void prvPcieConfigCtrlRC (uint8_t ucId)
{
	out_le32((uint32_t *)GEUL_PCIE_RO_WR_EN(ucId), 0x1);
	out_le16((uint32_t *)GEUL_PCIE_CLASS(ucId),
			 (uint16_t)GEUL_PCIE_CLASS_BRIDGE);
	out_le16((uint32_t *)GEUL_PCIE_HEADER_TYPE(ucId),
			 (in_le16((uint32_t *)GEUL_PCIE_HEADER_TYPE(ucId)) & 0xff00)
			 |  GEUL_PCIE_HEADER_TYPE_BRIDGE);
	out_le32((uint32_t *)GEUL_PCIE_STRFMR1(ucId),
			 (in_le32((uint32_t *)GEUL_PCIE_STRFMR1(ucId)) & 0xDFFFFFFF));
	out_le32((uint32_t *)GEUL_PCIE_BUS(ucId), (uint32_t)0x00010100);
	out_le32((uint32_t *)GEUL_PCIE_RO_WR_EN(ucId), 0x0);
	out_le32((uint32_t *)GEUL_PCIE_NPMEM(ucId), GEUL_PCIE_NPMEM_VAL(ucId));
	out_le32((uint32_t *)GEUL_PCIE_PMEM(ucId), GEUL_PCIE_PMEM_VAL(ucId));
	out_le32((uint32_t *)GEUL_PCIE_PMEM_UB(ucId), GEUL_PCIE_PMEM_VAL_UB(ucId));
	out_le32((uint32_t *)GEUL_PCIE_PMEM_UL(ucId), GEUL_PCIE_PMEM_VAL_UL(ucId));
	out_le32((uint32_t *)GEUL_PCIE_DCTRL(ucId), GEUL_PCIE_DCTRL_VAL);
	out_le16((uint32_t *)GEUL_PCIE_CMD(ucId), (uint16_t)GEUL_PCIE_CMD_RC);
}

/*!
 * \fn static void prvPciePopulateCtrlRC (uint8_t eId, void *pvDevData)
 * @brief Function to populate PCIe RC controller
 *
 * @param[in]	ucId        PCIe controller ID
 * @param[in]	pvDevData   Represents PCIe task
 *
 * @return
 *   - Nothing
 */
static void prvPciePopulateCtrlRC (uint8_t eId, void *pvDevData)
{
	uint32_t ulPos;
	uint32_t ulReg;
	xModemPciDev_t *pxModemPciDev = NULL;

	pxModemPciDev = (xModemPciDev_t *)&xModemPciDev[eId];
	memset(pxModemPciDev, 0, (sizeof(xModemPciDev_t)));

	pxModemPciDev->eId = eId;
	pxModemPciDev->eHdrType = 1;
	pxModemPciDev->ucEnable = 1;
	pxModemPciDev->pvDevData = pvDevData;
	pxModemPciDev->ulCurrentMemPos = PCIE_CPU_ADDR(eId);
	pxModemPciDev->ulTopMemPos = GEUL_PCIE_CPU_ADDRESS_AVAI_TOP(eId);
	pxModemPciDev->ulPcieCfgRegion = GEUL_PCIE_CFG_BASE(eId);

#ifdef LA12XX_DRIVER_PCI_LAT_FP
  	prvPcieConfigLatFp(pxModemPciDev);
#endif
	for (ulPos = 0; ulPos < GEUL_PCIE_NUM_BARS; ulPos++) {
		ulReg = GEUL_PCIE_BAR0_OFFSET + (ulPos << 2);
		ulPos += iPcieConfigBars(pxModemPciDev, ulReg);
	}
}

/*!
 * \fn static uint32_t ulPcieReadConfig (eModemPciCtrlId_t ucId,
										 uint32_t ulBusDev,
										 uint32_t ulOffset)
 * @brief Function to read PCIe config
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ulBusDev  BDF
 * @param[in]	ulOffset  Offset
 *
 * @return
 *   - EP's config value
 */
uint32_t ulPcieReadConfig (eModemPciCtrlId_t ucId,
							uint32_t ulBusDev,
							uint32_t ulOffset)
{
	uint32_t ulAddress, ulValue;

	if (ucId != PCIE_2) {
		log_err ("%s : PCIe%d : Operation not supported  \n\r", __func__,
				GEUL_PCIE_ID(ucId));
		return 0xffffffff;
	}

	PcieSetBusdev(ucId, ulBusDev);
	ulAddress = (uint32_t)xModemPciDev[ucId].ulPcieCfgRegion + ulOffset;
	ulValue = in_le32((uint32_t *)ulAddress);

	log_dbg ("%s: PCIE%d : ulBusDev=0x%x ulAddress=0x%x ulValue=0x%x\r\n",
				__func__, GEUL_PCIE_ID(ucId),  ulBusDev, ulAddress, ulValue);

	return ulValue;
}

/*!
 * \fn static void vPcieWriteConfig (eModemPciCtrlId_t ucId, uint32_t ulBusDev,
 *									 uint32_t ulOffset, uint32_t ulValue)
 * @brief Function to write PCIe config
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ulBusDev  BDF
 * @param[in]	ulOffset  Offset
 * @param[in]	ulValue   Value to be written
 *
 * @return
 *    - Nothing
 */
void vPcieWriteConfig (eModemPciCtrlId_t ucId,
						uint32_t ulBusDev,
						uint32_t ulOffset,
						uint32_t ulValue)
{
	uint32_t ulAddress;

	if (ucId != PCIE_2) {
		log_err ("%s : PCIe%d : Operation not supported  \n\r", __func__,
					GEUL_PCIE_ID(ucId));
	}

	PcieSetBusdev(ucId, ulBusDev);
	ulAddress = (uint32_t)xModemPciDev[ucId].ulPcieCfgRegion + ulOffset;

	out_le32((uint32_t *)ulAddress, ulValue);

	log_dbg	("%s : PCIE%d : ulBusDev=0x%x ulAddress=0x%x ulValue=0x%x\r\n",
			__func__, GEUL_PCIE_ID(ucId), ulBusDev, ulAddress, ulValue);
}

/*!
 * \fn uint32_t ulPcieReadMem(eModemPciCtrlId_t ucId, eModemPciMemRegion_t
								eBarNum, uint32_t ulOffset, uint8_t ucSize,
								uint32_t *pVal)
 * @brief Function for mmio read over the PCIe
 *
 * @param[in]	ucId	PCIe controller ID
 * @param[in]	eBarNum	PCIe BAR to read (0-5)
 * @param[in]	ulOffset	read offset
 * @param[in]	ucSize	read size
 * @param[out]	*pVal	read value
 *
 * @return
 *   - On Success, returns number of bytes read (should match ucSize)
 *   - On Failure, returns 0
 */
uint32_t ulPcieReadMem (eModemPciCtrlId_t ucId,
						eModemPciMemRegion_t eBar,
						uint32_t ulOffset,
						uint8_t ucSize,
						uint32_t *pVal)
{
	if (ucId != PCIE_2) {
		log_err ("%s : PCIe%d : Operation not supported  \n\r", __func__,
					GEUL_PCIE_ID(ucId));
		return 0xffffffff;
	}

	switch (ucSize) {
		case 4:
			*pVal = in_le32((uint32_t *)
					(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset));
			break;
		case 2:
			*pVal = in_le16((uint32_t *)
					(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset));
			break;
		case 1:
			*pVal = in_8((uint32_t *)
					(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset));
			break;
		default:
			log_err ("%s : PCIe%d : Invalid Size  \n\r", __func__,
						GEUL_PCIE_ID(ucId));
			return 0;
	}

	log_dbg ("%s : PCIE%d : Bar=0x%x address=0x%x ucSize=0x%x *pVal=0x%x\n\r",
				__func__, GEUL_PCIE_ID(ucId), eBar,
				(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset),
				ucSize, *pVal);

	return ucSize;
}

/*!
 * \fn uint32_t ulPcieWriteMem(eModemPciCtrlId_t ucId, eModemPciMemRegion_t
								eBarNum, uint32_t ulOffset, uint8_t ucSize,
								uint32_t ulValue)
 * @brief Function for mmio write over the PCIe
 *
 * @param[in]	ucId	PCIe controller ID
 * @param[in]	eBarNum eBarNum PCIe BAR to write (0-5)
 * @param[in]	ulOffset	write offset
 * @param[in]	ucSize	write size
 * @param[in]	ulValue	value to write
 *
 * @return
 *   - On Success, returns number of bytes written (should match ucSize)
 *   - On Failure, returns 0
 */
uint32_t ulPcieWriteMem (eModemPciCtrlId_t ucId,
						eModemPciMemRegion_t eBar,
						uint32_t ulOffset,
						uint8_t ucSize,
						uint32_t ulValue)
{
	if (ucId != PCIE_2) {
		log_err ("%s : PCIe%d : Operation not supported  \n\r", __func__, 
					GEUL_PCIE_ID(ucId));
		return 0;
	}

	switch (ucSize) {
		case 4:
			out_le32((uint32_t *)
			(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset),
			(uint32_t)ulValue);
			break;
		case 2:
			out_le16((uint32_t *)
			(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset),
			(uint16_t)ulValue);
			break;
		case 1:
			out_8((uint32_t *)
			(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset),
			(uint8_t)ulValue);
			break;
		default:
			log_err ("%s : PCIe%d : Invalid Size  \n\r", __func__,
						GEUL_PCIE_ID(ucId));
			return 0;
	}

	log_dbg ("%s : PCIE%d : Bar=0x%x address=0x%x ucSize=0x%x ulValue=0x%x  \n\r",
				__func__, GEUL_PCIE_ID(ucId), eBar,
				(xModemPciDev[ucId].xBarRegion[eBar].ulAddrS + ulOffset),
				ucSize, ulValue);

	return ucSize;
}

/*!
 * \fn uint32_t ulPcieGetMemregion(eModemPciCtrlId_t icId, eModemPciMemRegion_t
									eBarNum, uint32_t *pSaddr, uint32_t *pEaddr)
 * @brief Function to get pcie mapped memory region.
 *
 * @param[in]	ucId	PCIe controller ID
 * @param[in]	eBarNum PCIe BAR (0-5)
 * @param[out]	pSaddr	Start address of requested pcie mapped region
 * @param[out]	pEaddr	End address of requested pcie mapped region
 *
 * @return
 *   - pdTRUE
 */
uint32_t ulPcieGetMemregion (eModemPciCtrlId_t ucId,
							eModemPciMemRegion_t eBar,
							uint32_t *pSaddr,
							uint32_t *pEaddr)
{
	if (ucId != PCIE_2) {
		log_err ("%s : PCIe%d : Operation not supported  \n\r", __func__, GEUL_PCIE_ID(ucId));
		return pdFALSE;
	}

	if (eBar >= BAR_MAX) {
		log_err ("%s : PCIe%d : Invalid Region  \n\r", __func__, GEUL_PCIE_ID(ucId));
		return pdFALSE;
	}

	*pSaddr = xModemPciDev[ucId].xBarRegion[eBar].ulAddrS;
	*pEaddr = xModemPciDev[ucId].xBarRegion[eBar].ulAddrE;
	return pdTRUE;
}

/*!
 * \fn uint32_t PcieSetupRC (uint8_t ucId, uint8_t ucMode, void *pvDevData)
 * @brief Function to setup PCIe EP controller
 *
 * @param[in]	ucId        PCIe controller ID
 * @param[in]	ucMode      PCIe controler mode (RC/EP)
 * @param[in]	pvDevData   Represents PCIe task
 *
 * @return
 *   - pdTRUE      PcieSetupRC success
 *   - pdFALSE     PcieSetupRC failed
 */
uint32_t PcieSetupRC (uint8_t ucId, uint8_t ucMode, void * pvDevData)
{
	prvPcieErrataRC (ucId);
	PcieEnableLink (ucId, ucMode);

	if (!bPcieIsLinkUp(ucId)) {
		log_err ("%s : PCIE%d : %s : (RC) linkup not up.. \n\r", __func__, GEUL_PCIE_ID(ucId));
		return pdFALSE;
	}

	log_info ("%s : PCIE%d : (RC) Linkup Pass \n\r", __func__,GEUL_PCIE_ID(ucId));

	prvPcieSetupAtuRC (ucId);
	prvPcieConfigCtrlRC (ucId);
	prvPcieConfigBarsRC (ucId);
	prvPciePopulateCtrlRC (ucId, pvDevData);

	return pdTRUE;
}
/** @} */
