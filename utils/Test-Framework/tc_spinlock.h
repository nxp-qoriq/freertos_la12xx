// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef  _TEST_CASES_SPINLOCK_H_
#define _TEST_CASES_SPINLOCK_H_

#include "test_framework.h"

#if GEUL_DEMO_SPINLOCK_TEST
    extern volatile unsigned int ulFreeIndx;

    void vGeulDemoSpinlockTest( void );
    void vTaskCodeCore( void );
    void vTaskCode( void );
#endif /* GEUL_DEMO_SPINLOCK_TEST */

#endif /* _TEST_CASES_SPINLOCK_H_ */
