// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#ifndef SRC_QDMA_H_
#define SRC_QDMA_H_

/**
 * @file        qdma.h
 * @brief       QDMA-related APIs.
 * @addtogroup  QDMA_API
 * @{
 */

#include "platform_def.h"
#include "Time.h"
#include <geul_qdma.h>

/** Description:
 * ------------
 * There are 4 QDMA blocks available and each block can be affined to a core.
 * Each block can be configured with 8 RX queues and 1 status queue. Each Rx
 * queue can have maximum 64 descriptors of type "DescriptorFormat_t" (Command
 * descriptor aka CD), the main descriptor, we will use name "CD" for this
 * descriptor in this help. In Long format type of descriptors, each CD's
 * addr member points to another descriptor of type "NxpQdmaCLT_t" aka
 * "CLT" (command list table).
 * But In ultra short format type only main descriptor CD is required for processing.
 *
 * Below is sample diagram of a queue shows the descriptors for long format type:
 *
 *  --------------------------------------------------------------------------
 *  | CD0|CD1|CD2|CD3|           .........                         |CD62|CD63|
 *  |____|___|___|___|_____________________________________________|____|____|
 *    |    |   |    |                                                |    |
 *   CLT0 CLT1 CLT2 CLT3         .........                        CLT62  CLT63
 *
 * This help will explain how to use the QDMA APIs. Helper APIs usage is available
 * in "utils/Test-Framework/tc_qdma.c" and can be referred. Below are 2 methods
 * of QDMA APIs usage:
 *
 * 1. Usage of optimized long format type operations. In this implementation, user
 *    must have expertise to write the descriptors. This is best method for good
 *    throughput numbers. Following are the steps:
 *
 *    1. vFillQdmaValidBlk(): user need to call this API only for once to reserve
 *    			     memory required for QDMA blocks. It will reserve the
 *    			     memory for CDs.
 *    2. QdmaInitNoIntr() or QdmaInit(): These APIs should be called from each worker
 *    			     core to intialize QDMA and affined the QDMA block to core.
 *    3. QdmaFillCltDesc():  Call this API for each CLT (Max 64 CLTs possible per queue)
 *    			     of a qDMA queue to pre-fill with given values
 *    			     (can be modified during enqueue). In next API call
 *    			     these CLTs will be attached to their respective CDs. User
 *    			     will have to reserve the memory for CLTs from HIF memory.
 *    4. QdmaFillDesc():     Attach CLT memory to each of the HW descriptor (CD) with some
 *    			     default values of long format type. As NxpClt memory is
 *    			     given by the user so user has full control to modify the
 *    			     CLT descriptor.
 *
 *    Intialization is done till this point, following are the steps of datapath:
 *
 *    To enqueue a new job, user has to give source and destination addresses in
 *    the descriptor. sometimes there is also need to modify the descriptor for a
 *    particular job like enabling/disabling striding, update the SER settings etc.
 *    Following steps can be used to do this:
 *
 *	1. qDMA_get_clt_addr(): get "NxpQdmaQueue_t" pointer (queue) of a queue, This
 *			will have both CD and CLT descriptor pointers.
 *	2. Obtain the index of current CD and CLT pointers which are next to be
 *	   processed by the QDMA using queue->index.
 *
 * 		Current CDs pointer is "queue->DescHead[queue->index]"
 * 		and current CLTs pointer is "queue->NxpClt[queue->index]"
 * 		Now user have both current descriptors and these can be modified as per
 * 		the need (e.g. update source/destination addresses). Refer:
 * 		"How to write descriptors" later in this help.
 *
 * 	3. Last step is to tell the hardware that descriptor writing is done, so processes
 * 	   the job by calling API: "NxpCmdQueueEnqueueExt()"
 *	4. Next if user has written a descriptor with enable job completion status and
 *	   registered a callback then user can wait for the callback function to be called
 *	   by the driver or can proceed for further processing.
 *
 *
 * 2. QDMA has some helper APIs available and those can be used in the testing.
 *    Most of the helper APIs usage is demonstrated in test framework in file:
 *    "utils/Test-Framework/tc_qdma.c" and user can refer that for the usage.
 *    There is no need to gain knowledge of descriptor writing if using helper
 *    QDMA APIs, only need to understand the flow of APIs.
 *    e.g. below is one of the example of  ultra short format type usage:
 *
 *    1. vFillQdmaValidBlk(): user need to call this API only for once to reserve
 *    			     memory required for QDMA blocks. It will reserve the
 *    			     memory for CDs.
 *    2. QdmaInitNoIntr() or QdmaInit(): These APIs should be called from each worker
 *    			     core to intialize QDMA and affined the QDMA block to core.
 *    3. DmaUSFFillIssue(): helper API fill the descriptor with all required values and
 *       		    also issue the command to hardware.
 *
 *
 * How to write descriptors:
 * ------------------------
 *
 * User must check the header file "geul_qdma.h" to aware of how the descriptors structures
 * are written. Some of the explanation is available in the descriptor structure definition
 * itself and there are also lot of helper APIs in this file that can be referred to modify
 * the descriptors and for detail explanation one can refer QDMA reference manual,
 * section: "qDMA command descriptor format" as there is lot of configuration information
 * which cannot be explained in this help.
 *
 * few examples:
 * ------------
 * Disable status notification: cd->Cfg2 &= 0xFFFEFFFF;
 * set final bit in CLT: clt->dCmdListTable.Cfg1 = SWAP_32(QDMA_CLT_F);
 *
 *
 * QDMA APIs on LA12xxA0 vs LA12xxB0:
 * ---------------------------------
 *
 * There is no change in QDMA APIs or in QDMA Hardware IP from LA12xx A0 to
 * LA12xx B0. The number of QDMA blocks remains four. These APIs can be invoked
 * on four e200 cores core0/1/2/3, and behavior is undefined if application
 * invokes these APIs from 5th or 6th core.
 *
 * On LA12xx A0, the software driver affine core with QDMA block to avoid
 * software locks. For example, QDMA block 0 to core 0, QDMA block 1 to core 1,
 * QDMA block 2 to core 2, QDMA block 3 to core 3. This distribution is defined
 * by macro “NXP_QDMA_BLOCK_NUM” in include/qdma.h file in FreeRTOS Code.
 * The number of cores has now increased from four (in LA12xx A0) to six
 * (in LA12xx B0).
 *
 * On LA12xx B0, Since no change is expected in QDMA user application
 * (such as BBDEV IPC), the QDMA block distribution is kept same as La12xx A0
 * (viz QDMA block 0 to core 0, QDMA block 1 to core 1, QDMA block 2 to core 2,
 * QDMA block 3 to core 3). Applications need to make sure that QDMA is not
 * used on additional cores that are added in rev B0 (core 4 or core 5).
 * However, if you want to modify this QDMA block distribution, it can easily
 * be done by modifying the macro “NXP_QDMA_BLOCK_NUM” in include/qdma.h
 * file in FreeRTOS Code.
 *
 * Caveat: Since QDMA blocks are limited to four, therefore, applications
 * need to ensure that no two cores initialize same QDMA block. This caveat
 * is applicable for LA12xx A0 as well.
 *
 */

#define NXP_DEFAULT_QDMA_QUEUE 0
#define NXP_QDMA_BLOCK_NUM_MAX 4

/*
 * QDMA jobs from core0 can be submitted to block0
 * QDMA jobs from core1 can be submitted to block1
 * QDMA jobs from core2, core4 (for Geul_B0) can be submitted to block2
 * QDMA jobs from core3, core5 (for Geul_B0) can be submitted to block3
 */
#define NXP_QDMA_BLOCK_NUM(x) \
	((x < NXP_QDMA_BLOCK_NUM_MAX) ? (x) : (x - 2))


/**
 * Descriptor processing starts automatically when queue is enabled and is not empty.
 *
 */
#define NXP_QDMA_BCQMR_CQM_AUTO  0
/**
 *Descriptor processing starts when S/W trigger (TMS) is asserted.
 *
 */
#define NXP_QDMA_BCQMR_CQM_SW_TRIGGER  1
/**
 * Descriptor processing starts when external H/W command queue trigger is
 * asserted (also sets TM).
 *
 */
#define NXP_QDMA_BCQMR_CQM_HW_TRIGGER     2
/**
 * Descriptor processing starts when either S/W or H/W trigger is asserted.
 *
 */
#define NXP_QDMA_BCQMR_CQM_SW_HW_TRIGGER        3
/**
 * Maximum number of queues supported per block
 *
 */
#define NXP_QDMA_QUEUE_NUM_MAX                  8

/**
 * Number of QDMA queues for block 0, Maximum supported queues per block is 8.
 * Value is driven from flag QDMA_QUEUE_COUNT_BLOCK0 defined in CMakeLists.txt,
 * 0 Queue mean block is not in use.
 */
#define NXP_QDMA_BLK0_QUEUE				(QDMA_QUEUE_COUNT_BLOCK0 > NXP_QDMA_QUEUE_NUM_MAX ? NXP_QDMA_QUEUE_NUM_MAX : QDMA_QUEUE_COUNT_BLOCK0) 
#define NXP_QDMA_BLK0_IN_USE			(NXP_QDMA_BLK0_QUEUE > 0 ? 1 : 0)

/**
 * Number of QDMA queues for block 1, Maximum supported queues per block is 8.
 * Value is driven from flag QDMA_QUEUE_COUNT_BLOCK1 defined in CMakeLists.txt,
 * 0 Queue mean block is not in use.
 */
#define NXP_QDMA_BLK1_QUEUE				(QDMA_QUEUE_COUNT_BLOCK1 > NXP_QDMA_QUEUE_NUM_MAX ? NXP_QDMA_QUEUE_NUM_MAX : QDMA_QUEUE_COUNT_BLOCK1) 
#define NXP_QDMA_BLK1_IN_USE			(NXP_QDMA_BLK1_QUEUE > 0 ? 1 : 0)

/**
 * Number of QDMA queues for block 2, Maximum supported queues per block is 8.
 * Value is driven from flag QDMA_QUEUE_COUNT_BLOCK2 defined in CMakeLists.txt,
 * 0 Queue mean block is not in use.
 */
#define NXP_QDMA_BLK2_QUEUE				(QDMA_QUEUE_COUNT_BLOCK2 > NXP_QDMA_QUEUE_NUM_MAX ? NXP_QDMA_QUEUE_NUM_MAX : QDMA_QUEUE_COUNT_BLOCK2) 
#define NXP_QDMA_BLK2_IN_USE			(NXP_QDMA_BLK2_QUEUE > 0 ? 1 : 0)

/**
 * Number of QDMA queues for block 3, Maximum supported queues per block is 8.
 * Value is driven from flag QDMA_QUEUE_COUNT_BLOCK3 defined in CMakeLists.txt,
 * 0 Queue mean block is not in use.
 */
#define NXP_QDMA_BLK3_QUEUE				(QDMA_QUEUE_COUNT_BLOCK3 > NXP_QDMA_QUEUE_NUM_MAX ? NXP_QDMA_QUEUE_NUM_MAX : QDMA_QUEUE_COUNT_BLOCK3) 
#define NXP_QDMA_BLK3_IN_USE			(NXP_QDMA_BLK3_QUEUE > 0 ? 1 : 0)

/**
 * Total supported queues. sum of supported queues of all blocks.
 */
#define NXP_QDMA_TOTAL_QUEUE		(NXP_QDMA_BLK0_QUEUE + NXP_QDMA_BLK1_QUEUE + NXP_QDMA_BLK2_QUEUE + NXP_QDMA_BLK3_QUEUE)
/**
 * Total blocks in use. sum of number of blocks in use. Maximum value is 4.
 */
#define NXP_QDMA_TOTAL_BLK_IN_USE	(NXP_QDMA_BLK0_IN_USE + NXP_QDMA_BLK1_IN_USE + NXP_QDMA_BLK2_IN_USE + NXP_QDMA_BLK3_IN_USE)

/* Callback function prototype to be called on job completion */
typedef void (*DmaCallback)(void *DmaParam, uint32_t addr);

/* QDMA queue structure */
typedef struct NXP_QDMA_QUEUE
{
        DescriptorFormat_t      *DescHead;
        DescriptorFormat_t      *DescTail;
        DescriptorFormat_t      *Cq;
        uint8_t                 Id;
	NxpQdmaCLT_t		*NxpClt;
	uint8_t			index;
        void                    *Params;
        void            *BlockBase;
        DmaCallback             Callback;
} NxpQdmaQueue_t;

//#define HW_PERF                                 1

/**
 * QdmaInit(): Initializes QDMA engine for current core
 *
 * @param core_id: Pass value returned by ulMpicCurrentCore();
 *
 * @param cqm: Command Queue Mode
 * NXP_QDMA_BCQMR_CQM_AUTO/
 * NXP_QDMA_BCQMR_CQM_SW_TRIGGER/
 * NXP_QDMA_BCQMR_CQM_HW_TRIGGER/
 * NXP_QDMA_BCQMR_CQM_SW_HW_TRIGGER
 * Tested for NXP_QDMA_BCQMR_CQM_AUTO
 *
 * @param nCq: Number of Command queues needs to be initialized
 * for this core.
 * Maximum and recommended value is 8.
 * This impacts memory footprint.
 *
 * @param ag_val:  Pass information related to Arbitration Group(AG).
 * The ag_val value can differ on different qDMA block.
 * To disable AG, clear bit 31, pass value 0
 * To enable AG, set bit 31
 * Other bits are used to configure AG number for respective command queue.
 * and is used to set B0CQDSCR0..B3CQDSCR0 QDMA register of
 * respective qDMA block.
 * Different command queue can belong to same or different AG number,
 * AG number assignment for each CQ is done via 3 bits (which contains
 * AG number values, ranges from 0-7).
 * AG number assignment to CQ should follow below rules.
 * bit 31: '1' means AG enabled, 0 means disabled
 * bits 30-28: AG for CQ7, same or 1 higher than CQ6-AG
 * bits 26-24: AG for CQ6, same or 1 higher than CQ5-AG
 * bits 22-20: AG for CQ5, same or 1 higher than CQ4-AG
 * bits 18-16: AG for CQ4, same or 1 higher than CQ3-AG
 * bits 14-12: AG for CQ3, same or 1 higher than CQ2-AG
 * bits 10-8: AG for CQ2, same or 1 higher than CQ1-AG
 * bits 6-4: AG for CQ1, same or 1 higher than CQ0-AG
 * bits 2-0: AG for CQ0
 * For example:
 * a)To assign CQ0 to AG0, other queues to AG1,
 * use value 0x91111110
 * b)To assign CQ0 to AG0, CQ1 to AG1, other queues to AG2,
 * use value 0xa2222210
 * c)To assign all CQs to AG1, use value 0x91111111
 * d)To assign all CQs to AG2, use value 0xa2222222
 *
 * @param ag_engine: This argument is used to set DMA engine Arbitration
 * Group execution privileges. This argument is used to set DEAGAR0 QDMA register.
 * ag_engine value is used only when ag_val value is non-zero
 * bits 15-8: DMA_1_AG, 15-8 bits of DEAGAR0, set execution privilege
 * 		for DMA_Engine1 for arbitration group. One bit for
 * 		each AG starting from bit 8 to represent AG0
 * bits 7-0: DMA_0_AG, 7-0 bits of DEAGAR0, set execution privilege
 * 		for DMA_Engine0 for arbitration group. One bit for
 * 		each AG starting from bit 0 to represent AG0
 * value 0 means that execution is not allowed on the engine
 * value 1 means that execution is allowed on the engine
 *For example:
 * a)To allow AG0 to only execute on DMA_Engine0, others on either
 * DMA_Engine, use value: 0xfeff.
 * As AG0 is of highest priority on DMA_Engine0, there will be no context
 * switching of AG0 tasks in case AG0 task is active.
 * b)To allow AG0 to only execute on DMA_Engine0, AG1 only on DMA_Engine1,
 * others on either DMA_Engine, use value: 0xfefd
 * As AG0 is of highest priority on DMA_Engine0, there will be no context
 * switching of AG0 tasks in case AG0 task is active.
 * Also as AG1 is of highest priority on DMA_Engine1, there will be no
 * context switching of AG1 tasks in case AG1 task is active.
 *
 * @param ErrCb: Register error callback function to be called
 * in case of Error Interrupt. Valid only for core0
 * @param ErrParams: Register argument to be passed to CallBack
 * function
 * @return
 *   - On success, pdPASS
 *   - On failure, pdFAIL
 */

BaseType_t QdmaInit(uint32_t core_id, uint8_t cqm, uint32_t nCq,
		    uint32_t ag_val, uint16_t ag_engine, void *ErrCb,
		    void *ErrParams);

/**
 * QdmaInitNoIntr(): Initializes QDMA engine for current core.
 *		Does not call interrupt for completed jobs.
 *
 * @param core_id: Pass value returned by ulMpicCurrentCore();
 *
 * @param cqm: Command Queue Mode
 * NXP_QDMA_BCQMR_CQM_AUTO/
 * NXP_QDMA_BCQMR_CQM_SW_TRIGGER/
 * NXP_QDMA_BCQMR_CQM_HW_TRIGGER/
 * NXP_QDMA_BCQMR_CQM_SW_HW_TRIGGER
 * Tested for NXP_QDMA_BCQMR_CQM_AUTO
 *
 * @param nCq: Number of Command queues must be initialized
 * for this core.
 * MAX and Recommended value is 8.
 * This impacts memory footprint
 *
 * @param ag_val:  Pass information related to Arbitration Group(AG).
 * The ag_val value can differ on different qDMA block.
 * To disable AG, clear bit 31, pass value 0
 * To enable AG, set bit 31
 * Other bits are used to configure AG number for respective command queue.
 * and is used to set B0CQDSCR0..B3CQDSCR0 QDMA register of
 * respective qDMA block.
 * Different command queue can belong to same or different AG number,
 * AG number assignment for each CQ is done via 3bits (which contains
 * AG number values, ranges from 0-7).
 * AG number assignment to CQ should follow below rules.
 * bit 31: '1' means AG enabled, 0 means disabled
 * bits 30-28: AG for CQ7, same or 1 higher than CQ6-AG
 * bits 26-24: AG for CQ6, same or 1 higher than CQ5-AG
 * bits 22-20: AG for CQ5, same or 1 higher than CQ4-AG
 * bits 18-16: AG for CQ4, same or 1 higher than CQ3-AG
 * bits 14-12: AG for CQ3, same or 1 higher than CQ2-AG
 * bits 10-8: AG for CQ2, same or 1 higher than CQ1-AG
 * bits 6-4: AG for CQ1, same or 1 higher than CQ0-AG
 * bits 2-0: AG for CQ0
 * For example:
 * a)To assign CQ0 to AG0, CQ1 to AG1, other queues to AG2,
 * use value 0xa2222210
 * b)To assign CQ0 to AG0, other queues to AG1,
 * use value 0x91111110
 * c)To assign all CQs to AG1, use value 0x91111111
 * d)To assign all CQs to AG2, use value 0xa2222222
 *
 * @param ag_engine: This argument is used to set DMA engine Arbitration
 * Group execution privileges. This argument is used to set DEAGAR0 QDMA register.
 * ag_engine value is used only when ag_val value is non-zero
 * bits 15-8: DMA_1_AG, 15-8 bits of DEAGAR0, set execution privilege
 * 		for DMA_Engine1 for arbitration group. One bit for
 * 		each AG starting from bit 8 to represent AG0
 * bits 7-0: DMA_0_AG, 7-0 bits of DEAGAR0, set execution privilege
 * 		for DMA_Engine0 for arbitration group. One bit for
 * 		each AG starting from bit 0 to represent AG0
 * value 0 means that execution is not allowed on the engine
 * value 1 means that execution is allowed on the engine
 * For example:
 * a)To allow AG0 to only execute on DMA_Engine0, others on either
 * DMA_Engine, use value: 0xfeff.
 * As AG0 is of highest priority on DMA_Engine0, there will be no context
 * switching of AG0 tasks in case AG0 task is active.
 * b)To allow AG0 to only execute on DMA_Engine0, AG1 only on DMA_Engine1,
 * others on either DMA_Engine, use value: 0xfefd
 * As AG0 is of highest priority on DMA_Engine0, there will be no context
 * switching of AG0 tasks in case AG0 task is active.
 * Also as AG1 is of highest priority on DMA_Engine1, there will be no
 * context switching of AG1 tasks in case AG1 task is active.
 *
 * @param ErrCb: Register error callback function to be called
 * in case of Error Interrupt. Valid only for core0
 * @param ErrParams: Register argument to be passed to CallBack
 * function
 * @return
 *   - On success, pdPASS
 *   - On failure, pdFAIL
 */
BaseType_t QdmaInitNoIntr(uint32_t core_id, uint8_t cqm, uint32_t nCq,
		    uint32_t ag_val, uint16_t ag_engine, void *ErrCb,
		    void *ErrParams);

/*
 * Callback function prototype to be called on job completion
 */
typedef void (*Callback)(void *DmaParam, uint32_t addr);

/**
 * RegisterCQueueCallback(): Registers Callback for CommandQueue[cqn_id]
 * This callback function will be called on job completion
 * of descriptor of this particular queue. Can be useful when
 * user is waiting for job completion to proceed further.
 *
 * @param Callback: Pointer to callback function to be called
 * on completion of QDMA job.
 *
 * @param Params: Pointer to argument to be passed to Callback
 *		function
 *
 * @param cqn_id : Queue-ID(Possible values : 0-7)
 *
 * @return
 *   - On success, pdPASS
 *   - On failure, pdFAIL
 */

BaseType_t RegisterCQueueCallback(void *Callback, void *Params,
				  uint32_t cqn_id);
/**
 * DmaIssue(): Helper function to issues QDMA job descriptor
 * to command queue of QDMA block affined to current core.
 * User will have to prepare the QDMA job descriptor to enqueue.
 * It involves memory copy so can be used for functionality
 * verification purposes. Refer DmaUSFFillIssue() for optimized
 * API.
 *
 * @param *NxpDesc: Pointer to command descriptor which must be queued
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 *
 * @return
 *   - On success, 0
 *   - QDMA Engine not initialized, -ENODATA
 *   - Invalid Queue-ID, -EINVAL
 *   - Queue is full, -EAGAIN
 *
 */

int DmaIssue(DescriptorFormat_t *NxpDesc, uint32_t cqn_id);

/**
 * DmaIssueCore(): Helper function to issues QDMA job descriptor
 * to command queue of QDMA block affined to target core ID.
 * User will have to prepare the QDMA jobcdescriptor to enqueue.
 * It involves memory copy so can be used for functionality
 * verification purposes. Refer DmaUSFFillIssue() for optimized
 * API.
 *
 * @param *NxpDesc: Pointer to command descriptor which must be queued
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 *
 * @param tgt_core_id: Target Core ID
 *
 * @warning This is a non-standard use of QDMA. Caller should ensure that no race
 * condition occurs when issuing a QDMA job for the same queue of the same target core
 *
 * @return
 *   - On success, 0
 *   - QDMA Engine not initialized, -ENODATA
 *   - Invalid Queue-ID, -EINVAL
 *   - Queue is full, -EAGAIN
 *
 */

int DmaIssueCore(DescriptorFormat_t *NxpDesc, uint32_t cqn_id, uint8_t tgt_core_id);

/**
 * iDmaIssueCore(): same as DmaIssueCore() without error prints and with status return
 */

int iDmaIssueCore(DescriptorFormat_t *NxpDesc, uint32_t cqn_id, uint8_t tgt_core_id);

/**
 * DmaUSFFillIssue(): Helper APIs to prepare descriptors
 * Prepares command descriptor of USF (Ultra Short Format) type and issues
 * the job descriptor to command queue.
 * Doing both operations in single function optimize
 * the throughput as memcpy can now be avoided.
 *
 * @param Dst: Pointer to destination buffer
 *
 * @param Src: Pointer to source buffer
 *
 * @param Len: Length of buffer
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 *
 * @param rbp: Pass information related to rbp,
 * TODO: rbp bits details
 *
 * @return
 *   - On success, 0
 *   - QDMA Engine not initialized, -ENODATA
 *   - Invalid Queue-ID, -EINVAL
 *   - Queue is full, -EAGAIN
 *
 */

int DmaUSFFillIssue(BaseType_t Dst, BaseType_t Src, size_t Len,
		     uint32_t cqn_id, uint32_t rbp);
/**
 * QdmaCltSgCdDump(): Helper APIs that can be used to dump
 * structure of NxpQdmaCltCd_t type
 *
 * @param NxpCltSgCd:
 * Contains pointer to structure of NxpQdmaCltCd_t type
 *
 */

void
QdmaCltSgCdDump(NxpQdmaCltCd_t *NxpCltSgCd);
/**
 * QdmaSgEntriesDump(): Helper APIs that can be used to dump
 * structure of ScatterGatherTableFormat_t type *
 *
 * @param SgTableHead:
 * Contains pointer to array of Scatter Gather (SG) entries
 *
 * @param SgNum:
 * Contains count of SG entries in array
 *
 */

void
QdmaSgEntriesDump(ScatterGatherTableFormat_t *SgTableHead, uint32_t SgNum);
/**
 * QdmaFillSGEntries(): Helper APIS which can be used to create
 * array of SG entries
 *
 * @param NxpSG: Contains memory address of array where SG
 * entries  will be created
 *
 * @param *DataArr: The array contains pointer to SG buffers
 *
 * @param *LenArr: The array contains length of SG buffers
 *
 * @param SgNum: Number of SG entries.
 *
 *
 */

void
QdmaFillSGEntries(ScatterGatherTableFormat_t *NxpSG,
		  BaseType_t *DataArr, size_t *LenArr, uint32_t SgNum);
/**
 * QdmaFillCltLongCdSG(): Helper APIs that can be used to
 * create command descriptor and CLT for Long Scatter-Gather format type
 *
 * @param NxpCltSgCd: Contains memory address where Command Descriptor
 *                        will be created
 *
 * @param TotalDstLength: Total length of all Destination entries
 *
 * @param TotalSrcLength: Total length of all Source entries
 *
 * @param rbp: Pass information related to rbp,
 *          TODO : rbp bits details
 *
 * @return
 *   - On success, pdPASS
 *   - On failure, pdFAIL
 *
 */

BaseType_t
QdmaFillCltLongCdSG(NxpQdmaCltCd_t *NxpCltSgCd,
		    uint32_t TotalDstLength, uint32_t TotalSrcLength,
		    uint32_t rbp);
/**
 * QdmaInitCltLongCdSG(): Helper APIs that can be used to
 * initialize command descriptor and CLT for
 * long Scatter-Gather type with some constant values
 *
 * @param NxpCltSgCd_addr: Contains memory address to NxpQdmaCltCd_t type
 * structure which must be initialized.
 *
 * @return
 *   - On success, pdPASS
 *   - On failure, pdFAIL
 *
 */

BaseType_t
QdmaInitCltLongCdSG(BaseType_t NxpCltSgCd_addr);

/**
 * QdmaCltDstEnableStride(): Helper API that can be used to
 * enable stride with address-hold functionality in
 * destination buffer
 *
 * @param NxpClt: Contains memory address to NxpQdmaCLT_t type structure
 *
 * @param dst_addr: Address of destination buffer
 *
 * @param dLen: Total length of buffer that must be copied
 *              at destination
 * @param stride_size: Number of bytes that will be copied in one stride.
 * This must be in power of 2 to support address-hold
 * feature
 */

void QdmaCltDstEnableStride(NxpQdmaCLT_t *NxpClt, BaseType_t dst_addr,
			    size_t dLen, uint32_t stride_size);

/**
 * QdmaCltDst(): Helper API that can be used to
 * set Dest addr in Destination Cmd List Entry.
 *
 * @param NxpClt: Contains memory address to NxpQdmaCLT_t type structure
 *
 * @param dst_addr: Address of destination buffer
 *
 * @param dLen: Total length of buffer that must be copied
 *              at destination
 */

void QdmaCltDst(NxpQdmaCLT_t *NxpClt, BaseType_t dst_addr, size_t dLen);

/**
 * QdmaDescAddrSet(): Helper API to fill Address field in descriptor
 *
 * @param DescFmt: Contains memory address to descriptor
 *
 * @param Addr: Address value
 *
 */

void
QdmaDescAddrSet(DescriptorFormat_t *DescFmt, BaseType_t Addr);

/**
 * QdmaFillDstCltDesc(): Helper API that can be used to fill buffer
 * address and length in Destination Command List Table
 * Descriptor
 *
 * @param NxpClt: Contains memory address to NxpQdmaCLT_t type
 *  structure
 *
 * @param Dst: Address of destination buffer
 *
 * @param Len: Total length of buffer that must be copied
 * at destination
 *
 * @param Cfg: Destination address related configuration, such as
 * final entry, SG table or single buffer
 *
 */

void
QdmaFillDstCltDesc(NxpQdmaCLT_t *NxpClt, BaseType_t Dst, size_t Len,
		uint32_t Cfg);

/**
 * QdmaFillAllDesc(): Fill all QDMA descriptors in a Queue ID with a
 * given descriptor. Function will do the memcpy of given descriptor
 * to all HW descriptors of the queue.
 *
 * @param *NxpDesc: Pointer to command descriptor which is populated in
 * QDMA HW descriptors
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 *
 * @return
 *   - On success, 0
 *   - QDMA Engine not initialized, -ENODATA
 *   - Invalid Queue-ID, -EINVAL
 *
 */
int QdmaFillAllDesc(DescriptorFormat_t *NxpDesc, uint32_t cqn_id);

/**
 * QdmaFillDesc(): Filling the QDMA descriptors in the HW queue,
 * starting memory pointer of descriptors given by argument
 * NxpClt. Also fill default values of long format type in all
 * descriptors. Function will not use any memcpy.
 *
 * @param *NxpClt: starting pointer of descriptors to be attached
 * to each HW descriptor (CD) of the queue.
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 */
void QdmaFillDesc(NxpQdmaCLT_t *NxpClt, uint32_t cqn_id);

/**
  * QdmaFillCltDesc(): Helper API that can be used to fill buffer
 * address, length and rbp in descriptor along with some default
 * configuration.
 *
 * @param NxpClt: Contains memory address to NxpQdmaCLT_t type
 *  structure
 *
 * @param Dst: Address of destination buffer
 *
 * @param Len: Total length of buffer that must be copied
 * at destination
 *
 * @param Cfg: Destination address related configuration, such as
 * final entry, SG table or single buffer
 */
void QdmaFillCltDesc(NxpQdmaCLT_t *NxpClt, size_t dLen, size_t sLen,
                     uint32_t rbp);

/**
 * qDMA_get_clt_addr(): Return pointer of NxpQdmaQueue_t. Useful
 * to modify current descriptor(enable/disable striding, disable job
 * completion status etc.) of the queue before submitting the
 * QDMA job to QDMA HW block for process.
 *
 * @param QueueId: Queue-ID (Possible values: 0-7)
 *
 */
NxpQdmaQueue_t *qDMA_get_clt_addr(uint32_t QueueId);

/**
 * QdmaGetBlockReg(): Get Block CQMR register value with enable bit set
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 *
 * @return: Block CQMR register value with enable bit set
 */
uint32_t QdmaGetBlockCQMRReg(uint32_t cqn_id);

/**
 * QdmaSetBlockCQMRReg(): Set Block CQMR register value
 *
 * @param Reg: Register value to set
 *
 * @param cqn_id: Queue-ID (Possible values: 0-7)
 *
 */
void QdmaSetBlockCQMRReg(uint32_t Reg, uint32_t cqn_id);

/**
 * NxpCmdQueueEnqueueBbdev(): Enqueue command to QDMA on a particular Queue
 *
 * @param QueueId: Queue-ID (Possible values: 0-7)
 */
void NxpCmdQueueEnqueueExt(uint32_t QueueId);

/**
 * NxpQdmaProcessStatusQueue(): Process any pending jobs completions from the
 * status queue
 */
void NxpQdmaProcessStatusQueue(void);

/**
* vFillQdmaValidBlk(): Fill Valid QDMA Blocks Info
* Store QDMA Block is in use or not
* Store QDMA Block Queue Count
* Store qdma_command address for each valid qdma block
* Store qdma_status address for each valid qdma block
*/
void vFillQdmaValidBlk(void);
/** @} */
#endif /* SRC_QDMA_H_ */
