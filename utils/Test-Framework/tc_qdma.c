// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "tc_qdma.h"

#if GEUL_DEMO_QDMA_TEST
#include "FreeRTOS.h"
#include "task.h"
#include <types.h>
#include <ppc.h>
#include "qdma.h"
#ifdef QDMA_PEB_TO_FRAM
#include "mpu.h"
#define SRC_SG_NUM_FRAM	25	/*value sould be <= SRC_SG_MAX*/
#define SRC_SG_MEMORY_SIZE_FRAM (0x1000)
#define DST_MEMORY_SIZE_FRAM (SRC_SG_MEMORY_SIZE_FRAM * SRC_SG_NUM_FRAM)
#define SRC_PEB_MEM_SIZE		(0x19000)
#define MPU_REGION_PEB_QDMA	   ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define DST_FRAM_ADDR			(0xe6000000)
#endif

static u32 iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM_MAX] __attribute__ ((section (".smem")));

#define LOOP_COUNT 0x100000
#define TEST_MEMORY_SIZE 0x100

/*Due to memory limitation for unit testing, SG_NUM is kept at low value*/
#define SRC_SG_NUM	4	/*value sould be <= SRC_SG_MAX*/
#define SRC_SG_MEMORY_SIZE (0x10)
#define DST_MEMORY_SIZE (SRC_SG_MEMORY_SIZE * SRC_SG_NUM)
/*DST_STRIDE_SIZE Must be power of 2 and less than DST_SG_MEMORY_SIZE*/
#define DST_STRIDE_SIZE	(0x10)

#define PERF 1
#ifdef HW_PERF
extern uint32_t TimeDiffInUsHw;
#endif

volatile uint32_t flag;
mod_mem_region_t *ddr_addr_m;
uint32_t used_size = 0x100;
#ifdef DEBUG
static void vCallback(void *param, uint32_t LowAddrBase_val)
#else
static void vCallback(void *param, __attribute__((unused))uint32_t LowAddrBase_val)
#endif
{
	int *p = param;
	*p = *p + 1;
#ifdef DEBUG
	log_info("LowAddrBase_val %x\r\n",SWAP_32(LowAddrBase_val));
#endif
}

int vQdmaUSFTestFunc(uint32_t rbp, uint32_t nCq)
{
	static BaseType_t src_addr;
	static BaseType_t dst_addr;
	size_t len;
	uint8_t cqm = NXP_QDMA_BCQMR_CQM_AUTO;
	uint32_t loop_count = LOOP_COUNT;
	uint32_t i;
	u32 uiCurrentCore = ulMpicCurrentCore();
#ifdef PERF
	struct Time QdmaTestStartTime;
	uint32_t TimeDiffInUs = 0;
#endif

	len = TEST_MEMORY_SIZE;
	if (!ddr_addr_m) {
		ddr_addr_m = (mod_mem_region_t *)
				bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
		//ddr_addr_m = (mod_mem_region_t *)
		//		bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef DEBUG
		log_info(" ddr_addr_m %x, size %x \r\n", ddr_addr_m->addr_v,
			 (uint32_t)ddr_addr_m->size);
#endif
	}

	if( !src_addr ) {
		src_addr = ddr_addr_m->addr_v + used_size +
				uiCurrentCore * 0x100000;
		used_size += len;
		memset((char *)src_addr, 0xef, len);
	}

	if( !src_addr ) {
		log_err("qDMA:Failed to alloc src\r\n");
		return -1 ;
	}

	if( !dst_addr )
		dst_addr = (BaseType_t)pvGeulMalloc(len);

	if( !dst_addr ) {
		log_err("qDMA:Failed to alloc dst\r\n");
		return -1;
	}

	memset((char *)dst_addr, 0x55, len);

	if (!iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)]) {
		if (!QdmaInit(uiCurrentCore, cqm, NXP_QDMA_QUEUE_NUM_MAX,
			      0, 0, NULL, NULL)) {
			log_err("qDMA:Failed to qdma init\r\n");
			return -1 ;
		}
		for (i = 0; i < NXP_QDMA_QUEUE_NUM_MAX ; i++)
			if (!RegisterCQueueCallback(vCallback,
			    (uint32_t *)&flag, i))
				return -1;
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] = 1 + uiCurrentCore;
	} else if (
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] != (1 + uiCurrentCore)) {
		log_err("qDMA:Block %d already in use by another core\r\n", NXP_QDMA_BLOCK_NUM(uiCurrentCore));
		return -1;
	}

	flag = 0;
#ifdef PERF
	vGetCurrentTime(&QdmaTestStartTime);
#endif
	for (i = 0; i < nCq; i++) {
		DmaUSFFillIssue(dst_addr + (BaseType_t)(len/(size_t)nCq * i),
				src_addr + (BaseType_t)(len/(size_t)nCq * i),
				len/(size_t)nCq, i, rbp);
	}

#ifdef DEBUG
	log_info("qDMA:Waiting transfer completion of buf in queues %u\r\n",
		 nCq);
#endif
	while ((flag != nCq) && (loop_count > 0))
		loop_count--;

#ifdef PERF
	TimeDiffInUs = ulGetElapsedTime(&QdmaTestStartTime);
#endif
	if (flag != nCq) {
		log_err("qDMA:Time-out in transmitting job\r\n");
		return -1;
	}

	vL1DCacheInvLine((uint32_t)src_addr, len);
	vL1DCacheInvLine((uint32_t)dst_addr, len);

	if ( strncmp((char *)src_addr, (char *)dst_addr, len))
		return -1 ;
#ifdef PERF
	/* To remove unused variable warning */
	(void)TimeDiffInUs;
	log_info("qDMA test time (SW + HW) %u us, len %u bytes\r\n",
		 TimeDiffInUs, len);
#endif
#ifdef HW_PERF
	log_info("qDMA test time for HW operations %u us, len %u bytes\r\n",
		 TimeDiffInUsHw, len);
#endif
	return 0;
}

void vQdmaTest( void )
{
	int ret = 0;
	uint32_t rbp = 0;
	uint32_t nCq = 1;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("qDMA:Ultra short format transfer test\r\n");
	ret = vQdmaUSFTestFunc(rbp, nCq);
	if (ret == 0) {
		log_info("qDMA:successful transfer single buffer (USF)\r\n");
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_TEST_STATUS);
	} else {
		log_info("qDMA:Failed transfer of single buffer (USF)\r\n");
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_TEST_STATUS);
	}
}

void vQdmaMulTest( void )
{
	int ret = 0;
	uint32_t rbp = 0;
	uint32_t nCq = 8;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("qDMA:Ultra short format transfer %u Queues\r\n", nCq);
	ret = vQdmaUSFTestFunc(rbp, nCq);
	if (ret == 0) {
		log_info("qDMA:successful transfer multiple Queues(USF)\r\n");
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_MUL_TEST_STATUS);
	} else {
		log_info("qDMA:Failed transfer of multiple Queues(USF)\r\n");
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_QDMA_MUL_TEST_STATUS);
	}
}

int vQdmaSGTestFunc(uint32_t rbp, uint32_t nCq, uint32_t dst_stride_size)
{
	static BaseType_t src_addr[SRC_SG_NUM];
	static BaseType_t dst_addr;
	static BaseType_t sgSrc;
	ScatterGatherTableFormat_t *sgSrcTableHead;
	size_t srclenArr[SRC_SG_NUM];
	uint32_t SrcSgNum = SRC_SG_NUM;
	uint8_t cqm = NXP_QDMA_BCQMR_CQM_AUTO;
	uint32_t loop_count = LOOP_COUNT;
	uint32_t i;
	static NxpQdmaCltCd_t *NxpSgCd[NXP_QDMA_QUEUE_NUM_MAX];
	u32 uiCurrentCore = ulMpicCurrentCore();
#ifdef DEBUG
	char *tempdstaddr;
#endif
	uint32_t TotalDstLength = 0;
	uint32_t TotalSrcLength = 0;
#ifdef PERF
	struct Time QdmaTestStartTime;
	uint32_t TimeDiffInUs = 0;
#endif
	/* To remove unused variable warnings */
	(void)TimeDiffInUs;

	if (SRC_SG_NUM > SRC_SG_MAX) {
		log_err("qDMA:Unsupported values of number of SG desc");
		return -1;
	}
	if (dst_stride_size > DST_MEMORY_SIZE) {
		log_err("qDMA: dst_stride_size > Destinaton length");
		return -1;
	}
	if (!ddr_addr_m) {
		ddr_addr_m = (mod_mem_region_t *)
				bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
#ifdef DEBUG
		log_info(" ddr_addr_m %x, size %x \r\n", ddr_addr_m->addr_v,
			 (uint32_t)ddr_addr_m->size);
#endif
	}
	for (i = 0; i < SrcSgNum; i++) {
		srclenArr[i] = (size_t)SRC_SG_MEMORY_SIZE;
		if (!src_addr[i]) {
			src_addr[i] = ddr_addr_m->addr_v + used_size +
				uiCurrentCore * 0x100000;
			used_size += srclenArr[i];
#ifdef DEBUG
			log_info(" src_Addr %x i %x, used_size %x\r\n",
				 src_addr[i], i, used_size);
#endif
		}
		if (!src_addr[i]){
			log_err("qDMA:failed to malloc src %d", i);
			return -1;
		}
		memset((char *)src_addr[i], (int)i + 0xef, srclenArr[i]);
	}

	if (!dst_addr)
		dst_addr = (BaseType_t)pvGeulMalloc((size_t)dst_stride_size);
	if (!dst_addr) {
		log_err("qDMA:failed to malloc dst");
		return -1;
	}
	memset((char *)dst_addr, 0x55, dst_stride_size);

	if (!sgSrc)
		sgSrc = (BaseType_t)pvGeulMalloc(
				sizeof(ScatterGatherTableFormat_t) *
				SrcSgNum + 0x40);
	if (!sgSrc) {
		log_err("qDMA:failed to malloc Src SGE descriptor %d");
		return -1;
	}
	sgSrcTableHead =
		(ScatterGatherTableFormat_t *)(((uint32_t)sgSrc + 0x40) &
					       (uint32_t)(~(0x3f)));
	memset(sgSrcTableHead, 0, SrcSgNum *
	       sizeof(ScatterGatherTableFormat_t));

	/*For SG, Length is total length of SG data entries*/
	TotalDstLength = (size_t)DST_MEMORY_SIZE;
	TotalSrcLength = (size_t)SRC_SG_MEMORY_SIZE * SrcSgNum;

	if (!iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)]) {
		if (!QdmaInit(uiCurrentCore, cqm, NXP_QDMA_QUEUE_NUM_MAX,
			      0, 0, NULL, NULL)) {
			log_err("qDMA:Failed to qdma init\r\n");
			return -1;
		}
		for (i = 0; i < NXP_QDMA_QUEUE_NUM_MAX; i++)
			if (!RegisterCQueueCallback(vCallback,
			    (uint32_t *)&flag, i))
				return -1;
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] = 1 + uiCurrentCore;
	} else if (
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] != (1 + uiCurrentCore)) {
		log_err("qDMA:Block %d already in use by another core\r\n", NXP_QDMA_BLOCK_NUM(uiCurrentCore));
		return -1;
	}

	for (i = 0; i < nCq; i++) {
		if (!NxpSgCd[i])
			NxpSgCd[i] = (NxpQdmaCltCd_t *)
					pvGeulMalloc(sizeof(NxpQdmaCltCd_t));
		if (!NxpSgCd[i]) {
			log_err("qDMA:Failed to alloc NxpSgCd\r\n");
			return -1;
		}

		/*Prepare CLT, Long Desc*/
		if (!QdmaFillCltLongCdSG(NxpSgCd[i], TotalDstLength,
					 TotalSrcLength, rbp)) {
			log_err("qDMA:Failed to fill NxpSgCd\r\n");
			return -1;
		}

		/*Prepare SG Entries*/
		QdmaFillSGEntries(sgSrcTableHead, (int32_t *)src_addr,
				  srclenArr, SrcSgNum);

		QdmaDescAddrSet(&NxpSgCd[i]->NxpClt.sCmdListTable,
			    (BaseType_t)sgSrcTableHead);

		QdmaCltDstEnableStride(&NxpSgCd[i]->NxpClt, dst_addr,
				       TotalDstLength, dst_stride_size);


#ifdef DEBUG
		/*Dump CLT, CD Descriptor*/
		QdmaCltSgCdDump(NxpSgCd[i]);

		/*Dump SG Entries*/
		QdmaSgEntriesDump((ScatterGatherTableFormat_t *)
				  sgSrcTableHead,
				  SrcSgNum);
#endif
	}

	flag = 0;
#ifdef PERF
	vGetCurrentTime(&QdmaTestStartTime);
#endif
	for (i = 0; i < nCq; i++)
		DmaIssue(&NxpSgCd[i]->Cd, i);

#ifdef DEBUG
	log_info("qDMA:Waiting transfer completion of buf in queues %u\r\n",
		 nCq);
#endif
	while ((flag != nCq) && (loop_count > 0))
		loop_count--;

#ifdef PERF
	TimeDiffInUs = ulGetElapsedTime(&QdmaTestStartTime);
#endif
	if (flag != nCq) {
		log_err("qDMA:Time-out in transmitting NxpSgCd\r\n");
		return -1;
	}

	for (i = 0; i < SrcSgNum; i++)
		vL1DCacheInvLine((uint32_t)src_addr[i], srclenArr[i]);
	vL1DCacheInvLine((uint32_t)dst_addr, dst_stride_size);

#ifdef DEBUG
	/*
	 * TODO: Add Src, Dst comparison code
	 * Current code is tested by dumping destination buffer
	 */
	tempdstaddr = (char *)dst_addr;
	for (i = 0; i < dst_stride_size; i++) {
		log_info("i %d dst %x \r\n", i, *tempdstaddr);
		tempdstaddr++;
	}
#endif
#ifdef PERF
	log_info("qDMA test time (SW + HW) %u us, len %u bytes\r\n",
		 TimeDiffInUs, DST_MEMORY_SIZE);
#endif
#ifdef HW_PERF
	log_info("qDMA test time for HW operations %u us, len %u bytes\r\n",
		 TimeDiffInUsHw, DST_MEMORY_SIZE);
#endif
	return 0;
}

void vQdmaSGTest(void)
{
	int ret = 0;
	uint32_t nCq = 1;
	uint32_t dst_stride_size = DST_STRIDE_SIZE ;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("qDMA:Scatter-Gather transfer test\r\n");
	ret = vQdmaSGTestFunc(0, nCq, dst_stride_size);
	if (ret == 0) {
		log_info("qDMA:successful transfer of SG(Long) nCq %d\r\n",
			 nCq);
		SET_TEST_STATUS(uiCurrentCore,
				GEUL_DEMO_QDMA_SG_TEST_STATUS);
	} else {
		log_info("qDMA:Failed transfer of SG(Long)\r\n");
		RESET_TEST_STATUS(uiCurrentCore,
				  GEUL_DEMO_QDMA_SG_TEST_STATUS);
	}
}

#ifdef QDMA_PEB_TO_FRAM
int vQdmaSGNoStrideTestFunc(uint32_t rbp, uint32_t nCq)
{
	static BaseType_t src_addr[SRC_SG_NUM_FRAM];
	static BaseType_t dst_addr;
	static BaseType_t sgSrc;
	ScatterGatherTableFormat_t *sgSrcTableHead;
	size_t srclenArr[SRC_SG_NUM_FRAM];
	uint32_t SrcSgNum = SRC_SG_NUM_FRAM;
	uint8_t cqm = NXP_QDMA_BCQMR_CQM_AUTO;
	uint32_t loop_count = LOOP_COUNT;
	uint32_t i;
	static NxpQdmaCltCd_t *NxpSgCd[NXP_QDMA_QUEUE_NUM_MAX];
	u32 uiCurrentCore = ulMpicCurrentCore();
#ifdef DEBUG
	char *tempdstaddr;
#endif
	uint32_t TotalDstLength = 0;
	uint32_t TotalSrcLength = 0;
#ifdef PERF
	struct Time QdmaTestStartTime;
	uint32_t TimeDiffInUs = 0;
#endif
	/* To remove unused variable warnings */
	(void)TimeDiffInUs;

	if (SRC_SG_NUM_FRAM > SRC_SG_MAX) {
		log_err("qDMA:Unsupported values of number of SG desc");
		return -1;
	}
	if (!ddr_addr_m) {
		ddr_addr_m = (mod_mem_region_t *)
				bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
#ifdef DEBUG
		log_info(" ddr_addr_m %x, size %x \r\n", ddr_addr_m->addr_v,
			 (uint32_t)ddr_addr_m->size);
#endif
	}
	for (i = 0; i < SrcSgNum; i++) {
		srclenArr[i] = (size_t)SRC_SG_MEMORY_SIZE_FRAM;
		if (!src_addr[i]) {
			src_addr[i] = ddr_addr_m->addr_v + used_size +
				uiCurrentCore * 0x100000;
			used_size += srclenArr[i];
#ifdef DEBUG
			log_info(" src_Addr %x i %x, used_size %x\r\n",
				 src_addr[i], i, used_size);
#endif
		}
		if (!src_addr[i]){
			log_err("qDMA:failed to malloc src %d", i);
			return -1;
		}
		memset((char *)src_addr[i], (int)i + 0xef, srclenArr[i]);
	}

	if (!dst_addr)
		dst_addr = (BaseType_t)DST_FRAM_ADDR;
	if (!dst_addr) {
		log_err("qDMA:failed to malloc dst");
		return -1;
	}
	memset((char *)dst_addr, 0x55, DST_MEMORY_SIZE_FRAM);

	if (!sgSrc)
		sgSrc = (BaseType_t)pvGeulMalloc(
				sizeof(ScatterGatherTableFormat_t) *
				SrcSgNum + 0x40);
	if (!sgSrc) {
		log_err("qDMA:failed to malloc Src SGE descriptor %d");
		return -1;
	}
	sgSrcTableHead =
		(ScatterGatherTableFormat_t *)(((uint32_t)sgSrc + 0x40) &
					       (uint32_t)(~(0x3f)));
	memset(sgSrcTableHead, 0, SrcSgNum *
	       sizeof(ScatterGatherTableFormat_t));

	/*For SG, Length is total length of SG data entries*/
	TotalDstLength = (size_t)DST_MEMORY_SIZE_FRAM;
	TotalSrcLength = (size_t)SRC_SG_MEMORY_SIZE_FRAM * SrcSgNum;

	if (!iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)]) {
		if (!QdmaInit(uiCurrentCore, cqm, NXP_QDMA_QUEUE_NUM_MAX,
			      0, 0, NULL, NULL)) {
			log_err("qDMA:Failed to qdma init\r\n");
			return -1;
		}
		for (i = 0; i < NXP_QDMA_QUEUE_NUM_MAX; i++)
			if (!RegisterCQueueCallback(vCallback,
			    (uint32_t *)&flag, i))
				return -1;
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] = 1 + uiCurrentCore;
	} else if (
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] != (1 + uiCurrentCore)) {
		log_err("qDMA:Block %d already in use by another core\r\n", NXP_QDMA_BLOCK_NUM(uiCurrentCore));
		return -1;
	}

	for (i = 0; i < nCq; i++) {
		if (!NxpSgCd[i])
			NxpSgCd[i] = (NxpQdmaCltCd_t *)
					pvGeulMalloc(sizeof(NxpQdmaCltCd_t));
		if (!NxpSgCd[i]) {
			log_err("qDMA:Failed to alloc NxpSgCd\r\n");
			return -1;
		}

		/*Prepare CLT, Long Desc*/
		if (!QdmaFillCltLongCdSG(NxpSgCd[i], TotalDstLength,
					 TotalSrcLength, rbp)) {
			log_err("qDMA:Failed to fill NxpSgCd\r\n");
			return -1;
		}

		/*Prepare SG Entries*/
		QdmaFillSGEntries(sgSrcTableHead, (int32_t *)src_addr,
				  srclenArr, SrcSgNum);

		QdmaDescAddrSet(&NxpSgCd[i]->NxpClt.sCmdListTable,
			    (BaseType_t)sgSrcTableHead);

		QdmaCltDst(&NxpSgCd[i]->NxpClt, dst_addr,
				       TotalDstLength);


#ifdef DEBUG
		/*Dump CLT, CD Descriptor*/
		QdmaCltSgCdDump(NxpSgCd[i]);

		/*Dump SG Entries*/
		QdmaSgEntriesDump((ScatterGatherTableFormat_t *)
				  sgSrcTableHead,
				  SrcSgNum);
#endif
	}

	flag = 0;
#ifdef PERF
	vGetCurrentTime(&QdmaTestStartTime);
#endif
	for (i = 0; i < nCq; i++)
		DmaIssue(&NxpSgCd[i]->Cd, i);

#ifdef DEBUG
	log_info("qDMA:Waiting transfer completion of buf in queues %u\r\n",
		 nCq);
#endif
	while ((flag != nCq) && (loop_count > 0))
		loop_count--;

#ifdef PERF
	TimeDiffInUs = ulGetElapsedTime(&QdmaTestStartTime);
#endif
	if (flag != nCq) {
		log_err("qDMA:Time-out in transmitting NxpSgCd\r\n");
		return -1;
	}

#ifdef DEBUG
	/*
	 * TODO: Add Src, Dst comparison code
	 * Current code is tested by dumping destination buffer
	 */
	tempdstaddr = (char *)dst_addr;
	for (i = 0; i < DST_MEMORY_SIZE_FRAM; i++) {
		log_info("i %d dst %x \r\n", i, *tempdstaddr);
		tempdstaddr++;
	}
#endif
#ifdef PERF
	log_info("qDMA test time (SW + HW) %u us, len %u bytes\r\n",
		 TimeDiffInUs, DST_MEMORY_SIZE_FRAM);
#endif
#ifdef HW_PERF
	log_info("qDMA test time for HW operations %u us, len %u bytes\r\n",
		 TimeDiffInUsHw, DST_MEMORY_SIZE_FRAM);
#endif
	return 0;
}

void vQdmaSGNoStrideTest(void)
{
	int ret = 0;
	uint32_t nCq = 1;
	extern char shdata_end __asm__("__shdata_end");

	log_info("qDMA:Scatter-Gather transfer test without Striding\r\n");
	vCreateMpuEntry(MPU_REGION_PEB_QDMA, (uint32_t)&shdata_end, (uint32_t)&shdata_end + DST_MEMORY_SIZE_FRAM);
	ret = vQdmaSGNoStrideTestFunc(0, nCq);
	vDeleteMpuEntry(MPU_REGION_PEB_QDMA ^ MAS0_VALID);
	if (ret == 0) {
		log_info("qDMA:successful transfer of SG(Long) without Striding nCq %d\r\n",
			 nCq);
	} else {
		log_info("qDMA:Failed transfer of SG(Long) without striding\r\n");
	}
}

int vQdmaSingleBufferTestFunc(uint32_t rbp, uint32_t nCq)
{
	extern char shdata_end __asm__("__shdata_end");
	static BaseType_t src_addr = (BaseType_t)&shdata_end;
	static BaseType_t dst_addr;
	size_t srclenArr;
	uint8_t cqm = NXP_QDMA_BCQMR_CQM_AUTO;
	uint32_t loop_count = LOOP_COUNT;
	uint32_t i;
	u32 uiCurrentCore = ulMpicCurrentCore();
#ifdef DEBUG
	char *tempdstaddr;
#endif
	uint32_t TotalDstLength = 0;
	uint32_t TotalSrcLength = 0;
	static NxpQdmaCltCd_t *NxpSBCd[NXP_QDMA_QUEUE_NUM_MAX];
#ifdef PERF
	struct Time QdmaTestStartTime;
	uint32_t TimeDiffInUs = 0;
#endif
	/* To remove unused variable warnings */
	(void)TimeDiffInUs;

	if (!ddr_addr_m) {
		ddr_addr_m = (mod_mem_region_t *)
				bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
#ifdef DEBUG
		log_info(" ddr_addr_m %x, size %x \r\n", ddr_addr_m->addr_v,
			 (uint32_t)ddr_addr_m->size);
#endif
	}
		srclenArr = (size_t)SRC_PEB_MEM_SIZE;
		if (!src_addr) {
			src_addr = ddr_addr_m->addr_v + used_size +
				uiCurrentCore * 0x100000;
			used_size += srclenArr;
#ifdef DEBUG
			log_info(" src_Addr %x , used_size %x\r\n",
				 src_addr, used_size);
#endif
		}
		if (!src_addr){
			log_err("qDMA:failed to malloc src");
			return -1;
		}
		memset((char *)src_addr, 0xef, srclenArr);

	if (!dst_addr)
		dst_addr = (BaseType_t)DST_FRAM_ADDR;
	if (!dst_addr) {
		log_err("qDMA:failed to malloc dst");
		return -1;
	}
	memset((char *)dst_addr, 0x55, DST_MEMORY_SIZE_FRAM);

	TotalDstLength = (size_t)SRC_PEB_MEM_SIZE;
	TotalSrcLength = (size_t)SRC_PEB_MEM_SIZE;

	if (!iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)]) {
		if (!QdmaInit(uiCurrentCore, cqm, NXP_QDMA_QUEUE_NUM_MAX,
			      0, 0, NULL, NULL)) {
			log_err("qDMA:Failed to qdma init\r\n");
			return -1;
		}
		for (i = 0; i < NXP_QDMA_QUEUE_NUM_MAX; i++)
			if (!RegisterCQueueCallback(vCallback,
			    (uint32_t *)&flag, i))
				return -1;
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] = 1 + uiCurrentCore;
	} else if (
		iIsQdmaInitialized[NXP_QDMA_BLOCK_NUM(uiCurrentCore)] != (1 + uiCurrentCore)) {
		log_err("qDMA:Block %d already in use by another core\r\n", NXP_QDMA_BLOCK_NUM(uiCurrentCore));
		return -1;
	}

	for (i = 0; i < nCq; i++) {
		if (!NxpSBCd[i])
			NxpSBCd[i] = (NxpQdmaCltCd_t *)
					pvGeulMalloc(sizeof(NxpQdmaCltCd_t));
		if (!NxpSBCd[i]) {
			log_err("qDMA:Failed to alloc NxpSBCd\r\n");
			return -1;
		}

		/*Prepare CLT, Long Desc*/
		if (!QdmaFillCltLongCdSG(NxpSBCd[i], TotalDstLength,
					 TotalSrcLength, rbp)) {
			log_err("qDMA:Failed to fill NxpSBCd\r\n");
			return -1;
		}
		/* Make FMT as Single Buffer */
		NxpSBCd[i]->NxpClt.sCmdListTable.Cfg1 = 0x0;

		QdmaDescAddrSet(&NxpSBCd[i]->NxpClt.sCmdListTable,
			    (BaseType_t)src_addr);

		QdmaCltDst(&NxpSBCd[i]->NxpClt, dst_addr,
				       TotalDstLength);


#ifdef DEBUG
		/*Dump CLT, CD Descriptor*/
		QdmaCltSgCdDump(NxpSBCd[i]);
#endif
	}

	flag = 0;
#ifdef PERF
	vGetCurrentTime(&QdmaTestStartTime);
#endif
	for (i = 0; i < nCq; i++)
		DmaIssue(&NxpSBCd[i]->Cd, i);

#ifdef DEBUG
	log_info("qDMA:Waiting transfer completion of buf in queues %u\r\n",
		 nCq);
#endif
	while ((flag != nCq) && (loop_count > 0))
		loop_count--;

#ifdef PERF
	TimeDiffInUs = ulGetElapsedTime(&QdmaTestStartTime);
#endif
	if (flag != nCq) {
		log_err("qDMA:Time-out in transmitting NxpSgCd\r\n");
		return -1;
	}

#ifdef DEBUG
	/*
	 * TODO: Add Src, Dst comparison code
	 * Current code is tested by dumping destination buffer
	 */
	tempdstaddr = (char *)dst_addr;
	for (i = 0; i < SRC_PEB_MEM_SIZE; i++) {
		log_info("i %d dst %x \r\n", i, *tempdstaddr);
		tempdstaddr++;
	}
#endif
#ifdef PERF
	log_info("qDMA test time (SW + HW) %u us, len %u bytes\r\n",
		 TimeDiffInUs, SRC_PEB_MEM_SIZE);
#endif
#ifdef HW_PERF
	log_info("qDMA test time for HW operations %u us, len %u bytes\r\n",
		 TimeDiffInUsHw, SRC_PEB_MEM_SIZE);
#endif
	return 0;
}

void vQdmaSingleBufferTest(void)
{
	int ret = 0;
	uint32_t nCq = 1;
	extern char shdata_end __asm__("__shdata_end");

	log_info("qDMA:Single Buffer transfer test\r\n");
	vCreateMpuEntry(MPU_REGION_PEB_QDMA, (uint32_t)&shdata_end, (uint32_t)&shdata_end + SRC_PEB_MEM_SIZE);
	ret = vQdmaSingleBufferTestFunc(0, nCq);
	vDeleteMpuEntry(MPU_REGION_PEB_QDMA ^ MAS0_VALID);
	if (ret == 0) {
		log_info("qDMA:successful transfer of Single Buffer(Long) nCq %d\r\n",
			 nCq);
	} else {
		log_info("qDMA:Failed transfer of Single Buffer(Long)\r\n");
	}
}
#endif

#endif	/* GEUL_DEMO_QDMA_TEST */
