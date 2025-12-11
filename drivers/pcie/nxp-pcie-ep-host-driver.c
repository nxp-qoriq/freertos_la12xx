
// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023-2024 NXP
 */

#include "FreeRTOS.h"
#include "nxp-pcie.h"
#include "nxp-pcie-ep-host-driver.h"
#ifdef LA12XX_DRIVER_PCI_LAT_FP
#include "lattice-pcie-ep-fpga.h"
#include "pcie_ep_lat_fpga.h"
#endif

xPciEpDev_t xPciEpDev __attribute__ ((section (".smem")));

/*!
 * \fn void prvSetupPcieRemoteOb (uint8_t ucIndex, uint32_t ulPciCtrlAddr, uint8_t ucType,
 *									uint32_t ucRegion, uint32_t ulSaddr, uint32_t ulTaddr,
									uint32_t ulSize)
 * @brief Funtion to setup PCIe outbound
 *
 * @param[in]  ucIndex           Index
 * @param[in]  ulPciCtrlAddr     PCIe controller address
 * @param[in]  ucType            Type
 * @param[in]  ucRegion          Region
 * @param[in]  ulSaddr           Source address
 * @param[in]  ulTaddr           Target address
 * @param[in]  ulSize            Size
 *
 * @return
 *   - Returns Nothing
 */
void prvSetupPcieRemoteOb (uint8_t ucIndex, uint32_t ulPciCtrlAddr, uint8_t ucType,
							uint32_t ucRegion, uint32_t ulSaddr, uint32_t ulTaddr, uint32_t ulSize)
{
	out_le32((uint32_t *)GEUL_PCIE_RATU_VIEWPORT_E (ulPciCtrlAddr), ((uint32_t)ucRegion | ucIndex));
	out_le32((uint32_t *)GEUL_PCIE_RATU_LOWER_BASE (ulPciCtrlAddr), ulSaddr);
	out_le32((uint32_t *)GEUL_PCIE_RATU_UPPER_BASE (ulPciCtrlAddr), (uint32_t)0x0);
	out_le32((uint32_t *)GEUL_PCIE_RATU_UPPER_LIMIT_ADDRESS (ulPciCtrlAddr), (ulSaddr + ulSize - 1));
	out_le32((uint32_t *)GEUL_PCIE_RATU_LOWER_TARGET_E (ulPciCtrlAddr), ulTaddr );
	out_le32((uint32_t *)GEUL_PCIE_RATU_UPPER_TARGET_E (ulPciCtrlAddr), (uint32_t)0x0);
	out_le32((uint32_t *)GEUL_PCIE_RATU_CR1_E (ulPciCtrlAddr), ucType);
	out_le32((uint32_t *)GEUL_PCIE_RATU_CR2_E (ulPciCtrlAddr), (uint32_t)(GEUL_PCIE_ATU_ENABLE));
}

/*!
 * \fn void prvPcieEpConfigureRemoteOb (xPciEpDev_t *pxPciEpDev)
 * @brief Funtion to configure EP device outbound
 *
 * @param[in]  pxPciEpDev   Pointer to EP device structure
 *
 * @return
 *   - Returns Nothing
 */
void prvPcieEpConfigureRemoteOb (xPciEpDev_t *pxPciEpDev)
{
	uint32_t ulPciCtrlAddr;
	uint32_t ulBusDev;

	ulBusDev = GEUL_PCIE_ATU_BUS(1) | GEUL_PCIE_ATU_DEV(0) | GEUL_PCIE_ATU_FUNC(0);
	ulPciCtrlAddr = pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_CCSR].ulAddrS + 
					ulPcieReadConfig (pxPciEpDev->eId, ulBusDev, 0x2c);

	log_dbg ("%s : PCIE%d ulPciCtrlAddr=0x%x \n\r", __func__,GEUL_PCIE_ID(pxPciEpDev->eId),
			ulPciCtrlAddr);

	prvSetupPcieRemoteOb (0x0, ulPciCtrlAddr, GEUL_PCIE_ATU_TYPE_MEM, GEUL_PCIE_ATU_REGION_OUTBOUND,
							GEUL_PCIE_RC_PEB_REGION_CPU_ADDRESS(pxPciEpDev->eId) ,
							pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_PEB].ulAddrS,
							(pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_PEB].ulAddrE -
							pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_PEB].ulAddrS + 1));

	prvSetupPcieRemoteOb (0x1, ulPciCtrlAddr, GEUL_PCIE_ATU_TYPE_MEM, GEUL_PCIE_ATU_REGION_OUTBOUND,
							GEUL_PCIE_RC_HRAM_REGION_CPU_ADDRESS (pxPciEpDev->eId),
							pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_HRAM].ulAddrS,
							(pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_HRAM].ulAddrE -
							pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_HRAM].ulAddrS + 1));
}

/*!
 * \fn void prvPcieRcPrepareMemRegions (xPciEpDev_t *pxPciEpDev)
 * @brief Funtion to prepare memory regions of RC which should be accessable from EP device
 * Note that RC's memory regions (GEUL_PCIE_RC_REGION_PEB_ADDRESS,
 * GEUL_PCIE_RC_REGION_HRAM_ADDRESS etc) are the PCIe address.
 * NXP EP requires address translation to map to these PCIe address which may/may not
 * be required for other EP devices.
 *
 * @param[in]  pxPciEpDev   Pointer to EP device structure
 *
 * @return
 *   - Returns Nothing
 */
void prvPcieRcPrepareMemRegions (xPciEpDev_t *pxPciEpDev)
{
	uint8_t i;
	pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_PEB].ulAddrS = GEUL_PCIE_RC_REGION_PEB_ADDRESS;
	pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_PEB].ulAddrE = GEUL_PCIE_RC_REGION_PEB_ADDRESS +
															GEUL_PCIE_RC_REGION_PEB_SIZE - 1;

	pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_HRAM].ulAddrS = GEUL_PCIE_RC_REGION_HRAM_ADDRESS;
	pxPciEpDev->xMemRegionRC[PCIE_RC_REGION_HRAM].ulAddrE = GEUL_PCIE_RC_REGION_HRAM_ADDRESS +
															GEUL_PCIE_RC_REGION_HRAM_SIZE - 1;

	for (i = 0; i < PCIE_RC_REGION_MAX ; i++) 
		log_dbg ("%s : PCIE%d Region [0x%x - 0x%x] \n\r", __func__, GEUL_PCIE_ID(pxPciEpDev->eId),
				 pxPciEpDev->xMemRegionRC[i].ulAddrS, pxPciEpDev->xMemRegionRC[i].ulAddrE);
#ifdef LA12XX_DRIVER_PCI_LAT_FP
	/* No need to configure FPGA(EP) outbounds.
	 * It's taken care by the EP device.
	 */
	return;
#endif
	prvPcieEpConfigureRemoteOb (pxPciEpDev);
}

/*!
 * \fn void prvPcieEpPrepareMemRegions (xPciEpDev_t *pxPciEpDev)
 * @brief Funtion to prepare memory regions for EP device
 *
 * @param[in]  pxPciEpDev   Pointer to EP device structure
 *
 * @return
 *   - Returns Nothing
 */
void prvPcieEpPrepareMemRegions (xPciEpDev_t *pxPciEpDev)
{
	uint32_t Saddr=0;
	uint32_t Eaddr=0;

#ifdef LA12XX_DRIVER_PCI_LAT_FP
	ulPcieGetMemregion (pxPciEpDev->eId, 0x0, &Saddr, &Eaddr);
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_CCSR].ulAddrS = Saddr;
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_CCSR].ulAddrE = Eaddr;
	
	log_dbg ("%s : PCIE%d Region [0x%x - 0x%x] \n\r", __func__,
			GEUL_PCIE_ID(pxPciEpDev->eId),
			pxPciEpDev->xMemRegionEP[i].ulAddrS,
			pxPciEpDev->xMemRegionEP[i].ulAddrE);
#else
	uint8_t i = 0;
	ulPcieGetMemregion (pxPciEpDev->eId, 0x0, &Saddr, &Eaddr);
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_CCSR].ulAddrS = Saddr + PCIE_EP_REGION_CCSR_OFFSET;
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_CCSR].ulAddrE = Saddr + PCIE_EP_REGION_CCSR_OFFSET +
															PCIE_EP_REGION_CCSR_SIZE - 1;
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_DCSR].ulAddrS = Saddr + PCIE_EP_REGION_DCSR_OFFSET;
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_DCSR].ulAddrE = Saddr + PCIE_EP_REGION_DCSR_OFFSET +
															PCIE_EP_REGION_DCSR_SIZE - 1;

	ulPcieGetMemregion (pxPciEpDev->eId, 0x2, &Saddr, &Eaddr);
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_PEBM].ulAddrS = Saddr + PCIE_EP_REGION_PEBM_OFFSET;
	pxPciEpDev->xMemRegionEP[PCIE_EP_REGION_PEBM].ulAddrE = Saddr + PCIE_EP_REGION_PEBM_OFFSET +
															PCIE_EP_REGION_PEBM_SIZE - 1;
	for (i = 0; i < PCIE_EP_REGION_MAX ; i++) 
		log_dbg ("%s : PCIE%d Region [0x%x - 0x%x] \n\r", __func__, GEUL_PCIE_ID(pxPciEpDev->eId),
				 pxPciEpDev->xMemRegionEP[i].ulAddrS, pxPciEpDev->xMemRegionEP[i].ulAddrE);
#endif
}

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
uint32_t ulPcieGetEpMemregion (eModemPciCtrlId_t ucId, ePciEpMemRegionLa12xx_t eRegion,
								uint32_t *pSaddr, uint32_t *pEaddr)
{
	if (ucId != PCIE_2) {
		log_err ("%s : PCIe%d : Operation not supported  \n\r", __func__, GEUL_PCIE_ID(ucId));
		return pdFALSE;
	}

	*pSaddr = xPciEpDev.xMemRegionEP[eRegion].ulAddrS;
	*pEaddr = xPciEpDev.xMemRegionEP[eRegion].ulAddrE;

	log_dbg ("%s : PCIE%d *pSaddr=0x%x *pEaddr=0x%x \n\r", __func__,GEUL_PCIE_ID(ucId), *pSaddr,
			*pEaddr);

	return pdTRUE;
}

/*!
 * \fn void PcieEpHostDriverNxp (uint32_t ulVidDid)
 * @brief EP host driver probe for NXP platforms
 *
 * @param[in]	ulVidDid   VendorID/DeviceID value
 *
 * @return
 *   - Returns Nothing
 */
void PcieEpHostDriverNxp (uint32_t ulVidDid)
{
	xPciEpDev_t *pxPciEpDev = NULL;

	(void) ulVidDid;

	pxPciEpDev = (xPciEpDev_t *)&xPciEpDev;
	memset (pxPciEpDev, 0, (sizeof(xPciEpDev_t)));
	pxPciEpDev->eId = 1;

	log_dbg ("%s : PCIE%d ulVidDid=0x%x \n\r", __func__,GEUL_PCIE_ID(pxPciEpDev->eId), ulVidDid);

	prvPcieEpPrepareMemRegions (pxPciEpDev);
	prvPcieRcPrepareMemRegions (pxPciEpDev);
}

#ifdef LA12XX_DRIVER_PCI_LAT_FP
/*!
 * \fn void PcieEpConfigDmaBuf (xConfigChannels_t *slat_ctrl_reg)
 * @brief EP host driver to Config DMA Buffer.
 *
 * @param[in]	xConfigChannels_t DMA configuration.
 *
 * @return
 *   - Returns Nothing
 */
void PcieEpConfigDmaBuf (xConfigChannels_t *channel_config)
{
	uint8_t isChannelSelected = 0;
	uint8_t selChannelx = 0;
	uint32_t Saddr=0;
	uint32_t Eaddr=0;
	uint32_t channel_offset = 0;
	xlatCtrlReg *lat_ctrl_reg;

	ulPcieGetEpMemregion (PCIE_2, PCIE_EP_REGION_CCSR, &Saddr, &Eaddr);
	lat_ctrl_reg = (xlatCtrlReg *)Saddr;
	configASSERT( lat_ctrl_reg );

	/* Configure RX Channels */
	selChannelx = (uint8_t)(channel_config->channels & 0xf);
	for (uint8_t i = 0; i < CHANNEL_MAX; i++)
	{	
		isChannelSelected = (selChannelx >> i) & 1;
		if (isChannelSelected) {
			channel_offset = i * 0x100;
			lat_ctrl_reg = (xlatCtrlReg *)((uint32_t)Saddr + channel_offset );
			
			/* Disable DMA */
			out_le32(&lat_ctrl_reg->RX_DMA_EN, 0x0);

			if ((channel_config->channelCfg[i].rx_config.channelSize)&0xfff)
				log_info(" Warn: CH%d RX_TBCNT is not 4k aligned!\r\n",i);

			if (channel_config->channelCfg[i].rx_config.channelTotalSizeAddr) {
				out_le32(&lat_ctrl_reg->TOTAL_WR_SIZE_ADDR,
					channel_config->channelCfg[i].rx_config.channelTotalSizeAddr );
			}

			out_le32(&lat_ctrl_reg->RXD_ADDR,
				channel_config->channelCfg[i].rx_config.channelAddr);
			out_le32(&lat_ctrl_reg->RBCNT ,
				channel_config->channelCfg[i].rx_config.channelSize);
		}
	}

	/* Configure TX Channels */
	selChannelx = (uint8_t)((channel_config->channels  & 0xf0) >> 4);
	for (uint8_t i = 0; i < CHANNEL_MAX; i++)
	{	
		isChannelSelected = (selChannelx >> i) & 1;
		if (isChannelSelected) {
			channel_offset = i * 0x100;
			lat_ctrl_reg = (xlatCtrlReg *)((uint32_t)Saddr + channel_offset );

			out_le32(&lat_ctrl_reg->TX_DMA_EN, 0x0);
			if ((channel_config->channelCfg[i].tx_config.channelSize)&0xfff)
				log_info(" Warn: CH%d TX_TBCNT is not 4k aligned!\r\n",i);

			if (channel_config->channelCfg[i].tx_config.channelTotalSizeAddr) {
				out_le32(&lat_ctrl_reg->TOTAL_RD_SIZE_ADDR,
					channel_config->channelCfg[i].tx_config.channelTotalSizeAddr );
			}
			out_le32(&lat_ctrl_reg->TXD_ADDR,
				channel_config->channelCfg[i].tx_config.channelAddr);
			out_le32(&lat_ctrl_reg->TBCNT ,
				channel_config->channelCfg[i].tx_config.channelSize);
		}
	}	
}


/*!
 * \fn void PcieEpSetDma (xlatCtrlReg *slat_ctrl_reg)
 * @brief EP host driver to Enable/Disable DMA.
 *
 * @param[in]	xlatCtrlReg DMA configuration.
 *
 * @return
 *   - Returns Nothing
 */
void PcieEpSetDma (xSetDma_t *set_dma)
{
	uint8_t isChannelSelected = 0;
	uint8_t setChannelAs = 0;
	uint8_t selChannelx = 0;
	uint8_t setChannelx = 0;
	uint32_t Saddr = 0;
	uint32_t Eaddr = 0;
	uint32_t channel_offset = 0;
	xlatCtrlReg *lat_ctrl_reg;

	ulPcieGetEpMemregion (PCIE_2, PCIE_EP_REGION_CCSR, &Saddr, &Eaddr);
	lat_ctrl_reg = (xlatCtrlReg *)Saddr;
	configASSERT( lat_ctrl_reg );

	/* Set RX Channels */
	selChannelx = (uint8_t)(set_dma->selectChannel & 0xf);
	setChannelx = (uint8_t)(set_dma->setChannel & 0xf);
	for ( uint8_t i = 0; i < CHANNEL_MAX; i++)
	{
		isChannelSelected = (selChannelx >> i) & 1;
		if (isChannelSelected) {
			channel_offset = i*0x100;
			lat_ctrl_reg = (xlatCtrlReg *)((uint32_t)Saddr + channel_offset);
			setChannelAs = (setChannelx >> i) & 1;
			out_le32(&lat_ctrl_reg->RX_DMA_EN, setChannelAs);
		}
	}
	
	/* Set TX Channels */
	selChannelx = (uint8_t)((set_dma->selectChannel & 0xf0) >> 4);
	setChannelx = (uint8_t)((set_dma->setChannel & 0xf0) >> 4);
	for ( uint8_t i = 0; i < CHANNEL_MAX; i++)
	{
		isChannelSelected = (selChannelx >> i) & 1;
		if (isChannelSelected) {
			channel_offset = i*0x100;
			lat_ctrl_reg = (xlatCtrlReg *)((uint32_t)Saddr + channel_offset);
			setChannelAs = (setChannelx >> i) & 1;
			out_le32(&lat_ctrl_reg->TX_DMA_EN, setChannelAs);
		}
	}
}
#endif
