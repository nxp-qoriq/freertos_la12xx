// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#ifndef _BBDEV_IPC_H_
#define _BBDEV_IPC_H_

#include <geul_qdma.h>
#include <geul_bbdev_ipc.h>


/**
 * @file        bbdev_ipc.h
 * @brief       BBDEV_IPC related APIs.
 * @addtogroup  BBDEV_API
 * @{
 */


#define pr_debug	PRINTF
#define pr_err		PRINTF
#define fsl_print	PRINTF

#define BBDEV_IPC_DEV_ID_0	0

#define BBDEV_IPC_ENC_OP_TYPE	0x1000000
#define BBDEV_IPC_DEC_OP_TYPE	0x2000000

#define BBDEV_IPC_MAX_CORES	GEUL_E200_CORE_GLOBAL_NUM

#define BBDEV_IPC_MAX_QUEUES	(MAX_LDPC_ENC_FECA_QUEUES + \
				MAX_LDPC_DEC_FECA_QUEUES + \
				MAX_POLAR_ENC_FECA_QUEUES + \
				MAX_POLAR_DEC_FECA_QUEUES + \
				MAX_RAW_QUEUES)

/** For internal BSP use */
extern struct gul_hif ipc_hif_area;

/** Attributes of a queue */
struct queue_attr_t {
	/* Queue ID */
	uint32_t queue_id;
	/* Type of operation supported on this queue */
	uint32_t op_type;
	/* Queue depth */
	uint32_t depth;
	/* FECA Transport Block ID to be used for this queue */
	uint32_t feca_blk_id;
	/** FECA input circular buffer size */
	uint32_t feca_input_circ_size;
	/* Core ID on which this queue should be used */
	uint32_t core_id;
};

/** Attributes of the device */
struct dev_attr_t {
	/** Total number of FECA SE channels required for BBDEV */
	int num_feca_se_channels;
	/** Total number of FECA SD channels required for BBDEV */
	int num_feca_sd_channels;
	/** Total number of FECA CE channels required for BBDEV */
	int num_feca_ce_channels;
	/** Total number of FECA CD channels required for BBDEV */
	int num_feca_cd_channels;
	/** Use single QDMA for FECA SD input */
	int use_feca_sd_single_qdma;
	/** Total number of Queues configured */
	int num_queues;
	/** Per queue configuration */
	struct queue_attr_t qattr[BBDEV_IPC_MAX_QUEUES];
};

/**
 * Init function to initialize the BBDEV IPC subsystem. This API will
 * also internally wait on the BBDEV to be initialized on the host (DPDK).
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param core_id
 *   Core ID from where this API is being called
 *
 * @return
 *   - 0  on success, error code otherwise
 */
int bbdev_ipc_init(uint8_t dev_id, uint8_t core_id);

/**
 * Get attributes of the device
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 *
 * @return
 *   - pointer to device attributes
 */
struct dev_attr_t *
bbdev_ipc_get_dev_attr(uint8_t dev_id);

/**
 * Set input circular buffer size to shared memory area.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @size
 *   The input circular buffer size
 * @queue
 *   Queue index
 *
 */
void
bbdev_ipc_set_cb_size(uint8_t dev_id, uint32_t size, uint32_t queue);
/**
 * Check if BBDEV FECA reset is requested by Host
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 *
 * @return
 *   1 - if reset is requested, 0 otherwise
 */
int
bbdev_ipc_soft_reset_request(uint8_t dev_id);

/**
 * Reset BBDEV IPC queues to initial configured state and notify
 * host that the reset is complete.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 */
void
bbdev_ipc_soft_reset_complete(uint8_t dev_id);

/**
 * Configures a queue on BBDEV IPC device. This API should be called from
 * where I/O needs to be done.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param queue_id
 *   The index of the queue.
 *
 * @return
 *   - 0 on success, error code otherwise
 */
int bbdev_ipc_queue_configure(uint8_t dev_id, uint16_t queue_id);

/**
 * Checks if device on host has been initialized yet or not. As FreeRTOS
 * runs on multi-core, this API is used to determine if HOST LIB has
 * been completed or not. 'bbdev_ipc_queue_configure' API should be called
 * before this API.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 *
 * @return
 *   - 1 if device has been initialized
 *   - 0 if device has not been initialized yet
 */
int bbdev_ipc_is_host_initialized(uint8_t dev_id);

/**
 * Signal the Host device that BBDEV is now ready to process packets.
 * This will signal the Host only when all the cores have been initialized.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 *
 */
void bbdev_ipc_signal_ready(uint8_t dev_id);

/**
 * To dequeue a burst of operations to encode/decode from a queue.
 * This function returns only the current contents of the queue, and does not
 * block until num_ops is available.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array where operations will be dequeued to. Must have at least
 *   num_ops entries
 * @param num_ops
 *   The maximum number of operations to dequeue.
 *
 * @return
 *   The number of operations actually dequeued (this is the number of processed
 *   entries in the ops array).
 */
uint16_t bbdev_ipc_dequeue_ops(uint8_t dev_id, uint16_t queue_id,
		struct bbdev_ipc_dequeue_op **ops, uint16_t num_ops);

/**
 * To enqueue a burst of processed encode/decode operations to a queue.
 * This function only enqueues as many operations as currently possible and
 * does not block until num_ops entries in the queue are available.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param queue_id
 *   The index of the queue.
 * @param ops
 *   Pointer array containing operations to be enqueued. Must have at least
 *   num_ops entries
 * @param num_ops
 *   The maximum number of operations to enqueue.
 *
 * @return
 *   The number of operations actually enqueued (this is the number of processed
 *   entries in the ops array).
 */
uint16_t bbdev_ipc_enqueue_ops(uint8_t dev_id, uint16_t queue_id,
		struct bbdev_ipc_enqueue_op **ops, uint16_t num_ops);

/**
 * Consume raw operation. Valid for HOST->MODEM queues only. This will mark
 * internal BD rings as free and do internal cleanups.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param queue_id
 *   The index of the queue.
 * @param op
 *   Pointer containing operation to be consumed.
 *
 * @return
 *   Status of consume operation.
 */
uint16_t bbdev_ipc_consume_raw_op(uint16_t dev_id, uint16_t queue_id,
		struct bbdev_ipc_raw_op_t *op);

/**
 * Enqueue a RAW operation to a queue of the device.
 *
 * If confirmation is required then the memory for the ‘op’ structure should
 * be allocated from heap/mempool and should be freed only after confirmation.
 * Otherwise, it shall be on stack or if on heap, should be freed after enqueue
 * operation.
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param queue_id
 *   The index of the queue.
 * @param op
 *   Pointer containing operation to be enqueued.
 *
 * @return
 *   Status of enqueue operation.
 */
int bbdev_ipc_enqueue_raw_op(uint16_t dev_id, uint16_t queue_id,
		struct bbdev_ipc_raw_op_t *op);

/**
 * Dequeue a raw operation.
 * For MODEM->HOST queues, this would provide RAW op which had ‘conf_required’
 * set in the ‘rte_bbdev_raw_op; structure.
 * For HOST->MODEM queues, this would provide RAW op which are sent from MODEM.
 * ‘op’ memory would be internally allocated
 *
 * @param dev_id
 *   The identifier of the device. Currently only one device id is
 *   supported - 'BBDEV_IPC_DEV_ID_0'
 * @param queue_id
 *   The index of the queue.
 *
 * @return
 *   Pointer containing dequeued operation.
 */
struct bbdev_ipc_raw_op_t *bbdev_ipc_dequeue_raw_op(uint16_t dev_id,
		uint16_t queue_id);

/** @} */
#endif
