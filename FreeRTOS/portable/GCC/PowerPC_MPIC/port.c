/* SPDX-License-Identifier: BSD-3-Clause */

/*==================================================================================================
*
*   (c) Copyright 2015 Freescale Semiconductor Inc.
*   Copyright 2018-2022 NXP.
*
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"

#include "common.h"
#include "ppc.h"
#include "soc.h"

/*-----------------------------------------------------------*/

/* Definitions to set the initial MSR of each task. */
#define portCRITICAL_INTERRUPT_ENABLE   ( 1UL << 17UL )
#define portEXTERNAL_INTERRUPT_ENABLE   ( 1UL << 15UL )
#define portPROBLEM_STATE               ( 1UL << 14UL )
#define portMACHINE_CHECK_ENABLE        ( 1UL << 12UL )

#define portINITIAL_MSR                 ( portCRITICAL_INTERRUPT_ENABLE | \
                                          portEXTERNAL_INTERRUPT_ENABLE | \
                                          portMACHINE_CHECK_ENABLE )

#define TEST_MODE	TBGEN_QUICK_VERFIY
#define portTIMER_SETUP()   tbgen_OsTickEnable(TEST_MODE, 0x1000)
#define portTIMER_RESET()   wreg32le(TBGEN_TI_INT_FLAG, TBGEN_INTSTAT)

#define TICK_INTERVAL  ((configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1)

/*-----------------------------------------------------------*/

uint32_t *pxSystemStackPointer = NULL;

/*
 * Function to start the scheduler running by starting the highest
 * priority task that has thus far been created
 */
extern void vPortStartFirstTask( void );

/*-----------------------------------------------------------*/

/* The stack frame created on an interrupt (size 0x98):
 * ----------------------------------
 *      | last backchain  | <- SP + 0x98
 *      |-----------------|
 *      | R0, 3-12, 14-31 | <- (SP + 0x24) to (SP + 0x94)   R31 is stored in higher memory, R0 in lower memory
 *      |-----------------|
 *      |       XER       | <- SP + 0x20
 *      |-----------------|
 *      |       CTR       | <- SP + 0x1C
 *      |-----------------|
 *      |        LR       | <- SP + 0x18
 *      |-----------------|
 *      |        CR       | <- SP + 0x14
 *      |-----------------|
 *      |       SRR1      | <- SP + 0x10
 *      |-----------------|
 *      |       SRR0      | <- SP + 0x0C
 *      |-----------------|
 *      |   nest count    | <- SP + 0x08 (Nested critical section count)
 *      |-----------------|
 *      |  LR save word   | <- SP + 0x04
 *      |-----------------|
 *      |    backchain    | <- SP + 0x00 (saved in R1)
 *      |-----------------|
 */

// Initialize the stack of a task to look exactly as if the task had been
// interrupted
// See the header file portable.h.
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters)
{
    register portSTACK_TYPE msr, srr1;
    portSTACK_TYPE *pxBackchain;

    msr = mfmsr();

    srr1 = portINITIAL_MSR | msr;

    /* Place a known value at the bottom of the stack for debugging */
    *pxTopOfStack = 0xDEADBEEF;

    /* Create a root frame header */
    pxTopOfStack--;
    *pxTopOfStack = 0x0L; /* Root lr save word */

    pxTopOfStack--;
    *pxTopOfStack = 0x0L; /* Root backchain */
    pxBackchain = pxTopOfStack;

    pxTopOfStack--;
    *pxTopOfStack = 0x1FL;                                                    /* r31  - 0x94 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1EL;                                                    /* r30  - 0x90 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1DL;                                                    /* r29  - 0x8C */
    pxTopOfStack--;
    *pxTopOfStack = 0x1CL;                                                    /* r28  - 0x88 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1BL;                                                    /* r27  - 0x84 */
    pxTopOfStack--;
    *pxTopOfStack = 0x1AL;                                                    /* r26  - 0x80 */
    pxTopOfStack--;
    *pxTopOfStack = 0x19L;                                                    /* r25  - 0x7C */
    pxTopOfStack--;
    *pxTopOfStack = 0x18L;                                                    /* r24  - 0x78 */
    pxTopOfStack--;
    *pxTopOfStack = 0x17L;                                                    /* r23  - 0x74 */
    pxTopOfStack--;
    *pxTopOfStack = 0x16L;                                                    /* r22  - 0x70 */
    pxTopOfStack--;
    *pxTopOfStack = 0x15L;                                                    /* r21  - 0x6C */
    pxTopOfStack--;
    *pxTopOfStack = 0x14L;                                                    /* r20  - 0x68 */
    pxTopOfStack--;
    *pxTopOfStack = 0x13L;                                                    /* r19  - 0x64 */
    pxTopOfStack--;
    *pxTopOfStack = 0x12L;                                                    /* r18  - 0x60 */
    pxTopOfStack--;
    *pxTopOfStack = 0x11L;                                                    /* r17  - 0x5C */
    pxTopOfStack--;
    *pxTopOfStack = 0x10L;                                                    /* r16  - 0x58 */
    pxTopOfStack--;
    *pxTopOfStack = 0xFL;                                                     /* r15  - 0x54 */
    pxTopOfStack--;
    *pxTopOfStack = 0xEL;                                                     /* r14  - 0x50 */
    pxTopOfStack--;
    *pxTopOfStack = 0xCL;                                                     /* r12  - 0x4C */
    pxTopOfStack--;
    *pxTopOfStack = 0xBL;                                                     /* r11  - 0x48 */
    pxTopOfStack--;
    *pxTopOfStack = 0xAL;                                                     /* r10  - 0x44 */
    pxTopOfStack--;
    *pxTopOfStack = 0x9L;                                                     /* r09  - 0x40 */
    pxTopOfStack--;
    *pxTopOfStack = 0x8L;                                                     /* r08  - 0x3C */
    pxTopOfStack--;
    *pxTopOfStack = 0x7L;                                                     /* r07  - 0x38 */
    pxTopOfStack--;
    *pxTopOfStack = 0x6L;                                                     /* r06  - 0x34 */
    pxTopOfStack--;
    *pxTopOfStack = 0x5L;                                                     /* r05  - 0x30 */
    pxTopOfStack--;
    *pxTopOfStack = 0x4L;                                                     /* r04  - 0x2C */
    pxTopOfStack--;
    *pxTopOfStack = ( portSTACK_TYPE ) pvParameters;                          /* r03  - 0x28 */
    pxTopOfStack--;
    *pxTopOfStack = 0x0L;                                                     /* r00  - 0x24 */

    pxTopOfStack--;
    *pxTopOfStack = 0x0L;                                                     /* XER  - 0x20 */

    pxTopOfStack--;
    *pxTopOfStack = 0x0L;                                                     /* CTR  - 0x1C */

    pxTopOfStack--;
    *pxTopOfStack = ( portSTACK_TYPE ) pxCode;                                /* LR   - 0x18 */

    pxTopOfStack--;
    *pxTopOfStack = 0x0L;                                                     /* CR   - 0x14 */

    pxTopOfStack--;
    *pxTopOfStack = srr1;                                                     /* SRR1 - 0x10 */

    pxTopOfStack--;
    *pxTopOfStack = ( portSTACK_TYPE ) pxCode;                                /* SRR0 - 0x0C */

    pxTopOfStack--;
    *pxTopOfStack = 0x0L;                                                     /* nest cnt - 0x08 */

    pxTopOfStack--;
    *pxTopOfStack = 0x0L;                                                     /* LR save word - 0x04 */

    pxTopOfStack--;
    *pxTopOfStack = ( portSTACK_TYPE ) pxBackchain;                           /* SP(r1) - 0x00 */

    return pxTopOfStack;
}

/*-----------------------------------------------------------*/
void vTimerInit(void)
{
	u8 i = 0;

	/* Set current task priority */
	for( i = 0; i < get_soc_numcores(); i++ )
		bSetCurrentTskPrio(i, 0);

	/* SYSTICK */
	lRegisterIrq(148 + INTERNAL_IRQ_OFFSET, vPortTickISR, NULL);
	ulSetMpicDivFactor( MPIC_DIV_FACTOR );
	bMpicConfigTimer( MPIC_SYS_TICK_COUNT );
	bMpicConfigGlobalTimerB();
}

/* Note that you must setup and install the timer interrupt before calling this */
portBASE_TYPE xPortStartScheduler( void )
{
	/* FIXME: Enable tick interrupt*/
	//prvPortTimerSetup(vPortTickISR, TICK_INTERVAL);
	vTimerInit();

    vPortStartFirstTask();

    /* Should not get here as the tasks are now running! */
    return pdFALSE;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	for(;;)
	{
		portNOP();
	}
}
/*-----------------------------------------------------------*/
bool_t vPortTickISR(uint32_t ulirq_no, void *dev_data)
{
    /* The SysTick runs at the lowest interrupt priority, and xTaskIncrementTick
     * deals with unprotected data structures that can also be updated by
     * vTaskSwitchContext which can be called from higher priority interrupts
     * via portYIELD_FROM_ISR, so raise priority to MAX_SYSCALL to disable
     * API interrupts.
     */
    UBaseType_t uxSavedInterruptStatus = ulPortMaskInterruptsFromISR();

    BaseType_t xHigherPriorityTaskWoken = xTaskIncrementTick();

    (void)ulirq_no;
	
    #if (configUSE_PREEMPTION == 1)
        /* Instead of calling portYIELD_FROM_ISR here, call vTaskSwitchContext directly
         * if xHigherPriorityTaskWoken is true to avoid the redundant calls to
         * ulPortMaskInterruptsFromISR and vPortUnmaskInterrupts in portYIELD_FROM_ISR
         */
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            vTaskSwitchContext();
        }
    #else
        /* Avoid unused variable warnings if preemption is disabled */
        (void)xHigherPriorityTaskWoken;
    #endif

    if (dev_data != NULL)
	log_info("Inside vPortTickISR %d\r\n\n", ulirq_no);
    vPortUnmaskInterrupts(uxSavedInterruptStatus);

    return true;
}

void vPortTaskEnterCritical( void )
{
    /* Disable interrupts to create critical section */
    portDISABLE_INTERRUPTS();

    /* Increment critical nesting count */
    portINCREMENT_CRITICAL_NESTING();
}

void vPortTaskExitCritical( void )
{
    /* If exiting a critical section, re-enable interrupts */
    if ( portDECREMENT_CRITICAL_NESTING() == 0U )
    {
        portENABLE_INTERRUPTS();
    }
}
