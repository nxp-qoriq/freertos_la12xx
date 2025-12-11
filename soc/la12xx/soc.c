// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include <common.h>
#include "soc.h"
#include "config.h"
#include "immap.h"
#include <mpic.h>
#include <platform_def.h>
#include "mpic_regs.h"
#include <FreeRTOS.h>
#include "ppc.h"

uint32_t crt_soc_svr;
uint32_t crt_soc_rev;
uint32_t crt_soc_numcores;

static const char *SOC[]= {
	"LA1200",
	"LA1201",
	"LA1212",
	"LA1214",
	"LA1215",
	"LA1216",
	"LA1223",
	"LA1225",
	"LA1234",
	"LA1235",
	"LA1236",
	"LA1218",
	"LA1238",
	"LA1232",
	"RESV",
	"LA1224",
};

static const char *SOC_DCS[] = {
	"NO_HSDCS",
	"HSDCS_RXONLY",
	"HSDCS_FULL"
};

static char *soc_rev_str()
{
	switch (crt_soc_svr & DCSR_SVR_REV_MASK)
	{
		case GEUL_SVR_REVA_VAL:
			return "A0";
		case GEUL_SVR_REVB_VAL:
			return "B0";
	}
	return "Unknown";
}

uint32_t get_hsdcs_support(void)
{
	struct ccsr_dcsr *dcsr;

	if (in_le16(((uint16_t *)(PCIE1_DEV_ID_BASE_ADDR))) == PCIE1_1238_DEV_ID)
		return HSDCS_RXONLY;

	dcsr = (struct ccsr_dcsr *)CCSR_DCFG_BASE_ADDR;
	crt_soc_svr  = in_le32(&dcsr->ulSvr);
	switch (crt_soc_svr & GEUL_SVR_HSDCS_MASK)
	{
		case GEUL_SVR_HSDCS_NO:
			return HSDCS_NO;
		default:
			return HSDCS_FULL;
	}
	return HSDCS_FULL;
}

uint32_t __get_soc_revision(void)
{
	struct ccsr_dcsr *dcsr;
	uint32_t rev = 0;

	dcsr = (struct ccsr_dcsr *)CCSR_DCFG_BASE_ADDR;
	crt_soc_svr = in_le32(&dcsr->ulSvr);
	switch (crt_soc_svr & DCSR_SVR_REV_MASK)
	{
		case DCSR_SVR_REVA_MASK:
			rev = GEUL_SVR_REVA_VAL;
		break;
		case DCSR_SVR_REVB_MASK:
			rev = GEUL_SVR_REVB_VAL;
		break;
		default:
			rev = GEUL_SVR_UNKNOWN_VAL;
		break;
	}
	return rev;
}

uint32_t __get_soc_numcores(void)
{
	uint32_t rev = __get_soc_revision();
	uint32_t numcores;

	switch (rev)
	{
		case GEUL_SVR_REVA_VAL:
			numcores = GEUL_E200_CORE_REVA_NUM;
		break;
		case GEUL_SVR_REVB_VAL:
			numcores = GEUL_E200_CORE_REVB_NUM;
		break;
		default:
			numcores = GEUL_E200_CORE_REVA_NUM;
		break;
	}

	return numcores;
}

uint32_t get_processor_version(void)
{
	struct ccsr_dcsr *dcsr;

	dcsr = (struct ccsr_dcsr *)CCSR_DCFG_BASE_ADDR;
	return in_le32(&dcsr->ulPvr);
}

uint32_t get_fusesr(void)
{
	struct ccsr_dcsr *dcsr;

	dcsr = (struct ccsr_dcsr *)CCSR_DCFG_BASE_ADDR;
	return (in_le32(&dcsr->ulFusesr) >> 12);
}

char *get_lsdcs_info(void)
{

	uint32_t uiFuseVal = get_fusesr();

	switch (uiFuseVal & 0xf) {
		case FUSE_LA1200:
		case FUSE_LA1201:
		case FUSE_LA1215:
		case FUSE_LA1225:
		case FUSE_LA1235:
			return "LSDCS-NO";
		case FUSE_LA1212:
		case FUSE_LA1232:
		case FUSE_LA1216:
		case FUSE_LA1236:
			return "LSDCS-2T2R";
		default:
			return "LSDCS-4T4R";
	}
	return NULL;
}

/* L1 DCache Invalidate Line*/
void vL1DCacheInvLine(uint32_t addr, size_t mem_size)
{
	uint32_t mem_offset;

	for (mem_offset = 0; mem_offset <= mem_size; mem_offset += L1_DCACHE_LINE_SIZE)
		dcbi( addr + mem_offset );
	msync();
}

/* L1 ICache Invalidate Line*/
void vL1ICacheInvLine(void)
{
	/* TODO:
	 * Currently cache works in write-through policy.
	 * If in future the policy changes this function needs
	 * to be implemented.
	 * Use icbi() instead of dcbi() to invalidate
	 * for ICache. */
}

void vSocInit(uint8_t core_id)
{
	/* update the globals for faster access */
	crt_soc_rev = __get_soc_revision();
	crt_soc_numcores = __get_soc_numcores();

	PRINTF("SVR: 0x%x PVR: 0x%x SoC:%s %s with %s %s\r\n",
			get_soc_version(), get_processor_version(), SOC[get_fusesr()&0xf],
			soc_rev_str(), SOC_DCS[get_hsdcs_support()], get_lsdcs_info());

	if (core_id == GEUL_E200_MASTER_CORE)
		bMpicInitialize(core_id);

	isync();
	msync();

	l1_icache_enable();

	isync();
	msync();

	l1_dcache_enable();
	bp_enable();
}

void vBootRelease(uint8_t master_core_id)
{
	extern char entry_point __asm__("__start");

	if (master_core_id != 0) {vPortMaskInterrupts(); for(;;);} // otherwise we must modify the scratch address structure

	/* Program FreeRTOS image entry address to scratch register */
	out_le32(SCRATCHRW1_ADDR, (uint32_t)&entry_point);
	out_le32(SCRATCHRW2_ADDR, (uint32_t)&entry_point);
	out_le32(SCRATCHRW3_ADDR, (uint32_t)&entry_point);

	if (get_soc_revision() == GEUL_SVR_REVB_VAL) {
		out_le32(SCRATCHRW4_ADDR, (uint32_t)&entry_point);
		out_le32(SCRATCHRW5_ADDR, (uint32_t)&entry_point);
		/* Release the other cores using boot release register */
		out_le32(BOOT_RELEASE_REGISTER, 0x3f ^ (1 << master_core_id));
	} else {
		 /* Release the other cores using boot release register */
		out_le32(BOOT_RELEASE_REGISTER, 0xf ^ (1 << master_core_id));
	}
}

void vInitSmem(void)
{
    memset( ( uint32_t * )SMEM_BASE_ADDR, 0, SMEM_SIZE );
	return;
}

void vInitSmemText(void)
{
    memset( ( uint32_t * )SMEM_TEXT_BASE_ADDR, 0, SMEM_TEXT_SIZE );
	return;
}

void vInitCoreSmem(void)
{
	memset( ( uint32_t * )CORE_SMEM_BASE_ADDR, 0, CORE_SMEM_SIZE );
	return;
}
void vInitCoreSmemText(void)
{
	memset( ( uint32_t * )CORE_SMEM_TEXT_BASE_ADDR, 0, CORE_SMEM_TEXT_SIZE );
	return;
}
