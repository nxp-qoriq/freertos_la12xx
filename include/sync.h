// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2021 NXP
 */

#ifndef _SYNC_H_
#define _SYNC_H_

#include "ppc.h"


/* data memory barrier */
static inline void sync_dmb() {msync();}

/* instruction sync barrier */
static inline void sync_imb() {isync();}

/* force ordering of instructions */
#define sync_order() __asm__ __volatile__("":::"memory");

#endif /* _SYNC_H_ */
