// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#ifndef _TEST_CASE_HEAP_USAGE_H_
#define _TEST_CASE_HEAP_USAGE_H_

#include "test_framework.h"

#if GEUL_HEAP_USAGE_TEST
void vHeapMemoryUsageInfo(void);
void vHeapMemoryUsage( void );
#endif /* GEUL_HEAP_USAGE_TEST */

#endif /* _TEST_CASE_HEAP_USAGE_H_ */
