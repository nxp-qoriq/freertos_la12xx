// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef _AXIQ_H_
#define _AXIQ_H_

#include <types.h>
#include <axiq_regs.h>
#include <debug_console.h>
#include "mpic.h"


/******************************************************************************
*
* 							Defines
*
******************************************************************************/
#define DCFG_DEVICE_DISABLE_REG3_BIT_PHY_SUBSYSTEM1	21
#define DCFG_DEVICE_DISABLE_REG3_BIT_PHY_SUBSYSTEM2	22

#define SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_H	0
#define SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_L0	2
#define SCFG_CONFIG_CONTROL0_BIT_LB_AXIQ_L1	5

#define FLAG_0b11	0b11	/* Mask for Two-bit bit field */


/******************************************************************************
*
* 							Error Codes
*
******************************************************************************/
#define NUM_AXIQ_SUBSYSTEM     2
#define NUM_AXIQ_INTERRUPTS    3
#define NUM_AXIQ_IP_BLOCKS     NUM_AXIQ_INTERRUPTS

/******************************************************************************
*
* 								Enum Types
*
******************************************************************************/
typedef enum {
	AXIQ_H = 0,
	AXIQ_L0,
	AXIQ_L1
} AxiqNum_t;

typedef enum {
	LOOPBACK_MODE_DISABLED_0,
	INTER_LOOPBACK_MODE_ENABLED_1,
	INTRA_LOOPBACK_MODE_ENABLED_2,
	LOOPBACK_MODE_DISABLED_3
} AxiqMode_t;

typedef enum {
	SUBSYSTEM_DISABLE = 0,
	SUBSYSTEM_ENABLE
} SubsysStat_t;

typedef enum {
	AXI_IRQ_UNREGISTERED = 0,
	AXI_IRQ_REGISTERED
} IrqAxiqStat_t;
/******************************************************************************
*
* 							Data Structures
*
******************************************************************************/
typedef struct AxiqLibHndlr {
    IrqAxiqStat_t eIrqRegistered[ NUM_AXIQ_INTERRUPTS ];
    SubsysStat_t eSubsystemState[ NUM_AXIQ_SUBSYSTEM ];
    AxiqMode_t eAxiqMode[ NUM_AXIQ_IP_BLOCKS ];
} AxiqLibHndlr_t;

/******************************************************************************
*
* 						Function prototypes
*
******************************************************************************/
void vEnableAxiqHSubsystem2( AxiqLibHndlr_t * );
void vDisableAxiqHSubsystem2( AxiqLibHndlr_t * );
void vEnableAxiqLSubsystem1( AxiqLibHndlr_t * );
void vDisableAxiqLSubsystem1( AxiqLibHndlr_t * );

/* Control Loopback between AXIQ-H and HS-DCS */
void vLbConfigAxiqH( AxiqLibHndlr_t *, AxiqMode_t eMode );

/* Control Loopback between AXIQ-L0 and LS-DCS0 */
void vLbConfigAxiqL0( AxiqLibHndlr_t *, AxiqMode_t eMode );

/* Control Loopback between AXIQ-L1 and LS-DCS1 */
void vLbConfigAxiqL1( AxiqLibHndlr_t *, AxiqMode_t eMode );

void vRegisterAxiqErrorInterrupt( AxiqLibHndlr_t * pxAxiqLib, bIsrFunc AxiqIntHandler, void *AxiqData, AxiqNum_t eAxiqNum );
void vUnRegisterAxiqErrorInterrupt( AxiqLibHndlr_t * pxAxiqLib, AxiqNum_t eAxiqNum );
AxiqLibHndlr_t * pxAxiqInit( void );
void vAxiqClose( void );
#endif /* _AXIQ_H_ */
