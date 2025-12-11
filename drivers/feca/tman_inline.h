// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */

#ifndef __TMAN_INLINE_H_
#define __TMAN_INLINE_H_

#include <stdint.h>
#include <ppc.h>



/* Alternate Time Base */
#define SPR_ATBL        526
#define SPR_ATBU        527

static inline uint64_t mfatb(void)
{
        uint32_t hi, lo, chk;
        do {
                hi = mfspr(SPR_ATBU);
                lo = mfspr(SPR_ATBL);
                chk = mfspr(SPR_ATBU);
        } while (hi != chk);
        return (uint64_t) hi << 32 | (uint64_t) lo;
}

static inline void tman_get_timestamp(uint64_t *timestamp)
{
	// TODO: Read MFTAB - this isn;t working on simulator for some reason
	// *timestamp = mfatb();
        *timestamp = *timestamp + 1;
}


#endif
