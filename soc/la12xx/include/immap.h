// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef __IMMAP_H__
#define __IMMAP_H__

#include "config.h"

#define CONFIG_WDOG_LE

#define MPIC_BASE_ADDR		(CCSR_BASE_ADDR + 0x2040000)

#define UART_BASE_ADDR_CORE0 (CCSR_BASE_ADDR + 0x21c0500)
#define UART_BASE_ADDR_CORE1 (CCSR_BASE_ADDR + 0x21c0600)
#define UART_BASE_ADDR_CORE2 (CCSR_BASE_ADDR + 0x21d0500)
#define UART_BASE_ADDR_CORE3 (CCSR_BASE_ADDR + 0x21d0600)
#define UART_BASE_ADDR_CORE4 (CCSR_BASE_ADDR + 0x21e0500)
#define UART_BASE_ADDR_CORE5 (CCSR_BASE_ADDR + 0x21e0600)

#define DCFG_OFFSET             0x01e00000
#define CCSR_DCFG_BASE_ADDR     (CCSR_BASE_ADDR + DCFG_OFFSET)
#define BOOT_RELEASE_REGISTER    0xF9E60060

#define SCRATCHRW1_OFFSET       0x200
#define SCRATCHRW2_OFFSET       0x204
#define SCRATCHRW3_OFFSET       0x208
#define SCRATCHRW4_OFFSET       0x20C
#define SCRATCHRW5_OFFSET       0x210
#define FLEXSPICR1_OFFSET       0x900

#define TBGEN_TSTAMP_TRIGGER_OFFSET (0x4000)

#define SCRATCHRW1_ADDR         (CCSR_DCFG_BASE_ADDR + SCRATCHRW1_OFFSET)
#define SCRATCHRW2_ADDR         (CCSR_DCFG_BASE_ADDR + SCRATCHRW2_OFFSET)
#define SCRATCHRW3_ADDR         (CCSR_DCFG_BASE_ADDR + SCRATCHRW3_OFFSET)
#define SCRATCHRW4_ADDR         (CCSR_DCFG_BASE_ADDR + SCRATCHRW4_OFFSET)
#define SCRATCHRW5_ADDR         (CCSR_DCFG_BASE_ADDR + SCRATCHRW5_OFFSET)

#define FLEXSPICR1_ADDR         (CCSR_DCFG_BASE_ADDR + FLEXSPICR1_OFFSET)

#define PCIE_DEV_ID_REG_OFFSET	0x2
#define AEM_BASE_ADDR		(CCSR_BASE_ADDR + 0x1320000)
#define SERDES1_BASE_ADDR	(CCSR_BASE_ADDR + 0x31E0000)
#define PMUXCR_BASE_ADDR_BANK1	(CCSR_BASE_ADDR + 0x1ff0e00)
#define PMUXCR_BASE_ADDR_BANK2	(CCSR_BASE_ADDR + 0x1ff4e00)
#define PMUX2CR_BASE_ADDR_IIC	(PMUXCR_BASE_ADDR_BANK1 + 0x4)
#define PMUX8CR_BASE_ADDR_IIC	(PMUXCR_BASE_ADDR_BANK2 + 0xC)
#define DCFG_BASE_ADDR		(CCSR_BASE_ADDR + 0x1e00000)
#define SCFG_BASE_ADDR		(CCSR_BASE_ADDR + 0x1e10000)
#define PCIE1_BASE_ADDR		(CCSR_BASE_ADDR + 0x3400000)
#define PCIE2_BASE_ADDR		(CCSR_BASE_ADDR + 0x3500000)
#define PCIE1_CONTROL_BASE_ADDR	(CCSR_BASE_ADDR + 0x34C0000)
#define PCIE2_CONTROL_BASE_ADDR	(CCSR_BASE_ADDR + 0x35C0000)
#define PCIE1_DEV_ID_BASE_ADDR	(PCIE1_BASE_ADDR + PCIE_DEV_ID_REG_OFFSET)
#define PCIE1_1238_DEV_ID	0x1c42

#define VSPA_BASE_ADDR          (CCSR_BASE_ADDR + 0x1000000)
#define VSPA_INST_BASE_ADDR(x)  (VSPA_BASE_ADDR + (x) * 0x4000)

#define QDMA_BASE_ADDR		(CCSR_BASE_ADDR + 0x22c0000)
#define WDOG_BASE_ADDR		(CCSR_BASE_ADDR + 0x23c0000)

#define PCIE_MSI_ADDR_REG	(0x54)
#define PCIE_MSI_DATA_REG_1	(0x5c)

#define DCFG_SCRATCH10_OFFSET   0x224
#define DCFG_SCRATCH11_OFFSET   0x228

#define PCTB_BASE_ADDR          (CCSR_BASE_ADDR + 0x1e30000)

/* CCSR Reset base addr */
#define CCSR_RST_BASE_ADDRESS	(CCSR_BASE_ADDR + 0x1E60000)

/* I2C Base addr */
#define I2C1_BASE_ADDR           (CCSR_BASE_ADDR + 0x01150000)
#define I2C1_IIC1_GPIO_SCL       18
#define I2C1_IIC1_GPIO_SDA       19

#define I2C2_BASE_ADDR           (CCSR_BASE_ADDR + 0x01154000)
#define I2C2_IIC2_GPIO_SCL       20
#define I2C2_IIC2_GPIO_SDA       21

#define I2C3_BASE_ADDR           (CCSR_BASE_ADDR + 0x01158000)
#define I2C3_IIC3_GPIO_SCL       22
#define I2C3_IIC3_GPIO_SDA       23

#define I2C4_BASE_ADDR           (CCSR_BASE_ADDR + 0x0115C000)
#define I2C4_IIC4_GPIO_SCL       24
#define I2C4_IIC4_GPIO_SDA       25

#define I2C5_BASE_ADDR           (CCSR_BASE_ADDR + 0x01160000)
#define I2C5_IIC5_GPIO_SCL       26
#define I2C5_IIC5_GPIO_SDA       27

#define I2C6_BASE_ADDR           (CCSR_BASE_ADDR + 0x01164000)
#define I2C6_IIC6_GPIO_SCL       28
#define I2C6_IIC6_GPIO_SDA       29

#define I2C7_BASE_ADDR           (CCSR_BASE_ADDR + 0x01284000)
#define I2C8_BASE_ADDR           (CCSR_BASE_ADDR + 0x01288000)

/* I2C Slave Base addr */
#ifdef GEUL_LA1246
#define EEPROM_BASE_ADDRESS         0x50
#endif /* GEUL_LA1246 */

/* TMU Base addr */
#define TMU_BASE_ADDR           (CCSR_BASE_ADDR + 0x1F80000)

#define TMU_BASE_ADDRESS            0x4c

#ifdef GEUL_LA1224
#define PCA9547PWMUX_BASE_ADDRESS   0x77
#define TMU_CHANNEL_EN              (1 << 1)
#define VID_CHANNEL_EN              (1 << 2)

/* I2C VID Slave address */
#define VID_BASE_ADDRESS	0x46
#define VID_PAGE_SEL		0x0
#define VID_READ_VOUT		0x8B

#define VID_PAGE_0		0x0

#define PCAL6524_BASE_ADDR			0x22
#define IO_EXAPNDER_INPUT_REG		0x2
#define IO_EXAPNDER_CONF_REG		0xE
#define BOARD_REV_SHIFT_MASK		6
#define BOARD_REV_MASK				3
#endif /* GEUL_LA1224 */

#ifndef __ASSEMBLER__
#include <types.h>
#include <stdint.h>

/* Pin Mux configuration registers */
struct ccsr_pmux {
	uint32_t	ulPMuxCR[ 8 ];	/* PMUXCR1 - PMUXCR8*/
#define	PMUXCR0_UART_PIN	0x0
};

#define DCSR_SVR_REV_MASK	(0xffff0000)
#define DCSR_SVR_REVA_MASK	(0x81520000)
#define DCSR_SVR_REVB_MASK	(0x81530000)

/* Global Utilities Block */
struct ccsr_dcsr {
	uint32_t	ulPorsr1;		/* POR status 1 */
	uint32_t	ulPorsr2;		/* POR status 2 */
	uint8_t		ucRes008[0x60-0x08];
	uint32_t	ulFusesr;	/* Fuse status register */
	uint8_t		ucRes064[0x70-0x64];
	uint32_t	ulDevdisr1;	/* Device disable control 1 */
	uint32_t	ulDevdisr2;	/* Device disable control 2 */
	uint32_t	ulDevdisr3;	/* Device disable control 3 */
	uint32_t	ulDevdisr4;	/* Device disable control 4 */
	uint32_t	ulDevdisr5;	/* Device disable control 5 */
	uint8_t		ucRes084[0xa0-0x84];
	uint32_t	ulPvr;		/* Processor version register */
	uint32_t	ulSvr;		/* System version */
	uint8_t		ucRes0a8[0x200-0xa8];
	uint32_t	ulScratchrw[32];	/* Scratch Read/Write */
	uint8_t		ucRes280[0x300-0x280];
	uint32_t	ulScratchw1r[4];	/* Scratch Read (Write once) */
	uint8_t		ucRes858[0xbfc-0x310];
};

struct ccsr_reset {
	u32 rstcr;			/* 0x000 */
	u32 rstcrsp;			/* 0x004 */
	u8 res_008[0x10 - 0x08];	/* 0x008 */
	u32 rstrqmr1;			/* 0x010 */
	u32 rstrqmr2;			/* 0x014 */
	u32 rstrqsr1;			/* 0x018 */
	u32 rstrqsr2;			/* 0x01c */
	u32 rstrqwdtmrl;		/* 0x020 */
	u32 rstrqwdtmru;		/* 0x024 */
	u8 res_028[0x30 - 0x28];	/* 0x028 */
	u32 rstrqwdtsrl;		/* 0x030 */
	u32 rstrqwdtsru;		/* 0x034 */
	u8 res_038[0x60 - 0x38];	/* 0x038 */
	u32 brrl;			/* 0x060 */
	u32 brru;			/* 0x064 */
	u8 res_068[0x80 - 0x68];	/* 0x068 */
	u32 pirset;			/* 0x080 */
	u32 pirclr;			/* 0x084 */
	u8 res_088[0x90 - 0x88];	/* 0x088 */
	u32 brcorenbr;			/* 0x090 */
	u8 res_094[0x100 - 0x94];	/* 0x094 */
	u32 rcw_reqr;			/* 0x100 */
	u32 rcw_completion;		/* 0x104 */
	u8 res_108[0x110 - 0x108];	/* 0x108 */
	u32 pbi_reqr;			/* 0x110 */
	u32 pbi_completion;		/* 0x114 */
	u8 res_118[0xa00 - 0x118];	/* 0x118 */
	u32 qmbm_warmrst;		/* 0xa00 */
	u32 soc_warmrst;		/* 0xa04 */
	u8 res_a08[0xbf8 - 0xa08];	/* 0xa08 */
	u32 ip_rev1;			/* 0xbf8 */
	u32 ip_rev2;			/* 0xbfc */
};

struct ccsr_serdes {
	struct {
		u32     rstctl; /* Reset Control Register */
		u32     pllcr0; /* PLL Control Register 0 */
		u32     pllcr1; /* PLL Control Register 1 */
		u32     pllcr2; /* PLL Control Register 2 */
		u32     pllcr3; /* PLL Control Register 3 */
		u32     pllcr4; /* PLL Control Register 4 */
		u32     pllcr5; /* PLL Control Register 5 */
		u8      res[0x20 - 0x1c];
	} bank[2];
	u8      res1[0x90 - 0x40];
	u32     srdstcalcr;     /* TX Calibration Control */
	u32     srdstcalcr1;    /* TX Calibration Control1 */
	u8      res2[0xa0 - 0x98];
	u32     srdsrcalcr;     /* RX Calibration Control */
	u32     srdsrcalcr1;    /* RX Calibration Control1 */
	u8      res3[0xb0 - 0xa8];
	u32     srdsgr0;        /* General Register 0 */
	u8      res4[0x800 - 0xb4];
	struct serdes_lane {
		u32     gcr0;   /* General Control Register 0 */
		u32     gcr1;   /* General Control Register 1 */
		u32     gcr2;   /* General Control Register 2 */
		u32     ssc0;   /* Speed Switch Control 0 */
		u32     rec0;   /* Receive Equalization Control 0 */
		u32     rec1;   /* Receive Equalization Control 1 */
		u32     tec0;   /* Transmit Equalization Control 0 */
		u32     ssc1;   /* Speed Switch Control 1 */
		u8      res1[0x840 - 0x820];
	} lane[8];
	u8 res5[0x19fc - 0xa00];
};

/* AEM Registers */
struct ccsr_aem {
	u32	err_detect;
	u32	err_status;
	u32	int_enable;
	u8	res[0x100 - 0x08];
	u32	capture_addr;
	u32	capture_ext_addr;
	u8	res1[0x110 - 0x104];
	u32	capture_attr1;
	u32	capture_attr2;
	u32	capture_attr3;
};

#endif /* __ASSEMBLER__ */

#endif /* __IMMAP_H__ */
