/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2022 NXP */

#ifndef _L1C_DMA_H
#define _L1C_DMA_H

#include "ppc.h"
#include "ppu_intrinsics.h"

#define DMA_GO_TO_VCPU		(0x00000001U << 13)
#define DMA_AXI2DMEM		(0x00000000U << 8)

#define L1C_BUF_BASE_ADDR	(0x00000000)

#define L1C_E2V_DMA_CHAN	(31)

#define STW_SWAP(_val, _base) __stwbrx(_base, _val)

static inline void iowrite32(uint32_t val, volatile uint32_t *addr)
{
	STW_SWAP(val, addr);
	__asm__ ("msync");
}

static inline void iowrite32_fast(uint32_t val, volatile uint32_t *addr)
{
	STW_SWAP(val, addr);
}

static inline void iowrite32be(uint32_t val, volatile uint32_t *addr)
{
	*addr = val;
	__asm__ ("msync");
}

static inline uint32_t ioread32_fast(const volatile uint32_t *addr)
{
	uint32_t ret = (uint32_t)__lwbrx(addr);
	return ret;
}

static inline uint32_t ioread32(const volatile uint32_t *addr)
{
	uint32_t ret = (uint32_t)__lwbrx(addr);

	__asm__ ("msync");
	return ret;
}

static inline uint64_t swap_uint64(uint64_t val)
{
	return (uint64_t)__ldbrx(&val);
}

static inline uint32_t swap_uint32(uint32_t val)
{
	return (uint32_t)__lwbrx(&val);
}

static inline int32_t swap_cint32(uint32_t val)
{
	return (int32_t)__lwbrx(&val);
}

static inline uint16_t swap_uint16(uint16_t val)
{
	return (uint16_t)__lhbrx(&val);
}

static inline void vspa_set_irqen(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulVspaIrqEn);
}

static inline void vspa_set_dmareg_irq_stat(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulDmaIrqStat);
}

static inline void vspa_set_vspa_status(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulVspaStatus);
}

static inline void vspa_set_dmareg_comp_stat(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulDmaCompStat);
}

static inline void vspa_set_dmareg_xfer_ctrl(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulDmaXfrCtrl);
}

static inline void vspa_set_dmareg_dmem_addr(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *) VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulDmaDmemPramAddr);
}

static inline void vspa_set_dmareg_axi_addr(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulDmaAxiAddress);
}

static inline void vspa_set_dmareg_size_bytes(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulDmaAxiByteCnt);
}

static inline uint32_t vspa_get_dmareg_fifo_stat(uint32_t vspa_idx, uint32_t dma_ch_mask)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    return (uint32_t)(ioread32(&vspa_regs_p->ulDnaFifoStat) & dma_ch_mask);
}

static inline uint32_t vspa_get_dmareg_irq_stat(uint32_t vspa_idx, uint32_t dma_ch_mask)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    return (uint32_t) (ioread32(&vspa_regs_p->ulDmaIrqStat) & dma_ch_mask);
}

static inline uint32_t vspa_get_dmareg_comp_stat(uint32_t vspa_idx, uint32_t dma_ch_mask)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    return (uint32_t) (ioread32(&vspa_regs_p->ulDmaCompStat) & dma_ch_mask);
}

static inline uint32_t vspa_get_dmareg_stat_abort(uint32_t vspa_idx, uint32_t dma_ch_mask)
{
    VspaRegs_t * vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    return (uint32_t) (ioread32(&vspa_regs_p->ulDmaStatAbort) & dma_ch_mask);
}

static inline uint32_t vspa_get_host_to_vspa_flags0(uint32_t vspa_idx)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    return (uint32_t)ioread32(&vspa_regs_p->ulHostVcpuFlags0);
}

static inline void vspa_set_host_to_vspa_flags0(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulHostVcpuFlags0);
}

static inline uint32_t vspa_get_vspa_to_host_flags0(uint32_t vspa_idx)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    return (uint32_t)ioread32(&vspa_regs_p->ulVcpuHostFlags0);
}

static inline void vspa_set_vspa_to_host_flags0(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32_fast(value, &vspa_regs_p->ulVcpuHostFlags0);
}

static inline void vspa_write_host_to_vspa_mbox0(uint32_t vspa_idx, uint32_t lsb, uint32_t msb)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32(msb, &vspa_regs_p->ulHostOut0Msb);
    iowrite32(lsb, &vspa_regs_p->ulHostOut0Lsb);
}

static inline void vspa_write_host_to_vspa_mbox1(uint32_t vspa_idx, uint32_t lsb, uint32_t msb)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32(msb, &vspa_regs_p->ulHostOut1Msb);
    iowrite32(lsb, &vspa_regs_p->ulHostOut1Lsb);
}

static inline void vspa_wait_host_to_vspa_mbox1(uint32_t vspa_idx)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    while (ioread32(&vspa_regs_p->ulHostMboxStatus) & (1 << 1)) {}
}

static inline void vspa_set_ext_go_stat(uint32_t vspa_idx, uint32_t value)
{
    VspaRegs_t *vspa_regs_p = (VspaRegs_t *)VSPA_INST_BASE_ADDR(vspa_idx);
    iowrite32(value, &vspa_regs_p->ulExtGoStat);
}

#endif
