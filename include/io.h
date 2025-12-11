// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#ifndef IO_H
#define IO_H

#include <stdio.h>

#define isb()	__asm__("isb" : : : "memory")
#define dsb()	__asm__("dsb" : : : "memory")
#define dmb()	__asm__("dmb" : : : "memory")
#define sync()  __asm__("sync")
#define wfe()	__asm__("wfe" : : : "memory")
#define wfi()	__asm__("wfi" : : : "memory")
#define sev()	__asm__("sev" : : : "memory")

#define SWAP_16(x) \
		((((x) & 0xff00) >> 8) | \
		(((x) & 0x00ff) << 8))
#define SWAP_32(x) \
		((((x) & 0xff000000) >> 24) | \
		(((x) & 0x00ff0000) >> 8) | \
		(((x) & 0x0000ff00) << 8) | \
		(((x) & 0x000000ff) << 24))

#define VAL_le(type, x)			SWAP_##type(x)
#define VAL_be(type, x)			(x)

#define write_8(v, a)			(*(volatile unsigned char *)(a) = (v))
#define write_16(v, a)			(*(volatile unsigned short *)(a) = (v))
#define write_32(v, a)			(*(volatile unsigned int *)(a) = (v))

#define read_8(a)			(*(volatile unsigned char *)(a))
#define read_16(a)			(*(volatile unsigned short *)(a))
#define read_32(a)			(*(volatile unsigned int *)(a))

#define write_val(type, endian, a, v)	write_##type(VAL_##endian(type, v), a)
#define read_val(type, endian, a)	VAL_##endian(type, read_##type(a))
#define read_dspival(type, endian, a)	({	\
					__typeof__(uint32_t) x = (uint32_t)read_##type(a);	\
					(VAL_##endian(type, x));})
#define out_ppc_le32(addr, val)         \
	{__asm__ __volatile__("stwbrx %1,0,%2; sync" : "=m" (*(volatile uint32_t *)addr) \
                             : "r" (val), "r" (addr));}
#define out_ppc_le32_no_sync(addr, val)         \
	{__asm__ __volatile__("stwbrx %1,0,%2" : "=m" (*(volatile uint32_t *)addr) \
                             : "r" (val), "r" (addr));}

#define out_le32(addr, value)		out_ppc_le32(addr, value)

#define in_ppc_le32(addr)    ({unsigned ret;   \
	__asm__ __volatile__("lwbrx %0,0,%1; se_isync"    \
	: "=r" (ret) : "r" (addr), "m" (*addr));ret;})

#define in_le32(addr)			in_ppc_le32(addr)

#define out_be32(addr, value)		write_val(32, be, addr, value)
#define in_be32(addr)			read_val(32, be, addr)
#define in_dspile32(addr)		read_dspival(32, le, addr)

static inline void out_ppc_le16(volatile uint32_t *addr, int val)
{
	__asm__ __volatile__("sthbrx %1,0,%2; sync" : "=m" (*addr)
			     : "r" (val), "r" (addr));
}

#define out_le16(addr, value)		out_ppc_le16(addr, value)
#define in_ppc_le16(addr) ({unsigned ret;   \
        __asm__ __volatile__("lhbrx %0,0,%1; se_isync"  \
	: "=r" (ret) : "r" (addr), "m" (*addr));ret;})
#define in_le16(addr)			in_ppc_le16(addr)

#define out_be16(addr, value)		read_val(16, be, addr, value)
#define in_be16(addr)			write_val(16, be, addr)

#define out_8(addr, value)		write_8(value, addr)
#define in_8(addr)			read_8(addr)

#define clrbits_be32(addr, clear) \
	out_be32((addr), in_be32(addr) & ~(clear))
#define setbits_be32(addr, set) \
	out_be32((addr), in_be32(addr) | (set))

#define clrbits_le32(addr, clear) \
	out_ppc_le32((addr), in_ppc_le32(addr) & ~(clear))
#define setbits_le32(addr, set) \
	out_ppc_le32((addr), in_ppc_le32(addr) | (set))

#define OUT_32	out_le32
#define IN_32	in_le32
#define OUT_16	out_le16
#define IN_16	in_le16
#define OUT_8	out_8
#define IN_8	in_8

#define h2m_32(addr)		( in_le32(addr) )
#define m2h_32(val, addr)	( out_le32(addr, val) )

#endif /* ifndef IO_H */
