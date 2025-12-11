// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#ifndef _GEUL_AVI_DS_H_
#define _GEUL_AVI_DS_H_

#include "queue.h"

/**
 * @file	geul_avi_ds.h
 * @brief	This file contains the VSPA AVI data structure
 * 		and macros.
 * @addtogroup	VSPA_API
 * @{
 */

/*************************************************************
*			Defines
*************************************************************/
#define VSPA_BACKDOOR_HANDSHAKE   	( 0 )      /**< Simulator */

#define VSPA_CLEAR_PENDING_INTERRUPTS 	( 0x0000007F )

/* Keep seperate variable for Recv and Send IRQ to configure for particular
 * interrupt
 */
#define IRQEN_EN_MBOX_R			( 0x1000 )
#define IRQEN_EN_MBOX_W			( 0x4000 )

#define HOST_MBOX_MSG_IN_0_VALID	( 0x4 )
#define	SPM_BUFFER_BYTES		( 0x70 << 24 )
#define VSPA_BOOT_OK			( 0xF1000000 )
#define VSPA_SPM_BUF_ACK		( 0xF0700000 )

#define VSPA_MBOX0_STATUS_SHIFT 	( 12 )
#define VSPA_MBOX1_STATUS_SHIFT 	( 13 )
#define E200_MBOX0_STATUS_SHIFT 	( 14 )
#define E200_MBOX1_STATUS_SHIFT 	( 15 )
#define VSPA_MBOX0_STATUS 		( 1 << VSPA_MBOX0_STATUS_SHIFT )
#define VSPA_MBOX1_STATUS 		( 1 << VSPA_MBOX1_STATUS_SHIFT )
#define E200_MBOX0_STATUS 		( 1 << E200_MBOX0_STATUS_SHIFT )
#define E200_MBOX1_STATUS 		( 1 << E200_MBOX1_STATUS_SHIFT )

#define VSPA_CM4_Q_LEN			16
/*************************************************************
*		Enum & Typedef's
*************************************************************/
typedef enum IrqStat {
        IRQ_UNREGISTERED = 0,
        IRQ_REGISTERED
} IrqStat_t;

typedef uint8_t AviIntr_t;

/*************************************************************
*		Data Structures
*************************************************************/
typedef struct VspaRegs {
	uint32_t ulHwVersion;
	uint32_t ulSwVersion;
	uint32_t ulVcpuControl;
	uint32_t ulVspaIrqEn;
	uint32_t ulVspaStatus;
	uint32_t ulVcpuHostFlags0;
	uint32_t ulVcpuHostFlags1;
	uint32_t ulHostVcpuFlags0;
	uint32_t ulHostVcpuFlags1;
	uint8_t  ucRes28[ 0x28 - 0x24 ];
	uint32_t ulExtGoEna;
	uint32_t ulExtGoStat;
	uint32_t ulIllOpStatus;
	uint8_t  ucRes40[ 0x40 - 0x34 ];
	uint32_t ulParam0;
	uint32_t ulParam1;
	uint32_t ulParam2;
	uint32_t ulVcpuDmemBytes;
	uint32_t ulThreadCtrlStat;
	uint32_t ulProtFaultStat;
	uint32_t ulExceptionCtrl;
	uint32_t ulExceptionStat;
	uint32_t ulAxislvFlags0;
	uint32_t ulAxislvFlags1;
	uint32_t ulAxislvGoen0;
	uint32_t ulAxislvGeon1;
	uint32_t ulPlatIn0;
	uint8_t  ulRes80[ 0x80 - 0x74 ];
	uint32_t ulPlatOut0;
	uint8_t  ulRes98[ 0x98 - 0x84 ];
	uint32_t ulCycCounterMsb;
	uint32_t ulCycCounterLsb;
	uint8_t  ulResB0[ 0xB0 - 0xA0 ];
	uint32_t ulDmaDmemPramAddr;
	uint32_t ulDmaAxiAddress;
	uint32_t ulDmaAxiByteCnt;
	uint32_t ulDmaXfrCtrl;
	uint32_t ulDmaStatAbort;
	uint32_t ulDmaIrqStat;
	uint32_t ulDmaCompStat;
	uint32_t ulDmaXfrErrStat;
	uint32_t ulDmaCfgErrStat;
	uint32_t ulDmaXrunStat;
	uint32_t ulDmaGoStat;
	uint32_t ulDnaFifoStat;
	uint8_t  ulRes100[ 0x100 - 0xE0 ];
	uint32_t ulLdRfControl;
	uint32_t ulLdRfTbReal0;
	uint32_t ulLdRfTbImag0;
	uint32_t ulLdRfTbReal1;
	uint32_t ulLdRfTbImag1;
	uint32_t ulLdRfTbReal2;
	uint32_t ulLdRfTbImag2;
	uint32_t ulLdRfTbReal3;
	uint32_t ulLdRfTbImag3;
	uint32_t ulLdRfTbReal4;
	uint32_t ulLdRfTbImag4;
	uint32_t ulLdRfTbReal5;
	uint32_t ulLdRfTbImag5;
	uint32_t ulLdRfTbReal6;
	uint32_t ulLdRfTbImag6;
	uint32_t ulLdRfTbReal7;
	uint32_t ulLdRfTbImag7;
	uint8_t  ucRes400[ 0x400 - 0x144 ];
	uint32_t ulVcpuMode0;
	uint32_t ulVcpuMode1;
	uint32_t ulVcpuCreg0;
	uint32_t ulVcpuCreg1;
	uint32_t ulStUlVecLen;
	uint8_t  ulRes500[ 0x500 - 0x414 ];
	uint32_t ulGpIn[ 16 ];
	uint8_t  ulRes580[ 0x580 - 0x540 ];
	uint32_t ulGpOut[ 16 ];
	uint8_t  ulRes600[ 0x600 - 0x5c0 ];
	uint32_t ulDqmSmall;
	uint32_t ulDqmLargeMsb;
	uint32_t ulDqmLargeLsb;
	uint8_t  ulRes620[ 0x620 - 0x60c ];
	uint32_t ulVcpuDbgOut32;
	uint32_t ulVcpuDbgOut64Msb;
	uint32_t ulVcpuDbgOut64Lsb;
	uint32_t ulVcpuDbgIn32;
	uint32_t ulVcpuDbgIn64Msb;
	uint32_t ulVcpuDbgIn64Lsb;
	uint32_t ulVcpuDbgMboxStatus;
	uint8_t  ulRes640[ 0x640 - 0x63c ];
	uint32_t ulVcpuOut0Msb;
	uint32_t ulVcpuOut0Lsb;
	uint32_t ulVcpuOut1Msb;
	uint32_t ulVcpuOut1Lsb;
	uint32_t ulVcpuIn0Msb;
	uint32_t ulVcpuIn0Lsb;
	uint32_t ulVcpuIn1Msb;
	uint32_t ulVcpuIn1Lsb;
	uint32_t ulVcpuMboxStatus;
	uint8_t  ulRes680[ 0x680 - 0x664 ];
	uint32_t ulHostOut0Msb;
	uint32_t ulHostOut0Lsb;
	uint32_t ulHostOut1Msb;
	uint32_t ulHostOut1Lsb;
	uint32_t ulHostIn0Msb;
	uint32_t ulHostIn0Lsb;
	uint32_t ulHostIn1Msb;
	uint32_t ulHostIn1Lsb;
	uint32_t ulHostMboxStatus;
	uint8_t  ulRes700[ 0x700 - 0x6A4 ];
	uint32_t ulIppuControl;
	uint32_t ulIppuStatus;
	uint32_t ulIppuRc;
	uint32_t ulIppuArgBaseAddr;
	uint32_t ulIppuHwVer;
	uint32_t ulIppuSwVer;
	uint8_t  ulRes2000[ 0x2000 - 0x718 ];
} VspaRegs_t;

typedef struct VspaIntr {
	AviIntr_t      xIrqNo;
	IrqStat_t      xIrqState;
	bool      QueueInit;
	QueueHandle_t VspaToCm4QMbox;
} VspaIntr_t;

struct AviHandle {
#if VSPA_BACKDOOR_HANDSHAKE == 1
        uint8_t ucVspaHandshake;
#endif
        bool bVspaBoot;
        struct SpinLock *pxMailboxSpinlock[ VSPA_CORE_MAX ];
        VspaIntr_t xVspaIntrNo[ VSPA_CORE_MAX ][ VSPA_GROUP_MAX ];
};

/** @} */
#endif
