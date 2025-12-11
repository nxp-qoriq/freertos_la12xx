// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023 NXP
 */

#include <stdio.h>
#include "tc_la12xx_float.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if GEUL_DEMO_FLOAT_TEST

int float_log(void)
{
	double x = 7468.19246, l1 = 8.918408, l2, y;

	y = (double)((int)(1000000*log(x)))/1000000;

	if (!((y > (l1 - 1e-9)) && (y < (l1 + 1e-9))))
		return -1;

	x = 23429.231239;
	l2 = 10.061739;
	y = (double)((int)(1000000*log(x)))/1000000;

	if (!((y > (l2 - 1e-9)) && (y < (l2 + 1e-9))))
		return -1;

	return 0;
}

int nearestInt(void)
{
	int r_val = 0;
	return r_val;
}

void vFloatTest(void)
{
	int ret = 0;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("Float: Floating point log test\r\n");
	ret = float_log();
	if (ret == 0) {
		log_info("Float: Floating point test successful\r\n");
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_FLOAT_TEST_STATUS);
	} else {
		log_info("Float: Floating point test unsuccessful\r\n");
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_FLOAT_TEST_STATUS);
	}
}
#endif
