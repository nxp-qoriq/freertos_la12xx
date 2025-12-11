/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

#ifndef _L1C_VSPA_BENCH_H
#define _L1C_VSPA_BENCH_H

#define VSPA_BENCH_CORE            4

#define MAX_BUFF_SIZE              64*1024  /* bytes */
#define DMEM_MAX_BUFF_SIZE         1*1024   /* bytes */
#define SRAM_MAX_BUFF_SIZE         12*1024  /* bytes */

#define SCRATCH_BUFFER_PAGE_SIZE   0x1000
#define SCRATCH_BUFFER_USED_MEMORY (MAX_BUFF_SIZE + SCRATCH_BUFFER_PAGE_SIZE)
#define SCRATCH_BUFFER_OFFSET      (get_modem_share_area_size() - SCRATCH_BUFFER_USED_MEMORY)

/* 5 ms delay */
#define MS_DELAY                   (5 / portTICK_PERIOD_MS)
/* 1000 * 5 ms = 5 sec */
#define TIMEOUT_COUNT              1000

#define VSPA_ERROR_CODE            0xffffffffffffffff

#define E200_VSPA_FREQ 614.4 /* MHz */

typedef enum {
	MEM_UNKW  = -1,
	MEM_PEB   =  0,
	MEM_SRAM  =  1,
	MEM_DDR   =  2,
	MEM_FRAM  =  3,
	MEM_HRAM  =  4,
	MEM_DMEM0 =  5,
	MEM_DMEM1 =  6,
	MEM_DMEM2 =  7,
	MEM_DMEM3 =  8
	/* Geul B0 (6 e200 cores)
	MEM_DMEM4 =  9,
	MEM_DMEM5 =  10
	*/
} mem_type_e;

typedef enum {
	DIR_UNKW = -1,
	DIR_RD   =  0,
	DIR_WR   =  1
} direction_e;

typedef enum {
	UNKW_BENCH = -1,
	VSPA_BENCH =  0,
	E200_BENCH =  1,
	QDMA_BENCH =  2
} bench_type_e;

void vL1CBench(bench_type_e uBenchType, mem_type_e uMemType1, mem_type_e uMemType2, u32 uSizeInBytes, direction_e uTransferDirection, u32 uRepeats);
void vL1CVersion();
int l1c_qdma_init();

#endif
