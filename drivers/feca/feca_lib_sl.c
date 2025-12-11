// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "feca_lib_sl.h"
#include "feca_api.h"

void *feca_alloc_mem(int size)
{
	void *mem;
	mem = fsl_malloc((unsigned long)size, 16);
	memset(mem, 0, size);
	return mem;
}

void feca_mem_free(void *mem)
{
	fsl_free(mem);
}
