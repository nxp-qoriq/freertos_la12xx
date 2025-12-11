// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */

#ifndef _LA12XX_TBGEN_H_
#define _LA12XX_TBGEN_H_

#include <types.h>
#include <tbgen_regs_new.h>

#define Request_Irq								lRegisterIrq
#define Free_Irq								vUnregisterIrq
#define INTERNAL_IRQ_OFFSET						16
#define MPIC_INTERNAL_TBGEN_IRQ(x)				( 54 + ( 4 * ( x - 1 ) ) )
#define MPIC_EXTERNAL_TBGEN1_IRQ_STROBE(x)		( 5 + x - ( 4 * (x / 3) ) )
#define MPIC_EXTERNAL_TBGEN2_IRQ_STROBE(x)		( 8 + x )

typedef struct TimerIntCb
{
    TimerCallbackFn pvCb; /**< Timer callback*/
    void * pvConfData;     /**< Points to timer params structure */
} TimerIntCb_t;

typedef struct RFGIntCb
{
    RFGCallbackFn pvCb; /**< RFG callback*/
} RFGIntCb_t;

TimerIntCb_t xTddIntCb[ TBGEN_MAX ][ TDD_MAX_INSTANCE ];
TimerIntCb_t xGpeIntCb[ TBGEN_MAX ][ GPE_MAX_INSTANCE ];
TimerIntCb_t xRxAlignIntCb[ TBGEN_MAX ][ RX_ALIGNMENT_MAX_INSTANCE ];
RFGIntCb_t xRFGIntCb[ TBGEN_MAX ];
struct SpinLock *pxTbgenSpinLockVar;

/**
 * This API will request Irq for MPIC Internal Interrupt through TBGEN(Both TBGEN1 and TBGEN2)
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(i.e. TBGEN_1 or TBGEN_2)
 *
 * @return
 * 	- On Success, Returns 1
 * 	- On Failure, Returns -1
 */
int iTbgenDevOpen( uint8_t ucTbgenNo );

/**
 * This API will free Irq for MPIC Internal Interrupt through TBGEN(Both TBGEN1 and TBGEN2)
 *
 * @param[in] ucTbgenNo
 * 	Tbgen Instance(i.e. TBGEN_1 or TBGEN_2)
 *
 * @return
 *  NONE
 */
void vTbgenDevClose( uint8_t ucTbgenNo );

#endif /* _LA12XX_TBGEN_H_ */
