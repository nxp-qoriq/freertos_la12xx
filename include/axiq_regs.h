// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2017-2021 NXP
 */

#ifndef _SRC_AXIQ_REGS_H_
#define _SRC_AXIQ_REGS_H_

#include <types.h>
#include <immap.h>

/* SCFG Regs */
#define CONFIG_CONTROL0_OFFSET	0x0
#define SCFG_CONFIG_CONTROL0	( SCFG_BASE_ADDR + CONFIG_CONTROL0_OFFSET )

/* DCFG Regs */
#define DEVDISR3_OFFSET			0X78
#define DEVDISR3_BASE			( DCFG_BASE_ADDR + DEVDISR3_OFFSET )

/******************************************************************************
* Register Data Structures
******************************************************************************/
/* SCFG Config control 0 structure */
typedef struct ScfgConfigControl0 {
	volatile uint32 RESERVED_31_22:10;
	volatile uint32 SCFG_RES_CAL_MUX_SEL:1; /* select between "SCFG"/"Fuse" driven values */
	volatile uint32 SCFG_RES_CAL:1;	/* Resistance calibration for SCFG */
	volatile uint32 WDOGTOUT_POL_CTRL:1;	/* Bit for controlling polarity of WDOG_TOUT SoC Pin */
	volatile uint32 WDOGTOUT_MASK:1;	/* Bit for masking WDOGRES that drives WDOG_TOUT SoC Pin */
	volatile uint32 WDOGRES_MASK3:1; /* WDOGRES Mask bit for core3 */
	volatile uint32 WDOGRES_MASK2:1; /* WDOGRES Mask bit for core2 */
	volatile uint32 WDOGRES_MASK1:1; /* WDOGRES Mask bit for core1 */
	volatile uint32 WDOGRES_MASK0:1;	/* WDOGRES Mask bit for core0 */
	volatile uint32 RESERVED_13_12:2;
	volatile uint32 EONCE3:1;		/* Eonce enable for z720-3 */
	volatile uint32 EONCE2:1;		/* Eonce enable for z720-2 */
	volatile uint32 EONCE1:1;		/* Eonce enable for z720-1 */
	volatile uint32 EONCE0:1;		/* Eonce enable for z720-0 */
	volatile uint32 RESERVED_7:1;
	volatile uint32 LB_AXIQ_L1:2;	/* Control Loopback between AXIQ-L1 and LS-DCS1 */
	volatile uint32 RESERVED_4:1;
	volatile uint32 LB_AXIQ_L0:2;	/* Control Loopback between AXIQ-L0 and LS-DCS0 */
	volatile uint32 RESERVED_1:1;
	volatile uint32 LB_AXIQ_H:1;		/* Control Loopback between AXIQ-H and HS-DCS */
} ScfgConfigControl0_t;

/* DCFG Device Disable Register 3 structure */
typedef struct DcfgDeviceDisableReg3 {
	volatile uint32 RESERVED_31_26:6;
	volatile uint32 FECA:1;				/* FECA Block Disable */
	volatile uint32 HS_DCS_SUBSYSTEM:1;	/* HS_DCS_Subsystem Block Disable */
	volatile uint32 LS_DCS_SUBSYSTEM:1;	/* LS_DCS_Subsystem Block Disable */
	volatile uint32 PHY_SUBSYSTEM2:1;	/* VSPA4-VSPA7, AXIQ_H (PHY_Subsystem2) Block Disable */
	volatile uint32 PHY_SUBSYSTEM1:1;	/* VSPA0-VSPA3, AXIQ_L0, AXIQ_L1 (PHY_Subsystem1) Block Disable */
	volatile uint32 RESERVED_20_2:19;
	volatile uint32 PCIE2:1;				/* PCIe2 Block Disable */
	volatile uint32 PCIE1:1;				/* PCIe1 Block Disable */
} DcfgDeviceDisableReg3_t;

#endif /* _SRC_AXIQ_REGS_H_ */
