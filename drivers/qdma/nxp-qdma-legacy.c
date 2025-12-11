// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include "nxp-qdma-legacy.h"

int QdmaLegacyCopy(BaseType_t src_addr, BaseType_t dest_addr, int block_size)
{
	int ret = FAILURE;

	out_ppc_le32_no_sync((volatile uint32_t *)DLSAR, src_addr);
	out_ppc_le32_no_sync((volatile uint32_t *)DLDAR, dest_addr);
	out_ppc_le32_no_sync((volatile uint32_t *)DLBCR, block_size);
	out_ppc_le32_no_sync((volatile uint32_t *)DLMR, CHANNEL_HALT);
	out_ppc_le32_no_sync((volatile uint32_t *)DLMR, CHANNEL_START);
	while (in_le32((volatile uint32_t *) DLSR) & QDMA_LEGACY_CHANNEL_BITMASK)
		;//Waiting for transfer to complete
	ret = QdmaTransferStatus();
	return ret;
}

void QdmaLegacyInit(void)
{
	out_ppc_le32_no_sync((volatile uint32_t *)DLSATR, DLATTR);
	out_ppc_le32_no_sync((volatile uint32_t *)DLDATR, DLATTR);
}

int QdmaTransferStatus(void)
{

	int err = in_le32((volatile uint32_t *)DLSR);

	switch (err) {
	case QDMA_LEGACY_IDLE:
		log_info("LEGACY_MODE QDMA IDLE: %x\n\r", err);
		break;
	case QDMA_LEGACY_END_OF_SEGMENT:
		log_info("LEGACY_MODE QDMA End Of Segment %x\n\r", err);
		break;
	case DMA_TRANSFER_IN_PROGRESS:
		log_info("LEGACY_MODE DMA Transfer in progress %x\n\r", err);
		break;
	case QDMA_LEGACY_TRANSFER_IN_PROGRESS:
		log_err("LEGACY_MODE QDMA Transfer in Progress %x\n\r", err);
		break;
	case QDMA_LEGACY_PROGRAMMING_ERROR:
		log_err("LEGACY_MODE QDMA Programming ERROR %x\n\r", err);
		break;
	case QDMA_LEGACY_TRANSFER_HALTED_BY_SOFTWARE:
		log_err("LEGACY_MODE QDMA Transfer halted by software %x\n\r", err);
		break;
	case QDMA_LEGACY_TRANSFER_ERROR:
		log_err("LEGACY_MODE QDMA Transfer error %x\n\r", err);
		break;
	case QDMA_LEGACY_PROGRAMMING_TRANSFER_ERROR:
		log_err("LEGACY_MODE QDMA Programming ERROR %x\n\r", err);
		break;
	default:
		log_err("LEGACY_MODE QDMA ERROR %x\n\r", err);
	}
	return err;
}
