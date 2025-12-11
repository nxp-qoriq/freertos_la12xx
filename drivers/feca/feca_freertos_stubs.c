// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */

/*
 * Set of external functions that the FECA implementation calls
 */

#include "geul_bsp_init.h"
#include <config.h>

#define NUM_FECA_DEVS	1

static feca_device_t the_devices[NUM_FECA_DEVS];

feca_device_t *bsp_get_feca_dev(feca_dev_id id)
{
	if (id >= NUM_FECA_DEVS)
		return NULL;

	return &the_devices[id];
}

int bsp_update_feca_dev(feca_dev_id id, feca_device_t *dev)
{
	if (id >= NUM_FECA_DEVS)
		return -1;

	the_devices[id] = *dev;

	return 0;
}


mod_mem_region_t *bsp_feca_get_mem_region(enum mem_region_id reg_id)
{
	return bsp_get_mem_region(reg_id);
}
