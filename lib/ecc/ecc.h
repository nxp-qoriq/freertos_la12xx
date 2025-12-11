// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#include <mpic.h>
void vEccDisable() ;
void vEccEnable() ;

bool_t ECCIRQHandlermulti(uint32_t ulIrq_No, void * vDev_Data);


#define ECC_MULTIBIT_IRQ_NUM 			25
#define ECC_SRAM_SET   				3
#define ECC_PEBMEM_SET 				4
#define DCFG_DSCR OFFSET         		0X80000
#define DCFG_DSCR_BASE_ADDR      		(DCSR_BASE_ADDR + 0x80000)


#define MULTI_BIT_SRAM_CHECK   			3
#define MULTI_BIT_PEBMEM_CHECK  		4

//ECC control Register 2
#define ECC_CNTRL_REG2_OFFSET     		0x524
#define ECC_CNTRL_REG2_ADDR      		(DCFG_DSCR_BASE_ADDR + ECC_CNTRL_REG2_OFFSET)
#define ECC_CNTRL_REG1_OFFSET     		0x520
#define ECC_CNTRL_REG1_ADDR      		(DCFG_DSCR_BASE_ADDR + ECC_CNTRL_REG1_OFFSET)

//ECC Single Bit Status Register 2
#define SINGLE_BIT_ECC_STATUS_REG2_OFFSET 	0X534
#define SINGLE_BIT_ECC_STATUS_REG2_ADDR  	(DCFG_DSCR_BASE_ADDR + SINGLE_BIT_ECC_STATUS_REG_OFFSET)

//ECC Multi Bit Status Register 2
#define MULTI_BIT_ECC_STATUS_REG2_OFFSET  	0X544
#define MULTI_BIT_ECC_STATUS_REG2_ADDR 		(DCFG_DSCR_BASE_ADDR + MULTI_BIT_ECC_STATUS_REG2_OFFSET )


#define CHECK_BIT(addr, bit)         		in_le32((volatile uint32_t *) addr) &  (1 <<  bit)
#define SET_BIT(addr, bit)           		out_le32((volatile uint32_t *) addr, in_le32((volatile uint32_t *) addr) |  (1 <<  bit))
#define RESET_BIT(addr, bit)           		out_le32((volatile uint32_t *) addr, in_le32((volatile uint32_t *) addr) &  ~(1UL <<  bit))

