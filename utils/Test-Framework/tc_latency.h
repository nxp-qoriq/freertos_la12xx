// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef  _TEST_CASES_LATENCY_H_
#define _TEST_CASES_LATENCY_H_

#include "test_framework.h"
int ipi_latency;
#if GEUL_DEMO_LATENCY_TEST
    void vGeulLatencyTest( void );
    void vIPILatencyTest( void );
#endif /* GEUL_DEMO_IPI_QUEUE_TEST */

#endif /* _TEST_CASES_IPI_H_ */
