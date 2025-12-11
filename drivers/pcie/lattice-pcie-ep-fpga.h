// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2024 NXP
 */

#ifndef _LATTICE_PCI_EP_FPGA_H
#define _LATTICE_PCI_EP_FPGA_H

#define PCIE_CMD_REG_OFFSET (0x4)
#define PCIE_EP_LAT_DEV_CFG (0x48)
#define PCIE_EP_DEV_CFG_VAL (0x2830)

typedef struct ctrl_reg 
{
 uint32_t TOTAL_RD_SIZE_ADDR; // 0x00
 uint32_t TXD_ADDR; // 0x04
 uint32_t TBCNT; // 0x08
 uint32_t TX_DMA_EN; // 0x0C
 uint32_t TOTAL_WR_SIZE_ADDR; // 0x10
 uint32_t RXD_ADDR; // 0x14
 uint32_t RBCNT; // 0x18
 uint32_t RX_DMA_EN; // 0x1C
 uint32_t RSV_00; // 0x20
 uint32_t RSV_01; // 0x24
 uint32_t RSV_02; // 0x28
 uint32_t RSV_03; // 0x2C
 uint32_t RSV_04; // 0x30
 uint32_t RSV_05; // 0x34
 uint32_t RSV_06; // 0x38
 uint32_t TX_RX_STATUS; // 0x3C
 uint32_t DBG_SCRATCHPAD; // 0x40
} xlatCtrlReg;

#endif /* _LATTICE_PCI_EP_FPGA_H */