// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef  _TEST_CASES_EXCEPTION_H_
#define _TEST_CASES_EXCEPTION_H_

#include "test_framework.h"

#if GEUL_DEMO_DATA_EXCEPTION
void vGeulDataExceptionTest( void );
#endif	/* GEUL_DEMO_DATA_EXCEPTION */

#if GEUL_DEMO_PROGRAM_EXCEPTION
void vGeulProgramExceptionTest( void );
#endif	/* GEUL_DEMO_PROGRAM_EXCEPTION */

#if GEUL_DEMO_ALIGNMENT_EXCEPTION
void vGeulAlignExceptionTest( void );
#endif	/* GEUL_DEMO_ALIGNMENT_EXCEPTION */

#if GEUL_DEMO_INSTR_STORAGE_EXCEPTION
void vGeulInstrStorageExceptionTest( void );
#endif	/* GEUL_DEMO_INSTR_STORAGE_EXCEPTION */

#if GEUL_DEMO_EFPU_DATA_EXCEPTION
void vGeulEFPUDataExceptionTest( void );
#endif	/* GEUL_DEMO_EFPU_DATA_EXCEPTION */

#if GEUL_DEMO_EFPU_ROUND_EXCEPTION
void vGeulEFPURoundExceptionTest( void );
#endif	/* GEUL_DEMO_EFPU_ROUND_EXCEPTION */

#endif	/* _TEST_CASES_EXCEPTION_H_ */
