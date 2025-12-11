// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2024 NXP
 */

#include "types.h"
#include "common.h"
#include "config.h"
#include "platform_def.h"
#include "soc.h"
#include "ppc.h"
#include "mpu.h"

void vCreateMpuEntry( u32 ulMpuEntry, u32 start_addr, u32 end_addr )
{
	mtspr( SPR_MAS0, ulMpuEntry );
	mtspr( SPR_MAS1, 0 );
	mtspr( SPR_MAS2, end_addr - 1);
	mtspr( SPR_MAS3, start_addr );
	mpuwe();
	mpusync();
}

void vDeleteMpuEntry( u32 ulMpuEntry )
{
	mtspr( SPR_MAS0, ulMpuEntry );
	mpuwe();
	mpusync();
}

void vMpuEnable( void )
{
	extern char dmem_start __asm__("__dmem_start");
	extern char dmem_end __asm__("__dmem_end");
	extern char smem_start __asm__("__smem_start");
#if RELEASE_MODE
	extern char smem_end __asm__("__smem_end");
#endif
	extern char smem_text_start __asm__ ("__smem_text_start");
	extern char smem_text_end __asm__ ("__smem_text_end");
	extern char hif_start __asm__("__hif_start");
#if RELEASE_MODE
	extern char hif_end __asm__("__hif_end");
#endif
	extern char text_start __asm__("__text_start");
	extern char text_end __asm__("__text_end");
	extern char rodata_start __asm__("__rodata_start");
	extern char rodata_end __asm__("__rodata_end");
	extern char shdata_start __asm__("__shdata_start");
#ifdef VSPA_OV_PEBM_ENABLED
	extern char shdata_end __asm__("__shdata_end");
#endif
#ifdef GEUL_LA1238CPE
	extern char cshdata_start __asm__("__cshdata_start");
	extern char cshdata_end __asm__("__cshdata_end");
#endif
	extern char core_smem_text_start __asm__("__core_smem_text_start");
	extern char core_smem_text_end __asm__("__core_smem_text_end");

	/* CCSR + DCSR */
	vCreateMpuEntry(MPU_REGION_xCSR, CCSR_BASE_ADDR, DCSR_END_ADDR);
	/* DMEM of current core */
	vCreateMpuEntry(MPU_REGION_DMEM, (uint32_t)&dmem_start, (uint32_t)&dmem_end);
	/* SMEM */
#if RELEASE_MODE
	vCreateMpuEntry(MPU_REGION_SMEM, (uint32_t)&smem_start, (uint32_t)&smem_end);
#else
	vCreateMpuEntry(MPU_REGION_SMEM, (uint32_t)&smem_start, (uint32_t)&smem_text_end);
#endif
	/* SMEM_TEXT */
	vCreateMpuEntry(MPU_REGION_SMEM_TEXT, (uint32_t)&smem_text_start, (uint32_t)&smem_text_end);
	/* HIF area in PEBM */
#if RELEASE_MODE
	vCreateMpuEntry(MPU_REGION_HIF, (uint32_t)&hif_start, (uint32_t)&hif_end);
#else
	vCreateMpuEntry(MPU_REGION_HIF, (uint32_t)&hif_start, (uint32_t)&text_end);
#endif
	/* TEXT area in PEBM */
	vCreateMpuEntry(MPU_REGION_TEXT, (uint32_t)&text_start, (uint32_t)&text_end);
	/* Flex spi flash */
	vCreateMpuEntry(MPU_REGION_FSPI, FSPI_AHB_BASE_ADDR, FSPI_AHB_END_ADDR);
	/* RODATA area in PEBM */
	 vCreateMpuEntry(MPU_REGION_RODATA, (uint32_t)&rodata_start, (uint32_t)&rodata_end);
#ifndef VSPA_OV_PEBM_ENABLED
	 vCreateMpuEntry(MPU_REGION_DATA, (uint32_t)&shdata_start, PEBM_OVERLAY_END);
#else
	 vCreateMpuEntry(MPU_REGION_DATA, (uint32_t)&shdata_start, (uint32_t)&shdata_end);
#endif
	/* FECA memory space (HRAM, FRAM, FECA_CBs) */
	vCreateMpuEntry(MPU_REGION_FECA, FECA_RAM_BASE_ADDR, FECA_RAM_END_ADDR);
	/* VSPA Buffer Access */
#ifdef LA12XX_DRIVER_PCI_LAT_FP
	/* VSPA[0-7] Buffere access */
  	vCreateMpuEntry(MPU_REGION_VSPA, VSPA0_DMEM_BASE_ADDR, VSPA7_END_ADDR);
#else
  	vCreateMpuEntry(MPU_REGION_VSPA, VSPA0_DMEM_BASE_ADDR, VSPA0_DMEM_END_ADDR);
#endif
	/* PCIe1 */
	vCreateMpuEntry(MPU_REGION_PCIE1, PCIE1_PHY_ADDR, PCIE1_END_ADDR);
	/* PCIe2 */
	vCreateMpuEntry(MPU_REGION_PCIE2, PCIE2_PHY_ADDR, PCIE2_END_ADDR);
	/* DMEM_EXT */
	if ( __get_soc_revision() == GEUL_SVR_REVB_VAL) {
		vCreateMpuEntry(MPU_REGION_DMEM_EXT, EXT_DMEM_START, EXT_DMEM_END_B0);
	} else {
		vCreateMpuEntry(MPU_REGION_DMEM_EXT, EXT_DMEM_START, EXT_DMEM_END);
	}
	/* RW CACHEABLE DATA area in PEBM */
#ifdef GEUL_LA1238CPE
	vCreateMpuEntry(MPU_REGION_CDATA, (uint32_t)&cshdata_start, (uint32_t)&cshdata_end);
#endif
	/* CORE SRAM */
	if ( __get_soc_revision() == GEUL_SVR_REVB_VAL) {
		vCreateMpuEntry(MPU_REGION_CORE_SMEM, CORE_SMEM_BASE_ADDR, CORE_SMEM_END_ADDR);
		vCreateMpuEntry(MPU_REGION_CORE_SMEM_TEXT, (uint32_t)&core_smem_text_start, (uint32_t)&core_smem_text_end);
		vCreateMpuEntry(MPU_REGION_PEB_PORT3, PEBM_PORT3_BASE_ADDR, PEBM_PORT3_END_ADDR);
	}
	mtspr( MPU0CSR0, MPU0CSR0_REG );
}
