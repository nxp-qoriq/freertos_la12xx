// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2024 NXP
 */

#ifndef __MPU_H__
#define __MPU_H__

#include "bit.h"

#define MAS0                624
#define MAS1                625
#define MAS2                626
#define MAS3                627
#define MPU0CSR0            1014
#define MPU0CFG             692


#define MAS0_G_SHIFT        1                       /* Guarded */
#define MAS0_G              BIT( MAS0_G_SHIFT )     /* Guarded */
#define MAS0_I_SHIFT        3                       /* Cache Inhibited */
#define MAS0_I              BIT( MAS0_I_SHIFT )     /* Cache Inhibited */
#define MAS0_SX_SR_SHIFT    8                       /* Supervisor Mode Execute / Read Permission */
#define MAS0_SX_SR          BIT( MAS0_SX_SR_SHIFT ) /* Supervisor Mode Execute / Read Permission */
#define MAS0_UX_UR_SHIFT    9                       /* User Mode Execute / Read Permission */
#define MAS0_UX_UR          BIT( MAS0_UX_UR_SHIFT ) /* User Mode Execute / Read Permission */
#define MAS0_SW_SHIFT       10                      /* Supervisor Mode Write / Read Permission */
#define MAS0_SW             BIT( MAS0_SW_SHIFT )    /* Supervisor Mode Write / Read Permission */
#define MAS0_UW_SHIFT       11                      /* User Mode Write Permission */
#define MAS0_UW             BIT( MAS0_UW_SHIFT )    /* User Mode Write Permission */
#define MAS0_SHD_SHIFT      23                      /* Shared Entry Select */
#define MAS0_SHD            BIT( MAS0_SHD_SHIFT )   /* Shared Entry Select */
#define MAS0_INST_SHIFT     24                      /* Instruction Entry */
#define MAS0_INST           BIT( MAS0_INST_SHIFT )  /* Instruction Entry */
#define MAS0_SEL_0_SHIFT    28                      /* Selects MPU for access */
#define MAS0_SEL_0          BIT( MAS0_SEL_0_SHIFT ) /* Selects MPU for access */
#define MAS0_SEL_1_SHIFT    29                      /* Selects MPU for access */
#define MAS0_SEL_1          BIT( MAS0_SEL_1_SHIFT ) /* Selects MPU for access */
#define MAS0_IPROT_SHIFT    30                      /* IPROT */
#define MAS0_IPROT          BIT( MAS0_IPROT_SHIFT ) /* IPROT */
#define MAS0_VALID_SHIFT    31                      /* MPU Entry Valid */
#define MAS0_VALID          BIT( MAS0_VALID_SHIFT ) /* MPU Entry Valid */

/*MPU has region descriptor tables
 * Region's are primarily divided into:
 * 1. INST
 * 2. DATA
 *
 * INST has 6 entries for itself
 * DATA has 12 entries for itself
 * INST/DATA has 6 shared entries
 * */

/* INST entry indexes */
#define MAS0_ESEL_INST_0           0x00000000
#define MAS0_ESEL_INST_1           0x00010000
#define MAS0_ESEL_INST_2           0x00020000
#define MAS0_ESEL_INST_3           0x00030000
#define MAS0_ESEL_INST_4           0x00040000
#define MAS0_ESEL_INST_5           0x00050000

/* DATA entry indexes */
#define MAS0_ESEL_DATA_0           0x00000000
#define MAS0_ESEL_DATA_1           0x00010000
#define MAS0_ESEL_DATA_2           0x00020000
#define MAS0_ESEL_DATA_3           0x00030000
#define MAS0_ESEL_DATA_4           0x00040000
#define MAS0_ESEL_DATA_5           0x00050000
#define MAS0_ESEL_DATA_6           0x00060000
#define MAS0_ESEL_DATA_7           0x00070000
#define MAS0_ESEL_DATA_8           0x00080000
#define MAS0_ESEL_DATA_9           0x00090000
#define MAS0_ESEL_DATA_10          0x000a0000
#define MAS0_ESEL_DATA_11          0x000b0000

/* INST or DATA entry indexes
 * IMP: Use always with MAS0_SHD */
#define MAS0_ESEL_INST_DATA_0      0x00000000
#define MAS0_ESEL_INST_DATA_1      0x00010000
#define MAS0_ESEL_INST_DATA_2      0x00020000
#define MAS0_ESEL_INST_DATA_3      0x00030000
#define MAS0_ESEL_INST_DATA_4      0x00040000
#define MAS0_ESEL_INST_DATA_5      0x00050000

#define MPU0CSR0_MPUBYPSR_SHIFT    15                             /* MPU Bypass Supervisor Read Access */
#define MPU0CSR0_MPUBYPSW_SHIFT    14                             /* MPU Bypass Supervisor Write Access */
#define MPU0CSR0_MPUBYPSX_SHIFT    13                             /* MPU Bypass Supervisor Instruction Access */

#define MPU0CSR0_MPUEN_SHIFT       0                              /* MPU Enable */
#define MPU0CSR0_MPUBYPSR          BIT( MPU0CSR0_MPUBYPSR_SHIFT ) /* MPU Enable */
#define MPU0CSR0_MPUBYPSW          BIT( MPU0CSR0_MPUBYPSW_SHIFT ) /* MPU Enable */
#define MPU0CSR0_MPUBYPSX          BIT( MPU0CSR0_MPUBYPSX_SHIFT ) /* MPU Enable */
#define MPU0CSR0_MPUEN             BIT( MPU0CSR0_MPUEN_SHIFT )    /* MPU Enable */

#define MPU0CSR0_REG               MPU0CSR0_MPUEN

#define MAS1_REG                   0

#define MPU_REGION_xCSR            ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_0 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I | MAS0_G )
#define MPU_REGION_DMEM            ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_1 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I | MAS0_G )
#define MPU_REGION_SMEM            ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_2 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_HIF             ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_3 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_RODATA          ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_4 | MAS0_UX_UR | MAS0_SX_SR )
#define MPU_REGION_DATA            ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_5 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_FECA            ( MAS0_VALID |              MAS0_SEL_1 | MAS0_ESEL_DATA_6 | MAS0_UW | MAS0_SW |             MAS0_SX_SR | MAS0_I | MAS0_G )
#define MPU_REGION_PCIE1           ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_7 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#ifdef LA12XX_DRIVER_PCI_LAT_FP
/* Enable VSPA[0-7] memory read/write access for FPGA DMA */
#define MPU_REGION_VSPA            ( MAS0_VALID |              MAS0_SEL_1 | MAS0_ESEL_DATA_8 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I | MAS0_G )
#else
#define MPU_REGION_VSPA            ( MAS0_VALID |              MAS0_SEL_1 | MAS0_ESEL_DATA_8 |                    MAS0_UX_UR | MAS0_SX_SR | MAS0_I | MAS0_G )
#endif
#define MPU_REGION_HRAM            ( MAS0_VALID |              MAS0_SEL_1 | MAS0_ESEL_DATA_9 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I | MAS0_G )
#define MPU_REGION_FSPI       	   ( MAS0_VALID |              MAS0_SEL_1 | MAS0_ESEL_DATA_10 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I | MAS0_G )
#define MPU_REGION_SMEM_TEXT_DATA  ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_SMEM_DATA  ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_FSPI_DMEM       ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_FSPI_RX_TX_FIFO ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_CORE_SMEM_TEXT_DATA  ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_CORE_SMEM_DATA  ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_DATA_11 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )

#define MPU_REGION_TEXT            ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_0 | MAS0_UX_UR | MAS0_SX_SR | MAS0_INST )
#if (defined LA12XX_DRIVER_PCI) || (defined LA12XX_DRIVER_PCI_LAT_FP)
#define MPU_REGION_PCIE2           ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_DATA_4 | MAS0_SHD | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#else
#define MPU_REGION_PCIE2           ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_1 | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_INST )
#endif
#define MPU_REGION_SMEM_TEXT       ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_2 | MAS0_UX_UR | MAS0_SX_SR | MAS0_INST )
#define MPU_REGION_CORE_SMEM_TEXT  ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_3 | MAS0_UX_UR | MAS0_SX_SR | MAS0_INST )

#define MPU_REGION_DMEM_EXT        ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_DATA_0 | MAS0_SHD | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_CDATA           ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_DATA_1 | MAS0_SHD | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR)
#define MPU_REGION_CORE_SMEM       ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_DATA_2 | MAS0_SHD | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )
#define MPU_REGION_PEB_PORT3       ( MAS0_VALID | MAS0_IPROT | MAS0_SEL_1 | MAS0_ESEL_INST_DATA_3 | MAS0_SHD | MAS0_UW | MAS0_SW | MAS0_UX_UR | MAS0_SX_SR | MAS0_I )

#ifndef __ASSEMBLY__
void vCreateMpuEntry( u32 ulMpuEntry, u32 start_addr, u32 end_addr );
void vDeleteMpuEntry( u32 ulMpuEntry );
void vMpuEnable( void );
#endif

#endif /* ifndef __MPU_H__ */
