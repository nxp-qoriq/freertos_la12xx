// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "tc_timer.h"

#ifdef GEUL_DEMO_TIMER_TEST
#include "FreeRTOS.h"
#include "task.h"
#include "mpic_regs.h"
#include "timers.h"

uint32_t ret1 = -1, ret2 = -1;
TickType_t xAutoReloadTime[20];
uint32_t time_counter;

#define mainONE_SHOT_TIMER_PERIOD pdMS_TO_TICKS(1000)
static void prvOneShotTimerCallback(TimerHandle_t xTimer)
{
	xAutoReloadTime[1] = xTaskGetTickCount();
	if (xTimerDelete(xTimer, 0) != pdPASS)
		log_err("\nLA12XX DemoTimerTest: xTimerDelete Failed !!!\r\n");

	ret1 = 0;
}

#define mainAUTO_RELOAD_TIMER_PERIOD pdMS_TO_TICKS(500)
static void prvAutoReloadTimerCallback(TimerHandle_t xTimer)
{
	(void)xTimer;

	if (time_counter == 5)
		ret2 = 0;

	if (time_counter != 0)
		xAutoReloadTime[time_counter] = xTaskGetTickCount();

	time_counter++;
}

void vGeulDemoTimerTest(void)
{
	TimerHandle_t xOneShotTimer, xAutoReloadTimer;
	BaseType_t xTimer1Started, xTimer2Started;
	u32 uiCurrentCore = ulMpicCurrentCore();

	log_info("%s: Timer create\n\r", __func__);
	xOneShotTimer = xTimerCreate(
			"OneShot",
			mainONE_SHOT_TIMER_PERIOD,
			pdFALSE,
			0,
			prvOneShotTimerCallback);

	if (xOneShotTimer != NULL) {
		xAutoReloadTime[0] = xTaskGetTickCount();
		xTimer1Started = xTimerStart(xOneShotTimer, 0);

		if (xTimer1Started == pdPASS) {
			log_dbg("%s: Task Yield\r\n", __func__);
			taskYIELD();
		}
	}
	vTaskDelay(pdMS_TO_TICKS(1000));
	log_info("LA12XX DemoTimerTest: One shot Timer End Time=0x%x, Start Time=0x%x, Diff=%d\n\r",
			xAutoReloadTime[0], xAutoReloadTime[1],
			(int)(xAutoReloadTime[1] - xAutoReloadTime[0]));
	xAutoReloadTime[0] = 0;
	if (xTimerDelete(xOneShotTimer, 0) != pdPASS)
		log_err("\nLA12XX DemoTimerTest: xTimerDelete Failed !!!\r\n");

	xAutoReloadTimer = xTimerCreate(
			"AutoReload",
			mainAUTO_RELOAD_TIMER_PERIOD,
			pdTRUE,
			0,
			prvAutoReloadTimerCallback);

	if (xAutoReloadTimer != NULL) {
		log_info("%s: Timer start=0x%x\n\r", __func__, xTaskGetTickCount());
		xTimer2Started = xTimerStart(xAutoReloadTimer, 0);

		if (xTimer2Started == pdPASS) {
			log_dbg("%s: Task Yield\r\n", __func__);
			taskYIELD();
		}
	}

	vTaskDelay(pdMS_TO_TICKS(5000));

	for (uint32_t i = 2; i < time_counter; i++) {
		log_info("LA12XX DemoTimerTest: Auto reload Current Timer=0x%x, Prev Timer=0x%x, Diff=%d\n\r",
			xAutoReloadTime[i], xAutoReloadTime[i-1],
			(int)(xAutoReloadTime[i]-xAutoReloadTime[i-1]));
		if ((xAutoReloadTime[i]-xAutoReloadTime[i-1]) != pdMS_TO_TICKS(500))
			ret2 = -1;
	}
	time_counter = 0;

	if (xTimerDelete(xAutoReloadTimer, 0) != pdPASS)
		log_err("\nLA12XX DemoTimerTest: xTimerDelete Failed !!!\r\n");

	if (ret1 == 0 && ret2 == 0) {
		log_info("Timer: Timing test successful\n\r");
		SET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_TIMER_TEST_STATUS);
	} else {
		log_info("Timer: Timimg test unsuccessful\r\n");
		RESET_TEST_STATUS(uiCurrentCore, GEUL_DEMO_TIMER_TEST_STATUS);
	}
}
#endif
