// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __FSL_MALLOC_H_
#define __FSL_MALLOC_H_

#include <portable.h>

static inline void* fsl_malloc(size_t size, unsigned int alignment)
{
	UNUSED(alignment);
	return pvPortMalloc(size);
}

static inline void fsl_free(void *p)
{
	vPortFree(p);
}

#endif
