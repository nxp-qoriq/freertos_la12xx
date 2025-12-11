// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef  _TEST_CASES_QDMA_H_
#define _TEST_CASES_QDMA_H_

#include "test_framework.h"

#if GEUL_DEMO_QDMA_TEST
void vQdmaTest( void );
void vQdmaMulTest( void );
void vQdmaLongTest(void);
void vQdmaLongMulTest(void);
void vQdmaSGTest(void);
#ifdef QDMA_PEB_TO_FRAM
void vQdmaSGNoStrideTest(void);
void vQdmaSingleBufferTest(void);
#endif
#endif	/* GEUL_DEMO_QDMA_TEST */

#endif	/* _TEST_CASES_QDMA_H_ */
