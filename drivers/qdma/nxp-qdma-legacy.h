// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#ifndef __QDMA_LEGACY__H
#define __QDMA_LEGACY__H

#include "qdma.h"

#define LEGACY_MODE

#ifdef LEGACY_MODE

#define FAILURE 0

#define QDMA_BASE (0x22C0000 + 0xF8000000)

#define DLMR	(QDMA_BASE + 0x1100)

#define DLSR	(QDMA_BASE + 0x1104)

#define DLSAR	(QDMA_BASE + 0x1114)
#define DLSATR (QDMA_BASE + 0x1110)
#define DLSAR (QDMA_BASE + 0x1114)
#define DLDATR (QDMA_BASE + 0x1118)
#define DLDAR (QDMA_BASE + 0x111C)

#define DLBCR (QDMA_BASE + 0x1120)

#define DLATTR 0x40000000

#define CHANNEL_HALT 0x0
#define CHANNEL_START 0x1

#define QDMA_LEGACY_IDLE 			0x0
#define QDMA_LEGACY_END_OF_SEGMENT		0x2
#define DMA_TRANSFER_IN_PROGRESS		0x4
#define QDMA_LEGACY_TRANSFER_IN_PROGRESS	0x6
#define QDMA_LEGACY_PROGRAMMING_ERROR		0x12
#define QDMA_LEGACY_TRANSFER_HALTED_BY_SOFTWARE 0x22
#define QDMA_LEGACY_TRANSFER_ERROR		0x82
#define QDMA_LEGACY_PROGRAMMING_TRANSFER_ERROR	0x92

#define QDMA_LEGACY_CHANNEL_BITMASK		0x4

#endif

/* QdmaTransferStatus - Check the status of QDMA Legacy mode transfer
 *      Check and return the contents of DLSR
*/
int QdmaTransferStatus(void);
#endif
