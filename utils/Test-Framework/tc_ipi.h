// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef  _TEST_CASES_IPI_H_
#define _TEST_CASES_IPI_H_

#include "test_framework.h"

#if GEUL_DEMO_IPI_QUEUE_TEST
    void vGeulIPIDemoEntry( void );
#endif /* GEUL_DEMO_IPI_QUEUE_TEST */

#if GEUL_IPI_STATS_TEST
    void vGeulIPIStats( void );
#endif /* GEUL_IPI_STATS_TEST */

#endif /* _TEST_CASES_IPI_H_ */
