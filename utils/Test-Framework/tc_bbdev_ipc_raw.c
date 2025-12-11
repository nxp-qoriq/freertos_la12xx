// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021-2022 NXP
 */

#include "FreeRTOS.h"
#include "test_framework.h"
#include "bbdev_ipc.h"

#define BBDEV_IPC_DEV_ID_0	0
#define TEST_REPETITIONS 	10000
#define TEST_BUFFER_INPUT_VAL 	0x01020304
#define PACKET_LENGTH 8

#define TOTAL_CORES			4

/*
 * This modem core mapping for specific test case is only for demo application.
 * User can use any core for any queue/test.
 */
#define CONF_VALIDATION_CORE		0
#define NON_CONF_VALIDATION_CORE	1
#define ROUND_TRIP_LATENCY_CORE		2
#define UNIDIRECTIONAL_LATENCY_CORE	3


uint32_t input_data[TOTAL_CORES][PACKET_LENGTH] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));;
uint32_t output_data[TOTAL_CORES][PACKET_LENGTH] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));;


static void
host_to_modem_test(int queue_id, int conf_mode, int latency_test)
{
	struct bbdev_ipc_raw_op_t *deq_raw_op;
	int ret, i, j, len;
	uint32_t *in_data, *out_data;

	i = 0;
	while (i < TEST_REPETITIONS) {

		deq_raw_op = bbdev_ipc_dequeue_raw_op(BBDEV_IPC_DEV_ID_0,
						      queue_id);
		if (deq_raw_op == NULL)
			continue;

		if (!latency_test && conf_mode) {
			if (deq_raw_op->out_addr) {
				in_data = (uint32_t *)deq_raw_op->in_addr;
				out_data = (uint32_t *)deq_raw_op->out_addr;
				len = deq_raw_op->in_len / sizeof(uint32_t);

				for (j = 0; j < len; j++)
					out_data[j] = in_data[j];

				deq_raw_op->out_len = len * sizeof(uint32_t);
			} else {
				log_err("out_addr is null\n\r");
				log_err("\n\r============================================================================================\n\r");
				log_err("HOST->MODEM test failed for confirmation mode.\n\r");
				log_err("============================================================================================\n\r");
				return;
			}
		}

		ret = bbdev_ipc_consume_raw_op(BBDEV_IPC_DEV_ID_0, queue_id,
					       deq_raw_op);
		if (ret) {
			log_err("rte_bbdev_consume_raw_op failed (%d)\n\r", ret);
			log_err("\n\r============================================================================================\n\r");
			if (conf_mode)
				log_err("HOST->MODEM test failed for confirmation mode.\n\r");
			else
				log_err("HOST->MODEM test failed for non confirmation mode.\n\r");
			log_err("============================================================================================\n\r");
			return;
		}

		i++;
	}

	log_info("\n\r============================================================================================\n\r");
	if (conf_mode) {
		log_info("HOST->MODEM test completed for confirmation mode. Check application logs for results.\n\r");
	} else {
		log_info("Received %d operations for HOST->MODEM\n\r",
			 TEST_REPETITIONS);
		log_info("HOST->MODEM test successful for non confirmation mode\n\r");
	}
	log_info("============================================================================================\n\r");
}

static void
modem_to_host_test(int queue_id, int conf_mode)
{
	struct bbdev_ipc_raw_op_t enq_raw_op, *deq_raw_op;
	int ret, i, j;
	u8 core_id = (u8)ulMpicCurrentCore();

	enq_raw_op.in_len = PACKET_LENGTH * sizeof(uint32_t);
	enq_raw_op.in_addr = (uint32_t)input_data[core_id];
	enq_raw_op.out_len = 0;
	if (conf_mode)
		enq_raw_op.out_addr = (uint32_t)output_data[core_id];
	else
		enq_raw_op.out_addr = 0;

	i = 0;
	while (i < TEST_REPETITIONS) {

		for (j = 0; j < PACKET_LENGTH; j++) {
			input_data[core_id][j] = TEST_BUFFER_INPUT_VAL;
			if (conf_mode)
				output_data[core_id][j] = 0;
		}

		ret = bbdev_ipc_enqueue_raw_op(BBDEV_IPC_DEV_ID_0, queue_id,
					       &enq_raw_op);
		if (ret < 0)
			continue;

		if (conf_mode) {
			do {
				deq_raw_op = bbdev_ipc_dequeue_raw_op(
							BBDEV_IPC_DEV_ID_0,
							queue_id);
			} while (!deq_raw_op);

			if (deq_raw_op->out_addr) {
				for (j = 0; j < PACKET_LENGTH; j++) {
					if (output_data[core_id][j] !=
					    input_data[core_id][j]) {
						log_err("output %x does not match expected output %x",
							output_data[core_id][j],
							input_data[core_id][j]);
						log_err("\n\r============================================================================================\n\r");
						log_err("MODEM->HOST test failed for confirmation mode\n\r");
						log_err("============================================================================================\n\r");
						return;
					}
				}
			} else {
				log_err("out_addr is null\n\r");
				log_err("\n\r============================================================================================\n\r");
				log_err("HOST->MODEM test failed for confirmation mode.\n\r");
				log_err("============================================================================================\n\r");
				return;
			}
		}

		i++;
	}

	log_info("\n\r============================================================================================\n\r");
	if (conf_mode) {
		log_info("Validated %d operations for MODEM->HOST\n\r",
			 TEST_REPETITIONS);
		log_info("MODEM->HOST test successful for confirmation mode\n\r");
	} else {
		log_info("MODEM->HOST test completed for non confirmation mode. Check application logs for results.\n\r");
	}
	log_info("============================================================================================\n\r");
}

static void
round_trip_latency_test(int h2m_queue_id, int m2h_queue_id)
{
	struct bbdev_ipc_raw_op_t enq_raw_op, *deq_raw_op;
	int ret, i;
	u8 core_id = (u8)ulMpicCurrentCore();

	enq_raw_op.in_len = PACKET_LENGTH * sizeof(uint32_t);
	enq_raw_op.in_addr = (uint32_t)input_data[core_id];
	enq_raw_op.out_len = 0;
	enq_raw_op.out_addr = (uint32_t)output_data[core_id];

	i = 0;
	while (i < TEST_REPETITIONS) {

		deq_raw_op = bbdev_ipc_dequeue_raw_op(BBDEV_IPC_DEV_ID_0,
						      h2m_queue_id);
		if (deq_raw_op == NULL)
			continue;

		i++;

		ret = bbdev_ipc_consume_raw_op(BBDEV_IPC_DEV_ID_0, h2m_queue_id,
					       deq_raw_op);
		if (ret < 0) {
			log_err("rte_bbdev_consume_raw_op failed (%d)", ret);
			log_err("\n\r============================================================================================\n\r");
			log_err("Round trip latency test failed.\n\r");
			log_err("============================================================================================\n\r");
			return;
		}

retry_enqueue:

		ret = bbdev_ipc_enqueue_raw_op(BBDEV_IPC_DEV_ID_0, m2h_queue_id,
					       &enq_raw_op);
		if (ret < 0)
			goto retry_enqueue;
	}

	log_info("\n\r============================================================================================\n\r");
	log_info("Round trip latency test completed with %d test cases. Check application logs for results.\n\r",
		 i);
	log_info("============================================================================================\n\r");
}

static int bbdev_ipc_raw_test(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();
	struct dev_attr_t *attr;
	int queue_id, num_queues_this_core = 0;
	int ret, i, queue_index = 0;
	int queue_ids[BBDEV_IPC_MAX_QUEUES];
	int h2m_queue_id, m2h_queue_id;

	/* Wait for bbdev device ready.*/
	while (!bbdev_ipc_is_host_initialized(BBDEV_IPC_DEV_ID_0)) {}

	attr = bbdev_ipc_get_dev_attr(BBDEV_IPC_DEV_ID_0);

	for (i = 0; i < attr->num_queues; i++) {
		if (attr->qattr[i].core_id != core_id)
			continue;

		queue_id = attr->qattr[i].queue_id;
		queue_index = num_queues_this_core;
		queue_ids[queue_index] = queue_id;
		ret = bbdev_ipc_queue_configure(BBDEV_IPC_DEV_ID_0, queue_id);
		if (ret != IPC_SUCCESS) {
			log_err("queue configure failed for queue %d error %d \n\r",
				queue_id, ret);
			return ret;
		}
		num_queues_this_core++;
	}

	log_info("\n%s: ==> Processing queues [", __func__);
	for (queue_index = 0; queue_index < num_queues_this_core; queue_index++) {
		if (queue_index == 0)
			log_info("%d", queue_ids[queue_index]);
		else
			log_info(", %d", queue_ids[queue_index]);
	}
	log_info("]\n");

	/* Signal Host to start sending packets for processing */
	bbdev_ipc_signal_ready(BBDEV_IPC_DEV_ID_0);

	if (num_queues_this_core < 2)
		return 0;

	h2m_queue_id = queue_ids[0];
	m2h_queue_id = queue_ids[1];

	if (core_id == CONF_VALIDATION_CORE) {
		host_to_modem_test(h2m_queue_id, 1, 0);
		modem_to_host_test(m2h_queue_id, 1);
	} else if (core_id == NON_CONF_VALIDATION_CORE) {
		host_to_modem_test(h2m_queue_id, 0, 0);
		modem_to_host_test(m2h_queue_id, 0);
	} else if (core_id == ROUND_TRIP_LATENCY_CORE) {
		round_trip_latency_test(h2m_queue_id, m2h_queue_id);
	} else if (core_id == UNIDIRECTIONAL_LATENCY_CORE) {
		host_to_modem_test(h2m_queue_id, 1, 1);
		modem_to_host_test(m2h_queue_id, 1);
	}

	return 0;
}

void vBbdevIpcRawTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	/*Start Rx and Tx on respective cores.*/
	if(bbdev_ipc_raw_test()!= 0)
		RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	else
		SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);

	return;
}
