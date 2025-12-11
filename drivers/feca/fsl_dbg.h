// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __FSL_DBG_H_
#define __FSL_DBG_H_

#include <assert.h>
#include "fsl_io.h"
#include "FreeRTOS.h"
#ifndef pr_info
#define pr_info log_info
#endif
#ifndef pr_debug
#define pr_debug log_info
#endif
#ifndef pr_err
#define pr_err log_err
#endif
#define fsl_print PRINTF


#ifndef ALIGN_UP
#define ALIGN_UP(ADDRESS, ALIGNMENT)           \
        ((((uint32_t)(ADDRESS)) + ((uint32_t)(ALIGNMENT)) - 1) & (~(((uint32_t)(ALIGNMENT)) - 1)))
        /**< Align a given address - equivalent to ceil(ADDRESS,ALIGNMENT) */
#endif /* ALIGN_UP */


// configASSERT
#define ASSERT_COND(_cond) \
	if (!(_cond)) \
 		pr_err("ASSERT: %s:%d\n", __FILE__, __LINE__);




#endif
