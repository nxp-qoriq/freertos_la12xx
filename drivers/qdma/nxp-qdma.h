// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef __NXP_QDMA_H
#define __NXP_QDMA_H

#include "qdma.h"

#ifdef ARCH64
#define upper_32_bits(n) ((BaseType_t)(((n) >> 16) >> 16))
#else
#define upper_32_bits(n) 0
#endif
#define lower_32_bits(n) ((uint32_t)(n))
#ifdef configUSE_QDMA_BIGENGINE
#define QDMA_WRITE	out_be32
#define QDMA_READ	in_be32
#else
#define QDMA_WRITE	out_ppc_le32_no_sync
#define QDMA_READ	in_le32

#endif

#define QDMA_QUEUE_SIZE		64
#define QDMA_STATUS_SIZE	64
#define QDMA_BLOCK_OFFSET	0x1000

#define NXP_QDMA_DMR		0x0
#define NXP_QDMA_DSR		0x4
#define NXP_QDMA_DEAGAR0	0x60
#define NXP_QDMA_DEIER		0xE00
#define NXP_QDMA_DEDR		0xE04
#define NXP_QDMA_DECCD0R	0xE10
#define NXP_QDMA_DECCD1R	0xE14
#define NXP_QDMA_DECCD2R	0xE18
#define NXP_QDMA_DECCD3R	0xE1C
#define NXP_QDMA_DECCD4R	0xE20
#define NXP_QDMA_DECCD5R	0xE24
#define NXP_QDMA_DECCD6R	0xE28
#define NXP_QDMA_DECCD7R	0xE2C
#define NXP_QDMA_DECCQIDR	0xE30
#define NXP_QDMA_DECBR		0xE34

#define NXP_QDMA_BCQMR(x)	(0x100 * (x))
#define NXP_QDMA_BCQSR(x)	(0x4  + 0x100 * (x))
#define NXP_QDMA_BCQDPAR(x)	(0x14 + 0x100 * (x))
#define NXP_QDMA_BCQEPAR(x)	(0x1C + 0x100 * (x))
#define NXP_QDMA_BCQIDR(x)	(0xA0 + 0x100 * (x))

#define NXP_QDMA_BSQMR		0x800
#define NXP_QDMA_BSQSR		0x804
#define NXP_QDMA_BSQDPAR	0x814
#define NXP_QDMA_BSQEPAR	0x81C
#define NXP_QDMA_BSQICR0	0x820
#define NXP_QDMA_BSQICR1	0x824
#define NXP_QDMA_BSQIDR		0x8A0
#define NXP_QDMA_BCQDSCR0	0xA10
#define NXP_QDMA_BIER		0xA20
#define NXP_QDMA_BIDR		0xA24
#define NXP_QDMA_BEIER		0xE00
#define NXP_QDMA_BEIDR		0xE04

#define NXP_QDMA_DMR_DQD	BIT(30)
#define NXP_QDMA_DSR_DB		BIT(31)

#define NXP_QDMA_BIDR_CQCIE	(BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15)) 

#define NXP_QDMA_BCQMR_CD_THLD(x)   ((x) << 20)
#define NXP_QDMA_BCQMR_CQ_SIZE(x)   ((x) << 16)
#define NXP_QDMA_BCQMR_CQM(x)		((x) << 28)
#define NXP_QDMA_BCQMR_DRTTYPE(x)	((x) << 12)
#define NXP_QDMA_BCQMR_QOS(x)		((x) << 8)
#define NXP_QDMA_BCQMR_EI			BIT(30)
#define NXP_QDMA_BCQMR_TMS			BIT(26)
#define NXP_QDMA_CQ_EN				BIT(31)
#define NXP_QDMA_CQ_IDLE			BIT(28)

#define NXP_QDMA_BCQSR_QF			BIT(30)
#define NXP_QDMA_BCQSR_XOFF			BIT(1)

#define NXP_QDMA_BSQICR_ICEN		BIT(31)
#define NXP_QDMA_BSQICR_ICST(x)		((x) << 0)
#define NXP_QDMA_BSQICR_CQCE(x)		((x) << 8)

#define NXP_QDMA_BSQMR_EN			BIT(31)
#define NXP_QDMA_BSQMR_DI			BIT(30)
#define NXP_QDMA_BSQMR_SNM(x)		((x) << 28)
#define NXP_QDMA_BSQMR_CQ_SIZE(x)   ((x) << 16)
#define NXP_QDMA_BSQMR_DWTTYPE(x)   ((x) << 12)
#define NXP_QDMA_BSQMR_QOS(x)		((x) << 8)

#define NXP_QDMA_BSQSR_SQE			BIT(31)
#define NXP_QDMA_BASE_OFFSET(x) \
	((QDMA_BLOCK_OFFSET) * NXP_QDMA_BLOCK_NUM(x))

#define NXP_QDMA_BLOCK_IRQ(x)		((45) + NXP_QDMA_BLOCK_NUM(x))
#define NXP_QDMA_ERROR_IRQ		50

#define QDMA_DQOS1(x)				(((x) & GENMASK(15,13)) >> 13)
#define QDMA_DLWC1(x)				(((x) & GENMASK(11,10)) >> 10)
#define QDMA_RDTTYPE1(x)			(((x) & GENMASK(7,4)) >> 4)
#define QDMA_SQOS1(x)				(((x) & GENMASK(3,1)) >> 1)

#define COMMAND_QUEUE_NUMBER(x)	(((x) & GENMASK(6,4)) >> 4)
#define QDMA_CQN(x)			((x) << 4)
#define QDMA_WRTTYPE(x)			((x) << 12)
#define QDMA_DQOS(x)			((x) << 16)
#define QDMA_DLWC(x)			((x) << 20)
#define QDMA_RDTTYPE(x)			((x) << 24)
#define QDMA_SQOS(x)			((x) << 28)

#define QDMA_DF_SER			BIT(8)
#define QDMA_DF_EOL			BIT(9)
#define QDMA_DF_SO			BIT(10)
#define QDMA_DF_WNS			BIT(11)
#define QDMA_DF_DRBP		BIT(19)
#define QDMA_DF_CI			BIT(22)
#define QDMA_DF_RNS			BIT(23)
#define QDMA_DF_SRBP		BIT(31)

#define QDMA_CLT_F				BIT(31)
#define QDMA_CLT_SL				BIT(30)
#define QDMA_CLT_SG				BIT(29)
#define QDMA_CLT_OFFSET(x)		((((x) << 16) & GENMASK(27,16)))

#define QDMA_DF_SL			BIT(30)
#define QDMA_DF_SHORT_FMT	BIT(29) | BIT(28)
#define QDMA_DF_LONG_FMT	BIT(28)

#define WRTTYPE_RDTTYPE_COHERENT 0xb
#define QDMA_LAST_WRITE_CONTROL	16

#define QDMA_SGT_F				BIT(31)
#define QDMA_SGT_SL				BIT(30)
#define QDMA_SGT_EXT			BIT(29)

/*
 * Queue Memeory needs to align at boundary of number of CDs * CD size
 * which equates to 64 * 256 bits
 */
#define QDMA_ADDR_ALIGNEMENT (uint32_t)(64 * 256 / 8)

#define NXP_QDMA_BLOCK_COUNT	4

typedef struct NXP_QDMA_ENGINE
{
	void		*CtrlBase;
	void		*StatusBase;
	void		*BlockBase;
	uint32_t	bcqmr;
	uint8_t		cqm;
	uint8_t		nQueues;
	NxpQdmaQueue_t	QueueHead[NXP_QDMA_QUEUE_NUM_MAX];
	NxpQdmaQueue_t	*Status;
	DmaCallback	ErrorCallback;
	void		*ErrorParams;
} NxpQdmaEngine_t;
NxpQdmaEngine_t *NxpQdma;
NxpQdmaEngine_t *NxpQdmaC[NXP_QDMA_BLOCK_COUNT] __attribute__((section(".smem")));

typedef struct NXP_VALID_QDMA_BLK
{
	uint8_t ucValidBlk;
	uint8_t ucValidQueue;
	DescriptorFormat_t * DescStatus;
	DescriptorFormat_t * DescCmd;
} NxpValidQdmaBlk_t;
NxpValidQdmaBlk_t NxpValidQdmaBlk[NXP_QDMA_BLOCK_COUNT] __attribute__((section(".smem")));

static inline uint32_t Ilog2(uint32_t x)
{
	uint32_t log = 0;
	x >>= 1;

	while(x)
	{
		log++;
		x >>= 1;
	}

	return log;
}
#endif /* __NXP_QDMA_H */
