// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */
#include <geul_qdma.h>
#ifndef __QDMA_LEGACY_H
#define __QDMA_LEGACY_H

#define BLOCK_SIZE 0x1000

/* QdmaLegacyInit - Begin initialisation to enable qdma transfer
 * using legacy mode
 *
 *      1. Initialize DLSATR and DLDATR.
*/
void QdmaLegacyInit(void);

/* QdmaLegacyCopy - Begin Copy transfer from src_addr to dest_addr
 *      1. Initialize DLSAR, DLDAR, DLBCR.
 *      2. Clear, then set the mode register channel start bit,
 *         DLMR[CS], to start the DMA transfer.
 *      3. Wait for transfer to complete.
 *      4. Return the value of DLSR.
*/
int QdmaLegacyCopy(BaseType_t src_addr, BaseType_t dest_addr, int block_size);
#endif
