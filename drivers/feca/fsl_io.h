// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */

#ifndef __FSL_IO_H_
#define __FSL_IO_H_

#include <stdint.h>
#include <sync.h>
#include <io.h>
#include "ppu_intrinsics.h"

#define STW_SWAP(_val, _base) __stwbrx(_base, _val)
static inline uint32_t ioread32(const volatile uint32_t *addr)
{
    uint32_t ret = (uint32_t)__lwbrx(addr);
//    __asm__ ("msync");
    return ret;
}

static inline void iowrite32(uint32_t val, volatile uint32_t *addr)
{
//	out_le32(addr, val);
    STW_SWAP(val, addr);
//    __asm__ ("msync");
}

static inline void iowrite32be_fast(uint32_t val, volatile uint32_t *addr)
{
	//out_be32(addr, val);
	*addr = val;
 	//__asm__ ("msync");
}

static inline uint32_t ioread32be(const volatile uint32_t *addr)
{
	return in_be32(addr);
}

static inline void iowrite8(uint8_t val, const volatile uint8_t *addr)
{
	out_8(addr, val);
}

static inline uint8_t ioread8(const volatile uint8_t *addr)
{
	return in_8(addr);
}

static inline void core_memory_barrier(void)
{
	sync_dmb();
}


#endif
