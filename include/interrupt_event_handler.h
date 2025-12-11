// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef MPIC_INCLUDE_INTERRUPT_EVENT_HANDLER_H_
#define MPIC_INCLUDE_INTERRUPT_EVENT_HANDLER_H_

#define MAX_STACKTRACE_DEPTH	8
#define SHOW_STACK_FRAMES	0

typedef bool_t ( *pbFunc ) ( u32, u32 );

extern pbFunc vInterruptEventHandler[ ];

struct StackFrame {
    uint32_t ulSp;
    uint32_t ulLrSave;
    uint32_t ulNestCnt;
    uint32_t ulSrr0;
    uint32_t ulSrr1;
    uint32_t ulCr;
    uint32_t ulLR;
    uint32_t ulCtr;
    uint32_t ulXer;
    uint32_t ulGpr_3_12[11];
    uint32_t ulGpr_14[18];
};

#endif /* MPIC_INCLUDE_INTERRUPT_EVENT_HANDLER_H_ */
