/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2022-2023 NXP */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "Time.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "debug_console.h"
#include "l1c_defs.h"
#include "l1c_cli.h"
#include "l1c_ipi.h"
#include "spinlock_api.h"
#include "semphr.h"
#include "l1c_fwk_tasks.h"
#include "l1c_debug.h"

extern portBASE_TYPE prvL1CDemoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
ipi_evt_entry ipi_evt_list[L1C_IPI_CMD_MAX] __attribute__ ((section (".smem")));

uint32_t cores_to_mask(uint8_t num_cores, ...)
{
	va_list valist;
	uint32_t core_mask = 0;
	uint8_t i;

	va_start(valist, num_cores);

	for (i = 0; i < num_cores; i++)
		core_mask |= (1 << va_arg(valist, int));

	va_end(valist);
	return core_mask;
}

bool_t l1c_bind_ipi_cmd(uint32_t ipi_cmd_id, uint32_t core_mask, ipi_funcptr callback)
{
	if (ipi_cmd_id >= L1C_IPI_CMD_MAX)
	{
		PRINTF("Invalid IPI command id !\r\n");
		return pdFALSE;
	}

	ipi_evt_list[ipi_cmd_id].core_mask = core_mask;
	ipi_evt_list[ipi_cmd_id].callback = callback;

	return pdTRUE;
}

bool_t l1c_send_ipi_cmd(uint32_t ipi_cmd_id)
{
	bool_t ret = pdTRUE;

	/* sanity check */
	if (ipi_cmd_id >= L1C_IPI_CMD_MAX)
	{
		PRINTF("Invalid IPI command id !\r\n");
		return pdFALSE;
	}

	/* exit early if no core assigned to cmd id */
	if (!ipi_evt_list[ipi_cmd_id].core_mask)
		return pdFALSE;

	for (uint8_t i = 0; i < GEUL_E200_CORE_GLOBAL_NUM; i++) {
		if (ipi_evt_list[ipi_cmd_id].core_mask & (1 << i)) {

			e200_trace_all(E200_TRACE_MSG_IPI_DEMO, E200_TRACE_PARAM_BEGIN);

			if (!vIPISendData(i, IPI_EVT_L1C_REFAPP_IPI, (void *)ipi_cmd_id)) {
				PRINTF("Failed to send ipi cmd (%d) to core %d\r\n", ipi_cmd_id, i);
				ret = pdFALSE;
			}
		}
	}

	return ret;
}

bool_t l1c_exec_ipi_cmd(uint32_t ipi_cmd_id)
{
	ipi_funcptr fptr;

	/* sanity check */
	if (ipi_cmd_id >= L1C_IPI_CMD_MAX)
	{
		PRINTF("Invalid IPI command id !\r\n");
		return pdFALSE;
	}

	if (!(ipi_evt_list[ipi_cmd_id].core_mask & (1 << crt_core_id)))
	{
		PRINTF("Got other core's IPI command !\r\n");
		return pdFALSE;
	}

	fptr = (ipi_funcptr) ipi_evt_list[ipi_cmd_id].callback;
	(*fptr)();

	return pdTRUE;
}

void l1c_ipi_agent_main(void *pvParameters)
{
	void *data;

	UNUSED(pvParameters);

	vIPIEventRegister(IPIGlobalEventID[IPI_EVT_L1C_REFAPP_IPI], &pxRxQueue[IPI_EVT_L1C_REFAPP_IPI], NULL, NULL);
	PRINTF("Registering L1C RefApp IPI Event = %d \r\n", IPI_EVT_L1C_REFAPP_IPI);

	while(1)
	{
		xQueueReceive(pxRxQueue[IPI_EVT_L1C_REFAPP_IPI], &data, portMAX_DELAY);

		e200_trace(E200_TRACE_MSG_IPI_DEMO, E200_TRACE_PARAM_END);

		//PRINTF("got IPI message (data=%#x)\r\n", &data);
		l1c_exec_ipi_cmd((uint32_t) data);
	}
}

void init_l1c_ipi(void)
{
	TaskHandle_t ipi_agent_handle;
	EventGroupHandle_t cli_group_handle, agent_group_handle;
	int	ret = 0;

	cli_group_handle = xEventGroupCreate();
	if (!cli_group_handle)
		PRINTF("Failed to create L1C RefApp CLI task [core %d] event group\n\r", crt_core_id);

	ret = xTaskCreate(l1c_ipi_agent_main, "IPI Agent", DEFAULT_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &ipi_agent_handle);
	if(ret != pdPASS)
		PRINTF("Failed to create RefApp's IPI Agent task on core %d\n\r", crt_core_id);

	agent_group_handle = xEventGroupCreate();
	if (!agent_group_handle)
		PRINTF("Failed to create RefApp's IPI Agent task event group on core %d\n\r", crt_core_id);
}
