// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "FreeRTOS.h"
#include "immap.h"
#include "list.h"
#include "nxp-qdma.h"
#include "bit.h"
#include <ppc.h>
#include "ppu_intrinsics.h"
#include <errno.h>

DescriptorFormat_t qdma_command_queue_memory[NXP_QDMA_TOTAL_QUEUE][QDMA_QUEUE_SIZE]
	__attribute__ ((section (".shared.bss"))) __attribute__ ((aligned (QDMA_ADDR_ALIGNEMENT)));
DescriptorFormat_t qdma_status [NXP_QDMA_TOTAL_BLK_IN_USE][QDMA_STATUS_SIZE]
	__attribute__ ((section (".shared.bss"))) __attribute__ ((aligned (QDMA_ADDR_ALIGNEMENT)));

#ifdef HW_PERF
struct Time QdmaTestStartTimeHw;
uint32_t TimeDiffInUsHw;
#endif

void vFillQdmaValidBlk()
{
	DescriptorFormat_t * TempStatus = qdma_status[0];
	DescriptorFormat_t * TempCmd = qdma_command_queue_memory[0];
	NxpValidQdmaBlk[0].ucValidBlk = NXP_QDMA_BLK0_IN_USE;
	NxpValidQdmaBlk[0].ucValidQueue = NXP_QDMA_BLK0_QUEUE;
	NxpValidQdmaBlk[0].DescStatus = NULL;
	NxpValidQdmaBlk[0].DescCmd = NULL;
	if( NXP_QDMA_BLK0_IN_USE ) {
		NxpValidQdmaBlk[0].DescStatus = TempStatus;
		TempStatus +=  QDMA_STATUS_SIZE;
		NxpValidQdmaBlk[0].DescCmd = TempCmd;
		TempCmd +=  (NxpValidQdmaBlk[0].ucValidQueue * QDMA_QUEUE_SIZE);
	}

	NxpValidQdmaBlk[1].ucValidBlk = NXP_QDMA_BLK1_IN_USE;
	NxpValidQdmaBlk[1].ucValidQueue = NXP_QDMA_BLK1_QUEUE;
	NxpValidQdmaBlk[1].DescStatus = NULL;
	NxpValidQdmaBlk[1].DescCmd = NULL;
	if( NXP_QDMA_BLK1_IN_USE ) {
		NxpValidQdmaBlk[1].DescStatus = TempStatus ;
		TempStatus +=  QDMA_STATUS_SIZE;
		NxpValidQdmaBlk[1].DescCmd = TempCmd;
		TempCmd +=  (NxpValidQdmaBlk[1].ucValidQueue * QDMA_QUEUE_SIZE);
	}

	NxpValidQdmaBlk[2].ucValidBlk = NXP_QDMA_BLK2_IN_USE;
	NxpValidQdmaBlk[2].ucValidQueue = NXP_QDMA_BLK2_QUEUE;
	NxpValidQdmaBlk[2].DescStatus = NULL;
	NxpValidQdmaBlk[2].DescCmd = NULL;
	if( NXP_QDMA_BLK2_IN_USE ) {
		NxpValidQdmaBlk[2].DescStatus = TempStatus ;
		TempStatus +=  QDMA_STATUS_SIZE;
		NxpValidQdmaBlk[2].DescCmd = TempCmd;
		TempCmd +=  (NxpValidQdmaBlk[2].ucValidQueue * QDMA_QUEUE_SIZE);
	}

	NxpValidQdmaBlk[3].ucValidBlk = NXP_QDMA_BLK3_IN_USE;
	NxpValidQdmaBlk[3].ucValidQueue = NXP_QDMA_BLK3_QUEUE;
	NxpValidQdmaBlk[3].DescStatus = NULL;
	NxpValidQdmaBlk[3].DescCmd = NULL;
	if( NXP_QDMA_BLK3_IN_USE ) {
		NxpValidQdmaBlk[3].DescStatus = TempStatus ;
		NxpValidQdmaBlk[3].DescCmd = TempCmd;
	}
}

static inline void
DescAddrSet(DescriptorFormat_t *DescFmt, BaseType_t Addr)
{
	DescFmt->LowAddrBase = (BaseType_t)VAL_le(32, lower_32_bits(Addr));
	DescFmt->HighAddrBase = upper_32_bits(Addr);
}

static inline uint32_t
DescAddrQueueGet(DescriptorFormat_t *DescFmt)
{
	return COMMAND_QUEUE_NUMBER(VAL_le(32, DescFmt->Cfg2));
}

static inline void
Fill_SGTable(ScatterGatherTableFormat_t *List, BaseType_t Addr,
	size_t Len, bool Fill)
{
	List->LowAddrBase = (BaseType_t)VAL_le(32, lower_32_bits(Addr));
	List->HighAddrBase = upper_32_bits(Addr);
	List->DataLen = (uint32_t)VAL_le(32, Len);
	if (Fill)
		List->Cfg = VAL_le(32, QDMA_SGT_F);
}

static inline void
sDescAddrSet(DescriptorFormat_t *DescFmt, BaseType_t Addr)
{
	DescFmt->LowAddrBase = (BaseType_t)VAL_le(32, lower_32_bits(Addr));
	DescFmt->HighAddrBase = upper_32_bits(Addr);
}

static inline void
dDescAddrSet(DescriptorFormat_t *DescFmt, BaseType_t Addr)
{
	DescFmt->dLowAddrBase = (BaseType_t)VAL_le(32, lower_32_bits(Addr));
	DescFmt->dHighAddrBase = upper_32_bits(Addr);
}

static inline void
DescAddrLengthSet(DescriptorFormat_t *DescFmt, size_t Len)
{
	DescFmt->DataLen = (uint32_t)VAL_le(32, Len);
}

static inline void
DescAddrAttrSet(DescriptorFormat_t *DescFmt, uint32_t Val)
{
	DescFmt->Cfg1 = VAL_le(32, Val);
}

static inline void
DescAddrSerSet(DescriptorFormat_t *DescFmt)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_DF_SER);
}

static inline void
DescAddrEolSet(DescriptorFormat_t *DescFmt)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_DF_EOL);
}

static inline void
DescAddrSoSet(DescriptorFormat_t *DescFmt)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_DF_SO);
}

static inline void
DescAddrWrtypeSet(DescriptorFormat_t *DescFmt, uint32_t Val)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_WRTTYPE(Val));
}

static inline void
DescAddrSrbpSet(DescriptorFormat_t *DescFmt)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_DF_SRBP);
}

static inline void
DescAddrDrbpSet(DescriptorFormat_t *DescFmt)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_DF_DRBP);
}

static inline void
DescAddrCiSet(DescriptorFormat_t *DescFmt)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_DF_CI);
}

static inline void
DescAddrRdtypeSet(DescriptorFormat_t *DescFmt, uint32_t Val)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_RDTTYPE(Val));
}

static inline void
DescAddrCqnSet(DescriptorFormat_t *DescFmt, uint32_t Val)
{
	DescFmt->Cfg2 |= VAL_le(32, QDMA_CQN(Val));
}

inline void
QdmaDescDump(uint32_t *addr, uint32_t len)
{
	uint32_t i = 0;
	uint32_t *tempaddr __attribute__((unused));

	tempaddr = (uint32_t *)(addr);
	log_info("Cd addr: %x\r\n", tempaddr);
	for (i = 0; i < len; i++)
		log_info("id: %u \t val: %x\r\n",
				i, SWAP_32(*(tempaddr + i)));
}

void
QdmaSgEntriesDump(ScatterGatherTableFormat_t *SgTableHead, uint32_t SgNum)
{
	uint32_t i = 0;
	uint32_t *tempaddr;

	for (i = 0; i < SgNum; i++) {
		tempaddr = (uint32_t *)&SgTableHead[i];
		log_info("sgSrc Entry[%u]\r\n", i);
		QdmaDescDump(tempaddr, 4);
	}
}

void
QdmaCltSgCdDump(NxpQdmaCltCd_t *NxpCltSgCd)
{
	uint32_t *tempaddr;

	if (!NxpCltSgCd) {
		log_err("qDMA Invalid NxpCltSgCd \r\n");
		return;
	}

	tempaddr = (uint32_t *)&NxpCltSgCd->Cd;
	log_info("Cd addr: %x\r\n", tempaddr);
	QdmaDescDump(tempaddr, 8);

	tempaddr = (uint32_t *)&NxpCltSgCd->NxpClt.CmdListTable;
	log_info("CmdListTable addr: %x\r\n", tempaddr);
	QdmaDescDump(tempaddr, 8);

	tempaddr = (uint32_t *)&NxpCltSgCd->NxpClt.sCmdListTable;
	log_info("sCmdListTable addr: %x\r\n", tempaddr);
	QdmaDescDump(tempaddr, 8);

	tempaddr = (uint32_t *)&NxpCltSgCd->NxpClt.dCmdListTable;
	log_info("dCmdListTable addr: %x\r\n", tempaddr);
	QdmaDescDump(tempaddr, 8);

	tempaddr = (uint32_t *)&NxpCltSgCd->NxpClt.SrcDescFmt;
	log_info("Source Descriptor(SD) addr: %x\r\n", tempaddr);
	QdmaDescDump(tempaddr, 4);
	tempaddr = (uint32_t *)&NxpCltSgCd->NxpClt.DstDescFmt;
	log_info("Destination Descriptor(DD) addr: %x\r\n", tempaddr);
	QdmaDescDump(tempaddr, 4);
}

static inline int
QdmaUSFDescFill(DescriptorFormat_t *DescFmtTable, BaseType_t Dst,
		BaseType_t Src, size_t Len, uint32_t cqn_id, uint32_t rbp)
{
	uint8_t srbp_en = (uint8_t)rbp & 0x1;
	uint8_t drbp_en = (uint8_t)rbp & 0x2;

	if (unlikely(!DescFmtTable)) {
		errno = ENODATA;
		log_err("Error: %s\r\n", strerror(errno));
		return -ENODATA;
	}

	memset(DescFmtTable, 0, (sizeof(DescriptorFormat_t)));
	sDescAddrSet(DescFmtTable, Src);
	dDescAddrSet(DescFmtTable, Dst);
	DescAddrLengthSet(DescFmtTable, Len);
	DescAddrAttrSet(DescFmtTable, QDMA_DF_SHORT_FMT);

	if (srbp_en)
		DescAddrSrbpSet(DescFmtTable);
	else
		DescAddrRdtypeSet(DescFmtTable,
				  WRTTYPE_RDTTYPE_COHERENT);
	if (drbp_en)
		DescAddrDrbpSet(DescFmtTable);
	else
		DescAddrWrtypeSet(DescFmtTable,
				  WRTTYPE_RDTTYPE_COHERENT);

	DescAddrEolSet(DescFmtTable);
	DescAddrSerSet(DescFmtTable);
	DescAddrCiSet(DescFmtTable);
	DescAddrSoSet(DescFmtTable);
	DescAddrCqnSet(DescFmtTable, cqn_id);

	return 0;
}

void QdmaFillLongCdDesc(DescriptorFormat_t *DescFmtTable, BaseType_t clt_addr)
{
	if (!DescFmtTable)
		return;

	memset(DescFmtTable, 0, (sizeof(DescriptorFormat_t)));
	DescAddrSet(DescFmtTable, clt_addr);
	DescAddrAttrSet(DescFmtTable, QDMA_DF_LONG_FMT);
	DescAddrSerSet(DescFmtTable);
	DescAddrCiSet(DescFmtTable);
	DescAddrSoSet(DescFmtTable);
}

void QdmaCltDstEnableStride(NxpQdmaCLT_t *NxpClt, BaseType_t dst_addr,
			    size_t dLen, uint32_t stride_size)
{
	DescAddrSet(&NxpClt->dCmdListTable, dst_addr);
	DescAddrLengthSet(&NxpClt->dCmdListTable, dLen);

	/*Set stride enable*/
	NxpClt->DstDescFmt.Cmd |= VAL_le(32, (uint32_t)1 << 19);

	/* Set stride size
	 * Stride distance unchanged,  value : 0 implying address hold*/
	NxpClt->DstDescFmt.StrideWay = VAL_le(32, (uint32_t)stride_size << 12);
}

void QdmaCltDst(NxpQdmaCLT_t *NxpClt, BaseType_t dst_addr,
			    size_t dLen)
{
	DescAddrSet(&NxpClt->dCmdListTable, dst_addr);
	DescAddrLengthSet(&NxpClt->dCmdListTable, dLen);
}

void
QdmaDescAddrSet(DescriptorFormat_t *DescFmt, BaseType_t Addr)
{
	DescAddrSet(DescFmt, Addr);
}

void QdmaFillCltDesc(NxpQdmaCLT_t *NxpClt, size_t dLen, size_t sLen,
		     uint32_t rbp)
{
	uint8_t srbp_en = (uint8_t)rbp & 0x1;
	uint8_t drbp_en = (uint8_t)rbp & 0x2;

	if (!NxpClt)
		return;

	memset(NxpClt, 0, (sizeof(NxpQdmaCLT_t)));

	DescAddrSet(&NxpClt->CmdListTable,
		    (BaseType_t)&NxpClt->SrcDescFmt);
	DescAddrLengthSet(&NxpClt->CmdListTable, 32);

	DescAddrLengthSet(&NxpClt->sCmdListTable, sLen);
	DescAddrAttrSet(&NxpClt->sCmdListTable, QDMA_CLT_SG);

	DescAddrLengthSet(&NxpClt->dCmdListTable, dLen);
	DescAddrAttrSet(&NxpClt->dCmdListTable, QDMA_CLT_F);

	if(srbp_en)
		NxpClt->SrcDescFmt.Cmd = VAL_le(32, (uint32_t)1 << 18);
	else
		NxpClt->SrcDescFmt.Cmd = VAL_le(32,
				(uint32_t)(WRTTYPE_RDTTYPE_COHERENT << 28));

	if(drbp_en)
		NxpClt->DstDescFmt.Cmd = VAL_le(32, (uint32_t)1 << 18);
	else
		NxpClt->DstDescFmt.Cmd = VAL_le(32,
				(uint32_t)((WRTTYPE_RDTTYPE_COHERENT << 28)));
}

void
QdmaFillDstCltDesc(NxpQdmaCLT_t *NxpClt, BaseType_t Dst, size_t Len, uint32_t Cfg)
{
	DescAddrSet(&NxpClt->dCmdListTable, Dst);
	DescAddrLengthSet(&NxpClt->dCmdListTable, Len);
	DescAddrAttrSet(&NxpClt->dCmdListTable, Cfg);
}

void
QdmaFillSGEntries(ScatterGatherTableFormat_t *NxpSG,
		  BaseType_t *DataArr, size_t *LenArr, uint32_t SgNum)
{
	bool Fill;
	uint32_t i;

	if (!NxpSG || !DataArr || !LenArr)
		return;

	memset(NxpSG, 0, (sizeof(ScatterGatherTableFormat_t) * SgNum));

	for (i = 0; i < SgNum; i++) {
		Fill = ((i == (SgNum - 1)) ? 1 : 0);
		Fill_SGTable(NxpSG, DataArr[i], LenArr[i], Fill);
		NxpSG++;
	}
}

BaseType_t
QdmaFillCltLongCdSG(NxpQdmaCltCd_t *NxpCltSgCd,
		    uint32_t TotalDstLength, uint32_t TotalSrcLength,
			uint32_t rbp)
{
	if (!NxpCltSgCd) {
		log_err("qDMA Invalid pointers \r\n");
		return pdFAIL;
	}

	/*Prepare CLT Descriptor*/
	QdmaFillCltDesc(&NxpCltSgCd->NxpClt,
			  TotalDstLength, TotalSrcLength, rbp);

	/*Prepare CD Descriptor*/
	QdmaFillLongCdDesc(&NxpCltSgCd->Cd,
			   (BaseType_t)(&NxpCltSgCd->NxpClt));
	return pdPASS;
}

BaseType_t
QdmaInitCltLongCdSG(BaseType_t NxpCltSgCd_addr)
{
	NxpQdmaCltCd_t *NxpCltSgCd =
		(NxpQdmaCltCd_t *)NxpCltSgCd_addr;

	if (!NxpCltSgCd) {
		log_err("qDMA Invalid pointers \r\n");
		return pdFAIL;
	}

	QdmaFillCltDesc(&NxpCltSgCd->NxpClt,
			  TX_TB_SG_TOTAL_SIZE, TX_TB_SG_TOTAL_SIZE,
			  TX_TB_RBP);

	QdmaFillLongCdDesc(&NxpCltSgCd->Cd,
			   (BaseType_t)(&NxpCltSgCd->NxpClt));
	return pdPASS;
}

static inline BaseType_t NxpQdmaAllocQueueResource(uint32_t uBlk, uint32_t nCq)
{
	uint32_t j;

	if (uBlk >= NXP_QDMA_BLOCK_COUNT || !NxpQdma)
		return pdFAIL;

	for (j = 0; j < nCq; j++) {
		NxpQdma->QueueHead[j].Cq = NxpValidQdmaBlk[uBlk].DescCmd + j * QDMA_QUEUE_SIZE;

		memset(NxpQdma->QueueHead[j].Cq, 0, sizeof(DescriptorFormat_t) * QDMA_QUEUE_SIZE);
#ifdef DEBUG
		log_info("Queue[%x]->Cq 0x%x\r\n", j, (uint32_t)NxpQdma->QueueHead[j].Cq);
#endif
		NxpQdma->QueueHead[j].BlockBase =
			(void *)((uint32_t)NxpQdma->BlockBase +
				NXP_QDMA_BASE_OFFSET(uBlk));
		NxpQdma->QueueHead[j].Id = (uint8_t)j;
		NxpQdma->QueueHead[j].DescHead = NxpQdma->QueueHead[j].Cq;
		NxpQdma->QueueHead[j].DescTail = NxpQdma->QueueHead[j].Cq +
						QDMA_QUEUE_SIZE;
		NxpQdma->QueueHead[j].Callback = NULL;
	}

	return pdPASS;
}

static NxpQdmaQueue_t *NxpQqdmaAllocStatusResource(uint32_t uBlk)
{
	if( uBlk >= NXP_QDMA_BLOCK_COUNT || !NxpValidQdmaBlk[uBlk].ucValidBlk ) {
		log_err("QDMA Blk %u is disabled\r\n", uBlk);
		return NULL;
	}
	NxpQdmaQueue_t *StatusHead = pvGeulMalloc(sizeof(*StatusHead));

	if (!StatusHead)
		return NULL;

	StatusHead->Cq = NxpValidQdmaBlk[uBlk].DescStatus;
#ifdef DEBUG
	log_info("StatusQueue->Cq 0x%x\r\n",(uint32_t)StatusHead->Cq);
#endif
	memset(StatusHead->Cq, 0, sizeof(DescriptorFormat_t) * QDMA_STATUS_SIZE);

	if (!StatusHead->Cq) {
		vGeulFree(StatusHead);
		return NULL;
	}

	StatusHead->DescHead = StatusHead->Cq;
	StatusHead->DescTail = StatusHead->Cq + QDMA_STATUS_SIZE;

	return StatusHead;
}

static void QdmaDelay(int Count)
{
	volatile int i;
	volatile int j;

	j = 10000;

	while (j--) {
		i = Count;
		while (i--)
			;
	}
}

static int WaitQueueComplete(BaseType_t Base, uint32_t Val)
{
	uint32_t Reg;
	int Count;

	Count = 5;

	while (1) {
		Reg = QDMA_READ((volatile uint32_t *) Base);
		if (!(Reg & Val))
			break;
		else if (Count-- < 0)
			return -1;
		else
			QdmaDelay(20);
	}
	return 0;
}

static BaseType_t NxpQdmaHalt(NxpQdmaEngine_t *NxpQdma, uint32_t core_id)
{
	BaseType_t	Ctrl;
	BaseType_t	Block;
	uint32_t	j;
	uint32_t	Reg;

	Ctrl = (BaseType_t)NxpQdma->CtrlBase;

	Block = (BaseType_t)NxpQdma->BlockBase +
		(BaseType_t)NXP_QDMA_BASE_OFFSET(core_id);

	for (j = 0; j < NXP_QDMA_QUEUE_NUM_MAX; j++) {
		Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQMR(j)));
		Reg &= ~NXP_QDMA_CQ_EN;
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQMR(j)), Reg);
	}

	Reg = QDMA_READ((volatile uint32_t *) (Ctrl + (BaseType_t)NXP_QDMA_DMR));
	Reg |= NXP_QDMA_DMR_DQD;
	QDMA_WRITE((volatile uint32_t *) (Ctrl + (BaseType_t)NXP_QDMA_DMR), Reg);

	WaitQueueComplete(Ctrl + (BaseType_t)NXP_QDMA_DSR, NXP_QDMA_DSR_DB);

	Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR));
	Reg &= ~NXP_QDMA_BSQMR_EN;
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR), Reg);

	return pdPASS;
}

static int NxpQdmaRegInit(NxpQdmaEngine_t *NxpQdma, uint32_t core_id,
			  uint8_t cqm, uint32_t ag_val, uint16_t ag_engine)
{
	NxpQdmaQueue_t *Temp;
	BaseType_t Ctrl;
	BaseType_t StatusBase;
	BaseType_t Block;
	uint32_t Reg;
	uint32_t j;
	BaseType_t Ret;

	Ctrl = (BaseType_t)NxpQdma->CtrlBase;
	StatusBase = (BaseType_t)NxpQdma->StatusBase;
	Temp = NxpQdma->QueueHead;

	Ret = NxpQdmaHalt(NxpQdma, core_id);
	if (!Ret) {
		log_err("DMA halt failed!\r\n");
		return Ret;
	}

	if (ag_val != 0)
		log_info("AG support will be provided in future release \r\n");

	Block = (BaseType_t)NxpQdma->BlockBase +
			(BaseType_t)NXP_QDMA_BASE_OFFSET(core_id);
	for (j = 0; j < NxpQdma->nQueues; j++) {
		Reg = NXP_QDMA_BCQMR_CQM((uint32_t)cqm);
		Reg |= NXP_QDMA_BCQMR_CQ_SIZE(Ilog2(QDMA_QUEUE_SIZE) - 6);
		Reg |= NXP_QDMA_BCQMR_DRTTYPE(WRTTYPE_RDTTYPE_COHERENT);
		Reg |= NXP_QDMA_BCQMR_QOS(0x0);
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQMR(j)), Reg);

		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQEPAR(j)),
			   (uint32_t)Temp->Cq);
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQDPAR(j)),
			   (uint32_t)Temp->Cq);
		Temp++;
	}

	//Reg = NXP_QDMA_BSQICR_ICEN;
	//Reg |= NXP_QDMA_BSQICR_ICST(3);
	Reg = NXP_QDMA_BSQICR_CQCE(0xff);
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQICR0), Reg);

	Reg = NXP_QDMA_BSQMR_SNM(0x0);
	Reg |= NXP_QDMA_BSQMR_CQ_SIZE(Ilog2(QDMA_STATUS_SIZE) - 6);
	Reg |= NXP_QDMA_BSQMR_DWTTYPE(WRTTYPE_RDTTYPE_COHERENT);
	Reg |= NXP_QDMA_BSQMR_QOS(0x0);
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR), Reg);

	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQEPAR),
		   (uint32_t)NxpQdma->Status->Cq);
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQDPAR),
		   (uint32_t)NxpQdma->Status->Cq);

	QDMA_WRITE((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DEIER), 0xf8000000);
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BEIER), (uint32_t)0x300ffff);
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BIER), (uint32_t)0xff00);

	if(ag_val & 0x80000000) {
		ag_val &= 0x7fffffff;
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQDSCR0), (uint32_t)ag_val);
		Reg = ag_engine;
		QDMA_WRITE((volatile uint32_t *) (Ctrl + (BaseType_t)NXP_QDMA_DEAGAR0), Reg);
	}
	Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR));
	Reg |= NXP_QDMA_BSQMR_EN;
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR), Reg);

	for (j = 0; j < NxpQdma->nQueues; j++) {
		Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQMR(j)));
		Reg |= NXP_QDMA_CQ_EN;
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQMR(j)), Reg);
		NxpQdma->bcqmr = Reg | NXP_QDMA_BCQMR_EI;
	}

	QDMA_WRITE((volatile uint32_t *) (Ctrl + (BaseType_t)NXP_QDMA_DMR), 0x0);
	return pdPASS;
}

static inline void NxpCmdQueueEnqueue(NxpQdmaEngine_t *qdma, BaseType_t Block, uint32_t QueueId)
{
	uint32_t Reg;

	Reg = qdma->bcqmr;
#ifdef HW_PERF
	vGetCurrentTime(&QdmaTestStartTimeHw);
#endif
    QDMA_WRITE( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BCQMR( QueueId ) ), Reg );
}

NxpQdmaQueue_t *qDMA_get_clt_addr(uint32_t QueueId)
{
	return &NxpQdma->QueueHead[ QueueId ];
}

void NxpCmdQueueEnqueueExt( uint32_t QueueId )
{
    BaseType_t Block = ( BaseType_t ) ( NxpQdma->QueueHead[ QueueId ].BlockBase );
    uint32_t Reg;

    NxpQdma->QueueHead[ QueueId ].index++;

   if (NxpQdma->QueueHead[ QueueId ].index >= QDMA_QUEUE_SIZE)
		NxpQdma->QueueHead[ QueueId ].index = 0;

    Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BCQMR( QueueId ) ) );
    Reg |= NXP_QDMA_BCQMR_EI;
	QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQMR(QueueId)), Reg);
}

static BaseType_t NxpQdmaQueueTransferComplete(NxpQdmaEngine_t *NxpQdma,
					       BaseType_t Block)
{
	NxpQdmaQueue_t	*NxpStatus = NxpQdma->Status;
	DescriptorFormat_t *StatusAddr;
	uint32_t Reg;
	uint32_t i;
	while(1)
	{
		portDISABLE_INTERRUPTS();
		Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQSR));
		if (Reg & NXP_QDMA_BSQSR_SQE) {
			portENABLE_INTERRUPTS();
			return pdPASS;
		}
		portENABLE_INTERRUPTS();

		vL1DCacheInvLine((uint32_t)NxpQdma->Status,
			      sizeof(NxpQdmaQueue_t));
		vL1DCacheInvLine((uint32_t)NxpStatus->DescHead,
			      sizeof(DescriptorFormat_t));

		StatusAddr = NxpStatus->DescHead;

#ifdef DEBUG
		for( i = 0; i < 32; i += 4 )
			log_info("val:%x\t id:%d\n\r",
				 SWAP_32(*(uint32_t *)
					((uint32_t)StatusAddr + i)),
				 i);
#endif

		portDISABLE_INTERRUPTS();
		NxpStatus->DescHead++;

		if ( NxpStatus->DescHead == NxpStatus->DescTail)
			NxpStatus->DescHead = NxpStatus->Cq;
		Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR));
		Reg |= NXP_QDMA_BSQMR_DI;
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BSQMR), Reg);
		portENABLE_INTERRUPTS();

#if DEBUG
            Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQEPAR ) );
            log_info( "NXP_QDMA_BSQEPAR %x\t %d\t%s\r\n", Reg,
                      __LINE__, __func__ );
            Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQDPAR ) );
            log_info( "NXP_QDMA_BSQDPAR %x\t %d\t%s\r\n", Reg,
                      __LINE__, __func__ );
#endif
		i = DescAddrQueueGet(StatusAddr);

		/*For performance optimization, there should be minimal
		 * flag update in this function.
         */
        if( NxpQdma->QueueHead[ i ].Callback )
        {
            NxpQdma->QueueHead[ i ].Callback( NxpQdma->QueueHead[ i ].Params,
                                              ( uint32_t ) ( ( DescriptorFormat_t * ) ( StatusAddr )->LowAddrBase ) );
        }
    }
}

void NxpQdmaProcessStatusQueue( void )
{
    NxpQdmaQueue_t * NxpStatus = NxpQdma->Status;
    DescriptorFormat_t * StatusAddr;
    BaseType_t Block;
    uint32_t Reg;
    uint32_t i;

    Block = ( BaseType_t ) NxpQdma->BlockBase +
            ( BaseType_t ) NXP_QDMA_BASE_OFFSET( ulMpicCurrentCore() );

    Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQSR ) );

    if( Reg & NXP_QDMA_BSQSR_SQE )
    {
        return;
    }

    while( 1 )
    {
        StatusAddr = NxpStatus->DescHead;

        #ifdef DEBUG
            for( i = 0; i < 32; i += 4 )
            {
                log_info( "val:%x\t id:%d\n\r",
                          SWAP_32( *( uint32_t * )
                                   ( ( uint32_t ) StatusAddr + i ) ),
                          i );
            }
        #endif

        Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQMR ) );
        Reg |= NXP_QDMA_BSQMR_DI;
        QDMA_WRITE( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQMR ), Reg );
        Reg = __lwbrx((uint32_t *)( Block + ( BaseType_t ) NXP_QDMA_BSQDPAR ));
        NxpStatus->DescHead = (DescriptorFormat_t *)Reg;

        #if DEBUG
            Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQEPAR ) );
            log_info( "NXP_QDMA_BSQEPAR %x\t %d\t%s\r\n", Reg,
                      __LINE__, __func__ );
            Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQDPAR ) );
            log_info( "NXP_QDMA_BSQDPAR %x\t %d\t%s\r\n", Reg,
                      __LINE__, __func__ );
        #endif
	i = DescAddrQueueGet( StatusAddr );
        /*For performance optimization, there should be minimal
         * flag update in this function.
         */
        NxpQdma->QueueHead[ NXP_DEFAULT_QDMA_QUEUE ].Callback( (void *)i,
                                              ( uint32_t ) ( ( DescriptorFormat_t * ) ( StatusAddr )->LowAddrBase ) );

        Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BSQSR ) );

        if( Reg & NXP_QDMA_BSQSR_SQE )
        {
            Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BIDR ) );
            QDMA_WRITE( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BIDR ), Reg );
            return;
        }
    }
}

void QdmaErrorHandler(uint32_t Irq __attribute__((unused)), void *dev_data)
{
	NxpQdmaEngine_t *NxpQdma = (NxpQdmaEngine_t *)dev_data;
	BaseType_t	StatusBase;
	BaseType_t Block;
	uint32_t LowAddrBase_val;
	uint32_t i;
	uint32_t Reg;

	log_err("QdmaErrorHandler Irq_num %x\r\n", Irq);
	StatusBase = (BaseType_t)NxpQdma->StatusBase;
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD0R));
	LowAddrBase_val = Reg;
	log_err("DMA error capture DECCD0R word 0 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD1R));
	log_err("DMA error capture DECCD1R word 1 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD2R));
	log_err("DMA error capture DECCD2R word 2 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD3R));
	log_err("DMA error capture DECCD3R word 3 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD4R));
	log_err("DMA error capture DECCD4R word 4 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD5R));
	log_err("DMA error capture DECCD5R word 5 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD6R));
	log_err("DMA error capture DECCD6R word 6 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCD7R));
	log_err("DMA error capture DECCD7R word 7 register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECCQIDR));
	log_err("DMA error capture command queue register val %x\r\n", Reg);
	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DECBR));
	log_err("DMA error capture byte count register val %x\r\n", Reg);

	Reg = QDMA_READ((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DEDR));
	log_err("NXP_QDMA_DEDR %x\r\n", Reg);
	QDMA_WRITE((volatile uint32_t *) (StatusBase + (BaseType_t)NXP_QDMA_DEDR), Reg);

	for (i = 0; i < NXP_QDMA_BLOCK_NUM_MAX; i++) {
		Block = (BaseType_t)NxpQdma->BlockBase +
				(BaseType_t)NXP_QDMA_BASE_OFFSET(i);
		Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BEIDR));
		if (Reg) {
			log_err("block %d\tNXP_QDMA_BEIDR %x\r\n", i, Reg);
			QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BEIDR), Reg);
		}
	}
	if (NxpQdma->ErrorCallback)
		NxpQdma->ErrorCallback(NxpQdma->ErrorParams, LowAddrBase_val);
}

void QdmaBlockHandler(uint32_t Irq, void *dev_data )
{
NxpQdmaEngine_t *NxpQdma = (NxpQdmaEngine_t *)dev_data;
BaseType_t Block;
uint32_t Reg_BIDR;
int Ret = 0;
uint32_t qset;
uint32_t Id = Irq - ( 45 + INTERNAL_IRQ_OFFSET );

#ifdef HW_PERF
	TimeDiffInUsHw = ulGetElapsedTime(&QdmaTestStartTimeHw);
#endif

	Block = (BaseType_t)NxpQdma->BlockBase +
			(BaseType_t)NXP_QDMA_BASE_OFFSET(Id);
	Reg_BIDR = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BIDR));

	if ((qset = (Reg_BIDR & NXP_QDMA_BIDR_CQCIE))) {
		/* Clear interrupt event detect register */
		QDMA_WRITE((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BIDR), qset);

		/* Process transfer completion */
		Ret = NxpQdmaQueueTransferComplete(NxpQdma, Block);

	} else {
		log_err("QdmaBlockHandler: spurious interrupt\r\n");
	}

	if (!Ret)
		log_err("NxpQdmaQueueTransferComplete failed!\r\n");
	return;
}

BaseType_t RegisterCQueueCallback(void *Callback, void *Params,
				  uint32_t cqn_id)
{
	if (!NxpQdma)
		return pdFAIL;

	NxpQdma->QueueHead[cqn_id].Callback = Callback;
	NxpQdma->QueueHead[cqn_id].Params = Params;
	return pdPASS;
}

int DmaUSFFillIssue(BaseType_t Dst, BaseType_t Src, size_t Len,
		     uint32_t cqn_id, uint32_t rbp)
{
	BaseType_t Block;
	uint32_t Reg;
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(ulMpicCurrentCore());
	uint32_t ret;

	if (!NxpQdma) {
		errno = ENODATA;
		log_err("Qdma Engine Not Initialized: %s\r\n",
				strerror(errno));
		return -errno;
	}
	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		errno = EINVAL;
		log_err("Valid Queues are %u\r\n",
				NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return -errno;
	}

	vL1DCacheInvLine((uint32_t)&NxpQdma->QueueHead[cqn_id],
			      sizeof(NxpQdmaQueue_t));

	Block = (BaseType_t)(NxpQdma->QueueHead[cqn_id].BlockBase);
	Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQSR(cqn_id)));

	if (Reg & (NXP_QDMA_BCQSR_QF)) {
		errno = EAGAIN;
		log_err("block %x queue %d is full: %s\r\n", Block, cqn_id,
				strerror(errno));
		return -errno;
	}


	ret = QdmaUSFDescFill(NxpQdma->QueueHead[cqn_id].DescHead, Dst, Src,
			Len, cqn_id, rbp);
	if (ret != 0)
		return ret;
#ifdef DEBUG
	QdmaDescDump((uint32_t *)NxpQdma->QueueHead[cqn_id].DescHead, 8);
#endif
	NxpQdma->QueueHead[cqn_id].DescHead++;

	if (NxpQdma->QueueHead[cqn_id].DescHead >=
			NxpQdma->QueueHead[cqn_id].DescTail)
		NxpQdma->QueueHead[cqn_id].DescHead =
			NxpQdma->QueueHead[cqn_id].Cq;

	NxpCmdQueueEnqueue(NxpQdma, Block, cqn_id);
	return ret;
}

int DmaIssue(DescriptorFormat_t *NxpDesc, uint32_t cqn_id)
{
	BaseType_t Block;
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(ulMpicCurrentCore());
#ifdef DEBUG
	uint32_t Reg;
	if (unlikely(!NxpQdma)) {
		errno = ENODATA;
		log_err("Qdma Engine Not Initialized: %s\r\n",
				strerror(errno));
		return -errno;
	}
#endif
	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		errno = EINVAL;
		log_err("Valid Queues are %u\r\n",
				NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return -errno;
	}

	Block = (BaseType_t)(NxpQdma->QueueHead[cqn_id].BlockBase);

#ifdef DEBUG
	Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQSR(cqn_id)));
	if (Reg & (NXP_QDMA_BCQSR_QF | NXP_QDMA_BCQSR_XOFF)) {
		errno = EAGAIN;
		log_err("block %x queue %d is full: %s\r\n", Block, cqn_id,
				strerror(errno));
		return -errno;
	}
#endif

	DescAddrCqnSet(NxpDesc, cqn_id);

	memcpy((NxpQdma->QueueHead[cqn_id].DescHead), NxpDesc,
	       sizeof(DescriptorFormat_t));
	NxpQdma->QueueHead[cqn_id].DescHead++;

	if (NxpQdma->QueueHead[cqn_id].DescHead >=
		NxpQdma->QueueHead[cqn_id].DescTail)
			NxpQdma->QueueHead[cqn_id].DescHead =
				NxpQdma->QueueHead[cqn_id].Cq;

	NxpCmdQueueEnqueue(NxpQdma, Block, cqn_id);
	return 0;
}

int DmaIssueCore(DescriptorFormat_t *NxpDesc, uint32_t cqn_id, uint8_t tgt_core_id)
{
	BaseType_t Block;
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(tgt_core_id);
#ifdef DEBUG
	uint32_t Reg;
#endif

	NxpQdmaEngine_t *NxpQdma = NxpQdmaC[uBlk];

#ifdef DEBUG
	if (unlikely(!NxpQdma)) {
		errno = ENODATA;
		log_err("Qdma Engine Not Initialized: %s\r\n",
				strerror(errno));
		return -errno;
	}
#endif
	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		errno = EINVAL;
		log_err("Valid Queues are %u\r\n",
				NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return -errno;
	}

	Block = (BaseType_t)(NxpQdma->QueueHead[cqn_id].BlockBase);

#ifdef DEBUG
	Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQSR(cqn_id)));
	if (Reg & (NXP_QDMA_BCQSR_QF | NXP_QDMA_BCQSR_XOFF)) {
		errno = EAGAIN;
		log_err("block %x queue %d is full %s\r\n", Block, cqn_id,
				strerror(errno));
		return -errno;
	}
#endif

	DescAddrCqnSet(NxpDesc, cqn_id);

	memcpy((NxpQdma->QueueHead[cqn_id].DescHead), NxpDesc,
	       sizeof(DescriptorFormat_t));
	NxpQdma->QueueHead[cqn_id].DescHead++;

	if (NxpQdma->QueueHead[cqn_id].DescHead >=
		NxpQdma->QueueHead[cqn_id].DescTail)
			NxpQdma->QueueHead[cqn_id].DescHead =
				NxpQdma->QueueHead[cqn_id].Cq;

	NxpCmdQueueEnqueue(NxpQdma, Block, cqn_id);
	return 0;
}

/* alternative to DmaIssueCore without prints and with status return
 * TODO: In future, error codes will be introduced and this API will
 * replace DmaIssueCore(). */
int iDmaIssueCore(DescriptorFormat_t *NxpDesc, uint32_t cqn_id, uint8_t tgt_core_id)
{
	BaseType_t Block;
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(tgt_core_id);
	uint32_t Reg;

	NxpQdmaEngine_t *NxpQdma = NxpQdmaC[uBlk];

	if (unlikely(!NxpQdma))
		return -1;

	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
		return -1;

	Block = (BaseType_t)(NxpQdma->QueueHead[cqn_id].BlockBase);

	Reg = QDMA_READ((volatile uint32_t *) (Block + (BaseType_t)NXP_QDMA_BCQSR(cqn_id)));
	if (Reg & (NXP_QDMA_BCQSR_QF | NXP_QDMA_BCQSR_XOFF))
		return -1;

	DescAddrCqnSet(NxpDesc, cqn_id);

	memcpy((NxpQdma->QueueHead[cqn_id].DescHead), NxpDesc,
	       sizeof(DescriptorFormat_t));
	NxpQdma->QueueHead[cqn_id].DescHead++;

	if (NxpQdma->QueueHead[cqn_id].DescHead >=
		NxpQdma->QueueHead[cqn_id].DescTail)
			NxpQdma->QueueHead[cqn_id].DescHead =
				NxpQdma->QueueHead[cqn_id].Cq;

	NxpCmdQueueEnqueue(NxpQdma, Block, cqn_id);

	return 0;
}

void QdmaFillDesc(NxpQdmaCLT_t *NxpClt, uint32_t cqn_id )
{
	uint32_t i;
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(ulMpicCurrentCore());

	if (!NxpQdma)
		return;

	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		log_err("Valid Queues are %u\r\n", NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return;
	}
	vL1DCacheInvLine( ( uint32_t ) &NxpQdma->QueueHead[ cqn_id ],
                           sizeof( NxpQdmaQueue_t ) );

	NxpQdma->QueueHead[ cqn_id ].NxpClt = NxpClt;
	for( i = 0; i < QDMA_QUEUE_SIZE; i++ ) {
		memset(( NxpQdma->QueueHead[ cqn_id ].DescHead ), 0, (sizeof(DescriptorFormat_t)));
		DescAddrCqnSet( ( NxpQdma->QueueHead[ cqn_id ].DescHead ), cqn_id );
		DescAddrSet(( NxpQdma->QueueHead[ cqn_id ].DescHead ), (BaseType_t)NxpClt);

		DescAddrAttrSet(( NxpQdma->QueueHead[ cqn_id ].DescHead ), QDMA_DF_LONG_FMT);
		DescAddrCiSet(( NxpQdma->QueueHead[ cqn_id ].DescHead ));
		DescAddrSoSet(( NxpQdma->QueueHead[ cqn_id ].DescHead ));

		NxpClt++;
		NxpQdma->QueueHead[ cqn_id ].DescHead++;

		if( NxpQdma->QueueHead[ cqn_id ].DescHead >=
			NxpQdma->QueueHead[ cqn_id ].DescTail )
		{
			NxpQdma->QueueHead[ cqn_id ].DescHead =
			NxpQdma->QueueHead[ cqn_id ].Cq;
			NxpQdma->QueueHead[ cqn_id ].index = 0;
		}
	}
}


int QdmaFillAllDesc( DescriptorFormat_t * NxpDesc,
                      uint32_t cqn_id )
{
    uint32_t i;
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(ulMpicCurrentCore());

	if (unlikely(!NxpQdma)) {
		errno = ENODATA;
		log_err("Error: %s\r\n", strerror(errno));
		return -errno;
	}

	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		errno = EINVAL;
		log_err("Valid Queues are %u\r\n", NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return -errno;
	}
    vL1DCacheInvLine( ( uint32_t ) &NxpQdma->QueueHead[ cqn_id ],
                           sizeof( NxpQdmaQueue_t ) );

    for( i = 0; i < QDMA_QUEUE_SIZE; i++ )
    {
        DescAddrCqnSet( NxpDesc, cqn_id );

        memcpy( ( NxpQdma->QueueHead[ cqn_id ].DescHead ), NxpDesc,
                sizeof( DescriptorFormat_t ) );
        NxpQdma->QueueHead[ cqn_id ].DescHead++;

        if( NxpQdma->QueueHead[ cqn_id ].DescHead >=
            NxpQdma->QueueHead[ cqn_id ].DescTail )
        {
            NxpQdma->QueueHead[ cqn_id ].DescHead =
                NxpQdma->QueueHead[ cqn_id ].Cq;
        }
    }
    return 0;
}

uint32_t QdmaGetBlockCQMRReg( uint32_t cqn_id )
{
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(ulMpicCurrentCore());
	if (!NxpQdma)
		return 0;
	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		log_err("Valid Queues are %u\r\n", NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return 0;
	}
    BaseType_t Block = ( BaseType_t ) ( NxpQdma->QueueHead[ cqn_id ].BlockBase );
    uint32_t Reg = QDMA_READ( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BCQMR( cqn_id ) ) ) |
                   NXP_QDMA_BCQMR_EI;

    return Reg;
}

void QdmaSetBlockCQMRReg( uint32_t Reg,
                          uint32_t cqn_id )
{
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(ulMpicCurrentCore());
	if (!NxpQdma)
		return;
	if( cqn_id >= NxpValidQdmaBlk[ uBlk ].ucValidQueue )
	{
		log_err("Valid Queues are %u\r\n", NxpValidQdmaBlk[ uBlk ].ucValidQueue);
		return;
	}
    BaseType_t Block = ( BaseType_t ) ( NxpQdma->QueueHead[ cqn_id ].BlockBase );

    QDMA_WRITE( ( volatile uint32_t * ) ( Block + ( BaseType_t ) NXP_QDMA_BCQMR( cqn_id ) ), Reg );
}

/*
 * cqm: Command Queue Mode
 * nCq: Number of Command Queues
 * ErrCb : Register Calllback function to be called in
 *		case of error. Required only for core0
 */
BaseType_t QdmaInit(uint32_t core_id, uint8_t cqm, uint32_t nCq,
		    uint32_t ag_val, uint16_t ag_engine, void *ErrCb,
		    void *ErrParams)
{
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(core_id);
#ifdef GEUL_LA1224
	if (core_id == 4 || core_id == 5) {
		log_err("QDMA disabled on core 4 and 5 for LA1224\r\n");
		return pdFAIL;
	}
#endif
	if( ! NxpValidQdmaBlk[uBlk].ucValidBlk ) {
		log_err("QDMA Blk %u is disabled\r\n", uBlk);
		return pdFAIL;
	}
	/*NxpQdma is already initialized*/
	if (NxpQdma)
		return pdPASS;

	NxpQdma = pvGeulMalloc(sizeof(*NxpQdma));
	if ( !NxpQdma )
		return pdFAIL;

	NxpQdmaC[uBlk] = NxpQdma;

	NxpQdma->CtrlBase = (void *)QDMA_BASE_ADDR;
	NxpQdma->StatusBase = (void *)((BaseType_t)NxpQdma->CtrlBase +
				       QDMA_BLOCK_OFFSET);
	NxpQdma->BlockBase = (void *)((BaseType_t)NxpQdma->CtrlBase +
				      3 * QDMA_BLOCK_OFFSET);
	NxpQdma->cqm = cqm;

	if( nCq > NxpValidQdmaBlk[uBlk].ucValidQueue )
	{
		nCq = NxpValidQdmaBlk[uBlk].ucValidQueue;
		log_err("Valid Queues for QDMA Blk %u are %u\r\n", uBlk, nCq);
	}
	NxpQdma->nQueues = (uint8_t)nCq;

	NxpQdma->Status = NxpQqdmaAllocStatusResource(uBlk);
	if (!NxpQdma->Status)
	{
		log_err("Failed to alloc status resource\r\n");
		return pdFAIL;
	}

	if (!NxpQdmaAllocQueueResource(uBlk, nCq))
	{
		log_err("Failed to alloc queue resource\r\n");
		return pdFAIL;
	}

	if (!(lRegisterIrq(NXP_QDMA_BLOCK_IRQ(core_id) + INTERNAL_IRQ_OFFSET,
			    (bIsrFunc)QdmaBlockHandler,
			    (void *)NxpQdma) == 1))
	{
		log_err("Failed register irq \r\n");
		return pdFAIL;
	}

	bMpicEnable(DEVICE_INTERNAL, NXP_QDMA_BLOCK_IRQ(core_id));

	/*Register error handler for core0*/
	if (core_id == 0) {
		if (!(lRegisterIrq(NXP_QDMA_ERROR_IRQ + INTERNAL_IRQ_OFFSET,
				    (bIsrFunc)QdmaErrorHandler,
				    (void *)NxpQdma) == 1)) {
			log_err("Failed register error irq \r\n");
			return pdFAIL;
		}

		if (ErrCb)
			NxpQdma->ErrorCallback = ErrCb;
		else
			NxpQdma->ErrorCallback = NULL;

		if (ErrParams)
			NxpQdma->ErrorParams = ErrParams;
		else
			NxpQdma->ErrorParams = NULL;
		bMpicEnable(DEVICE_INTERNAL, NXP_QDMA_ERROR_IRQ);
	}

	if (!NxpQdmaRegInit(NxpQdma, core_id, cqm, ag_val, ag_engine)) {
		log_err("Can not initialize the qdma engine. \r\n");
		return pdFAIL;
	}

	return pdPASS;
}

/*
 * cqm: Command Queue Mode
 * nCq: Number of Command Queues
 * ErrCb : Register Calllback function to be called in
 *		case of error. Required only for core0.
 *		Does not interrupt for processed packets.
 */
BaseType_t QdmaInitNoIntr( uint32_t core_id,
                           uint8_t cqm,
                           uint32_t nCq,
                           uint32_t ag_val,
                           uint16_t ag_engine,
                           void * ErrCb,
                           void * ErrParams )
{
	uint32_t uBlk = NXP_QDMA_BLOCK_NUM(core_id);
#ifdef GEUL_LA1224
    if (core_id == 4 || core_id == 5) {
        log_err("QDMA disabled on core 4 and 5 for LA1224\r\n");
        return pdFAIL;
    }
#endif

	if( ! NxpValidQdmaBlk[uBlk].ucValidBlk ) {
		log_err("QDMA Blk %u is disabled\r\n", uBlk);
		return pdFAIL;
	}
    /*NxpQdma is already initialized*/
    if( NxpQdma )
    {
        return pdPASS;
    }

    NxpQdma = pvGeulMalloc( sizeof( *NxpQdma ) );

    if( !NxpQdma )
    {
        return pdFAIL;
    }

    NxpQdma->CtrlBase = ( void * ) QDMA_BASE_ADDR;
    NxpQdma->StatusBase = ( void * ) ( ( BaseType_t ) NxpQdma->CtrlBase +
                                       QDMA_BLOCK_OFFSET );
    NxpQdma->BlockBase = ( void * ) ( ( BaseType_t ) NxpQdma->CtrlBase +
                                      3 * QDMA_BLOCK_OFFSET );
    NxpQdma->cqm = cqm;

	if( nCq > NxpValidQdmaBlk[uBlk].ucValidQueue )
	{
		nCq = NxpValidQdmaBlk[uBlk].ucValidQueue;
		log_err("Valid Queues for QDMA Blk %u are %u\r\n", uBlk, nCq);
	}

    NxpQdma->nQueues = ( uint8_t ) nCq;

    NxpQdma->Status = NxpQqdmaAllocStatusResource( uBlk );

    if( !NxpQdma->Status )
    {
        log_err( "Failed to alloc status resource\r\n" );
        return pdFAIL;
    }

    if( !NxpQdmaAllocQueueResource( uBlk, nCq ) )
    {
        log_err( "Failed to alloc queue resource\r\n" );
        return pdFAIL;
    }

    /*Register error handler for core0*/
    if( core_id == 0 )
    {
        if( !( lRegisterIrq( NXP_QDMA_ERROR_IRQ + INTERNAL_IRQ_OFFSET,
                             ( bIsrFunc ) QdmaErrorHandler,
                             ( void * ) NxpQdma ) == 1 ) )
        {
            log_err( "Failed register error irq \r\n" );
            return pdFAIL;
        }

        if( ErrCb )
        {
            NxpQdma->ErrorCallback = ErrCb;
        }
        else
        {
            NxpQdma->ErrorCallback = NULL;
        }

        if( ErrParams )
        {
            NxpQdma->ErrorParams = ErrParams;
        }
        else
        {
            NxpQdma->ErrorParams = NULL;
        }

        bMpicEnable( DEVICE_INTERNAL, NXP_QDMA_ERROR_IRQ );
    }

    if( !NxpQdmaRegInit( NxpQdma, core_id, cqm, ag_val, ag_engine ) )
    {
        log_err( "Can not initialize the qdma engine. \r\n" );
        return pdFAIL;
    }

    return pdPASS;
}
