// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef _EPU_H_
#define _EPU_H_

#include <types.h>

/*************************************************************
*			Defines
*************************************************************/
/* EPU - 4KB */
#define EPU_BASE    ( DCSR_BASE_ADDR + 0x45000 )

/* Counter #defines */

#define EPU_COUNTER_NUM_COUNTERS_MASK     0x1F

#define EPU_COUNTER_ENABLE                0x80000000

#define EPU_COUNTER_EDE_OFFSET            30
#define EPU_COUNTER_EDE_MASK              0x40000000

#define EPU_COUNTER_ISEL_OFFSET           28
#define EPU_COUNTER_ISEL_MASK             0x30000000

#define EPU_COUNTER_GEV_OFFSET            25
#define EPU_COUNTER_GEV_MASK              0x02000000

#define EPU_COUNTER_LEV_OFFSET            23
#define EPU_COUNTER_LEV_MASK              0x00800000

#define EPU_COUNTER_INPUT_PLATFORM_CLK    0

#define EPU_COUNTER_AC_OFFSET             16
#define EPU_COUNTER_AC_MASK               0x000F0000

#define GC_NUM_COUNTERS_MASK              0x1
#define GC_NUM_COMPARATORS                0x4

/* SCU #defines */

#define SCU_EPECR_IC0_SHIFT          30
#define SCU_EPECR_IC1_SHIFT          28
#define SCU_EPECR_IC2_SHIFT          26
#define SCU_EPECR_IC3_SHIFT          24
#define SCU_EPECR_IC_MASK            0x3
#define SCU_EPECR_ALL_IC_MASK        0xFF
#define SCU_EPECR_IC_SUFFICIENT      0b10
#define SCU_EPECR_IC_DISABLED        0b00

#define SCU_EPECR_IIE_EN             0b1
#define SCU_EPECR_IIE0_SHIFT         12
#define SCU_EPECR_IIE1_SHIFT         13
#define SCU_EPECR_IIE2_SHIFT         14
#define SCU_EPECR_IIE3_SHIFT         15

#define SCU_EPECR_ICE_EN             0b1
#define SCU_EPECR_ICE_SHIFT          5

#define SCU_EPECR_EDE_EN             0b1
#define SCU_EPECR_EDE_SHIFT          4

#define SCU_EPECR_SSE_EN             0b1
#define SCU_EPECR_SSE_SHIFT          2

#define SCU_EPECR_STS_EN             0b1
#define SCU_EPECR_STS_SHIFT          0

#define SCU_EPSMCR_ISEL_MASK         0x7F
#define SCU_EPSMCR_ALL_ISEL_MASK     0x7F7F7F7F
#define SCU_EPSMCR_ISEL0_SHIFT       24
#define SCU_EPSMCR_ISEL1_SHIFT       16
#define SCU_EPSMCR_ISEL2_SHIFT       8
#define SCU_EPSMCR_ISEL3_SHIFT       0

#define SCU_EPEVTCR_SCU_SEL_SHIFT    25
#define SCU_EPEVCTR_SCU_SEL_MASK     0xFE
#define SCU_EPEVTCR_DIR_EN           0b1

/*************************************************************
*		Enum & Typedef's
*************************************************************/

enum epu_counter_input_select
{
    FREE_RUNNING = 0,
    SCU_EVENT,
    EVT_NXT_CNTR,
    SEL_CNTR_MUX
};

enum epu_counter_event_trigger
{
    OVERFLOW = 0,
    CMP_MATCH
};

enum epu_counter_local_action
{
    NO_EFFECT = 0,
    INTR_REQ = 1,
    RST_CNTR = 2,
    FRZ_CNTR = 4,
    WATCH_TRACE = 8
};

enum epu_scu_sel
{
    EPU_SCU_0 = 0,
    EPU_SCU_1,
    EPU_SCU_2,
    EPU_SCU_3,
    EPU_SCU_4,
    EPU_SCU_5,
    EPU_SCU_6,
    EPU_SCU_7,
    EPU_SCU_8,
    EPU_SCU_9,
    EPU_SCU_10,
    EPU_SCU_11,
    EPU_SCU_12,
    EPU_SCU_13,
    EPU_SCU_14,
    EPU_SCU_15
};

enum epu_scu_input_sel
{
    EPU_SCU_INPUT_SEL0 = 0,
    EPU_SCU_INPUT_SEL1,
    EPU_SCU_INPUT_SEL2,
    EPU_SCU_INPUT_SEL3
};

enum epu_scu_core_cts
{
    EPU_SCU_CORE_CTS_0 = 0,
    EPU_SCU_CORE_CTS_1,
    EPU_SCU_CORE_CTS_2,
    EPU_SCU_CORE_CTS_3,
    EPU_SCU_CORE_CTS_4,
    EPU_SCU_CORE_CTS_5,
    EPU_SCU_CORE_CTS_6,
    EPU_SCU_CORE_CTS_7,
    EPU_SCU_CORE_CTS_8,
    EPU_SCU_CORE_CTS_9
};

enum epu_scu_input_control_sel
{
    EPU_SCU_INPUT_NA = 0,
    EPU_SCU_INPUT_RST,
    EPU_SCU_INPUT_OR,
    EPU_SCU_INPUT_AND
};

enum epu_event_input_sel
{
    EPU_EVT_INPUT_0 = 0
};

/*************************************************************
*		Data Structures
*************************************************************/

struct scu_event
{
    uint32_t isel;
    uint32_t isel_val;
};

struct epu_counter_cfg
{
    uint32_t edge_detect;
    uint32_t isel;
    uint32_t lt;
    uint32_t action;
    uint32_t gt;
    uint32_t comparator_val;
};

struct ulEpsmCRn
{
    uint32_t ulEpsmCR;
    uint8_t ulReserved[ 4 ];
};

typedef struct EpuRegs
{
    /* Global Control and Status Register */
    uint32_t ulEpgcr;
    uint8_t ulReserved10[ 0xF - 0x4 ];
    uint32_t ulEpesr;
    uint8_t ulReserved14[ 0x14 - 0x10 ];
    uint8_t ulReserved18[ 0x18 - 0x14 ];
    uint8_t ulReserved1C[ 0x1C - 0x18 ];
    uint32_t ulEpisr0;
    uint32_t ulEpisr1;
    uint32_t ulEpisr2;
    uint32_t ulEpisr3;
    uint32_t ulEpctrIsr0;
    uint32_t ulEpctrIsr1;
    uint32_t ulEpctrCsr;
    uint8_t ulReserved4F[ 0x4F - 0x44 ];
    uint32_t ulEpevtCRn[ 10 ];
    uint8_t ulReserved78[ 0x78 - 0x74 ];
    uint8_t ulReserved7C[ 0x7C - 0x78 ];
    uint8_t ulReserved8F[ 0x8F - 0x80 ];
    uint32_t ulEpxTrigCR;
    /* Counter Mux Control Register */
    uint8_t ulReserved100[ 0x100 - 0x94 ];
    uint32_t ulEpimCR0;
    uint32_t ulEpimCR1;
    uint32_t ulEpimCR2;
    uint32_t ulEpimCR3;
    uint32_t ulEpimCR4;
    uint32_t ulEpimCR5;
    uint32_t ulEpimCR6;
    uint32_t ulEpimCR7;
    uint32_t ulEpimCR8;
    uint32_t ulEpimCR9;
    uint32_t ulEpimCR10;
    uint32_t ulEpimCR11;
    uint32_t ulEpimCR12;
    uint32_t ulEpimCR13;
    uint32_t ulEpimCR14;
    uint32_t ulEpimCR15;
    uint32_t ulEpimCR16;
    uint32_t ulEpimCR17;
    uint32_t ulEpimCR18;
    uint32_t ulEpimCR19;
    uint32_t ulEpimCR20;
    uint32_t ulEpimCR21;
    uint32_t ulEpimCR22;
    uint32_t ulEpimCR23;
    uint32_t ulEpimCR24;
    uint32_t ulEpimCR25;
    uint32_t ulEpimCR26;
    uint32_t ulEpimCR27;
    uint32_t ulEpimCR28;
    uint32_t ulEpimCR29;
    uint32_t ulEpimCR30;
    uint32_t ulEpimCR31;
    uint8_t ulReserved200[ 0x200 - 0x180 ];
    /* SCU Mux Control Register */
    struct ulEpsmCRn ulEpsmCRN[ 16 ];
    uint8_t ulReserved300[ 0x300 - 0x280 ];
    /* Combining/Sequencing Control Registers */
    uint32_t ulEpecrn[ 16 ];
    uint32_t ulEpimpeCR1;
    uint32_t ulEpimpeCR2;
    uint32_t ulEpimpeCR3;
    uint32_t ulEpimpeCR4;
    uint32_t ulEpimpeCR5;
    uint32_t ulEpimpeCR6;
    uint32_t ulEpimpeCR7;
    uint32_t ulEpimpeCR8;
    uint32_t ulEpimpeCR9;
    uint32_t ulEpimpeCR10;
    uint32_t ulEpimpeCR11;
    uint32_t ulEpimpeCR12;
    uint32_t ulEpimpeCR13;
    uint32_t ulEpimpeCR14;
    uint32_t ulEpimpeCR15;
    uint8_t ulRePIMserved380[ 0x380 - 0x340 ];
    uint8_t ulRePIMserved400[ 0x400 - 0x380 ];
    /* Action CoPIMntrol/Status Registers */
    uint32_t ulEpimpaCRn[ 7 ];
    uint32_t ulEpaCR7;
    uint32_t ulEpaCR8;
    uint32_t ulEpaCR9;
    uint32_t ulEpaCR10;
    uint32_t ulEpaCR11;
    uint32_t ulEpaCR12;
    uint32_t ulEpaCR13;
    uint32_t ulEpaCR14;
    uint32_t ulEpaCR15;
    uint8_t ulReserved480[ 0x480 - 0x440 ];
    uint32_t ulEpgaCR0;
    uint32_t ulEpgaCR1;
    uint32_t ulEpgaCR2;
    uint32_t ulEpgaCR3;
    uint32_t ulEpgaCR4;
    uint32_t ulEpgaCR5;
    uint32_t ulEpgaCR6;
    uint32_t ulEpgaCR7;
    uint32_t ulEpgaCR8;
    uint32_t ulEpgaCR9;
    uint32_t ulEpgaCR10;
    uint32_t ulEpgaCR11;
    uint32_t ulEpgaCR12;
    uint32_t ulEpgaCR13;
    uint32_t ulEpgaCR14;
    uint32_t ulEpgaCR15;
    uint8_t ulReserved500[ 0x500 - 0x4C0 ];
    uint8_t ulReserved540[ 0x540 - 0x500 ];
    uint32_t ulEpcTrgCRa0;
    uint32_t ulEpcTrgCRb0;
    uint32_t ulEpcTrgCRa1;
    uint32_t ulEpcTrgCRb1;
    uint32_t ulEpcTrgCRa2;
    uint32_t ulEpcTrgCRb2;
    uint32_t ulEpcTrgCRa3;
    uint32_t ulEpcTrgCRb3;
    uint8_t ulReserved580[ 0x580 - 0x560 ];
    uint8_t ulEPEGCR0;
    uint32_t ulEPEGCR1;
    uint32_t ulEPEGCR2;
    uint8_t ulReserved590[ 0x590 - 0x58C ];
    uint8_t ulReserved600[ 0x600 - 0x590 ];
    /* FSM Control and Status Registers */
    uint32_t ulEpfsmSR0;
    uint8_t ulReserved610[ 0x610 - 0x604 ];
    uint32_t ulEpfsmCmpr0;
    uint32_t ulEpfsmCmpr1;
    uint32_t ulEpfsmCmpr2;
    uint32_t ulEpfsmCmpr3;
    uint32_t ulEpfsmCmpr4;
    uint32_t ulEpfsmCmpr5;
    uint32_t ulEpfsmCmpr6;
    uint32_t ulEpfsmCmpr7;
    uint32_t ulEpfsmCR0;
    uint32_t ulEpfsmCR1;
    uint32_t ulEpfsmCR2;
    uint32_t ulEpfsmCR3;
    uint32_t ulEpfsmCR4;
    uint32_t ulEpfsmCR5;
    uint32_t ulEpfsmCR6;
    uint32_t ulEpfsmCR7;
    uint8_t ulReserved700[ 0x700 - 0x650 ];
    uint8_t ulReserved800[ 0x800 - 0x700 ];
    /* Counter Control Registers */
    uint32_t ulEpccrn[ 32 ];
    uint8_t ulReserved900[ 0x900 - 0x880 ];
    /* Counter Compare Registers */
    uint32_t ulEpcmPRn[ 32 ];
    uint8_t ulReservedA00[ 0xA00 - 0x980 ];
    /* Counter Registers */
    uint32_t ulEpctrn[ 32 ];
    uint8_t ulReservedB00[ 0xB00 - 0xA80 ];
    /* Counter Capture Registers */
    uint32_t ulEpcaPRn[ 32 ];
    uint8_t ulReservedF00[ 0xF00 - 0xB80 ];
    uint32_t ulEprsrv59;
    uint32_t ulEprsrv58;
    uint32_t ulEprsrv57;
    uint32_t ulEprsrv56;
    uint32_t ulEprsrv55;
    uint32_t ulEprsrv54;
    uint32_t ulEprsrv53;
    uint32_t ulEprsrv52;
    uint32_t ulEprsrv51;
    uint32_t ulEprsrv50;
    uint32_t ulEprsrv49;
    uint32_t ulEprsrv48;
    uint32_t ulEprsrv47;
    uint32_t ulEprsrv46;
    uint32_t ulEprsrv45;
    uint32_t ulEprsrv44;
    uint32_t ulEprsrv43;
    uint32_t ulEprsrv42;
    uint32_t ulEprsrv41;
    uint32_t ulEprsrv40;
    uint32_t ulEprsrv39;
    uint32_t ulEprsrv38;
    uint32_t ulEprsrv37;
    uint32_t ulEprsrv36;
    uint32_t ulEprsrv35;
    uint32_t ulEprsrv34;
    uint32_t ulEprsrv33;
    uint32_t ulEprsrv32;
    uint32_t ulEprsrv31;
    uint32_t ulEprsrv30;
    uint32_t ulEprsrv29;
    uint32_t ulEprsrv28;
    uint32_t ulEprsrv27;
    uint32_t ulEprsrv26;
    uint32_t ulEprsrv25;
    uint32_t ulEprsrv24;
    uint32_t ulEprsrv23;
    uint32_t ulEprsrv22;
    uint32_t ulEprsrv21;
    uint32_t ulEprsrv20;
    uint32_t ulEprsrv19;
    uint32_t ulEprsrv18;
    uint32_t ulEprsrv17;
    uint32_t ulEprsrv16;
    uint32_t ulEprsrv15;
    uint32_t ulEprsrv14;
    uint32_t ulEprsrv13;
    uint32_t ulEprsrv12;
    uint32_t ulEprsrv11;
    uint32_t ulEprsrv10;
    uint32_t ulEprsrv9;
    uint32_t ulEprsrv8;
    uint32_t ulEprsrv7;
    uint32_t ulEprsrv6;
    uint32_t ulEprsrv5;
    uint32_t ulEprsrv4;
    uint32_t ulEprsrv3;
    uint32_t ulEprsrv2;
    uint32_t ulEprsrv1;
    uint32_t ulEprsrv0;
    uint32_t ulEphsr3;
    uint32_t ulEphsr2;
    uint32_t ulEphsr1;
    uint32_t ulEphsr0;
} EpuRegs_t;

/*************************************************************
*			Function Declarations
*************************************************************/

/* Counters */
void vEpuGlobalInit();
void vEpuCounterEnable( uint8_t epu_counter );
int iEpuCounterConfig( uint8_t epu_counter,
                       struct epu_counter_cfg * cfg );
void vEpuCounterDisable( uint8_t epu_counter );
int iEpuCounterCfgEdgeDetect( uint8_t epu_counter,
                              uint32_t edge_detect );
int iEpuCounterInputMuxSelect( uint8_t epu_counter,
                               enum epu_counter_input_select isel );
int iEpuCounterCfgCompare( uint8_t epu_counter,
                           uint32_t val );
int iEpuCounterCfgLocalTrigger( uint8_t epu_counter,
                                enum epu_counter_event_trigger lt );
int iEpuCounterCfgLocalAction( uint8_t epu_counter,
                               enum epu_counter_local_action action );
int iEpuCounterCfgGlobalTrigger( uint8_t epu_counter,
                                 enum epu_counter_event_trigger gt );

/* SCU */
int iEpuScuSetInputEvent( enum epu_scu_sel scu_sel,
                          enum epu_event_input_sel event_mask,
                          enum epu_scu_input_sel * input_sel );
int iEpuScuClearInputEvent( enum epu_scu_sel scu_sel,
                            enum epu_scu_input_sel input_sel );
int iEpuScuDisableAllEvents( enum epu_scu_sel scu_sel );
int iEpuScuCfgInputControl( enum epu_scu_sel scu_sel,
                            enum epu_scu_input_sel input_sel,
                            enum epu_scu_input_control_sel ctrl_sel );
int iEpuScuSetEdgeDetect( enum epu_scu_sel scu_sel );
int iEpuScuClrEdgeDetect( enum epu_scu_sel scu_sel );
int iEpuScuSetInputInversion( enum epu_scu_sel scu_sel,
                              enum epu_scu_input_sel input_sel );
int EpuScuClrInputInversion( enum epu_scu_sel scu_sel,
                             enum epu_scu_input_sel input_sel );
int iEpuScuSetInversion( enum epu_scu_sel scu_sel );
int iEpuScuClrInversion( enum epu_scu_sel scu_sel );
int iEpuScuSetSticky( enum epu_scu_sel scu_sel );
int iEpuScuClrSticky( enum epu_scu_sel scu_sel );
int iEpuScuSetEventStatus( enum epu_scu_sel scu_sel );
int iEpuScuGetEventStatus( enum epu_scu_sel scu_sel );
int iEpuScuClrEventStatusNoLock( enum epu_scu_sel scu_sel );
int iEpuScuSlrEventStatus( enum epu_scu_sel scu_sel );
int iEpuScuEventOutputToGpin( enum epu_scu_sel scu_sel,
                              enum epu_scu_core_cts bit );
#endif /* ifndef _EPU_H_ */
