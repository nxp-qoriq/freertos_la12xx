// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#ifndef _GEUL_PCI_DEF_H
#define _GEUL_PCI_DEF_H

/*!
 * @file    nxp-pcie.h
 * @brief   This file contains PCIe driver APIs and MACROS.
 * @defgroup   PCIE_API
 * @{
 */

#include <stdint.h>
#include "config.h"
#include "platform_def.h"
#include "types.h"
#include "bit.h"

#if (defined LA12XX_DRIVER_PCI) || (defined LA12XX_DRIVER_PCI_LAT_FP)

/**
 * PCIe Base Address
 */
#define PCIE_BASE_ADDR(n)        (CCSR_BASE_ADDR + 0x3400000 + ((n)*0x100000))
#define PCIE_CTRL_OFFSET(n)      ((uint32_t)0x3400000 + ((n)*0x100000))

/**
 * PCIe Bus Address
 */
#define BASE_ADDR_PCI0           (uint32_t)0x0
#define BASE_ADDR_PCI1           (uint32_t)0x80000000
#define ADDR_SIZE_PCI0           (uint32_t)0x80000000
#define ADDR_SIZE_PCI1           (uint32_t)0x40000000

#define CPU_ADDR_PCI0            (uint32_t)0x0
#define CPU_ADDR_PCI1            (uint32_t)0x80000000
#define CPU_ADDR_SIZE_PCI0       (uint32_t)0x80000000
#define CPU_ADDR_SIZE_PCI1       (uint32_t)0x40000000
/**
 * PCIe Bus Address wrapper
 */
#define PCIE_BUS_ADDR(n)         ((n) ? BASE_ADDR_PCI1 :BASE_ADDR_PCI0)
#define PCIE_BUS_ADDR_SIZE(n)    ((n) ? ADDR_SIZE_PCI1 : ADDR_SIZE_PCI0)
#define PCIE_CPU_ADDR(n)         ((n) ? CPU_ADDR_PCI1 : CPU_ADDR_PCI0)
#define PCIE_CPU_ADDR_SIZE(n)    ((n) ? CPU_ADDR_SIZE_PCI1 : CPU_ADDR_SIZE_PCI0)
/**
 * PCIe registers and configs
 */
#define GEUL_PEBM_BASE_ADDR      0xE0200000 /**< PEBM Address */

/**
 * PCIe DeviceID and Vendor ID registers
 */
#define GEUL_PCIE_DEVICE_ID          0x1c30 /**< Geul DeviceID */
#define GEUL_PCIE_VENDOR_ID          0x1957 /**< Geul VendorID */
#define GEUL_PCIE_VENDOR_ID_ADDR(n)  (PCIE_BASE_ADDR(n)) /**< PCIe VendorID address */
#define GEUL_PCIE_DEVICE_ID_ADDR(n)  (PCIE_BASE_ADDR(n) + 0x2) /**< PCIe DeviceID address */
#define GEUL_PCIE_CFG_READY          1 /**< PCIe config ready */

/**
 * PCIe configuration and flags
 */
#define GEUL_PCIE_LINK_RETRY               (3600) /**< Geul link retry cound */  
#define GEUL_PCIE_NUM_BARS                 6 /**< Geul number of bars */
#define GEUL_PCIE_RESOURCE_MEM_64          0x00000004 /**< Geul 64 Bit BAR */
#define GEUL_PCI_BASE_ADDRESS_MEM_MASK     0xfffffff0 /**< Geul Mem BAR mask */
#define GEUL_PCIE_BAR_ALIGN                0x100000 /**< Geul BAR alignment */

/**
 * PCIe iATU registers
 */
#define GEUL_PCIE_ATU_REGION_INBOUND    (0x1 << 31) /**<  PCIe ATU Region Inbound */
#define GEUL_PCIE_ATU_REGION_OUTBOUND   (0x0 << 31) /**<  PCIe ATU Region Outbound */
#define GEUL_PCIE_ATU_REGION_INDEX3     (0x3 << 0) /**<  PCIe ATU Index */
#define GEUL_PCIE_ATU_REGION_INDEX2     (0x2 << 0) /**<  PCIe ATU Index */
#define GEUL_PCIE_ATU_REGION_INDEX1     (0x1 << 0) /**<  PCIe ATU Index */
#define GEUL_PCIE_ATU_REGION_INDEX0     (0x0 << 0) /**<  PCIe ATU Index */
#define GEUL_PCIE_ATU_TYPE_MEM          (0x0 << 0) /**<  PCIe ATU Type */
#define GEUL_PCIE_ATU_TYPE_IO           (0x2 << 0) /**<  PCIe ATU Type */
#define GEUL_PCIE_ATU_TYPE_CFG0         (0x4 << 0) /**<  PCIe ATU Type */
#define GEUL_PCIE_ATU_TYPE_CFG1         (0x5 << 0) /**<  PCIe ATU Type */
#define GEUL_PCIE_ATU_ENABLE            (0x1 << 31) /**<  PCIe ATU Enable */
#define GEUL_PCIE_ATU_BAR_MODE_ENABLE   (0x1 << 30) /**<  PCIe ATU Mode */

/**
 * PCIe BDF and BAR registers
 */
#define GEUL_PCIE_ATU_BUS(x)            (((x) & 0xff) << 24) /**<  PCIe bus */
#define GEUL_PCIE_ATU_DEV(x)            (((x) & 0x1f) << 19) /**<  PCIe device */
#define GEUL_PCIE_ATU_FUNC(x)           (((x) & 0x7) << 16) /**<  PCIe function */
#define GEUL_PCIE_BAR0_OFFSET           0x10 /**<  PCIe BAR0 */
#define GEUL_PCIE_BAR1_OFFSET           0x14 /**<  PCIe BAR1 */
#define GEUL_PCIE_BAR2_OFFSET           0x18 /**<  PCIe BAR2 */

/**
 * PCIe LUT and PF config registers
 */
#define GEUL_PCIE_PEX1_LUT_BASE(n)       (CCSR_BASE_ADDR + 0x3480000 + ((n)*0x100000)) /**<  PCIe LUT base */
#define GEUL_PCIE_PEX_PF_CONTROL_BASE(n) (GEUL_PCIE_PEX1_LUT_BASE(n) + 0x40014) /**<  PCIe LUT PF Config */
#define GEUL_PCIE_PEX_PF_DBG_BASE(n)     (GEUL_PCIE_PEX1_LUT_BASE(n) + 0x407fc) /**<  PCIe LUT DBG register */
#define PEX_PF0_CONFIG(n)                GEUL_PCIE_PEX_PF_CONTROL_BASE(n) /**<  PCIe PF0 config */
#define PEX_PF0_DBG(n)                   GEUL_PCIE_PEX_PF_DBG_BASE(n) /**<  PCIe PF0 DBG */

/**
 * PCIe BAR Target address and size
 */

/* Addr:BIST, Config & Control, Size: 256 MB, 32Bit */
#define GEUL_PCIE_ATU_BAR0_TARGET        0xF0000000 /**<  PCIe BAR0 target */
#define GEUL_PCIE_ATU_BAR0_SIZE          0x0FFFFFFF /**<  PCIe BAR0 size */

/* Addr:External Flash, Size: 256 MB, 32Bit */
#define GEUL_PCIE_ATU_BAR1_TARGET        0xC0000000 /**<  PCIe BAR1 target */
#define GEUL_PCIE_ATU_BAR1_SIZE          0x0FFFFFFF /**<  PCIe BAR1 size */

/* Addr: On Chip SRAM,Size: 128 MB 64Bit */
#define GEUL_PCIE_ATU_BAR2_TARGET        0xE0000000 /**<  PCIe BAR2 target */
#define GEUL_PCIE_ATU_BAR2_SIZE          0x07FFFFFF /**<  PCIe BAR2 size */

/**
 * PCIe iATU config registers
 */
#define GEUL_PCIE_ATU_BASE(n)                (PCIE_BASE_ADDR(n) + 0x900) /**<  PCIe ATU Base */
#define GEUL_PCIE_ATU_VIEWPORT_E(n)          (GEUL_PCIE_ATU_BASE(n) + 0x0) /**<  PCIe viewport */
#define GEUL_PCIE_ATU_CR1_E(n)               (GEUL_PCIE_ATU_BASE(n) + 0x4) /**<  PCIe CR1 */
#define GEUL_PCIE_ATU_CR2_E(n)               (GEUL_PCIE_ATU_BASE(n) + 0x8) /**<  PCIe CR2 */
#define GEUL_PCIE_ATU_BAR_NUM(bar)           ((bar) << 8) /**<  PCIe BAR number */
#define GEUL_PCIE_ATU_LOWER_TARGET_E(n)      (GEUL_PCIE_ATU_BASE(n) + 0x18) /**<  PCIe target lower */
#define GEUL_PCIE_ATU_UPPER_TARGET_E(n)      (GEUL_PCIE_ATU_BASE(n) + 0x1C) /**<  PCIe target upper */
#define GEUL_PCIE_ATU_UPPER_LIMIT_ADDRESS(n) (GEUL_PCIE_ATU_BASE(n) + 0x14) /**<  PCIe ATU limit register */
#define GEUL_PCIE_ATU_LOWER_BASE(n)          (GEUL_PCIE_ATU_BASE(n) + 0xC) /**<  PCIe base lower */
#define GEUL_PCIE_ATU_UPPER_BASE(n)          (GEUL_PCIE_ATU_BASE(n) + 0x10) /**<  PCIe base upper */

/**
 * PCIe LTSSM
 */
#define GEUL_PCIE_LTSSM_EN                   (1<<12)

/**
 * PCIe config registers
 */
#define GEUL_PCIE_BAR0_BASE_ADDR(n)     (PCIE_BASE_ADDR(n) + 0x1010) /**< PCIe BAR0 address */
#define GEUL_PCIE_BAR1_BASE_ADDR(n)     (PCIE_BASE_ADDR(n) + 0x1014) /**< PCIe BAR1 address */
#define GEUL_PCIE_ROM_BASE_ADDR(n)      (PCIE_BASE_ADDR(n) + 0x1038) /**< PCIe ROM address */
#define GEUL_PCIE_RO_WR_EN(n)           (PCIE_BASE_ADDR(n) + 0x8bc) /**< PCIe write enable */
#define GEUL_PCIE_CLASS(n)              (PCIE_BASE_ADDR(n) + 0xa) /**< PCIe class */
#define GEUL_PCIE_HEADER_TYPE(n)        (PCIE_BASE_ADDR(n) + 0xe) /**< PCIe header type */
#define GEUL_PCIE_STRFMR1(n)            (PCIE_BASE_ADDR(n) + 0x71c) /**< PCIe STRFMR1 */
#define GEUL_PCIE_BUS(n)                (PCIE_BASE_ADDR(n) + 0x18) /**< PCIe bus config */
#define GEUL_PCIE_NPMEM(n)              (PCIE_BASE_ADDR(n) + 0x20) /**< PCIe non prefetch memory */
#define GEUL_PCIE_PMEM(n)               (PCIE_BASE_ADDR(n) + 0x24) /**< PCIe prefetch memory */
#define GEUL_PCIE_PMEM_UB(n)            (PCIE_BASE_ADDR(n) + 0x28) /**< PCIe prefetch memory upper */
#define GEUL_PCIE_PMEM_UL(n)            (PCIE_BASE_ADDR(n) + 0x2c) /**< PCIe prefetch memory upper limit*/
#define GEUL_PCIE_DCTRL(n)              (PCIE_BASE_ADDR(n) + 0x78) /**< PCIe device controll register */
#define GEUL_PCIE_CMD(n)                (PCIE_BASE_ADDR(n) + 0x4) /**< PCIe command register */
#define GEUL_PCIE_BUS(n)                (PCIE_BASE_ADDR(n) + 0x18) /**< PCIe bus register */
#define GEUL_PCIE_SUBS(n)               (PCIE_BASE_ADDR(n) + 0x2c) /**< PCIe bus register */
#define GEUL_PCIE_PMIE(n)               (PCIE_BASE_ADDR(n) + 0xc0028) /**< PCIe PMI enable register  */
#define GEUL_PCIE_PMIS(n)               (PCIE_BASE_ADDR(n) + 0xc0020) /**< PCIe PMI status register  */
#define GEUL_PCIE_CLASS_BRIDGE          0x0604 /**< PCIe class bridge  */
#define GEUL_PCIE_HEADER_TYPE_BRIDGE    0x1 /**< PCIe header type bridge  */
#define GEUL_PCIE_PMI_ENABLE            (BIT(7) | BIT(9) | BIT(10)) /**< PCIe PMI enable bit */
#define GEUL_PCIE_PMI_LUDIE             (BIT(7))

/**
 * PCIe Bus Address
 */
#define GEUL_PCIE_ID(n)                  ((n) + 1) /**< PCIe Ctrl ID */
#define GEUL_PCIE_NUM_CTRL               (0x2) /**< Number of PCIe controllers */

/**
 * PCIe Address
 */
#define GEUL_PCIE_CPU_ADDRESS_RESV_SIZE(n)  (uint32_t)0x10000000 /**< PCIe CPU Reserved size*/
#define GEUL_PCIE_CFG_SIZE(n)               (uint32_t)0x1000 /**< PCIe CFG size */
#define GEUL_PCIE_MEM_MASK                  (uint32_t)0xfff00000 /**< Mem mask */
#define GEUL_PCIE_CPU_ADDRESS_AVAI_SIZE(n)  (PCIE_CPU_ADDR_SIZE(n) - GEUL_PCIE_CPU_ADDRESS_RESV_SIZE(n))
#define GEUL_PCIE_CPU_ADDRESS_AVAI_TOP(n)   (PCIE_CPU_ADDR(n) + GEUL_PCIE_CPU_ADDRESS_AVAI_SIZE(n))
#define GEUL_PCIE_CFG_BASE(n)               (PCIE_CPU_ADDR(n) + PCIE_CPU_ADDR_SIZE(n) - GEUL_PCIE_CFG_SIZE(n)) /**< PCIe CFG base */
#define GEUL_PCIE_NPMEM_VAL(n)              (((GEUL_PCIE_CPU_ADDRESS_AVAI_TOP(n) - 1) & GEUL_PCIE_MEM_MASK) | \
                                            ((PCIE_CPU_ADDR(n) & GEUL_PCIE_MEM_MASK) >> 16))
#define GEUL_PCIE_PMEM_VAL(n)               (uint32_t)0x0000fff0
#define GEUL_PCIE_PMEM_VAL_UB(n)            (uint32_t)0x0
#define GEUL_PCIE_PMEM_VAL_UL(n)            (uint32_t)0x0
#define GEUL_PCIE_DCTRL_VAL                 0x00002830 /**< Device control*/
#define GEUL_PCIE_CMD_RC                    0x0106 /**< command register */
#define GEUL_PCIE_CMD_EP                    0x0406 /**< command register */
#define GEUL_PCIE_IRQ_BASE                  (41) /**< PCIe IRQ number */
#define GEUL_PCIE_IRQ(n)                    ((n) + GEUL_PCIE_IRQ_BASE) /**< PCIe Ctrl ID */

/**
 * RC address details , which should be accessble by EP device
 * These are the PCIe bus addresses
 */
#define GEUL_PCIE_RC_REGION_PEB_ADDRESS     (uint32_t)0xE0200000
#define GEUL_PCIE_RC_REGION_PEB_SIZE        (uint32_t)0x200000
#define GEUL_PCIE_RC_REGION_HRAM_ADDRESS    (uint32_t)0xE5000000
#define GEUL_PCIE_RC_REGION_HRAM_SIZE       (uint32_t)0x600000

/*
 * Reset
 */
 #define GEUL_PCIE_SRESET_DASSERT  0x80000000
 #define GEUL_PCIE_SRESET_ASSERT   0xC0000000

/**
 * \enum ePciRcMemRegionNxp_t
 * @brief enum to represent RC memory region
 */
typedef enum {
    PCIE_RC_REGION_PEB,  /**< RC Memory Region CCSR */
    PCIE_RC_REGION_HRAM, /**< RC Memory Region DCSR */
    PCIE_RC_REGION_MAX,  /**< RC Memory Region MAX */
}ePciRcMemRegionNxp_t;

/**
 * \enum eModemPciCtrlMode_t
 * @brief enum to represent Mode (EP or RC)
 */
typedef enum {
    PCIE_MODE_EP, /**< EP Mode */
    PCIE_MODE_RC, /**< RC Mode */
}eModemPciCtrlMode_t;

/**
 * @enum eModemPciCtrlId_t
 * @ingroup PCIE_API
 * @brief enum for PCIe controller number
 */
typedef enum {
    PCIE_1, /**< PCIe Controller 1 */
    PCIE_2, /**< PCIe Controller 2 */
    PCIE_MAX, /**< PCIe Controller Max */
}eModemPciCtrlId_t;

/**
 * @enum eModemPciCtrlReset_t
 * @ingroup PCIE_API
 * @brief enum for PCIe controller reset request
 */
typedef enum {
    PCIE_SRESET, /**< Soft Reset */
    PCIE_HRESET, /**< Hard Reset */
}eModemPciCtrlReset_t;

/**
 * @ingroup PCIE_API
 * @enum eModemPciMemRegion_t
 * @brief enum for memory region
 */
typedef enum {
    BAR0, /**< Memory Region BAR0 */
    BAR1, /**< Memory Region BAR1 */
    BAR2, /**< Memory Region BAR2 */
    BAR3, /**< Memory Region BAR3 */
    BAR4, /**< Memory Region BAR4 */
    BAR5, /**< Memory Region BAR5 */
    BAR_MAX, /**< Memory Region MAX */
}eModemPciMemRegion_t;

/**
 * @struct xModemPciMemRegionInfo_t
 * @brief Structure to hold memory region info
 */
typedef struct {
    uint32_t ulAddrS; /**< Start Address */
    uint32_t ulAddrE; /**< End Address */
    uint32_t ulValid; /**< Valid BAR */
    uint32_t ulFlags; /**< Flags */
}xModemPciMemRegionInfo_t;

/**
 * \struct xModemPciDev_t
 * @brief Structure to hold modem info
 */
typedef struct {
    eModemPciCtrlId_t eId; /**< Controller ID */
    eModemPciCtrlMode_t eHdrType; /**< Device type */
    xModemPciMemRegionInfo_t xBarRegion[BAR_MAX]; /**< Memory regions */
    uint8_t  ucEnable; /**< Is enabled */
	uint8_t  ucResetMode;
    void    * pvDevData; /**< PCIe task handler */
    uint32_t ulCurrentMemPos; /**< Current Mem Position */
    uint32_t ulTopMemPos; /**< Top PCIe Mem Position */
    uint32_t ulPmisValue;
    uint32_t ulPcieCfgRegion; /**< Cfg region */
}xModemPciDev_t;


extern xModemPciDev_t xModemPciDev[GEUL_PCIE_NUM_CTRL] __attribute__
									((section (".smem")));

/*!
 * \fn bool_t ulPcieCtrlReady (eModemPciCtrlId_t ucId)
 * @brief Function to check if PCIe controller is ready
 *
 * @param[in]	ucId	PCIe controller ID
 *
 * @return
 *   - On Success, 1
 *   - On Failure, 0
 */
bool_t
ulPcieCtrlReady (eModemPciCtrlId_t ucId);

/*!
 * \fn uint32_t ulPcieReadConfig(eModemPciCtrlId_t ucId, uint32_t ulBusDev,
									uint32_t ulOffset)
 * @brief Function to read config space
 *
 * @param[in]	ucId	PCIe controller ID
 * @param[in]	ulBusDev	BDF
 * @param[in]	ulOffset	offset
 *
 * @return
 *   - configuration read output
 */
uint32_t
ulPcieReadConfig (eModemPciCtrlId_t ucId, uint32_t ulBusDev, uint32_t ulOffset);

/*!
 * \fn void vPcieWriteConfig(eModemPciCtrlId_t ucId, uint32_t ulBusDev,
								uint32_t ulOffset, uint32_t ulValue)
 * @brief Function to write to config space
 *
 * @param[in]	ucId	PCIe controller ID
 * @param[in]	ulBusDev	BDF
 * @param[in]	ulOffset	offset
 * @param[in]	ulValue		value to write
 *
 * @return
 *   - Returns Nothing
 */
void
vPcieWriteConfig (eModemPciCtrlId_t ucId, uint32_t ulBusDev, uint32_t ulOffset,
					uint32_t ulValue);

/*!
 * \fn int32_t iPcieCheckAndInitCtrl(eModemPciCtrlId_t ucId, void * pvDevData)
 * @brief Function to check and Initialize PCIe controller.
 *
 * @param[in]	ucId        PCIe controller ID
 * @param[in]	pvDevData   Represents PCIe task
 *
 * @return
 *   - On Success, pdTRUE
 *   - On Failure, pdFALSE
 */
int32_t
iPcieCheckAndInitCtrl (eModemPciCtrlId_t ucId, void *pvDevData);

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
uint32_t
ulPcieReadMem (eModemPciCtrlId_t ucId, eModemPciMemRegion_t eBarNum,
				uint32_t ulOffset, uint8_t ucSize, uint32_t *pVal);

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
uint32_t
ulPcieWriteMem(eModemPciCtrlId_t ucId, eModemPciMemRegion_t eBarNum,
				uint32_t ulOffset, uint8_t ucSize, uint32_t ulValue);

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
uint32_t
ulPcieGetMemregion(eModemPciCtrlId_t icId, eModemPciMemRegion_t eBarNum,
					uint32_t *pSaddr, uint32_t *pEaddr);

/*!
 * \fn void SetupPcieInboundAtu (uint8_t ucId, uint8_t ucIndex, uint8_t ucType,
									uint8_t ucBarNum, uint32_t ucRegion,
									uint32_t ulTaddr)
 * @brief Function to create inbound window on NXP platforms
 *
 * @param[in]	ucId      PCIe controller ID
 * @param[in]	ucIndex   Index number
 * @param[in]	ucType    Type
 * @param[in]	ucBarNum  Bar Number
 * @param[in]	ucRegion  Region
 * @param[in]	ulTaddr   Target address
 *
 * @return
 *   - Returns Nothing
 */
void
SetupPcieInboundAtu (uint8_t ucId, uint8_t ucIndex, uint8_t ucType,
						uint8_t ucBarNum, uint32_t ucRegion,
						uint32_t ulTaddr);

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
void
SetupPcieOutboundAtu (uint8_t ucId, uint8_t ucIndex, uint8_t ucType,
						uint32_t ucRegion, uint32_t ulSaddr, uint32_t ulTaddr,
						uint32_t ulSize);

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
void
PcieEnableLink (uint8_t ucId, uint8_t ucMode);

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
void
PcieSetBusdev (uint8_t ucId, uint32_t ulBusDev);

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
bool_t
bPcieIsLinkUp (uint8_t ucId);

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
uint32_t
PcieSetupEP (uint8_t ucId, uint8_t ucMode, void *pvDevData);

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
uint32_t
PcieSetupRC (uint8_t ucId, uint8_t ucMode, void *pvDevData);

/*!
 * \fn void  PcieHandleInterrupt (uint8_t ucId)
 * @brief Function to handle PCIe interrupt
 *
 * @param[in]	ucId    PCIe controller ID
 *
 * @return
 *   - Returns Nothing
 */
void
PcieHandleInterrupt (uint8_t ucId);

/*!
 * \fn void  PcieInvokeHostDriver (uint8_t ucId)
 * @brief Function to invoke PCIe EP driver
 *
 * @param[in]	ucId    PCIe controller ID
 *
 * @return
 *   - Returns Nothing
 */
void
PcieInvokeHostDriver (uint8_t ucId);

/*!
 * \fn void  PcieEpHostDriverNxp (uint8_t ucId)
 * @brief Function to invoke NXP's PCIe host driver
 *
 * @param[in]	uint32VidDid    Vid/Did of EP device
 *
 * @return
 *   - Returns Nothing
 */
void
PcieEpHostDriverNxp (uint32_t ucId);

/*!
 * \fn void  PcieControllerReset (uint8_t ucId, uint8_t ucResetMode)
 * @brief Function to reset PCIe controller
 *
 * @param[in]	ucId          PCIe controller ID
 * @param[in]	ucResetMode   Reset Mode (PCIE_SRESET/PCIE_HRESET)
 *
 * @return
 *   - Returns Nothing
 */
void
PcieControllerReset (uint8_t ucId, uint8_t ucResetMode);

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
uint32_t
PcieGetMode (uint8_t ucId);

#endif // LA12XX_DRIVER_PCI
/** @} */
#endif
