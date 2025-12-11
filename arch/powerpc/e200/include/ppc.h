// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef _PPC_H_
#define _PPC_H_

#include "bit.h"
#include "types.h"
#include "common.h"

/* -- Special Register Offset Definitions -- */
#define SPR_XER             1       /* Integer Exception Register */
#define SPR_LR              8       /* Link Register */
#define SPR_PIR             286     /* Processor ID Register */
#define SPR_PVR             287     /* Processor Version Register */
#define SPR_L1CSR2          606     /* L1 Cache Control and Status Register 0 */
#define SPR_MAS0            624     /* MPU Assist Register 0 */
#define SPR_MAS1            625     /* MPU Assist Register 1 */
#define SPR_MAS2            626     /* MPU Assist Register 2 */
#define SPR_MAS3            627     /* MPU Assist Register 3 */
#define SPR_MPU0CFG         692     /* MPU0 Configuration Register */
#define SPR_L1CSR0          1010    /* L1 Cache Configuration Register 0 */
#define SPR_L1CSR1          1011    /* L1 Cache Configuration Register 1 */
#define SPR_MPU0CSR0        1014    /* MPU0 Control and Status Register 0 */
#define SPR_MMUCFG          1015    /* MMU/MPU Configuration Register */
#define SPR_BUCSR          1013


/* -- Performance Monitur APU Register Offset Definition */
#define PMR_PMC0            16      /* Performance monitor counter 0 */
#define PMR_PMC1            17      /* Performance monitor counter 1 */
#define PMR_PMC2            18      /* Performance monitor counter 2 */
#define PMR_PMC3            19      /* Performance monitor counter 3 */
#define PMR_PMGC0           400     /* Performance monitor global control register 0 */
#define PMR_PMLCa0          144     /* Performance monitor local control a0 */
#define PMR_PMLCa1          145     /* Performance monitor local control a1 */
#define PMR_PMLCa2          146     /* Performance monitor local control a2 */
#define PMR_PMLCa3          147     /* Performance monitor local control a3 */
#define PMR_PMLCb0          272     /* Performance monitor local control b0 */
#define PMR_PMLCb1          273     /* Performance monitor local control b1 */
#define PMR_PMLCb2          274     /* Performance monitor local control b2 */
#define PMR_PMLCb3          275     /* Performance monitor local control b3 */

#define PMGC0_FAC	0x80000000	/* Freeze all Counters */
#define PMGC0_PMIE	0x40000000	/* Interrupt Enable */
#define PMGC0_FCECE	0x20000000	/* Freeze countes on
					   Enabled Condition or
					   Event */
#define PMLCA_EVENT_MASK 0x01ff0000	/* Event field */
#define PMLCA_EVENT_SHIFT	16

#define PMLCA_FC        0x80000000      /* Freeze Counter */
#define PMLCA_FCS       0x40000000      /* Freeze in Supervisor */
#define PMLCA_FCU       0x20000000      /* Freeze in User */
#define PMLCA_FCM1      0x10000000      /* Freeze when PMM==1 */
#define PMLCA_FCM0      0x08000000      /* Freeze when PMM==0 */
#define PMLCA_CE        0x04000000      /* Condition Enable */
#define PMLCA_FGCS1     0x00000002      /* Freeze in guest state */
#define PMLCA_FGCS0     0x00000001      /* Freeze in hypervisor state */

/* -- Register Bitfield Definitions -- */
 /* Bit definitions for L1CSR0. */
#define L1CSR0_WID_MASK    GENMASK(31, 30)	/* Way Instruction Disable. */
#define L1CSR0_WID_SHIFT   30               /* Way Instruction Disable. */
#define L1CSR0_WDD_MASK    GENMASK(29, 28)	/* Way Data Disable. */
#define L1CSR0_WDD_SHIFT   28	            /* Way Data Disable. */
#define L1CSR0_DCWA        BIT(19)	        /* Data Cache Write Allocation Policy */
#define L1CSR0_DCECE       BIT(16)	        /* Data Cache Error Checking Enable */
#define L1CSR0_DCEI        BIT(15)	        /* Data Cache Error Injection */
#define L1CSR0_DCLOC_MASK  GENMASK(14, 13)	/* Data Cache Lockout Control */
#define L1CSR0_DCLOC_SHIFT 13	            /* Data Cache Lockout Control */
#define L1CSR0_DCEA_MASK   GENMASK(6, 5)	/* Data Cache Error Action */
#define L1CSR0_DCEA_SHIFT  5	            /* Data Cache Error Action */
#define L1CSR0_DCLOINV     BIT(4)	        /* Data Cache Lockout Indicator Invalidate */
#define L1CSR0_DCABT       BIT(2)	        /* Data Cache Operation Aborted */
#define L1CSR0_DCINV       BIT(1)	        /* Cache Flash Invalidate */
#define L1CSR0_DCE         BIT(0)	        /* Data Cache Enable */
 
 /* Bit definitions for L1CSR1. */
#define L1CSR1_ICECE       BIT(16)	        /* Instruction Cache Error Checking Enable */
#define L1CSR1_ICEI        BIT(15)	        /* Instruction Cache Error Injection */
#define L1CSR1_ICLOC_MASK  GENMASK(14, 13)	/* Instruction Cache Lockout Control */
#define L1CSR1_ICLOC_SHIFT 13	            /* Instruction Cache Lockout Control */
#define L1CSR1_ICEA_MASK   GENMASK(6, 5)	/* Instruction Cache Error Action */
#define L1CSR1_ICEA_SHIFT  5	            /* Instruction Cache Error Action */
#define L1CSR1_ICLOINV     BIT(4)	        /* Instruction Cache Lockout Indicator Invalidate */
#define L1CSR1_ICABT       BIT(2)	        /* Instruction Cache Operation Aborted */
#define L1CSR1_ICINV       BIT(1)	        /* Cache Flash Invalidate */
#define L1CSR1_ICE         BIT(0)	        /* Instruction Cache Enable */

 /* Bit definitions for L1CSR2. */
#define L1CSR2_STGC_MASK   GENMASK(29, 28)	/* Store Gather Control */
#define L1CSR2_STGC_SHIFT  28	            /* Store Gather Control */
#define L1CSR2_STGC_DEF    0<<L1CSR2_STGC_SHIFT /* Default Operation - Implementation defined. for e200 store gathering is disabled. */
#define L1CSR2_STGC_EN     1<<L1CSR2_STGC_SHIFT /* Gathering is enabled. No constraints on alignment */
#define L1CSR2_STGC_ALIGN  2<<L1CSR2_STGC_SHIFT /* Gathered stores are required to be contiguous once gathered. */
#define L1CSR2_STGC_DIS    3<<L1CSR2_STGC_SHIFT /* No Store Gathering is Performed */

 /* Bit definitions for BUCSR. */
#define BUCSR_BPEN			BIT(0)


 /* Bit definitions for MSR. */
#define MSR_RI             BIT(1)	        /* Recoverable Interrupt */
#define MSR_PMM            BIT(2)	        /* RPMM Performance monitor mark bit */
#define MSR_DS             BIT(4)	        /* Data Address Space */
#define MSR_IS             BIT(5)	        /* Instruction Address Space */
#define MSR_FE1            BIT(8)	        /* Floating-point exception mode 1 (not used by Zen) */
#define MSR_DE             BIT(9)	        /* Debug Interrupt Enable */
#define MSR_FE0            BIT(11)	        /* Floating-point exception mode 0 (not used by Zen) */
#define MSR_ME             BIT(12)	        /* Machine Check Enable */
#define MSR_FP             BIT(13)	        /* Floating-Point Available */
#define MSR_PR             BIT(14)	        /* Problem State */
#define MSR_EE             BIT(15)	        /* External Interrupt Enable */
#define MSR_CE             BIT(17)	        /* Critical Interrupt Enable */
#define MSR_WE             BIT(18)	        /* Wait State (Power management) enable. */
#define MSR_SPV            BIT(25)	        /* SP/Embedded FP/Vector available */

#define MAS0_SEL_MPU    2
#define MAS0_ESEL_0     0
#define MAS0_ESEL_1     1
#define MAS0_ESEL_2     2
#define MAS0_ESEL_3     3
#define MAS0_ESEL_4     4
#define MAS0_ESEL_5     5
#define MAS0_ESEL_6     6
#define MAS0_ESEL_7     7
#define MAS0_ESEL_8     8
#define MAS0_ESEL_9     9
#define MAS0_ESEL_10    10
#define MAS0_ESEL_11    11
#define MAS0_UAMSK_NO_BIT   0
#define MAS0_UAMSK_1_BIT    1
#define MAS0_UAMSK_2_BITS   2
#define MAS0_UAMSK_3_BITS   3
#define MAS0_UAMSK_4_BITS   4
#define MAS0_UAMSK_5_BITS   5

/* Bit definitions for MAS1 (625) */
#define MAS1_TIDMSK_SHIFT   0               /* Region ID mask */
#define MAS1_TIDMSK_WIDTH   8               /* Region ID mask */
#define MAS1_TIDMSK_MASK    GENMASK2(MAS1_TIDMSK_WIDTH, MAS1_TIDMSK_SHIFT)
#define MAS1_TID_SHIFT      16              /* Region ID bits */
#define MAS1_TID_WIDTH      8               /* Region ID bits */
#define MAS1_TID_MASK       GENMASK2(MAS1_TID_WIDTH, MAS1_TID_SHIFT)

/* Performance Monitor Local control A Registers (144 - 147) */
#define PMLCa_PMP_SHIFT     12              /* Performance Monitor Watchpoint Periodicity Select */
#define PMLCa_PMP_WIDTH     3               /* Performance Monitor Watchpoint Periodicity Select */
#define PMLCa_PMP_MASK      GENMASK2(PMLCa_PMP_WIDTH, PMLCa_PMP_SHIFT)
#define PMLCa_EVENT_SHIFT   16              /* Event selector */
#define PMLCa_EVENT_WIDTH   7               /* Event selector */
#define PMLCa_EVENT_MASK    GENMASK2(PMLCa_EVENT_WIDTH, PMLCa_EVENT_SHIFT)
#define PMLCa_CE_SHIFT      26                      /* Condition Enable*/
#define PMLCa_CE            BIT(PMLCa_CE_SHIFT)     /* Condition Enable*/
#define PMLCa_FCM0_SHIFT    27                      /* Freeze Counter while Mark is cleared*/
#define PMLCa_FCM0          BIT(PMLCa_FCM0_SHIFT)   /* Freeze Counter while Mark is cleared*/
#define PMLCa_FCM1_SHIFT    28                      /* Freeze Counter while Mark is set*/
#define PMLCa_FCM1          BIT(PMLCa_FCM1_SHIFT)   /* Freeze Counter while Mark is set*/
#define PMLCa_FCU_SHIFT     29                      /* Freeze Counter in User state*/
#define PMLCa_FCU           BIT(PMLCa_FCU_SHIFT)    /* Freeze Counter in User state*/
#define PMLCa_FCS_SHIFT     30                      /* Freeze Counter in Supervisor state */
#define PMLCa_FCS           BIT(PMLCa_FCS_SHIFT)    /* Freeze Counter in Supervisor state */
#define PMLCa_FC_SHIFT      31                      /* Freeze Counter */
#define PMLCa_FC            BIT(PMLCa_FC_SHIFT)     /* Freeze Counter */

#define PMLCa_EVENT_NOTHING                 0
#define PMLCa_EVENT_PROC_CYCLES             1
#define PMLCa_EVENT_INST_COMP               2
#define PMLCa_EVENT_0_INST_CYCLES           3
#define PMLCa_EVENT_1_INST_CYCLES           4
#define PMLCa_EVENT_2_INSTS_CYCLES          5
#define PMLCa_EVENT_INST_WORDS_FETCH        6
#define PMLCa_EVENT_PM_EVENT_TRANS          7
#define PMLCa_EVENT_PM_EVENT_CYCLES         8
#define PMLCa_EVENT_BRANCH_COMP             10
#define PMLCa_EVENT_BRANCH_LINK_COMP        11
#define PMLCa_EVENT_PIPELINE_STALLS         22
#define PMLCa_EVENT_PIPELINE_STALLS_2       23
#define PMLCa_EVENT_DCACHE_LD_HITS          25
#define PMLCa_EVENT_DCACHE_LINEFILLS        24
#define PMLCa_EVENT_DCACHE_LD_HITS          25
#define PMLCa_EVENT_ST_BUF_FULL_STALLS      26
#define PMLCa_EVENT_ICACHE_LINEFILLS        27
#define PMLCa_EVENT_NUM_INST_FETCH          28
#define PMLCa_EVENT_BIU_INS_CYCLES          30
#define PMLCa_EVENT_BIU_DATA_CYCLES         32
#define PMLCa_EVENT_NUM_INST_FETCH          28
#define PMLCa_EVENT_NUM_INTERRUPT_TAKEN     38
#define PMLCa_EVENT_NUM_EXT_INTERRUPT_TAKEN     39
#define PMLCa_EVENT_NUM_MAX		    73

/* Performance Monitor Local control B Registers (272 - 275) */
#define PMLCb_TRIGGERED_SHIFT       13                          /* Triggered */
#define PMLCb_TRIGGERED             BIT(PMLCb_TRIGGERED_SHIFT)  /* Triggered */
#define PMLCb_TRIGOFFSEL_SHIFT      14      /* Trigger-off Source Select */
#define PMLCb_TRIGOFFSEL_WIDTH      2       /* Trigger-off Source Select */
#define PMLCb_TRIGOFFSEL_MASK       GENMASK2(PMLCb_TRIGOFFSEL_WIDTH, PMLCb_TRIGOFFSEL_SHIFT)
#define PMLCb_TRIGONSEL_SHIFT       19      /* Trigger-on Source Select */
#define PMLCb_TRIGONSEL_WIDTH       2       /* Trigger-on Source Select */
#define PMLCb_TRIGONSEL_MASK        GENMASK2(PMLCb_TRIGONSEL_WIDTH, PMLCb_TRIGONSEL_SHIFT)
#define PMLCb_TRIGOFFCNTL_SHIFT     24      /* Trigger-off Control Class - Class of Trigger-off source */
#define PMLCb_TRIGOFFCNTL_WIDTH     2       /* Trigger-off Control Class - Class of Trigger-off source */
#define PMLCb_TRIGOFFCNTL_MASK      GENMASK2(PMLCb_TRIGOFFCNTL_WIDTH, PMLCb_TRIGOFFCNTL_SHIFT)
#define PMLCb_TRIGONCNTL_SHIFT      28      /* Trigger-on Control Class - Class of Trigger-on source */
#define PMLCb_TRIGONCNTL_WIDTH      2       /* Trigger-on Control Class - Class of Trigger-on source */
#define PMLCb_TRIGONCNTL_MASK       GENMASK2(PMLCb_TRIGONCNTL_WIDTH, PMLCb_TRIGONCNTL_SHIFT)



/* Processor ID Register (PIR) */
#define SPR_PIR_ID_XTC      0
#define SPR_PIR_ID_RTX      1

/* SPE- signal processing extension bit definition */
#define FOVFE	BIT(2)	/* FPU overflow exception */
#define FUNFE	BIT(3)	/* FPU underflow exception */
#define FDBZE	BIT(4)	/* FPU Divide by zero exception */
#define FINVE	BIT(5)	/* FPU invalid input/operation exception */
#define FINXE	BIT(6)	/* FPU inexact exception */

/* CPU specific assembler functions */


/* mtspr SPR,rS */
/* use: mtspr(SPR,value); */
#define mtspr(SPR,value) __asm__ __volatile__ ("mtspr %c0,%1" \
				: /* no outputs */                            \
				: "i" (SPR), "r" (value)                      \
				: );                                          

/* mfspr rD,SPR */
/* use: val = mfspr(SPR); */
#define mfspr(SPR) ({ u32 ret;                               \
	__asm__ __volatile__ ("mfspr %0,%c1"                      \
				: "=r" (ret)                                  \
				: "i" (SPR) : ); ret; })                      

/* mfmsr rD */
/* use: val = mfmsr(); */
#define mfmsr() ({ u32 ret;                               \
	__asm__ __volatile__ ("mfmsr %0"                      \
				: "=r" (ret)                                  \
				:  : ); ret; })                      

/* mtmsr rD */
/* use mtmsr(value) */
#define mtmsr(value) __asm__ __volatile__ ("mtmsr %0"	\
		: /* no output */								\
		:"r" (value)									\
		:);

/* dcbi rA,rB */
/* use: dcbi(addr); */
#define dcbi(addr) __asm__ __volatile__ ("dcbi 0, %0" : : "r"(addr) : "memory");                                     

#define isync() __asm__ __volatile__ ("se_isync" : : : "memory");
#define msync() __asm__ __volatile__ ("msync" : : : "memory");
#define mbar () __asm__ __volatile__ ("mbar" : : : "memory");

/* Branch to Link Register (and Link) */
#define se_blr() __asm__ __volatile__ ("se_blr");

#define mpure() __asm__ __volatile__ ("mpure");     /* MPU Read Entry */
#define mpuwe() __asm__ __volatile__ ("mpuwe");     /* MPU Write Entry */
#define mpusync() __asm__ __volatile__ ("mpusync");     /* MPU Synchronize */

/* mtpmr PMRN,rS */
/* use: mtpmr(PMRN,value); */
#define mtpmr(PMRN,value) __asm__ __volatile__ ("mtpmr %c0,%1" \
				: /* no outputs */                            \
				: "i" (PMRN), "r" (value)                      \
				: );                                          

/* mfpmr rD,PMRN */
/* use: val = mfpmr(PMRN); */
#define mfpmr(PMRN) ({ u32 ret;                               \
	__asm__ __volatile__ ("mfpmr %0,%c1"                      \
				: "=r" (ret)                                  \
				: "i" (PMRN) : ); ret; }) 

static inline void bp_enable() 
{
	mtspr(SPR_BUCSR,BUCSR_BPEN);
}

/* L1 ICache and DCache macros and functions */

#define L1_DCACHE_LINE_SIZE	32
#define L1_ICACHE_LINE_SIZE	32

void vL1DCacheInvLine(uint32_t addr, size_t mem_size);
void vL1ICacheInvLine(void);

/* Invalidate the complete Instruction Cache */
static inline void l1_icache_invalidate()
{
	bool icinv;

	msync();
	isync();
	mtspr( SPR_L1CSR1, ( mfspr( SPR_L1CSR1 ) | L1CSR1_ICINV ));
	isync();

	do {
		msync();
		icinv = mfspr(SPR_L1CSR1) & L1CSR1_ICINV;
	} while (icinv != 0);
}

/* Invalidate the complete Data Cache */
static inline void l1_dcache_invalidate()
{
	bool dcinv;
	msync();
	isync();
	mtspr(SPR_L1CSR0, ( mfspr( SPR_L1CSR0 ) | L1CSR0_DCINV ));
	isync();

	do {
		msync();
		dcinv = mfspr(SPR_L1CSR0) & L1CSR0_DCINV;
	} while (dcinv != 0);
}

/* Enable Instruction Cache */
static inline void l1_icache_enable()
{
	l1_icache_invalidate();

	mtspr(SPR_L1CSR1, L1CSR1_ICE);
}

/* Enable Data Cache */
static inline void l1_dcache_enable()
{
	l1_dcache_invalidate();

	mtspr(SPR_L1CSR0, L1CSR0_DCE);
}

/* mtspr SPR,rS */
/* use: mtspr(SPR,value); */
#define mtsprg(SPRG,value) __asm__ __volatile__ ("mtsprg %c0,%1" \
				: /* no outputs */                            \
				: "i" (SPRG), "r" (value)                      \
				: );       

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#endif /* _PPC_H_ */
