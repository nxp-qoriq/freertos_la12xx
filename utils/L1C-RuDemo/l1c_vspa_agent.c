/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2022, 2024 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "ppc.h"
#include "ppu_intrinsics.h"
#include "l1c_defs.h"
#include "l1c_vspa_agent.h"
#include "l1c_dma.h"
#include "l1c_fwk_tasks.h"
#include "l1c_ipi.h"
#include "l1c_debug.h"

l1c_vspa_ring_ctrl_t vspa_ring_ctrl[L1C_VSPA_NUM_CORES]  __attribute__ ((section (".smem")));
uint64_t vspa_agent_msgs __attribute__ ((section (".smem")));
QueueHandle_t vspa_agent_queue = NULL;
vspa_envelope_t envelope_pool[L1C_VSPA_NUM_CORES];

uint8_t vspa_core_ovly_mode[L1C_VSPA_NUM_CORES] __attribute__ ((section (".smem")));

static char ovly_option_names[4][20] = {
        "FR1 TDD",
        "Benchmark",
        "DPDH",
        "FR2 TDD",
};

char *ovly_option_name(ovly_options_t ovly)
{
	return &ovly_option_names[ovly][0];
}

uint8_t l1c_vspa_msg_send(uint8_t vspa_idx, void *msg_src_addr)
{
	uint32_t        time_stamp = 0;
	uint32_t        dst_addr, src_addr, ctrl;
	uint8_t         seq_id;
	l1c_vspa_msg_t *msg_p      = (l1c_vspa_msg_t *) msg_src_addr;
	uint8_t         msg_type   = MSG_HDR_GET_MSGID(msg_p->msg_hdr);

	e200_trace(E200_TRACE_MSG_VSPA, E200_TRACE_PARAM_BEGIN);

	seq_id = vspa_ring_ctrl[vspa_idx].seq_id++;
	msg_p->msg_hdr = swap_uint32(MSG_HDR_BUILD(msg_type, seq_id, time_stamp));
	src_addr = (uint32_t)msg_p + 0x100000 + CORE_DMEM_SIZE * crt_core_id; /* e200 core id */;
	dst_addr = (uint32_t)L1C_BUF_BASE_ADDR + vspa_ring_ctrl[vspa_idx].ring_pos * L1C_VSPA_MSG_SIZE;

	/* DMA to respective buffer location in DMEM VSPA memory */
	vspa_set_dmareg_dmem_addr(vspa_idx, dst_addr);
	vspa_set_dmareg_axi_addr(vspa_idx, src_addr);
	vspa_set_dmareg_size_bytes(vspa_idx, L1C_VSPA_MSG_SIZE);

	__asm__ ("msync");

	ctrl = (uint32_t)(L1C_E2V_DMA_CHAN | DMA_GO_TO_VCPU | DMA_AXI2DMEM);
	vspa_set_host_to_vspa_flags0(vspa_idx,
								 (uint32_t)(
									 (1 << vspa_ring_ctrl[vspa_idx].ring_pos) |
									 (1 << (L1C_IF_A2V_HOST_FLAGS_MSG_RCVD_LSB + vspa_ring_ctrl[vspa_idx].ring_pos))
								 )
								);

	vspa_set_dmareg_xfer_ctrl(vspa_idx, ctrl);

	__asm__ ("msync");

#if 0
    PRINTF("--->l1c_vspa_msg_send(core=%d, ring_pos=%d, src_addr=%#x\r\n",
		    vspa_idx,
		    vspa_ring_ctrl[vspa_idx].ring_pos,
		    msg_src_addr);
#endif

	vspa_ring_ctrl[vspa_idx].ring_pos++;
	vspa_ring_ctrl[vspa_idx].ring_pos %= L1C_VSPA_RING_DEPTH;
	vspa_ring_ctrl[vspa_idx].msg_count++;

	e200_trace(E200_TRACE_MSG_VSPA, E200_TRACE_PARAM_END);

	return 1;
}
void dump_core_msg_count()
{
	uint8_t i;

	for (i = 0; i < L1C_VSPA_NUM_CORES; i++)
	{
		PRINTF("core %d: msg_count = %d\r\n", i, vspa_ring_ctrl[i].msg_count);
	}
}

void l1c_vspa_agent_enqueue_msg(uint8_t vspa_dst_core, l1c_msg_types msg_type, void *payload, size_t payload_size)
{
	vspa_envelope_t *e;
	l1c_vspa_msg_t msg_to_send = { 0, { 0 } };
	static uint8_t pool_idx = 0;

	if (!vspa_agent_queue)
		return;

	e = &envelope_pool[pool_idx];
	pool_idx++;
	pool_idx %= L1C_VSPA_NUM_CORES;

	msg_to_send.msg_hdr = MSG_HDR_BUILD(msg_type, 0, 0);
	memcpy((void *)&msg_to_send.payload,
		   (void *)payload,
		   (payload_size > L1C_VSPA_PAYLOAD_SIZE) ? L1C_VSPA_PAYLOAD_SIZE : payload_size);
	e->vspa_core = vspa_dst_core;
	e->vspa_msg = msg_to_send;

	xQueueSend(vspa_agent_queue, &e, (TickType_t)0);
}

void l1c_vspa_set_runtime_overlay(uint8_t vspa_core, ovly_options_t ovly)
{
	ovly_msg_t msg = {
		.ovly_selector = swap_uint32(ovly),
	};

	/* check if the desired overlay mode is already set */
	if (vspa_core_ovly_mode[vspa_core] == ovly)
		return;

	vspa_core_ovly_mode[vspa_core] = ovly;
	l1c_vspa_agent_enqueue_msg(vspa_core, L1C_MSG_A2V_OVERLAY, &msg, sizeof(msg));
}

int l1c_vspa_get_runtime_overlay(uint8_t vspa_core)
{
	if (vspa_core_ovly_mode[vspa_core] == RT_OVLY_CNT)
		return -1;

	return vspa_core_ovly_mode[vspa_core];
}

int wait_for_l1c_config(void);

#if 1
void l1c_vspa_agent_main(void *pvParameters)
{
	UNUSED(pvParameters);

	while (1) {
		int ret = wait_for_l1c_config();

		if (ret < 0)
			log_err("wait_for_l1c_config failed\n\r");
		else if (ret)
			break;
	}

	while (1) {
		vTaskDelay(portMAX_DELAY);
	}
}
#else
void l1c_vspa_agent_main(void *pvParameters)
{
	vspa_envelope_t *e;
	uint8_t i;

	UNUSED(pvParameters);

	for (i = 0; i < L1C_VSPA_NUM_CORES; i++)
		vspa_core_ovly_mode[i] = RT_OVLY_CNT;

	/* create receive queue at the first run */
	if (!vspa_agent_queue)
		vspa_agent_queue = xQueueCreate(VSPA_AGENT_NUM_Q_ENTRIES, sizeof(vspa_envelope_t *));

	if (!vspa_agent_queue)
	{
		/* Queue was not created and must not be used. */
		PRINTF("Error creating VSPA Agent message queue!\r\n");
		return;
	}

	/* wait for messages in a loop */
	for( ;; )
	{
		xQueueReceive(vspa_agent_queue, (void *)&e, portMAX_DELAY);
		l1c_vspa_msg_send(e->vspa_core, &e->vspa_msg);
		vspa_agent_msgs++;
	}
}
#endif
