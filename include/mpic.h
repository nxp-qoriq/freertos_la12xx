// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef SRC_MPIC_H_
#define SRC_MPIC_H_

#include "types.h"
#include <platform_def.h>

/**
 * @file        mpic.h
 * @brief       MPIC-related APIs.
 * @addtogroup  MPIC_API
 * @{
 */

#define    EDGE_SENSITIVE		0
#define    LEVEL_SENSITIVE              1
#define    EXT_INT_SENSE_OFFSET		22
#define    DEFAULT_PRIORITY		8
/* MPIC Errata A-008312 workaround. Edge triggered
 * interrupt must be no high priority than other
 * interrupt. Increasing priority of timer to avoid
 * false edge interrupt
 * */
#define    TIMER_PRIORITY		8
#define    HIGHEST_PRIORITY		0xF
#define    DEFAULT_PRIORITY_OFFSET	16
#define    MPIC_PRIORITY_MASK		0xF0000
#define	   DEFAULT_POLARITY		1
#define	   DEFAULT_POLARITY_OFFSET	23
#define    MASK_DISABLE			(unsigned int)1
#define    MSK				31
#define    U_INT_ONE			(unsigned int)1
#define    MPIC_RST			31
#define    MPIC_MODE			29
#define    NIRQ_OFFSET			5
#define    VECTOR_TBL_SIZE		1 //4
#define    EP				(1<<31)
#define    CI0				(1<<30)
#define    CI1				(1<<29)
#define	   P0				1
#define	   P1				2
#define	   P2				4
#define	   P3				8
#define	   P4				16
#define	   P5				32
#define    MPIC_DIV_FACTOR  8
#define    DIV_FACT_SHIFT   8
#ifndef UNUSED
#define    UNUSED(x)			(void)(x)
#endif
/*
 * We want SYSTICK every 1 ms that is 1 kHz frequency
 * Deriving the same from the SYSCLK frequency
 **/
#define	MPIC_TIMER_CLOCK	( PLAT_FREQ / MPIC_DIV_FACTOR )
#define	MPIC_SYS_TICK_COUNT	( MPIC_TIMER_CLOCK / TICK_FREQ )

/* Device list */
#define	DEVICE_INTERNAL		0
#define	DEVICE_INTER_PROC	1
#define	DEVICE_SHARE_MESSAGE	3
#define	DEVICE_TIMER_GRP_A	4
#define	DEVICE_TIMER_GRP_B	5
#define	DEVICE_EXTERNAL		6
#define	DEVICE_SHARE_MESSAGE_B	7
#define	DEVICE_SHARE_MESSAGE_C	8

//RTL not implemented yet!! should be zero
#define PER_CPU_PRIVATE_REG_WORKARD 0x20000

/* Min and Max IRQ limits */
#define MAX_IRQ_INDEX 256
#define INTERNAL_IRQ_OFFSET	16

#define MPIC_IPIVPR_VECTOR_MASK 0xffff

/* Interrupt Vector Table */
#define EXTERNAL_INTR_START	0
#define EXTERNAL_INTR_END	11
#define INTERNAL_INTR_START	(0 + INTERNAL_IRQ_OFFSET)
#define INTERNAL_INTR_END	(127 + INTERNAL_IRQ_OFFSET)
#define IPI_INTR_START		144
#define IPI_INTR_END		147
#define MSG_INTR_START		148
#define MSG_INTR_END		155
#define MSI_INTR_START		156
#define MSI_INTR_END		163
#define TIMER_A_INTR_START	164
#define TIMER_A_INTR_END	167
#define TIMER_B_INTR_START	168
#define TIMER_B_INTR_END	171
#define MSIB_INTR_START     172
#define MSIB_INTR_END     179
#define MSIC_INTR_START		180
#define MSIC_INTR_END		187


/* Global Timer B Config Params to cascade Timers 0 and 1 */
#define TIMER_ROVR_BIT		24
#define TIMER_ROVR_0_1		0x1	// 0b001
#define TIMER_CLKR_BIT		8
#define TIMER_CLKR_DEFAULT	0x0	// 0b00
#define TIMER_CASC_BIT		0
#define TIMER_CASC_0_1		0x1	// 0b001

typedef bool_t ( *bIsrFunc ) ( uint32_t ulIrqNo, void *pvDevData );

/**
 * @details Initializes MPIC registers
 * @param[in]	ulTarget_Cpu	CoreID of the Targeted Cpu
 * @return
 *	- TRUE	Success
 *	- FALSE	Failure to Initialize the MPIC
 */
bool_t bMpicInitialize( u32 ulTarget_Cpu );

/**
 * @details Configures 	Global Timer Register
 * @param[in]	ulValue		MPIC Timer count value to be filled in Base Count register
 * @return
 *	- TRUE	Success
 *	- FALSE Failure to Configure the Timer
 */
bool_t bMpicConfigTimer( u32 ulValue );

/**
 * @details Enables(UnMask) device interrupt
 * @param[in]	ulDevice_Group  Device Group to which device belongs
 * @param[in]	ulDevice_Num    Index Number of device in device group
 * @return
 *	- TRUE	Successfully enabled(Unmasked)
 *      - FALSE Failure to Unmask the interrupt line
 */
bool_t bMpicEnable( u32 ulDevice_Group, u32 ulDevice_Num );

/**
 * @details 	Disables device interrupt
 * @param[in]  	ulDevice_Group	Device Group as listed in mpic.h
 * @param[in]  	ulDevice_Num	Index Number of device in Device Group
 * @return
 *	- TRUE	Success
 *	- FALSE	Failed to displace the MPIC for the particular
 *		device
 */
bool_t bMpicDisable( u32 ulDevice_Group, u32 ulDevice_Num );

/**
 * @details Sets CORE0/CORE1/CORE2/CORE3 interrupt priority
 * @param[in]	ulCpu		CPU0/CPU1/CPU2/CPU3
 * @param[in]	ulPrio		priority
 * @return
 *	- TRUE	Success
 *	- FALSE Failed to set the task priority
 */
bool_t bSetCurrentTskPrio( u32 ulCpu, u32 ulPrio );

/**
 * @details Sets device interrupt priority
 * @param[in]   ulDevice_Group	Device Group as listed in mpic.h
 * @param[in]   ulDevice_Num	Index Number of device in device group
 * @param[in]   ulPriority  	The priority to be set for upDevice_Num
 * @return
 *	- TRUE  Success
 *	- FALSE Error in setting the device priority
 */
bool_t bMpicSetDevicePriority( u32 ulDevice_Group, u32 ulDevice_Num,
	u32 ulPriority );

/**
 * @details Sets the device interrupt priority
 * @param[in]   ulDevice_Group	device list in mpic.h
 * @param[in]   ulDevice_Num	number of devices in device group
 * @return
 *	- Device Interrupt Priority Number
 */
u32 ulMpicGetDevicePriority( u32 ulDevice_Group, u32 ulDevice_Num );


/**
 * Ack interrupt and write EOI
 * @param   ulCore CORE0 / CORE1
 * @return
 *      vector          vector
 */
u32 ulMpicAck( u32 ulCore );

/**
 * @details Registers the interrupt line with MPIC
 * It also registers the function blIsr which will
 * be called when the interrupt line is asserted
 * @param[in]	ulIrqVectNo	Interrupt line to register
 * @param[in]	bIsr		Function to be called when
 *				interrupt line is asserted
 * @param[in]	*pvData		Any data to be passed
 * @return
 *	-  1  Successfully registered the Interrupt line with bIsr
 *	- -1  Failed to register the Interrupt line
 */
int lRegisterIrq( uint32_t ulIrqVectNo, bIsrFunc bIsr, void *pvData );

/**
 * @details 	Unregisters interrupt line with MPIC
 * @param[in] 	ulIrqVectNo	Interrupt line to unregister
 * @return
 * 	- Nothing(void)
 */
void vUnregisterIrq( uint32_t ulIrqVectNo );

/**
 * @details Returns the ID of the processor core who is calling this function
 * @return
 *	- b0_0000 Processor core 0
 *	- b0_0001 Processor core 1
 *	- b0_0010 Processor core 2
 *	- b0_0011 Processor core 3
 *	- Note: b1_1111 refers to an illegal processor ID
 */
u32 ulMpicCurrentCore( void );


/**
 * @details 	Sets the Division Factor for TCRA
 * @param[in]  	divFactor	DivisionFactor
 * @return
 *	- division factor that is set
 */
u32 ulSetMpicDivFactor( u32 divFactor );

/**
 *  @details	Gets current Tick Timer count
 *  @return
 *	- Current tick timer count which is MPIC Global timer A0
*/
u32 ulGetMpicCurrentTickTimerCount( void );

/**
 *  @details    Raises message shared interrupt for a
 *		particular line
 *  @param[in]	ulIrqNo		Interrupt line number
 *  @return
 *      - Nothing(void)
*/
void vMpicRaiseMessageSharedInterrupt( uint32_t ulIrqNo );


/**
 * @details Masks all interrupts on current core
 * @return
 *	- TRUE	Success
 *	- FALSE Failure to Mask interrupts
 */
bool_t bMpicMaskInterrupts( void );


/**
 * @details Enables all interrupts on current core
 * @return
 *	- TRUE	Success
 *	- FALSE Failure to Enable Interrupts
 */
bool_t bMpicMaskInterruptsEnable( void );

/**
 * @details Configures Global Timer B to cascade Timer 0 and 1.
 * @return
 *	- TRUE	Success
 *	- FALSE Failure to Configure Global Timer B
 */
bool_t bMpicConfigGlobalTimerB( void );

/**
 * @details Returns Current Count of Global Timer B
 * cascaded count for Timer 0 and 1.
 * @return
 *	- 64 bit current count
 */
u64 ulGetMpicGloablTimerBCurrentCount( void );

/** @} */
#endif /* SRC_MPIC_H_ */
