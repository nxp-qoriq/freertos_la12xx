// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __GUEL_BSP_INIT__
#define __GUEL_BSP_INIT__

#include "feca_main_lib.h"

extern feca_device_t *bsp_get_feca_dev(feca_dev_id id);

extern mod_mem_region_t *bsp_feca_get_mem_region(enum mem_region_id reg_id);

extern int bsp_update_feca_dev(feca_dev_id id, feca_device_t *dev);

#endif
