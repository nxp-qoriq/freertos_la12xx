/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "ppc.h"
#include "qdma.h"
#include "ppu_intrinsics.h"
#include "l1c_defs.h"
#include "l1c_vspa_agent.h"
#include "l1c_dma.h"
#include "l1c_fwk_tasks.h"
#include "l1c_bench.h"
#include "l1c_ver.h"
#include "pmc.h"
#include "l1c_debug.h"

/* alocate buffers in different memories */
uint8_t dmem_buffer[DMEM_MAX_BUFF_SIZE] __attribute__((aligned(64))) = {0x00};
uint8_t sram_buffer[SRAM_MAX_BUFF_SIZE] __attribute__ ((section (".smem"))) __attribute__((aligned(64))) = {0x00};
uint8_t peb_buffer[MAX_BUFF_SIZE] __attribute__ ((section (".hif"))) __attribute__((aligned(64))) = {0x00};
uint8_t hram_buffer[MAX_BUFF_SIZE] __attribute__ ((section (".hram"))) __attribute__((aligned(64))) = {0x00};
uint8_t fram_buffer[MAX_BUFF_SIZE] __attribute__ ((section (".fram"))) __attribute__((aligned(64))) = {0x00};

#define QUEUE_SIZE 64

/* variable set to zero at the start of measurement and written by VSPA with measured cycles (non-zero value) */
volatile uint64_t meas_buffer __attribute__ ((aligned(64))) = 0;

uint32_t start_pmc, stop_pmc;
volatile uint32_t qdma_flag;

inline void l1c_vspa_dma_bench_msg_send(uint8_t core, vspa_dma_bench_msg_t *msg)
{
	l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_DMA_BENCH, msg, sizeof(vspa_dma_bench_msg_t));
}

void l1c_vspa_dma_bench_msg_dump(vspa_dma_bench_msg_t *s)
{
	/* transfer_mode is 0 for read */
	if (s->transfer_mode == 0)
		PRINTF("Read %u bytes from dma_axi_addr=0x%x %u time",
				s->axi_byte_count,
				s->dma_axi_addr,
				s->repeats);
	/* transfer_mode is 1 for write */
	else if (s->transfer_mode == 1)
		PRINTF("Write %u bytes at dma_axi_addr=0x%x %u time",
				s->axi_byte_count,
				s->dma_axi_addr,
				s->repeats);
	/* unknown transfer mode */
	else
		return;

	if (s->repeats > 1)
		PRINTF("s");
	PRINTF("\r\n");
}

static void qdma_cmd_cb(void *param, __attribute__((unused))uint32_t LowAddrBase_val)
{
	volatile uint32_t *p = param;

	stop_pmc = PMC_CTR_READ(PMR_PMC0);

	*p = *p + 1;
}

int l1c_qdma_init()
{
	static bool_t isQdmaInitialized = 0;

	if (isQdmaInitialized)
		return 1;

	qdma_flag = 0;

	if (!QdmaInit(crt_core_id,
				  NXP_QDMA_BCQMR_CQM_AUTO,
				  NXP_QDMA_QUEUE_NUM_MAX,
				  0, 0,
				  NULL /* qdma_error_cb*/,
				  NULL))
	{
		PRINTF("%s: QDMA init error\r\n", __func__);
		return -1;
	}

	isQdmaInitialized = 1;

	return 0;
}

static void l1c_qdma_bench_init()
{
	static bool_t isQdmaBenchInitialized = 0;

	if(isQdmaBenchInitialized)
		return;

	if (l1c_qdma_init() < 0)
		return;

	for (uint32_t i = 0; i < NXP_QDMA_QUEUE_NUM_MAX; i++)
	{
		if (RegisterCQueueCallback(qdma_cmd_cb, (void *)&qdma_flag, i) != pdPASS)
		{
			PRINTF("%s: QDMA register callback error\r\n", __func__);
			return;
		}
	}

	isQdmaBenchInitialized = 1;
}

static portFORCE_INLINE uint32_t externalize(void *dmem_ptr, uint8_t core_id)
{
	extern uint32_t __DMEM_START;
	extern uint32_t __DMEM_END;

	/* e200 has a different perspective on other cores' DMEM */
	if (((uint32_t)dmem_ptr >= (uint32_t)&__DMEM_START) &&
		((uint32_t)dmem_ptr < (uint32_t)&__DMEM_END))
		return (uint32_t)dmem_ptr + 0x100000 + CORE_DMEM_SIZE * core_id;

	/* pointer outside DMEM, nothing to externalize */
	return (uint32_t)dmem_ptr;
}

void qdma_memcpy(void *dst, void *src, uint32_t size, uint32_t times)
{
	qdma_flag = 0;
	start_pmc = PMC_CTR_READ(PMR_PMC0);

	for (uint32_t i = 0; i < times; i++)
	{
		if (DmaUSFFillIssue((BaseType_t)dst, (BaseType_t)src, size, 0, 0)) {
			PRINTF("DmaUSFFillIssue failed\n\r");
		}
	}

	while (qdma_flag != times);
}

uint32_t get_bench_memory_addr(bench_type_e uBenchType, mem_type_e uMemType, uint8_t **addr)
{
	uint32_t max_bytes_addr = MAX_BUFF_SIZE;
	mod_mem_region_t *ddr_addr_m;

	UNUSED(uBenchType);

	switch (uMemType)
	{
		case MEM_DMEM0:
			*addr = (uint8_t *)externalize((void *)dmem_buffer, 0);
			max_bytes_addr = DMEM_MAX_BUFF_SIZE;
			break;
		case MEM_DMEM1:
			*addr = (uint8_t *)externalize((void *)dmem_buffer, 1);
			max_bytes_addr = DMEM_MAX_BUFF_SIZE;
			break;
		case MEM_DMEM2:
			*addr = (uint8_t *)externalize((void *)dmem_buffer, 2);
			max_bytes_addr = DMEM_MAX_BUFF_SIZE;
			break;
		case MEM_DMEM3:
			*addr = (uint8_t *)externalize((void *)dmem_buffer, 3);
			max_bytes_addr = DMEM_MAX_BUFF_SIZE;
			break;
		/* Geul B0 (6 e200 cores)
		case MEM_DMEM4:
			*addr = (uint8_t *)externalize((void *)dmem_buffer, 4);
			max_bytes_addr = DMEM_MAX_BUFF_SIZE;
			break;
		case MEM_DMEM5:
			*addr = (uint8_t *)externalize((void *)dmem_buffer, 5);
			max_bytes_addr = DMEM_MAX_BUFF_SIZE;
			break;
		*/
		case MEM_SRAM:
			*addr = sram_buffer;
			max_bytes_addr = SRAM_MAX_BUFF_SIZE;
			break;
		case MEM_PEB:
			*addr = peb_buffer;
			break;
		case MEM_DDR:
			ddr_addr_m = (mod_mem_region_t *) bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
			*addr = (uint8_t *)(ddr_addr_m->addr_v + SCRATCH_BUFFER_OFFSET + crt_core_id * 0x100000);
			break;
		case MEM_HRAM:
			*addr = hram_buffer;
			break;
		case MEM_FRAM:
			*addr = fram_buffer;
			break;
		default:
			max_bytes_addr = 0;
	}

	return max_bytes_addr;
}

bool_t check_params(uint32_t max_bytes_src, uint32_t max_bytes_dst, uint32_t uSizeInBytes)
{
	if ((max_bytes_src == 0) || (max_bytes_dst == 0))
	{
		PRINTF("Wrong value for memory type!\r\n");
		return 0;
	}

	if ((uSizeInBytes > max_bytes_src) || (uSizeInBytes > max_bytes_dst))
	{
		PRINTF("The value for size exceeds the limit!\r\n");
		return 0;
	}

	return 1;
}

void vL1CBench(bench_type_e uBenchType, mem_type_e uMemType1, mem_type_e uMemType2, u32 uSizeInBytes, direction_e uTransferDirection, u32 uRepeats)
{
	uint8_t *src = NULL;
	uint8_t *dst = NULL;
	uint32_t max_bytes_src;
	uint32_t max_bytes_dst;
	uint32_t i;
	float delta_us = 0.0;
	uint32_t delta = 0;
	uint32_t loop_count = TIMEOUT_COUNT;
	vspa_dma_bench_msg_t pmessage;
	meas_buffer = 0;

	switch (uBenchType)
	{
		case VSPA_BENCH:
			e200_trace(E200_TRACE_MSG_BENCH_VSPA, E200_TRACE_PARAM_TRACK);
			l1c_vspa_set_runtime_overlay(VSPA_BENCH_CORE, RT_OVLY_BENCH);
			vTaskDelay(500);

			max_bytes_src = MAX_BUFF_SIZE;
			max_bytes_dst = get_bench_memory_addr(uBenchType, uMemType1, &dst);
			if (!check_params(max_bytes_src, max_bytes_dst, uSizeInBytes))
				return;

			pmessage.dma_axi_addr = (uint32_t)dst;
			pmessage.axi_byte_count = uSizeInBytes;
			pmessage.transfer_mode = uTransferDirection;
			pmessage.repeats = uRepeats;
			pmessage.meas_buff_addr = externalize((void *)&meas_buffer, crt_core_id);

			l1c_vspa_dma_bench_msg_dump(&pmessage);

			pmessage.dma_axi_addr = swap_uint32(pmessage.dma_axi_addr);
			pmessage.axi_byte_count = swap_uint32(pmessage.axi_byte_count);
			pmessage.transfer_mode = swap_uint32(pmessage.transfer_mode);
			pmessage.repeats = swap_uint32(pmessage.repeats);
			pmessage.meas_buff_addr = swap_uint32(pmessage.meas_buff_addr);

			l1c_vspa_dma_bench_msg_send(4, &pmessage);

			while ((meas_buffer == 0) && (loop_count > 0))
			{
				loop_count--;
				vTaskDelay(MS_DELAY);
			}

			if (meas_buffer == 0)
				PRINTF("Time-out in receiving the measurement from VSPA!\r\n");
			else if (meas_buffer == VSPA_ERROR_CODE)
				PRINTF("Something went wrong in VSPA!\r\n");
			else
			{
				meas_buffer = swap_uint64(meas_buffer);
				delta_us = meas_buffer/E200_VSPA_FREQ;
				PRINTF("Measured VSPA cycles = 0x%x%x (%d.%.2d us)\r\n",
					   (uint32_t) (meas_buffer >> 32), (uint32_t) (meas_buffer),
					   (int)delta_us, (int)((delta_us-(int)delta_us)*100));
			}
			return;

			break;
		case E200_BENCH:
			e200_trace(E200_TRACE_MSG_BENCH_E200, E200_TRACE_PARAM_TRACK);
			max_bytes_src = DMEM_MAX_BUFF_SIZE;
			max_bytes_dst = get_bench_memory_addr(uBenchType, uMemType1, &dst);
			if (!check_params(max_bytes_src, max_bytes_dst, uSizeInBytes))
				return;
			src = dmem_buffer;
			if (uTransferDirection == DIR_RD)
			{
				src = dst;
				dst = dmem_buffer;
			}
			for (i = 0; i < uRepeats; i++)
			{
				start_pmc = PMC_CTR_READ(PMR_PMC0);
				memcpy(dst, src, uSizeInBytes);
				stop_pmc = PMC_CTR_READ(PMR_PMC0);
				delta += stop_pmc-start_pmc;
			}
			break;
		case QDMA_BENCH:
			e200_trace(E200_TRACE_MSG_BENCH_QDMA, E200_TRACE_PARAM_TRACK);
			max_bytes_src = get_bench_memory_addr(uBenchType, uMemType1, &src);
			max_bytes_dst = get_bench_memory_addr(uBenchType, uMemType2, &dst);
			if (!check_params(max_bytes_src, max_bytes_dst, uSizeInBytes))
				return;
			if (uMemType1 == uMemType2)
			{
				dst += uSizeInBytes;
				if (max_bytes_dst < (uSizeInBytes * 2))
				{
					PRINTF("When src == dst, the size much not be more than half the buffer size (%d bytes)\r\n", max_bytes_dst);
					return;
				}
			}
			l1c_qdma_bench_init();
			while (uRepeats) {
				if (uRepeats > QUEUE_SIZE) {
					qdma_memcpy((void *)dst, (void *) src, uSizeInBytes, QUEUE_SIZE);
					uRepeats = uRepeats - QUEUE_SIZE;
				} else {
					qdma_memcpy((void *)dst, (void *) src, uSizeInBytes, uRepeats);
					uRepeats = 0;
				}
				delta += stop_pmc-start_pmc;
			}
			break;
		default:
			return;
	}

	delta_us = delta/E200_VSPA_FREQ;

	PRINTF("Transfer %d byte(s) from (%#x) into (%#x) %d time(s)\r\n", uSizeInBytes, src, dst, uRepeats);
	PRINTF("Measured E200 cycles = %#x (%d.%.2d us)\r\n ", delta, (int)delta_us, (int)((delta_us-(int)delta_us)*100));
}

inline void l1c_vspa_version_msg_send(uint8_t core, version_msg_t *msg)
{
	l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_VERSION, msg, sizeof(version_msg_t));
}

void vL1CVersion()
{
	version_msg_t msg;
	uint8_t core_idx;
	vspa_version_t vspa_ver;
	uint32_t uiCurrentCore = ulMpicCurrentCore();

	e200_trace(E200_TRACE_MSG_VERSION, E200_TRACE_PARAM_TRACK);

	PRINTF("VSPA Cores:\r\n");
	for (core_idx = 0; core_idx < 8; core_idx++)
	{
		uint8_t retries = 10;

		memset(&vspa_ver, 0, sizeof(vspa_version_t));

		msg.e200_vspa_version_buff = (void *) swap_uint32((uint32_t)((uint8_t *)&vspa_ver + 0x100000 + CORE_DMEM_SIZE * uiCurrentCore));
		l1c_vspa_version_msg_send(core_idx, &msg);

		while (--retries && !vspa_ver.tier)
			vTaskDelay(100);

		if (!retries)
		{
			PRINTF("Timeout on getting VSPA core %d version!\r\n", core_idx);
			continue;
		}

		PRINTF("[%d]: [Tier %d] %d.%d.%d [%s]\r\n",
			   core_idx,
			   swap_uint16(vspa_ver.tier),
			   swap_uint16(vspa_ver.major),
			   swap_uint16(vspa_ver.middle),
			   swap_uint16(vspa_ver.minor),
			   vspa_ver.description);
	}


	PRINTF("\r\nE200: [Tier %d] %d.%d.%d [%s]\r\n",
		E200_VERSION_TIER,
		E200_VERSION_MAJOR,
		E200_VERSION_MIDDLE,
		E200_VERSION_MINOR,
		E200_VERSION_DESCRIPTION);
}
