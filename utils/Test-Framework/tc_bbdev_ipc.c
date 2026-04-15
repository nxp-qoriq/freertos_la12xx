// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#include "FreeRTOS.h"
#include "tc_bbdev_ipc.h"
#include "bbdev_ipc.h"
#include "qdma.h"
#include "Time.h"
#include "timers.h"
#include "bit.h"
#include "feca_api.h"
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "semphr.h"

#define QDMA_CLT_F                              BIT(31)
#define QDMA_CLT_SG                             BIT(29)
#define QDMA_SGT_F                              BIT(31)

#define MAX_DST_SIZE				4096

/* Please enable "FECA_IPC_DEBUG 1" for debug prints. */
//#define FECA_IPC_DEBUG                         1

// Enable VSPA processing
//#define VSPA_ENABLE	1

#ifdef BBDEV_USE_SE_QDMA
#define FECA_SE_INPUT_MAX_SZ 0x30000
#else
#define FECA_SE_INPUT_MAX_SZ 0x18000
#endif
#define FECA_SE_OUTPUT_MAX_SZ 0xA000

#define FECA_SE_MULTI_INPUT_MAX_SZ 0x2000
#define FECA_SE_MULTI_OUTPUT_MAX_SZ 0x8000

#define FECA_SD_INPUT_MAX_SZ  0x10000
#define FECA_SD_OUTPUT_MAX_SZ 0x8000
#define FECA_SD_STATUS_MAX_SZ 0x0008

#define FECA_SD_MULTI_INPUT_MAX_SZ 0xA000
#define FECA_SD_MULTI_OUTPUT_MAX_SZ 0x1000

#define FECA_CD_INPUT_MAX_SZ  0x1000
#define FECA_CD_OUTPUT_MAX_SZ 0x1000
#define FECA_CD_STATUS_MAX_SZ 0x0100
#define FECA_CD_DEMUX_INPUT_MAX_SZ  0x2000

#define FECA_CE_INPUT_MAX_SZ  0x1000
#define FECA_CE_OUTPUT_MAX_SZ 0x1000

#define FECA_CD_NUM 1
#define FECA_CE_NUM 1
#define FECA_SD_NUM 8
#define FECA_SE_NUM 8
#define FECA_CD_DEMUX_NUM FECA_SD_NUM

/* This specifies job_type and blk_id combined size to skip
 * to get to FECA command.
 */
#define FECA_CMD_OFFSET	8

#define FECA_CD_ACK_INDEX \
	(FECA_JOB_CD_DCM_ACK - FECA_JOB_CD_DCM_ACK)
#define FECA_CD_CSI1_INDEX \
	(FECA_JOB_CD_DCM_CS1 - FECA_JOB_CD_DCM_ACK)
#define FECA_CD_CSI2_INDEX \
	(FECA_JOB_CD_DCM_CS2 - FECA_JOB_CD_DCM_ACK)

/* 3 UCI types - ACK, CSI1, CSI2 */
#define MAX_CD_TYPES 3

#define MAX_QUEUES_PER_CORE	8

#define NXP_QDMA_QUEUE_NUM	1

#define MAX_OP_RETRY	100000

#define MAX_CRC_STAT_BUF_SZ 40
struct sd_crc_sts_desc {
	uint8_t *crc_stat[MAX_CHANNEL_DEPTH];
	uint32_t crc_stat_idx;
};

struct sd_crc_sts_desc crc_desc[FECA_SD_NUM];

struct feca_res {
	dev_handle_t device;
	chain_handle_t se_chain;
	chain_handle_t sd_chain;
	chain_handle_t cd_chain;
	chain_handle_t ce_chain;
};

struct pending_jobs_t;
typedef int (*process_sd_pending_cb_t)(struct pending_jobs_t *pending_job,
		 int queue_index);

struct pending_jobs_t {
	struct bbdev_ipc_dequeue_op *deq_op;
	process_sd_pending_cb_t process_sd_pending_fn;
	uint32_t processing_len_left;
	uint32_t base_addr;
	uint32_t dst_offset;
	uint32_t data_to_read;
	uint32_t qdma_pending;
	uint32_t num_retries;
};

#define QDMA_QUEUE_SIZE 64
struct feca_res f_res __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
#if VSPA_ENABLE
/* For VSPA this is taken in hif and shared across cores, as VSPA interrupt for
 * job completion comes on any of the e200 core
 */
volatile int flag[BBDEV_IPC_MAX_QUEUES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
#else
/* dummy flag to reserve hif area which is used when VSPA is enabled */
volatile int dummy_flag[BBDEV_IPC_MAX_QUEUES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile int flag[MAX_QUEUES_PER_CORE];
#endif
int g_queue_ids[MAX_QUEUES_PER_CORE * BBDEV_IPC_MAX_CORES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

uint32_t queues_op_type[BBDEV_IPC_MAX_QUEUES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
NxpQdmaCLT_t common_qdma_descs[NXP_QDMA_BLOCK_NUM_MAX * NXP_QDMA_QUEUE_NUM][QDMA_QUEUE_SIZE] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
uint32_t g_input_circ_size[MAX_QUEUES_PER_CORE];

uint32_t use_feca_sd_single_qdma __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

/* Pending Jobs */
struct pending_jobs_t pending_jobs[MAX_QUEUES_PER_CORE];
uint32_t se_input_circ_size, se_output_circ_size;
uint32_t sd_input_circ_size, sd_output_circ_size;

volatile uint32_t feca_init_done __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile uint32_t feca_cmd_size[4] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile BaseType_t feca_sd_cmd_addr[FECA_SD_NUM] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile BaseType_t feca_se_cmd_addr[FECA_SE_NUM] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

volatile uint32_t bbdev_core_enabled[BBDEV_IPC_MAX_CORES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile uint32_t reset_sync_point1[BBDEV_IPC_MAX_CORES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile uint32_t reset_sync_point2[BBDEV_IPC_MAX_CORES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

BaseType_t feca_cd_demux_input_axi_addr_le[MAX_CD_TYPES][FECA_CD_DEMUX_NUM] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
BaseType_t feca_axi_addr_le[MAX_QUEUES_PER_CORE];

#if VSPA_ENABLE
/* VSPA related Global Variables */
AviHandle_t *p_avi_handle;
SemaphoreHandle_t vspa_semaphore = NULL;
static TaskHandle_t vspa_handler_task;
TickType_t vspa_ticks_to_wait = 10;
int vspa_msg_rcvd = 0;
#endif

#ifdef FECA_IPC_DEBUG
int error_printed = 0;
struct feca_circ_buf_regs feca_se_cb_regs;
struct feca_circ_buf_regs feca_sd_cb_regs;
struct feca_circ_buf_regs feca_ce_cb_regs;
struct feca_circ_buf_regs feca_cd_cb_regs;

void dump_se_cmd(feca_job_t *feca_job)
{
	se_command_t *se_cmd = &feca_job->command_chain_t.se_command_ch_obj;
	uint32_t i;
	size_t size = sizeof(se_command_t);
	uint8_t *p_se = (uint8_t *)se_cmd;
	
	/* To remove unused variable warnings */
	(void)p_se;

	log_info("SE Command address = %x\n\r", se_cmd);
	log_info("out_pad_bytes %d\n\r", se_cmd->se_cfg1.out_pad_bytes);
	log_info("num_code_blocks %d\n\r", se_cmd->se_cfg1.num_code_blocks);
	log_info("tb_24_bit_crc %d\n\r", se_cmd->se_cfg1.tb_24_bit_crc);
	log_info("complete_trig_en %d\n\r", se_cmd->se_cfg1.complete_trig_en);
	log_info("mod_order %d\n\r", se_cmd->se_cfg1.mod_order);
	log_info("control_data_mux %d\n\r", se_cmd->se_cfg1.control_data_mux);
	log_info("lifting_size %d\n\r", se_cmd->se_cfg1.lifting_size);
	log_info("base_graph2 %d\n\r", se_cmd->se_cfg1.base_graph2);
	log_info("set_index %d\n\r", se_cmd->se_cfg1.set_index);

	log_info("num_input_bytes %d\n\r", se_cmd->se_sizes1.num_input_bytes);
	log_info("e_floor_thresh %d\n\r", se_cmd->se_sizes1.e_floor_thresh);

	log_info("se_circ_buf %d\n\r", se_cmd->se_circ_buf);
	log_info("se_floor_num_output_bits %d\n\r", se_cmd->se_floor_num_output_bits);
	log_info("se_ceiling_num_output_bits %d\n\r", se_cmd->se_ceiling_num_output_bits);
	log_info("se_sc_x1_init %d\n\r", se_cmd->se_sc_x1_init);
	log_info("se_sc_x2_init %d\n\r", se_cmd->se_sc_x2_init);
	log_info("se_axi_in_addr_low %x\n\r", se_cmd->se_axi_in_addr_low);
	log_info("se_axi_in_addr_high %x\n\r", se_cmd->se_axi_in_addr_high);
	log_info("se_axi_in_num_bytes %d\n\r", se_cmd->se_axi_in_num_bytes);
	log_info("se_cb_mask0 %x\n\r", se_cmd->se_cb_mask[0]);
	log_info("se_cb_mask1 %x\n\r", se_cmd->se_cb_mask[1]);
	log_info("se_cb_mask2 %x\n\r", se_cmd->se_cb_mask[2]);
	log_info("se_cb_mask3 %x\n\r", se_cmd->se_cb_mask[3]);
	log_info("se_cb_mask4 %x\n\r", se_cmd->se_cb_mask[4]);
	log_info("se_cb_mask5 %x\n\r", se_cmd->se_cb_mask[5]);
	log_info("se_cb_mask6 %x\n\r", se_cmd->se_cb_mask[6]);
	log_info("se_cb_mask7 %x\n\r", se_cmd->se_cb_mask[7]);
	log_info("se_di_start_ofst_floor0 %d\n\r", se_cmd->se_di_start_ofst_floor[0]);
	log_info("se_di_start_ofst_floor1 %d\n\r", se_cmd->se_di_start_ofst_floor[1]);
	log_info("se_di_start_ofst_floor2 %d\n\r", se_cmd->se_di_start_ofst_floor[2]);
	log_info("se_di_start_ofst_floor3 %d\n\r", se_cmd->se_di_start_ofst_floor[3]);
	log_info("se_di_start_ofst_ceiling0 %d\n\r", se_cmd->se_di_start_ofst_ceiling[0]);
	log_info("se_di_start_ofst_ceiling1 %d\n\r", se_cmd->se_di_start_ofst_ceiling[1]);
	log_info("se_di_start_ofst_ceiling2 %d\n\r", se_cmd->se_di_start_ofst_ceiling[2]);
	log_info("se_di_start_ofst_ceiling3 %d\n\r", se_cmd->se_di_start_ofst_ceiling[3]);

	if (feca_job->job_type == FECA_JOB_SE_DCM) {
		size = sizeof(se_dcm_command_t);
		p_se = (uint8_t *)&feca_job->command_chain_t.se_dcm_command_ch_obj;
	}

	log_info("Complete Command Dump:\n\r");
	for (i = 0; i< size; i++) {
		if (i % 4 == 0)
			log_info("\r\n0x");
		log_info("%02x", p_se[i]);
	}
	log_info("\n\r");
}

void dump_sd_cmd(feca_job_t *feca_job)
{
	uint32_t i;
	size_t size = sizeof(sd_command_t);
	sd_command_t *sd_cmd = &feca_job->command_chain_t.sd_command_ch_obj;
	uint8_t *p_sd = (uint8_t *)sd_cmd;

	/* To remove unused variable warnings */
	(void)p_sd;

	log_info("SD Command address = %x\n\r", sd_cmd);
	log_info("set_index %d\n\r", sd_cmd->sd_cfg1.set_index);
	log_info("base_graph2 %d\n\r", sd_cmd->sd_cfg1.base_graph2);
	log_info("lifting_index %d\n\r", sd_cmd->sd_cfg1.lifting_index);
	log_info("one_code_block %d\n\r", sd_cmd->sd_cfg1.one_code_block);
	log_info("data_control_mux %d\n\r", sd_cmd->sd_cfg1.data_control_mux);
	log_info("tb_24_bit_crc %d\n\r", sd_cmd->sd_cfg1.tb_24_bit_crc);
	log_info("remove_tb_crc %d\n\r", sd_cmd->sd_cfg1.remove_tb_crc);
	log_info("min_num_iterations %d\n\r", sd_cmd->sd_cfg1.min_num_iterations);
	log_info("max_num_iterations %d\n\r", sd_cmd->sd_cfg1.max_num_iterations);

	log_info("mod_order %d\n\r", sd_cmd->sd_cfg2.mod_order);
	log_info("harq_en %d\n\r", sd_cmd->sd_cfg2.harq_en);
	log_info("complete_trig_en %d\n\r", sd_cmd->sd_cfg2.complete_trig_en);
	log_info("send_msi %d\n\r", sd_cmd->sd_cfg2.send_msi);

	log_info("num_output_bytes %d\n\r", sd_cmd->sd_sizes1.num_output_bytes);
	log_info("e_floor_thresh %d\n\r", sd_cmd->sd_sizes1.e_floor_thresh);

	log_info("bits_per_cb %d\n\r", sd_cmd->sd_sizes2.bits_per_cb);
	log_info("num_filler_bits %d\n\r", sd_cmd->sd_sizes2.num_filler_bits);

	log_info("sd_circ_buf %d\n\r", sd_cmd->sd_circ_buf);
	log_info("sd_hram_base %d\n\r", sd_cmd->sd_hram_base);
	log_info("sd_floor_num_input_bytes %d\n\r", sd_cmd->sd_floor_num_input_bytes);
	log_info("sd_ceiling_num_input_bytes %d\n\r", sd_cmd->sd_ceiling_num_input_bytes);
	log_info("sd_sc_x1_init %d\n\r", sd_cmd->sd_sc_x1_init);
	log_info("sd_sc_x2_init %d\n\r", sd_cmd->sd_sc_x2_init);
	log_info("sd_axi_data_addr_low %x\n\r", sd_cmd->sd_axi_data_addr_low);
	log_info("sd_axi_data_addr_high %x\n\r", sd_cmd->sd_axi_data_addr_high);
	log_info("sd_axi_data_num_bytes %d\n\r", sd_cmd->sd_axi_data_num_bytes);
	log_info("sd_axi_stat_addr_low %x\n\r", sd_cmd->sd_axi_stat_addr_low);
	log_info("sd_axi_stat_addr_high %x\n\r", sd_cmd->sd_axi_stat_addr_high);
	log_info("sd_cb_mask0 %x\n\r", sd_cmd->sd_cb_mask[0]);
	log_info("sd_cb_mask1 %x\n\r", sd_cmd->sd_cb_mask[1]);
	log_info("sd_cb_mask2 %x\n\r", sd_cmd->sd_cb_mask[2]);
	log_info("sd_cb_mask3 %x\n\r", sd_cmd->sd_cb_mask[3]);
	log_info("sd_cb_mask4 %x\n\r", sd_cmd->sd_cb_mask[4]);
	log_info("sd_cb_mask5 %x\n\r", sd_cmd->sd_cb_mask[5]);
	log_info("sd_cb_mask6 %x\n\r", sd_cmd->sd_cb_mask[6]);
	log_info("sd_cb_mask7 %x\n\r", sd_cmd->sd_cb_mask[7]);
	log_info("sd_di_start_ofst_floor0 %d\n\r", sd_cmd->sd_di_start_ofst_floor[0]);
	log_info("sd_di_start_ofst_floor1 %d\n\r", sd_cmd->sd_di_start_ofst_floor[1]);
	log_info("sd_di_start_ofst_floor2 %d\n\r", sd_cmd->sd_di_start_ofst_floor[2]);
	log_info("sd_di_start_ofst_floor3 %d\n\r", sd_cmd->sd_di_start_ofst_floor[3]);
	log_info("sd_di_start_ofst_ceiling0 %d\n\r", sd_cmd->sd_di_start_ofst_ceiling[0]);
	log_info("sd_di_start_ofst_ceiling1 %d\n\r", sd_cmd->sd_di_start_ofst_ceiling[1]);
	log_info("sd_di_start_ofst_ceiling2 %d\n\r", sd_cmd->sd_di_start_ofst_ceiling[2]);
	log_info("sd_di_start_ofst_ceiling3 %d\n\r", sd_cmd->sd_di_start_ofst_ceiling[3]);

	if (feca_job->job_type == FECA_JOB_SD_DCM) {
		size = sizeof(sd_dcm_command_t);
		p_sd = (uint8_t *)&feca_job->command_chain_t.sd_dcm_command_ch_obj;
	}

	log_info("Complete Command Dump:\n\r");
	for (i = 0; i< size; i++) {
		if (i % 4 == 0)
			log_info("\n\r0x");

		log_info("%02x", p_sd[i]);
	}
	log_info("\n\r");
}


void dump_cd_cmd(feca_job_t *feca_job)
{
	uint32_t i;
	size_t size = sizeof(cd_command_t);
	cd_command_t *cd_cmd = &feca_job->command_chain_t.cd_command_ch_obj;
	uint8_t *p_sd = (uint8_t *)cd_cmd;

	/* To remove unused variable warnings */
	(void)p_sd;

	log_info("cd Command address = %x\n\r", cd_cmd);
	log_info("K                   : %d\n\r", cd_cmd->cd_cfg1.K);
	log_info("output_deint_bypass : %d\n\r", cd_cmd->cd_cfg1.output_deint_bypass);
	log_info("input_deint_bypass  : %d\n\r", cd_cmd->cd_cfg1.input_deint_bypass);;
	log_info("complete_trig_en    : %d\n\r", cd_cmd->cd_cfg1.complete_trig_en);
	log_info("crc_type            : %d\n\r", cd_cmd->cd_cfg1.crc_type);
	log_info("pc_en               : %d\n\r", cd_cmd->cd_cfg1.pc_en);
	log_info("rm_mode             : %d\n\r", cd_cmd->cd_cfg1.rm_mode);
	log_info("pd_n                : %d\n\r", cd_cmd->cd_cfg1.pd_n);
	log_info("crc_rnti	      : %d\n\r", cd_cmd->cd_cfg2.crc_rnti);
	log_info("E		      : %d\n\r", cd_cmd->cd_cfg2.E);
	log_info("pc_index2	      : %d\n\r", cd_cmd->cd_pe_indices.pc_index2);
	log_info("pc_index1	      : %d\n\r", cd_cmd->cd_pe_indices.pc_index1);
	log_info("pc_index0	      : %d\n\r", cd_cmd->cd_pe_indices.pc_index0);
	log_info("cd_axi_data_addr_low  : 0x%08x\n\r", cd_cmd->cd_axi_data_addr_low);
	log_info("cd_axi_data_addr_high : 0x%08x\n\r", cd_cmd->cd_axi_data_addr_high);
	log_info("cd_axi_stat_addr_low  : 0x%08x\n\r", cd_cmd->cd_axi_stat_addr_low);
	log_info("cd_axi_stat_addr_high : 0x%08x\n\r", cd_cmd->cd_axi_stat_addr_high);

	log_info("cd_fz_lut		: \n\r");
	for (i = 0; i < 32; ) {
		log_info("0x%08x ", cd_cmd->cd_fz_lut[i++]);
		if (!(i % 4))
			log_info("\n\r");
	}

	log_info("Complete Command Dump:\n\r");
	for (i = 0; i< size; i++) {
		if (i % 4 == 0)
			log_info("\n\r0x");
		log_info("%02x", p_sd[i]);
	}
	log_info("\n\r");
}

void dump_ce_cmd(feca_job_t *feca_job)
{
	uint32_t i;
	size_t size = sizeof(ce_command_t);
	ce_command_t *ce_cmd = &feca_job->command_chain_t.ce_command_ch_obj;
	uint8_t *p_sd = (uint8_t *)ce_cmd;

	/* To remove unused variable warnings */
	(void)p_sd;

	log_info("ce Command address = %x\n\r", ce_cmd);
	log_info("K                   : %d\n\r", ce_cmd->ce_cfg1.K);
	log_info("output_int_bypass   : %d\n\r", ce_cmd->ce_cfg1.output_int_bypass);
	log_info("input_int_bypass    : %d\n\r", ce_cmd->ce_cfg1.input_int_bypass);;
	log_info("complete_trig_en    : %d\n\r", ce_cmd->ce_cfg1.complete_trig_en);
	log_info("crc_type            : %d\n\r", ce_cmd->ce_cfg1.crc_type);
	log_info("pc_en               : %d\n\r", ce_cmd->ce_cfg1.pc_en);
	log_info("rm_mode             : %d\n\r", ce_cmd->ce_cfg1.rm_mode);
	log_info("pe_n                : %d\n\r", ce_cmd->ce_cfg1.pe_n);
	log_info("dst_sel             : %d\n\r", ce_cmd->ce_cfg1.dst_sel);
	log_info("crc_rnti	      : %d\n\r", ce_cmd->ce_cfg2.crc_rnti);
	log_info("E		      : %d\n\r", ce_cmd->ce_cfg2.E);
	log_info("block_concat_en     : %d\n\r", ce_cmd->ce_cfg3.block_concat_en);
	log_info("out_pad_bytes       : %d\n\r", ce_cmd->ce_cfg3.out_pad_bytes);
	log_info("pc_index2	      : %d\n\r", ce_cmd->ce_pe_indices.pc_index2);
	log_info("pc_index1	      : %d\n\r", ce_cmd->ce_pe_indices.pc_index1);
	log_info("pc_index0	      : %d\n\r", ce_cmd->ce_pe_indices.pc_index0);
	log_info("ce_axi_addr_low     : 0x%08x\n\r", ce_cmd->ce_axi_addr_low);
	log_info("ce_axi_addr_high    : 0x%08x\n\r", ce_cmd->ce_axi_addr_high);

	log_info("ce_fz_lut		: \n\r");
	for (i = 0; i < 32; ) {
		log_info("0x%08x ", ce_cmd->ce_fz_lut[i++]);
		if (!(i % 4))
			log_info("\n\r");
	}

	log_info("Complete Command Dump:\n\r");
	for (i = 0; i< size; i++) {
		if (i % 4 == 0)
			log_info("\n\r0x");
		log_info("%02x", p_sd[i]);
	}
	log_info("\n\r");
}

void feca_job_dump(struct bbdev_ipc_dequeue_op *deq_op)
{
	uint32_t *feca_s, s_len, i, val;;
	feca_job_t local_feca_job;

	local_feca_job.job_type = deq_op->feca_job_type;
	local_feca_job.t_blk_id = deq_op->feca_blk_id;

	for (i = 0; i < sizeof(local_feca_job.command_chain_t); i += 4) {
		val = *((uint32_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET + i));
		*((uint32_t *)((uint8_t *)&local_feca_job.command_chain_t + i)) = SWAP_32(val);
	}

	feca_s = (uint32_t *)SWAP_32(deq_op->in_addr);
	s_len = SWAP_32(deq_op->in_len);
	log_info("Feca Block ID =%d\n\r", local_feca_job.t_blk_id);
	log_info("Input Source Data Length =%x\n\r", s_len);
	log_info("Printing input data at address = %x\n\r", feca_s);
	for (int i = 0; i< 30/*s_len*/; ) {
		if (i % 24 == 0)
			log_info("\n\r");
		log_info("0x%08x ", *feca_s);
		feca_s++;
		i = i + 4;
	}
	log_info("\n\r");
	log_info("**********************************\n");
	log_info("Printing command\n");

	if (local_feca_job.job_type == FECA_JOB_SE ||
		 local_feca_job.job_type == FECA_JOB_SE_DCM)
		dump_se_cmd(&local_feca_job);
	else if (local_feca_job.job_type == FECA_JOB_SD ||
		 local_feca_job.job_type == FECA_JOB_SD_DCM)
		dump_sd_cmd(&local_feca_job);
	else if (local_feca_job.job_type == FECA_JOB_CD ||
		 local_feca_job.job_type == FECA_JOB_CD_DCM_ACK ||
		 local_feca_job.job_type == FECA_JOB_CD_DCM_CS1 ||
		 local_feca_job.job_type == FECA_JOB_CD_DCM_CS2)
		dump_cd_cmd(&local_feca_job);
	else if (local_feca_job.job_type == FECA_JOB_CE ||
		 local_feca_job.job_type == FECA_JOB_CE_DCM)
		dump_ce_cmd(&local_feca_job);
}
#endif

#if VSPA_ENABLE
static bool_t get_vspa_core(uint32_t ul_irq, VspaCore_t *pe_vspa_core)
{
	ul_irq = ul_irq - INTERNAL_IRQ_OFFSET;

	switch(ul_irq) {
	case 66 :
	case 67 :
		*pe_vspa_core = VSPA_CORE_0;
		break;

	case 69 :
	case 70 :
		*pe_vspa_core = VSPA_CORE_1;
		break;

	case 72 :
	case 73 :
		*pe_vspa_core = VSPA_CORE_2;
		break;

	case 75 :
	case 76 :
		*pe_vspa_core = VSPA_CORE_3;
		break;

	case 78 :
	case 79 :
		*pe_vspa_core = VSPA_CORE_4;
		break;

	case 81 :
	case 82 :
		*pe_vspa_core = VSPA_CORE_5;
		break;

	case 84 :
	case 85 :
		*pe_vspa_core = VSPA_CORE_6;
		break;

	case 87 :
	case 88 :
		*pe_vspa_core = VSPA_CORE_7;
		break;

	default :
		log_isr( "%s : Invalid interrupt number :%u\n\r", __func__, ul_irq );
		return false;
	}

	return true;
}

bool_t irq_recv_msg_from_vspa(uint32_t ul_irq, void *pv_dev_data)
{
	VspaRegs_t *p_vspa_regs = NULL;
	VspaCore_t e_vspa_core;
	bool_t i_status = false;
	AviHandle_t *p_avi_handler = (AviHandle_t *)pv_dev_data;
	AviMboxData_t mbox;
	uint8_t *str;

	i_status = get_vspa_core(ul_irq, &e_vspa_core);
	if (false == i_status) {
		log_isr("%s : Invalid interrupt generated\n\r", __func__);
		return i_status;
	}

	p_vspa_regs = (VspaRegs_t *)VSPA_INST_BASE_ADDR(e_vspa_core);

	log_dbg("VSPA Status 0x%X\n\r", IN_32(&p_vspa_regs->ulVspaStatus));
	if (IN_32(&p_vspa_regs->ulVspaStatus) & E200_MBOX0_STATUS) {
		log_dbg("%s: VSPA : [%d] E200_MBOX0_STATUS, intr = %u\n\r", __func__, e_vspa_core, ul_irq );
		OUT_32(&p_vspa_regs->ulVspaStatus, E200_MBOX0_STATUS );
		i_status = true;
	}
	if (IN_32(&p_vspa_regs->ulVspaStatus) & VSPA_MBOX1_STATUS) {
		exGeulAviHostHandleMboxIrq(p_avi_handler, e_vspa_core, VSPA_MBOX_1, &mbox);
		log_dbg("%s : VSPA[%d]  VSPA_MBOX1_STATUS, intr = %u mbox.ulMsb = 0x%x mbox.ulLsb = 0x%x\n\r",
			__func__, e_vspa_core, ul_irq, mbox.ulMsb, mbox.ulLsb);
		str = (uint8_t *)(0xE1000000 | mbox.ulLsb);
		vspa_msg_rcvd++;
		PRINTF("\rVSPA[%d]:MBOX1:  %s \n\r", e_vspa_core, str);
		i_status = true;
	}
	if (IN_32(&p_vspa_regs->ulVspaStatus) & VSPA_MBOX0_STATUS) {
		/* SE completion is sent to mailbox 0 */
		exGeulAviHostHandleMboxIrq(p_avi_handler, e_vspa_core, VSPA_MBOX_0, &mbox);
		log_dbg("%s : VSPA[%d]  VSPA_MBOX0_STATUS, intr = %u mbox.ulMsb = 0x%x mbox.ulLsb = 0x%x\n\r",
			__func__, e_vspa_core, ul_irq, mbox.ulMsb, mbox.ulLsb);
		str = (uint8_t *)(0xE1000000 | mbox.ulLsb);
		vspa_msg_rcvd++;
		flag[mbox.ulLsb] = 1;
		i_status = true;
	} else {
		if (!vspa_msg_rcvd) {
			log_dbg( "%s ERR: Invalid VSPA Status\n\r", __func__);
			i_status = false;
		}
	}
	return i_status;
}

int init_vspa(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();
	uint32_t e_vspa_core;

	log_dbg("Task vLaunchVspaLogs is running\n\r");

	vspa_semaphore = xSemaphoreCreateBinary();
	if(vspa_semaphore)
		/* Keep the sema count as 1 initially */
		xSemaphoreGive(vspa_semaphore);
	vspa_handler_task = xTaskGetCurrentTaskHandle();

	/* Before proceeding further, first acquire sema */
	while(xSemaphoreTake(vspa_semaphore, vspa_ticks_to_wait) == pdFALSE)
		log_info("%s : Waiting for Semaphore... \n\r", __func__);

	/* Init AVI */
	p_avi_handle = pxGeulAviInit(vspa_handler_task);
	if (p_avi_handle == NULL) {
		log_err("ERR: %s: AVI Initialization is failed \n\r",__func__);
		return -1;
	}
	if (core_id == 0) {
		for (e_vspa_core = VSPA_CORE_0; e_vspa_core < VSPA_CORE_MAX;
		     e_vspa_core++) {
			exGeulRegisterVspaInterrupt(p_avi_handle, e_vspa_core,
				VSPA_MBOX_0, irq_recv_msg_from_vspa, p_avi_handle, VSPA_MBOX_RW, false );
			exGeulRegisterVspaInterrupt(p_avi_handle, e_vspa_core,
				VSPA_MBOX_1, irq_recv_msg_from_vspa, p_avi_handle, VSPA_MBOX_RW, false );
		}
	}

	log_dbg("%s: AVI Initialization done : %p\n\r",__func__, p_avi_handle);

	xSemaphoreGive(vspa_semaphore);

	return 0;
}

int send_msg_to_vspa(uint32_t msb, uint32_t lsb,
		     int mbox_id, uint32_t e_vspa_core)
{
	AviMboxData_t avi_mbox_send_data;
	int i_status;

	avi_mbox_send_data.ulMsb = msb;
	avi_mbox_send_data.ulLsb = lsb;
	if (mbox_id == 0)
		i_status = exGeulAviHostSendFastMboxToVspa(p_avi_handle,
							   e_vspa_core,
							   mbox_id,
							   avi_mbox_send_data);
	else
		i_status = exGeulAviHostSendSlowMboxToVspa(p_avi_handle,
							   e_vspa_core,
							   mbox_id,
							   avi_mbox_send_data);
	if (AVI_SUCCESS != i_status) {
		log_err("ERR: Fail to send MBox0 message i_status = %d\n\r",
			i_status);
		return -1;
	}

#ifdef FECA_IPC_DEBUG
	log_info("Sent MSG on VSPA Core%d MBox0 --> MSB 0x%X LSB 0x%X : i_status = %d\n\r",
		e_vspa_core, avi_mbox_send_data.ulMsb,
		avi_mbox_send_data.ulLsb, i_status);
#endif
	return 0;
}
#endif

#ifdef BBDEV_USE_SE_QDMA
BaseType_t g_feca_se_in_addr_swapped;
#endif

static void
init_qdma_encode_desc(struct feca_res *f_res,
		int tb_id, int queue_index, int ch_enc, u8 core_id)
{
	BaseType_t feca_out_addr;
	u8 blk_id;
#ifdef BBDEV_USE_SE_QDMA
	BaseType_t feca_in_addr;
#endif
	/* Initial one time QDMA desc configuration for SE/CE.
	 * All the descriptors are pre-filled. so that minimal
	 * QDMA parameters are requried to be set when doing I/O.
	 */
	blk_id = core_id * NXP_QDMA_QUEUE_NUM;
	for (int i = 0; i < QDMA_QUEUE_SIZE; i++) {
		QdmaFillCltDesc(&common_qdma_descs[blk_id + NXP_DEFAULT_QDMA_QUEUE][i], TX_TB_SG_TOTAL_SIZE, TX_TB_SG_TOTAL_SIZE, TX_TB_RBP);
		common_qdma_descs[blk_id + NXP_DEFAULT_QDMA_QUEUE][i].sCmdListTable.Cfg1 = 0;
		common_qdma_descs[blk_id + NXP_DEFAULT_QDMA_QUEUE][i].dCmdListTable.Cfg1 = SWAP_32(QDMA_CLT_F);
	}
	if (!ch_enc) {
		feca_out_addr = (BaseType_t)feca_get_ch_axi_addr(f_res->se_chain,
					feca_get_ch_id(f_res->se_chain,
					CH_OUT, tb_id));
#ifdef BBDEV_USE_SE_QDMA
		feca_in_addr = (BaseType_t)feca_get_ch_axi_addr(f_res->se_chain,
					feca_get_ch_id(f_res->se_chain,
					CH_IN, tb_id));
		g_feca_se_in_addr_swapped = SWAP_32(feca_in_addr);
#endif
	} else {
		feca_out_addr = (BaseType_t)feca_get_ch_axi_addr(f_res->ce_chain,
					feca_get_ch_id(f_res->ce_chain,
					CH_OUT, tb_id));
	}
	feca_axi_addr_le[queue_index] = SWAP_32(feca_out_addr);

	QdmaFillDesc(*(common_qdma_descs + (blk_id + NXP_DEFAULT_QDMA_QUEUE)), NXP_DEFAULT_QDMA_QUEUE);

#if VSPA_ENABLE
	/* Send feca address to VSPA */
	send_msg_to_vspa(feca_out_addr, 0, 0,
		g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]);
#endif
}

static void
init_qdma_decode_desc(struct feca_res *f_res,
		int tb_id, int queue_index, int ch_dec, u8 core_id)
{
	BaseType_t feca_in_addr;
	u8 blk_id;

	/* Initial one time QDMA desc configuration for SD/CD.
	 * All the descriptors are pre-filled. so that minimal
	 * QDMA parameters are requried to be set when doing I/O.
	 */
	blk_id = core_id * NXP_QDMA_QUEUE_NUM;
	for (int i = 0; i < QDMA_QUEUE_SIZE; i++) {
		QdmaFillCltDesc(&common_qdma_descs[blk_id + NXP_DEFAULT_QDMA_QUEUE][i], TX_TB_SG_TOTAL_SIZE, TX_TB_SG_TOTAL_SIZE, TX_TB_RBP);
		common_qdma_descs[blk_id + NXP_DEFAULT_QDMA_QUEUE][i].sCmdListTable.Cfg1 = 0;
		common_qdma_descs[blk_id + NXP_DEFAULT_QDMA_QUEUE][i].dCmdListTable.Cfg1 = SWAP_32(QDMA_CLT_F);
	}
	if (!ch_dec)
		feca_in_addr = (BaseType_t)feca_get_ch_axi_addr(f_res->sd_chain,
					feca_get_ch_id(f_res->sd_chain,
					CH_IN, tb_id));
	else
		feca_in_addr = (BaseType_t)feca_get_ch_axi_addr(f_res->cd_chain,
					feca_get_ch_id(f_res->cd_chain,
					CH_IN, tb_id));
	feca_axi_addr_le[queue_index] = SWAP_32(feca_in_addr);

	QdmaFillDesc(*(common_qdma_descs + (blk_id + NXP_DEFAULT_QDMA_QUEUE)), NXP_DEFAULT_QDMA_QUEUE);

#if VSPA_ENABLE
	/* Send feca address to VSPA */
	send_msg_to_vspa(feca_in_addr, 0, 0,
		g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]);
#endif
}

static int init_sd_crc_stat(int tb_id)
{
	BaseType_t stat;
	struct sd_crc_sts_desc *desc;
	int i;

	/* Allocate memory required for Maximum queue depth * Maximum SD channels */
	stat = (BaseType_t)pvGeulMalloc(MAX_CRC_STAT_BUF_SZ * MAX_CHANNEL_DEPTH);
	if (!stat) {
		log_info("Error: no memory\n");
		return -1;
	}

	desc = &crc_desc[tb_id];
	for (i = 0; i < MAX_CHANNEL_DEPTH; i++) {
		desc->crc_stat[i] = (uint8_t *)stat;
		desc->crc_stat[i] = (uint8_t *)SWAP_32((uint32_t)desc->crc_stat[i]);
		stat = stat + MAX_CRC_STAT_BUF_SZ;
		desc->crc_stat_idx = 0;
	}

	return 0;
}

static int
init_feca_se(struct feca_res *f_res, int tb_id, uint32_t input_circ_size)
{
	ch_param_t ch_param;
	ch_handle_t channel;

	ch_param.ch_size	= sizeof(se_dcm_command_t);
	ch_param.tb_num		= tb_id;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD;
	ch_param.xfer_size	= 0;
	channel = feca_ch_open(f_res->se_chain, ch_param);
	if (!channel) {
		log_info("SE CMD channel open failed");
		return -1;
	}

	ch_param.ch_size	= input_circ_size;
	ch_param.tb_num		= tb_id;
#ifdef BBDEV_USE_SE_QDMA
	ch_param.dma_disable	= 1;
	ch_param.ext_dma	= 1;
	ch_param.xfer_size	= 0;
#else
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.xfer_size	= 0x1000;
#endif
	ch_param.type		= CH_IN;
	channel = feca_ch_open(f_res->se_chain, ch_param);
	if (!channel) {
		log_info("SE CH_IN channel open failed");
		return -1;
	}

	ch_param.ch_size	= se_output_circ_size;
	ch_param.tb_num		= tb_id;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_OUT;
	ch_param.xfer_size	= 0x0;
	channel = feca_ch_open(f_res->se_chain, ch_param);
	if (!channel) {
		log_info("SE CH_OUT channel open failed");
		return -1;
	}

	return 0;
}

static int
init_feca_sd(struct feca_res *f_res, int tb_id, uint32_t input_circ_size)
{
	ch_param_t ch_param;
	ch_handle_t channel;

	ch_param.ch_size	= sizeof(sd_dcm_command_t);
	ch_param.tb_num		= tb_id;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD;
	ch_param.xfer_size	= 0;
	channel = feca_ch_open(f_res->sd_chain, ch_param);
	if (!channel) {
		log_info("SD CH_CMD channel open failed");
		return -1;
	}

	ch_param.ch_size	= input_circ_size;
	ch_param.tb_num		= tb_id;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 1;
	ch_param.type		= CH_IN;
	ch_param.xfer_size	= 0;
	channel = feca_ch_open(f_res->sd_chain, ch_param);
	if (!channel) {
		log_info("SD CH_IN channel open failed");
		return -1;
	}

	ch_param.ch_size	= sd_output_circ_size;
	ch_param.tb_num		= tb_id;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_OUT;
	ch_param.xfer_size	= 0x1000;
	channel = feca_ch_open(f_res->sd_chain, ch_param);
	if (!channel) {
		log_info("SD CH_OUT channel open failed");
		return -1;
	}

	ch_param.ch_size	= FECA_SD_STATUS_MAX_SZ;
	ch_param.tb_num		= tb_id;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CRC_OUT;
	ch_param.xfer_size	= 0x40;
	channel = feca_ch_open(f_res->sd_chain, ch_param);
	if (!channel) {
		log_info("SD CH_CRC_OUT channel open failed");
		return -1;
	}

	return 0;
}

static int init_feca_ce(struct feca_res *f_res)
{
	ch_param_t ch_param;
	ch_handle_t channel;

	/* Open all 3 channel of CE */
	ch_param.ch_size	= sizeof(ce_command_t);
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->ce_chain, ch_param);
	if (!channel) {
		log_info("CE CH_CMD channel open failed");
		return -1;
	}

	ch_param.ch_size	= FECA_CE_INPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_IN;
	ch_param.xfer_size	= 0x100;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->ce_chain, ch_param);
	if (!channel) {
		log_info("CE CH_IN channel open failed");
		return -1;
	}

	ch_param.ch_size	= FECA_CE_OUTPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_OUT;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->ce_chain, ch_param);
	if (!channel) {
		log_info("CE CH_OUT channel open failed");
		return -1;
	}

	return 0;
}

static int init_feca_cd(struct feca_res *f_res)
{
	ch_param_t ch_param;
	ch_handle_t channel;
	int j;

	/* Open all 4 channel of CD */
	ch_param.ch_size	= sizeof(cd_command_t);
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->cd_chain, ch_param);
	if (!channel) {
		log_info("CD CH_CMD channel open failed");
		return -1;
	}

	ch_param.ch_size	= FECA_CD_INPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_IN;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->cd_chain, ch_param);
	if (!channel) {
		log_info("CD CH_IN channel open failed");
		return -1;
	}

	ch_param.ch_size	= FECA_CD_OUTPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_OUT;
	ch_param.xfer_size	= 0x100;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->cd_chain, ch_param);
	if (!channel) {
		log_info("CD CH_OUT channel open failed");
		return -1;
	}

	ch_param.ch_size	= FECA_CD_STATUS_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CRC_OUT;
	ch_param.xfer_size	= 0x8;
	ch_param.tb_num		= 0;
	channel = feca_ch_open(f_res->cd_chain, ch_param);
	if (!channel) {
		log_info("CD CH_CRC_OUT channel open failed");
		return -1;
	}

	for (j = TB_0; j < TB_1; j++) {
		ch_param.ch_size	= FECA_CD_DEMUX_INPUT_MAX_SZ;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_CMD_DCM_ACK;
		ch_param.xfer_size	= 0;
		ch_param.tb_num		= j;
		channel = feca_ch_open(f_res->cd_chain, ch_param);
		if (!channel) {
			log_info("CH_CMD_DCM_ACK channel open failed");
			return -1;
		}
		feca_cd_demux_input_axi_addr_le[FECA_CD_ACK_INDEX][j] =
				SWAP_32((BaseType_t)feca_get_ch_axi_addr(f_res->cd_chain,
						feca_get_ch_id(f_res->cd_chain,
						CH_IN_DCM_ACK, j)));

		ch_param.ch_size	= FECA_CD_DEMUX_INPUT_MAX_SZ;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_CMD_DCM_CSI1;
		ch_param.xfer_size	= 0;
		ch_param.tb_num		= j;
		channel = feca_ch_open(f_res->cd_chain, ch_param);
		if (!channel) {
			log_info("CH_CMD_DCM_CSI1 channel open failed");
			return -1;
		}
		feca_cd_demux_input_axi_addr_le[FECA_CD_CSI1_INDEX][j] =
				SWAP_32((BaseType_t)feca_get_ch_axi_addr(f_res->cd_chain,
						feca_get_ch_id(f_res->cd_chain,
						CH_IN_DCM_CSI1, j)));

		ch_param.ch_size	= FECA_CD_DEMUX_INPUT_MAX_SZ;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_CMD_DCM_CSI2;
		ch_param.xfer_size	= 0;
		ch_param.tb_num		= j;
		channel = feca_ch_open(f_res->cd_chain, ch_param);
		if (!channel) {
			log_info("CH_CMD_DCM_CSI2 channel open failed");
			return -1;
		}
		feca_cd_demux_input_axi_addr_le[FECA_CD_CSI2_INDEX][j] =
				SWAP_32((BaseType_t)feca_get_ch_axi_addr(f_res->cd_chain,
						feca_get_ch_id(f_res->cd_chain,
						CH_IN_DCM_CSI2, j)));
	}

	return 0;
}

static int
init_feca_chains(struct feca_res *f_res, int init_core)
{
	u8 core_id = (u8)ulMpicCurrentCore();
	chain_param_t chain_param;

	if (core_id != init_core)
		return 0;

	/* Open FECA device */
	f_res->device = feca_dev_open("FECA_5G", NULL);
	if (!f_res->device) {
		log_info("FECA open failed \n");
		return -1;
	}

	/* Open SE chain */
	chain_param.type = FECA_SE_CHAIN;
	chain_param.irq_mask = 0;
	chain_param.dcm_irq_mask = 0;
	f_res->se_chain = feca_chain_open(f_res->device, chain_param, NULL);
	if (!f_res->se_chain) {
		log_info("Shared encoding chain open failed \n");
		return -1;
	}

	/* Open SD chain */
	chain_param.type = FECA_SD_CHAIN;
	chain_param.irq_mask = 0;
	chain_param.dcm_irq_mask = 0;
	f_res->sd_chain = feca_chain_open(f_res->device, chain_param, NULL);
	if (!f_res->sd_chain) {
		log_info("Shared decoding chain open failed \n");
		return -1;
	}

	/* Open CE chain */
	chain_param.type = FECA_CE_CHAIN;
	chain_param.irq_mask = 0;
	chain_param.dcm_irq_mask = 0;
	f_res->ce_chain = feca_chain_open(f_res->device, chain_param, NULL);
	if (!f_res->ce_chain) {
		log_info("Control encode chain open failed \n");
		return -1;
	}

	/* Open CD chain */
	chain_param.type = FECA_CD_CHAIN;
	chain_param.irq_mask = 0;
	chain_param.dcm_irq_mask = 0;
	f_res->cd_chain = feca_chain_open(f_res->device, chain_param, NULL);
	if (!f_res->cd_chain) {
		log_info("Control decoding chain open failed \n");
		return -1;
	}

	return 0;
}

static int
init_feca_channels(struct feca_res *f_res, int init_core)
{
	u8 core_id = (u8)ulMpicCurrentCore();
	struct dev_attr_t *attr;
	uint32_t feca_blk_id;
	uint32_t input_circ_size;
	int ret, i;

	if (core_id != init_core)
		return 0;

	attr = bbdev_ipc_get_dev_attr(BBDEV_IPC_DEV_ID_0);
	for (i = 0; i < attr->num_queues; i++) {
		input_circ_size = attr->qattr[i].feca_input_circ_size;
		feca_blk_id = attr->qattr[i].feca_blk_id;

		if (attr->qattr[i].op_type == BBDEV_IPC_OP_LDPC_ENC) {
			if (!input_circ_size)
				input_circ_size = se_input_circ_size;
			ret = init_feca_se(f_res, feca_blk_id,
					   input_circ_size);
			if (ret) {
				log_err("init_feca_se failed for tb_id: %d\n", i);
				return -1;
			}
			feca_se_cmd_addr[feca_blk_id] = SWAP_32((BaseType_t)feca_get_ch_axi_addr(f_res->se_chain,
						feca_get_ch_id(f_res->se_chain, CH_CMD, feca_blk_id)));
		}

		if (attr->qattr[i].op_type == BBDEV_IPC_OP_LDPC_DEC) {
			if (!input_circ_size)
				input_circ_size = sd_input_circ_size;
			ret = init_feca_sd(f_res, feca_blk_id,
					   input_circ_size);
			if (ret) {
				log_err("init_feca_sd failed for tb_id: %d\n", i);
				return -1;
			}
			feca_sd_cmd_addr[feca_blk_id] = SWAP_32((BaseType_t)feca_get_ch_axi_addr(f_res->sd_chain,
						feca_get_ch_id(f_res->sd_chain, CH_CMD, feca_blk_id)));
		}

		if (attr->qattr[i].op_type == BBDEV_IPC_OP_POLAR_ENC) {
			ret = init_feca_ce(f_res);
			if (ret) {
				log_err("init_feca_ce failed\n");
				return -1;
			}
		}

		if (attr->qattr[i].op_type == BBDEV_IPC_OP_POLAR_DEC) {
			ret = init_feca_cd(f_res);
			if (ret) {
				log_err("init_feca_cd failed\n");
				return -1;
			}
		}
	}

	feca_cmd_size[0] = SWAP_32(sizeof(se_command_t));
	feca_cmd_size[1] = SWAP_32(sizeof(se_dcm_command_t));
	feca_cmd_size[2] = SWAP_32(sizeof(sd_command_t));
	feca_cmd_size[3] = SWAP_32(sizeof(sd_dcm_command_t));
	feca_init_done = 1;

	return 0;
}

static void qdma_completion_callback(void *param,
		__attribute__((unused))uint32_t clt_addr)
{
	flag[(int)param] = 1;
}

static void
dma_harq_to_host(struct bbdev_ipc_dequeue_op *deq_op,
		 struct bbdev_ipc_enqueue_op *enq_op, int queue_index,
		 uint32_t start_cb_idx, uint32_t end_cb_idx,
		 uint32_t harq_size_per_cb, int wait)
{
	BaseType_t harq_addr;
	uint32_t harq_out_addr;
	uint32_t len_to_copy, swap_len_to_copy;
	feca_job_t *feca_job;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;
	UNUSED(queue_index);
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;

	PRINTF("[%s]: queue_index: %d, dma_harq_to_host: %d, end_cb_idx: %d, harq_size_per_cb: %d\n",
		__func__, queue_index, start_cb_idx, end_cb_idx, harq_size_per_cb);
#endif

	feca_job = (feca_job_t *)deq_op->feca_job_addr;
	harq_addr = (BaseType_t)feca_get_harq_axi_addr(f_res.sd_chain) +
		SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_hram_base);

	harq_addr += start_cb_idx * harq_size_per_cb;
	harq_out_addr = deq_op->harq_out_addr + enq_op->out_len;
	len_to_copy = (end_cb_idx - start_cb_idx + 1) * harq_size_per_cb;
	swap_len_to_copy = SWAP_32(len_to_copy);
	enq_op->out_len += len_to_copy;

	/* Populate QDMA Descriptor */
	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	if (wait) {
		flag[queue_index] = 0;
		queue->DescHead[queue->index].Cfg2 |= 0x10000;
		queue->DescHead[queue->index].Cfg2 &= 0x8FFFFFFF;
		queue->DescHead[queue->index].Cfg2 |= (queue_index << 28);
	} else {
		queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	}
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->DstDescFmt.StrideWay = 0;
	NxpClt->DstDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->sCmdListTable.LowAddrBase = SWAP_32(harq_addr);
	NxpClt->sCmdListTable.DataLen = swap_len_to_copy;
	NxpClt->dCmdListTable.LowAddrBase = SWAP_32(harq_out_addr);
	NxpClt->dCmdListTable.DataLen = swap_len_to_copy;

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);

	if (wait) {
		while (!flag[queue_index])
			NxpQdmaProcessStatusQueue();
		flag[queue_index] = 0;
	}
}

static void
dma_harq_from_host_partial(struct bbdev_ipc_dequeue_op *deq_op,
		int queue_index, uint32_t start_cb_idx, uint32_t end_cb_idx,
		uint32_t harq_index, uint32_t harq_size_per_cb,
		int wait)
{
	BaseType_t harq_addr;
	uint32_t len_to_copy, swap_len_to_copy;
	uint32_t harq_in_addr;
	feca_job_t *feca_job;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;

#ifdef FECA_IPC_DEBUG
	PRINTF("[%s]: queue_index: %d, start_cb_idx: %d, end_cb_idx: %d, "
		"harq_index: %d, harq_size_per_cb: %d\n", __func__, queue_index,
		start_cb_idx, end_cb_idx, harq_index, harq_size_per_cb);
#endif

	feca_job = (feca_job_t *)deq_op->feca_job_addr;
	harq_addr = (BaseType_t)feca_get_harq_axi_addr(f_res.sd_chain) +
		SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_hram_base) +
		start_cb_idx * harq_size_per_cb;
	harq_in_addr = SWAP_32(deq_op->harq_in_addr) +
		harq_index * harq_size_per_cb;

	len_to_copy = (end_cb_idx - start_cb_idx + 1) * harq_size_per_cb;
	swap_len_to_copy = SWAP_32(len_to_copy);

	/* Populate QDMA Descriptor */
	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	if (wait) {
		/* Unset flag and start QDMA */
		flag[queue_index] = 0;
		queue->DescHead[queue->index].Cfg2 |= 0x10000;
		/* Reset and fill queue */
		queue->DescHead[queue->index].Cfg2 &= 0x8FFFFFFF;
		queue->DescHead[queue->index].Cfg2 |= (queue_index << 28);
	} else {
		queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	}
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->DstDescFmt.StrideWay = 0;
	NxpClt->DstDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->sCmdListTable.LowAddrBase = SWAP_32(harq_in_addr);
	NxpClt->sCmdListTable.DataLen = swap_len_to_copy;
	NxpClt->dCmdListTable.LowAddrBase = SWAP_32(harq_addr);
	NxpClt->dCmdListTable.DataLen = swap_len_to_copy;


	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);

	if (wait) {
		/* Wait until QDMA operation completes */
		while (!flag[queue_index])
			NxpQdmaProcessStatusQueue();
		flag[queue_index] = 0;
	}
}

static void
dma_harq_from_host_complete(struct bbdev_ipc_dequeue_op *deq_op,
		int queue_index)
{
	BaseType_t harq_addr;
	feca_job_t *feca_job;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;

#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;

	PRINTF("[%s]: queue_index: %d, len: %d\n",
		__func__, queue_index, SWAP_32(deq_op->harq_in_len));
#endif

	feca_job = (feca_job_t *)deq_op->feca_job_addr;
	harq_addr = (BaseType_t)feca_get_harq_axi_addr(f_res.sd_chain) +
		SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_hram_base);

	/* Populate QDMA Descriptor */
	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 |= 0x10000;
	/* Reset and fill queue */
	queue->DescHead[queue->index].Cfg2 &= 0x8FFFFFFF;
	queue->DescHead[queue->index].Cfg2 |= (queue_index << 28);
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->DstDescFmt.StrideWay = 0;
	NxpClt->DstDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->sCmdListTable.LowAddrBase = deq_op->harq_in_addr;
	NxpClt->sCmdListTable.DataLen = deq_op->harq_in_len;
	NxpClt->dCmdListTable.LowAddrBase = SWAP_32(harq_addr);
	NxpClt->dCmdListTable.DataLen = deq_op->harq_in_len;

	/* start QDMA */
	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);

	/* Wait until QDMA operation completes */
	while (!flag[queue_index]) {
#ifdef FECA_IPC_DEBUG
		if (retries == MAX_OP_RETRY * 100) {
			PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
			PRINTF("Job dump:\n\r");
			feca_job_dump(deq_op);
			PRINTF("Start FECA regs dump:\n\r");
			feca_print_cb_reg(&feca_se_cb_regs);
			PRINTF("Current FECA regs dump:\n\r");
			feca_dump_cb_reg(f_res.se_chain, deq_op->feca_blk_id,
					deq_op->feca_job_type);
			error_printed = 1;
		}
		retries++;
#endif
		NxpQdmaProcessStatusQueue();
	}
	flag[queue_index] = 0;
}

static void
process_out_harq(struct bbdev_ipc_dequeue_op *deq_op,
		 struct bbdev_ipc_enqueue_op *enq_op,
		 int queue_index)
{
	uint8_t *crc_stat, *harq_mask;
	uint8_t complete_harq_dma = 1;
	int num_code_blocks;
	int byte_index, bit_index, i;
	uint32_t harq_size_per_cb;
	int start_index = -1, end_index = -1;
	uint32_t max_num_harq_contexts;
	uint32_t num_harq_contexts = 0;
	feca_job_t *feca_job;
	int k, last_harq_index = -1;

	feca_job = (feca_job_t *)deq_op->feca_job_addr;
	crc_stat = (uint8_t *)SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_axi_stat_addr_low);
	num_code_blocks = deq_op->num_code_blocks;
	enq_op->out_len = 0;
	harq_size_per_cb = deq_op->harq_len_per_cb;
	max_num_harq_contexts = deq_op->max_num_harq_contexts;
	byte_index = 0;
	bit_index = 0;

	if (!max_num_harq_contexts)
		max_num_harq_contexts = UINT32_MAX;

	if (deq_op->op_flags & BBDEV_LDPC_COMPACT_HARQ ||
	    deq_op->op_flags & BBDEV_LDPC_PARTIAL_COMPACT_HARQ)
		complete_harq_dma = 0;

	harq_mask = (uint8_t *)deq_op->harq_mask;

	/* Finding last harq cb index to wait for QDMA completion */
	for (k = 0; k < num_code_blocks; k++) {
		if (((deq_op->op_flags & BBDEV_LDPC_COMPACT_HARQ) &&
			((crc_stat[byte_index] & (1 << bit_index)) == 0)) ||
			((deq_op->op_flags & BBDEV_LDPC_PARTIAL_COMPACT_HARQ) &&
			((crc_stat[byte_index] & (1 << bit_index)) == 0) &&
			((harq_mask[byte_index] & (1 << bit_index)) != 0))) {
				last_harq_index = k;
		}
		bit_index++;
               if (bit_index == 8) {
			byte_index++;
			bit_index = 0;
                }
	}
	byte_index = 0;
	bit_index = 0;

	/* Check all code blocks for CRC */
	for (i = 0; i < num_code_blocks; i++) {
		/* In case non-compact HARQ is used DMA complete HARQ Context */
		if (complete_harq_dma &&
		    ((crc_stat[byte_index] & (1 << bit_index)) == 0)) {
				start_index = 0;
				goto dma_all_harq;
		}

		if (((deq_op->op_flags & BBDEV_LDPC_COMPACT_HARQ) &&
		    ((crc_stat[byte_index] & (1 << bit_index)) == 0)) ||
		    ((deq_op->op_flags & BBDEV_LDPC_PARTIAL_COMPACT_HARQ) &&
		    ((crc_stat[byte_index] & (1 << bit_index)) == 0) &&
		    ((harq_mask[byte_index] & (1 << bit_index)) != 0))) {
			if (start_index == -1)
				start_index = i;
			end_index = i;
			num_harq_contexts++;
		}

		/* In case number of HARQ context to copy limit is reached,
		 * dma the HARQ context and break.
		 */
		if (num_harq_contexts == max_num_harq_contexts) {
			dma_harq_to_host(deq_op, enq_op, queue_index,
				start_index, end_index,
				harq_size_per_cb, 1);
			start_index = -1;
			break;
		}

		/* In case CRC for this iteration passed, do the DMA */
		if ((start_index != -1) && (end_index != i)) {
			if (last_harq_index == end_index) {
				dma_harq_to_host(deq_op, enq_op, queue_index,
					start_index, end_index, harq_size_per_cb, 1);
				start_index = -1;
				break;
			} else {
				dma_harq_to_host(deq_op, enq_op, queue_index,
					start_index, end_index, harq_size_per_cb, 0);
			}
			start_index = -1;
		}

		/* Update bit and byte index */
		bit_index++;
		if (bit_index == 8) {
			byte_index++;
			bit_index = 0;
		}
	}
dma_all_harq:
	if (start_index != -1)
		dma_harq_to_host(deq_op, enq_op, queue_index, start_index,
			num_code_blocks - 1, harq_size_per_cb, 1);

	deq_op->max_num_harq_contexts = 0;
	deq_op->harq_in_addr = 0;
	deq_op->harq_out_addr = 0;
}

static void
process_in_harq(struct bbdev_ipc_dequeue_op *deq_op,
		int queue_index)
{
	uint8_t *harq_mask;
	uint32_t num_harq_contexts = 0, harq_index = 0;
	int num_code_blocks, byte_index, bit_index, i;
	uint32_t harq_size_per_cb;
	int start_index = -1, end_index = -1;
	int k, last_harq_index = -1;

	if (!(deq_op->op_flags & BBDEV_LDPC_PARTIAL_COMPACT_HARQ)) {
		dma_harq_from_host_complete(deq_op, queue_index);
		return;
	}

	harq_mask = (uint8_t *)deq_op->harq_mask;
	num_code_blocks = deq_op->num_code_blocks;
	harq_size_per_cb = deq_op->harq_len_per_cb;
	byte_index = 0;
	bit_index = 0;

	/* Finding last harq cb index to wait for QDMA completion
	 * Note: This logic can be improved further */
	for (k = 0; k < num_code_blocks; k++) {
		if ((harq_mask[byte_index] & (1 << bit_index)) != 0) {
			last_harq_index = k;
		}
		bit_index++;
                if (bit_index == 8) {
                        byte_index++;
                        bit_index = 0;
                }
	}
	byte_index = 0;
	bit_index = 0;

	/* Check all code blocks */
	for (i = 0; i < num_code_blocks; i++) {
		if ((harq_mask[byte_index] & (1 << bit_index)) != 0) {
			if (start_index == -1)
				start_index = i;
			end_index = i;
			num_harq_contexts++;
		}

		if ((start_index != -1) && ((end_index != i) ||
		    (end_index == (num_code_blocks - 1)))) {
			if (end_index == last_harq_index) {
				dma_harq_from_host_partial(deq_op, queue_index,
				start_index, end_index,
				harq_index, harq_size_per_cb, 1);
				break;
			} else {
				dma_harq_from_host_partial(deq_op, queue_index,
					start_index, end_index,
					harq_index, harq_size_per_cb, 0);
			}
			start_index = -1;
			harq_index = num_harq_contexts;
		}

		/* Update bit and byte index */
		bit_index++;
		if (bit_index == 8) {
			byte_index++;
			bit_index = 0;
		}
	}
}

static void
send_and_free_op(struct bbdev_ipc_dequeue_op *deq_op, int queue_index,
		 int q_id, int status)
{
	struct bbdev_ipc_enqueue_op eop, *enq_op = &eop;
	feca_job_t *feca_job;
	int ret;

	/* Status should be set after le32 swap */
	eop.status = status;

	if (deq_op->feca_job_type == FECA_JOB_SD ||
	    deq_op->feca_job_type == FECA_JOB_SD_DCM) {
		eop.out_len = 0;
		feca_job = (feca_job_t *)deq_op->feca_job_addr;
		out_le32(&(eop.crc_stat_addr),
			((unsigned int)SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_axi_stat_addr_low)
			- PEBM_BASE_ADDR));
		if (deq_op->harq_out_addr)
			process_out_harq(deq_op, &eop, queue_index);
	}
retry:
	ret = bbdev_ipc_enqueue_ops(BBDEV_IPC_DEV_ID_0,	q_id, &enq_op, 1);
	if (!ret)
		goto retry;
}

#if !VSPA_ENABLE
static void fill_se_qdma_desc_multi_qdma(struct pending_jobs_t *pending_job, int queue_index)
{
	struct bbdev_ipc_dequeue_op *deq_op = pending_job->deq_op;
	uint32_t swap_data_to_read;

	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 |= 0x10000;
	/* Reset and fill queue */
	queue->DescHead[queue->index].Cfg2 &= 0x8FFFFFFF;
	queue->DescHead[queue->index].Cfg2 |= (queue_index << 28);
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	/* Set the Global QDMA descriptor */
	if (pending_job->data_to_read > MAX_DST_SIZE) {
		/* SWAP_32((uint32_t)1 << 19) */
		NxpClt->SrcDescFmt.Cmd |= 0x800;
		/* Stride size 2k and distance 0;sizes(b):bit = 2k:23, 1k:22, 512:21,
		   256:20, 128:19, 64:18 ... 1:12*/
		/* SWAP_32(BIT(23)) */
		NxpClt->SrcDescFmt.StrideWay = 0x8000;
	} else {
		NxpClt->SrcDescFmt.StrideWay = 0;
		NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;
	}

	swap_data_to_read = SWAP_32(pending_job->data_to_read);
	NxpClt->dCmdListTable.LowAddrBase =
		SWAP_32(deq_op->out_addr + pending_job->dst_offset);
	NxpClt->sCmdListTable.LowAddrBase = feca_axi_addr_le[queue_index];
	NxpClt->dCmdListTable.DataLen = swap_data_to_read;
	NxpClt->sCmdListTable.DataLen = swap_data_to_read;
}
#endif

/* Return 0 if job complete, 1 otherwise */
static inline int
process_se_pending(struct pending_jobs_t *pending_job, int queue_index)
{
	struct bbdev_ipc_dequeue_op *deq_op = pending_job->deq_op;

	/*pending 0 mean , not process or qdma is in process */
	if (pending_job->qdma_pending == 0) {
#if VSPA_ENABLE
		/* QDMA operation still pending, return 1 */
		if (!flag[g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]])
			return 1;
		flag[g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]] = 0;
#else
		NxpQdmaProcessStatusQueue();
		/* QDMA operation still pending, return 1 */
		if (!flag[queue_index]) {
#ifdef FECA_IPC_DEBUG
			pending_job->num_retries++;
			if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
				PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
				PRINTF("Job dump:\n\r");
				feca_job_dump(deq_op);
				PRINTF("Start FECA regs dump:\n\r");
				feca_print_cb_reg(&feca_se_cb_regs);
				PRINTF("Current FECA regs dump:\n\r");
				feca_dump_cb_reg(f_res.se_chain, deq_op->feca_blk_id,
						deq_op->feca_job_type);
				error_printed = 1;
			}
#endif
			return 1;
		}
		flag[queue_index] = 0;
#endif

		/* In case of 'last' QDMA operation completed, return 0 */
		if (pending_job->processing_len_left == 0)
			return 0;

		pending_job->qdma_pending = 1;
	}

	if (pending_job->qdma_pending == 1) {
		/* Find out length to DMA in one iteration */
		if (pending_job->processing_len_left > se_output_circ_size/2)
			pending_job->data_to_read = se_output_circ_size/2;
		else
			pending_job->data_to_read = pending_job->processing_len_left;

		/* Wait for FECA to write bytes which we want to DMA in one iteration */
		if (feca_ch_out_valid_bytes(f_res.se_chain, deq_op->feca_blk_id) <
					pending_job->data_to_read) {
#ifdef FECA_IPC_DEBUG
			pending_job->num_retries++;
			if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
				PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
				PRINTF("Job dump:\n\r");
				feca_job_dump(deq_op);
				PRINTF("Start FECA regs dump:\n\r");
				feca_print_cb_reg(&feca_se_cb_regs);
				PRINTF("Current FECA regs dump:\n\r");
				feca_dump_cb_reg(f_res.se_chain, deq_op->feca_blk_id,
						deq_op->feca_job_type);
				error_printed = 1;
			}
#endif
			return 1;
		}

#if !VSPA_ENABLE
		fill_se_qdma_desc_multi_qdma(pending_job, queue_index);
#endif
	}

	/* Write to QDMA Block register to start DMA */
#if VSPA_ENABLE
	send_msg_to_vspa(deq_op->out_addr + pending_job->dst_offset,
			pending_job->data_to_read, 0,
			g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]);
#else
	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
#endif
	pending_job->qdma_pending = 0;
	pending_job->dst_offset += pending_job->data_to_read;
	pending_job->processing_len_left -= pending_job->data_to_read;

#ifdef FECA_IPC_DEBUG
	pending_job->num_retries++;
	if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
		PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
		PRINTF("Job dump:\n\r");
		feca_job_dump(deq_op);
		PRINTF("Start FECA regs dump:\n\r");
		feca_print_cb_reg(&feca_se_cb_regs);
		PRINTF("Current FECA regs dump:\n\r");
		feca_dump_cb_reg(f_res.se_chain, deq_op->feca_blk_id,
				deq_op->feca_job_type);
		error_printed = 1;
	}
#endif
	return 1;
}

#ifdef BBDEV_USE_SE_QDMA
static void start_se_qdma_input(struct bbdev_ipc_dequeue_op *deq_op, int queue_index)
{
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 |= 0x10000;
	/* Reset and fill queue */
	queue->DescHead[queue->index].Cfg2 &= 0x8FFFFFFF;
	queue->DescHead[queue->index].Cfg2 |= (queue_index << 28);
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];

	NxpClt->SrcDescFmt.StrideWay = 0;
	NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;

	/* Set the Global QDMA descriptor */
	if (SWAP_32(deq_op->in_len) > MAX_DST_SIZE) {
		NxpClt->DstDescFmt.Cmd |= 0x800;;
		NxpClt->DstDescFmt.StrideWay = 0x8000;
	} else {
		NxpClt->DstDescFmt.StrideWay = 0;
		NxpClt->DstDescFmt.Cmd &= 0xFFFFF7FF;
	}

	NxpClt->sCmdListTable.LowAddrBase = deq_op->in_addr;
	NxpClt->dCmdListTable.LowAddrBase = g_feca_se_in_addr_swapped;
	NxpClt->sCmdListTable.DataLen = deq_op->in_len;
	NxpClt->dCmdListTable.DataLen = deq_op->in_len;

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);

	/* Wait while QDMA operation still pending */
	while (!flag[queue_index])
		NxpQdmaProcessStatusQueue();
	flag[queue_index] = 0;
}
#endif

void dma_se_job_to_feca(struct bbdev_ipc_dequeue_op *deq_op)
{
	uint32_t cmd_size;
	se_command_t *se_cmd;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif
	se_cmd = (se_command_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET);

	cmd_size = deq_op->feca_job_type == FECA_JOB_SE ? feca_cmd_size[0] : feca_cmd_size[1];
	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->SrcDescFmt.StrideWay = 0;
	NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->dCmdListTable.LowAddrBase = feca_se_cmd_addr[deq_op->feca_blk_id];
	NxpClt->sCmdListTable.LowAddrBase = SWAP_32((BaseType_t)se_cmd);
	NxpClt->dCmdListTable.DataLen = cmd_size;
	NxpClt->sCmdListTable.DataLen = cmd_size;

	/* Unset flag and start QDMA */
	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
}

/* Return 0 if job complete, 1 otherwise */
static inline int
process_se_start(struct bbdev_ipc_dequeue_op *deq_op, int queue_index)
{
	struct pending_jobs_t *pending_job;
	int ret;

#ifdef FECA_IPC_DEBUG
	feca_get_cb_reg(f_res.se_chain, deq_op->feca_blk_id,
			deq_op->feca_job_type, &feca_se_cb_regs);
#endif

	dma_se_job_to_feca(deq_op);

#ifdef BBDEV_USE_SE_QDMA
	start_se_qdma_input(deq_op, queue_index);
#endif

	/* Create a pending op as this is pending after job submission,
	 * call process pending for this op for the first time here itself.
	 */
	pending_job = &pending_jobs[queue_index];
	pending_job->deq_op = deq_op;
	pending_job->dst_offset = 0;
	pending_job->qdma_pending = 1;
	pending_job->processing_len_left = deq_op->out_len;
	pending_job->num_retries = 0;

	ret = process_se_pending(pending_job, queue_index);

	return ret;
}

static void fill_decode_qdma_desc(struct bbdev_ipc_dequeue_op *deq_op, int queue_index)
{
	uint32_t data_len = deq_op->in_len;

	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	/* Set the Global QDMA descriptor */
	if (SWAP_32(data_len) > MAX_DST_SIZE) {
		/* SWAP_32((uint32_t)1 << 19) */
		NxpClt->DstDescFmt.Cmd |= 0x800;;
		/* Stride size 2k and distance 0;sizes(b):bit = 2k:23, 1k:22, 512:21,
		   256:20, 128:19, 64:18 ... 1:12*/
		/* SWAP_32(BIT(23)) */
		NxpClt->DstDescFmt.StrideWay = 0x8000;
	} else {
		NxpClt->DstDescFmt.StrideWay = 0;
		NxpClt->DstDescFmt.Cmd &= 0xFFFFF7FF;
	}

	NxpClt->dCmdListTable.LowAddrBase = feca_axi_addr_le[queue_index];
	NxpClt->sCmdListTable.LowAddrBase = deq_op->in_addr;
	NxpClt->dCmdListTable.DataLen = data_len;
	NxpClt->sCmdListTable.DataLen = data_len;
}

/* Return 0 if job complete, 1 otherwise */
static inline int
process_sd_pending(struct pending_jobs_t *pending_job, int queue_index)
{
	int status;

	UNUSED(queue_index);

	status = feca_get_sd_job_status(f_res.sd_chain,
			(feca_job_t *)pending_job->deq_op->feca_job_addr);
	if (status == FECA_SUCCESS)
		return 0;

#ifdef FECA_IPC_DEBUG
	pending_job->num_retries++;
	if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
		PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
		PRINTF("Job dump:\n\r");
		feca_job_dump(pending_job->deq_op);
		PRINTF("Start FECA regs dump:\n\r");
		feca_print_cb_reg(&feca_sd_cb_regs);
		PRINTF("Current FECA regs dump:\n\r");
		feca_dump_cb_reg(f_res.sd_chain, pending_job->deq_op->feca_blk_id,
				pending_job->deq_op->feca_job_type);
		error_printed = 1;
	}
#endif
	return 1;
}

void dma_sd_job_to_feca(struct bbdev_ipc_dequeue_op *deq_op)
{
	uint32_t cmd_size;
	sd_command_t *sd_cmd;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif
	sd_cmd = (sd_command_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET);

	cmd_size = deq_op->feca_job_type == FECA_JOB_SD ? feca_cmd_size[2] : feca_cmd_size[3];

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->SrcDescFmt.StrideWay = 0;
	NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->dCmdListTable.LowAddrBase = feca_sd_cmd_addr[deq_op->feca_blk_id];
	NxpClt->sCmdListTable.LowAddrBase = SWAP_32((BaseType_t)sd_cmd);
	NxpClt->dCmdListTable.DataLen = cmd_size;
	NxpClt->sCmdListTable.DataLen = cmd_size;

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
	/* No wait for SD QDMA job completion */
}

/* Return 0 if job complete, 1 otherwise */
static int
process_sd_start(struct bbdev_ipc_dequeue_op *deq_op,
		 int queue_index)
{
	sd_command_t *sd_cmd;
	struct sd_crc_sts_desc *desc;
	struct pending_jobs_t *pending_job;
	int ret;

	sd_cmd = (sd_command_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET);

	desc = &crc_desc[deq_op->feca_blk_id];
	sd_cmd->sd_axi_stat_addr_low =
		(uint32_t)desc->crc_stat[desc->crc_stat_idx];
	desc->crc_stat_idx = (desc->crc_stat_idx + 1) % MAX_CHANNEL_DEPTH;

	if (deq_op->harq_in_addr)
		process_in_harq(deq_op, queue_index);

	dma_sd_job_to_feca(deq_op);

#ifdef FECA_IPC_DEBUG
	feca_get_cb_reg(f_res.sd_chain, deq_op->feca_blk_id,
			deq_op->feca_job_type, &feca_sd_cb_regs);
#endif

#if VSPA_ENABLE
	send_msg_to_vspa(SWAP_32(deq_op->in_addr),
			SWAP_32(deq_op->in_len) | 0x80000000, 0,
			g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]);
#else
	fill_decode_qdma_desc(deq_op, queue_index);

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
#endif

	if (!deq_op->out_len)
		return 0;

	pending_job = &pending_jobs[queue_index];
	pending_job->deq_op = deq_op;
	pending_job->process_sd_pending_fn = process_sd_pending;

	ret = process_sd_pending(pending_job, queue_index);

	return ret;
}

#if !VSPA_ENABLE
static void fill_decode_qdma_desc_multi_qdma(struct pending_jobs_t *pending_job, int queue_index)
{
	uint32_t swap_data_to_write;
	uint32_t avail_bytes;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;

	avail_bytes = sd_input_circ_size - feca_ch_in_valid_bytes(f_res.sd_chain,
			pending_job->deq_op->feca_blk_id);

	/* Find out length to DMA in one iteration */
	if (pending_job->processing_len_left > avail_bytes)
		pending_job->data_to_read = avail_bytes;
	else
		pending_job->data_to_read = pending_job->processing_len_left;

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	if (pending_job->processing_len_left - pending_job->data_to_read > 0)
		queue->DescHead[queue->index].Cfg2 |= 0x10000;
	else
		queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* Reset and fill queue */
	queue->DescHead[queue->index].Cfg2 &= 0x8FFFFFFF;
	queue->DescHead[queue->index].Cfg2 |= (queue_index << 28);
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	/* Set the Global QDMA descriptor */
	if (pending_job->data_to_read > MAX_DST_SIZE) {
		/* SWAP_32((uint32_t)1 << 19) */
		NxpClt->DstDescFmt.Cmd |= 0x800;;
		/* Stride size 2k and distance 0;sizes(b):bit = 2k:23, 1k:22, 512:21,
		   256:20, 128:19, 64:18 ... 1:12*/
		/* SWAP_32(BIT(23)) */
		NxpClt->DstDescFmt.StrideWay = 0x8000;
	} else {
		NxpClt->DstDescFmt.StrideWay = 0;
		NxpClt->DstDescFmt.Cmd &= 0xFFFFF7FF;
	}

	swap_data_to_write = SWAP_32(pending_job->data_to_read);
	NxpClt->dCmdListTable.LowAddrBase = feca_axi_addr_le[queue_index];
	NxpClt->sCmdListTable.LowAddrBase =
		SWAP_32(pending_job->base_addr + pending_job->dst_offset);
	NxpClt->dCmdListTable.DataLen = swap_data_to_write;
	NxpClt->sCmdListTable.DataLen = swap_data_to_write;
}
#endif

/* Return 0 if job complete, 1 otherwise */
static int
process_sd_pending_multi_qdma(struct pending_jobs_t *pending_job, int queue_index)
{
	struct bbdev_ipc_dequeue_op *deq_op = pending_job->deq_op;

#if !VSPA_ENABLE
	uint32_t avail_bytes;
#endif

	if (pending_job->qdma_pending == 0) {
		if (pending_job->processing_len_left == 0) {
			/* Wait for FECA to read all the bytes */
			if (feca_get_sd_job_status(f_res.sd_chain,
					(feca_job_t *)deq_op->feca_job_addr)) {
#ifdef FECA_IPC_DEBUG
				pending_job->num_retries++;
				if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
					PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
					PRINTF("Job dump:\n\r");
					feca_job_dump(pending_job->deq_op);
					PRINTF("Start FECA regs dump:\n\r");
					feca_print_cb_reg(&feca_sd_cb_regs);
					PRINTF("Current FECA regs dump:\n\r");
					feca_dump_cb_reg(f_res.sd_chain, pending_job->deq_op->feca_blk_id,
							pending_job->deq_op->feca_job_type);
					error_printed = 1;
				}
#endif
				return 1;
			}
#if VSPA_ENABLE
			flag[g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]] = 0;
#else
#endif
			return 0;
		}

#if VSPA_ENABLE
		/* DMA operation still pending, return 1 */
		if (!flag[g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]])
			return 1;
		flag[g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]] = 0;
#else
		NxpQdmaProcessStatusQueue();

		/* QDMA operation still pending, return 1 */
		if (!flag[queue_index]) {
#ifdef FECA_IPC_DEBUG
			pending_job->num_retries++;
			if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
				PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
				PRINTF("Job dump:\n\r");
				feca_job_dump(pending_job->deq_op);
				PRINTF("Start FECA regs dump:\n\r");
				feca_print_cb_reg(&feca_sd_cb_regs);
				PRINTF("Current FECA regs dump:\n\r");
				feca_dump_cb_reg(f_res.sd_chain, pending_job->deq_op->feca_blk_id,
						pending_job->deq_op->feca_job_type);
				error_printed = 1;
			}
#endif
			return 1;
		}
		flag[queue_index] = 0;
#endif
		pending_job->qdma_pending = 1;
	}

#if VSPA_ENABLE
	if (feca_ch_in_valid_bytes(f_res.sd_chain, deq_op->feca_blk_id) >
			sd_input_circ_size/2)
		return 1;

	/* Find out length to DMA in one iteration */
	if (pending_job->processing_len_left > sd_input_circ_size/2)
		pending_job->data_to_read = sd_input_circ_size/2;
	else
		pending_job->data_to_read = pending_job->processing_len_left;

	send_msg_to_vspa(pending_job->base_addr + pending_job->dst_offset,
			pending_job->data_to_read | 0x80000000, 0,
			g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]);
#else
	avail_bytes = sd_input_circ_size -
		feca_ch_in_valid_bytes(f_res.sd_chain, deq_op->feca_blk_id);
	if (!avail_bytes) {
#ifdef FECA_IPC_DEBUG
		pending_job->num_retries++;
		if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
			PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
			PRINTF("Job dump:\n\r");
			feca_job_dump(pending_job->deq_op);
			PRINTF("Start FECA regs dump:\n\r");
			feca_print_cb_reg(&feca_sd_cb_regs);
			PRINTF("Current FECA regs dump:\n\r");
			feca_dump_cb_reg(f_res.sd_chain, pending_job->deq_op->feca_blk_id,
					pending_job->deq_op->feca_job_type);
			error_printed = 1;
		}
#endif
		return 1;
	}

	/* Fill QDMA descriptor */
	fill_decode_qdma_desc_multi_qdma(pending_job, queue_index);
	/* Write to QDMA Block register to start DMA */
	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
#endif

	pending_job->qdma_pending = 0;
	pending_job->dst_offset += pending_job->data_to_read;
	pending_job->processing_len_left -= pending_job->data_to_read;

#ifdef FECA_IPC_DEBUG
	pending_job->num_retries++;
	if ((pending_job->num_retries > MAX_OP_RETRY) && !error_printed) {
		PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
		PRINTF("Job dump:\n\r");
		feca_job_dump(pending_job->deq_op);
		PRINTF("Start FECA regs dump:\n\r");
		feca_print_cb_reg(&feca_sd_cb_regs);
		PRINTF("Current FECA regs dump:\n\r");
		feca_dump_cb_reg(f_res.sd_chain, pending_job->deq_op->feca_blk_id,
				pending_job->deq_op->feca_job_type);
		error_printed = 1;
	}
#endif
	return 1;
}

/* Return 0 if job complete, 1 otherwise */
static int
process_sd_start_multi_qdma(struct bbdev_ipc_dequeue_op *deq_op,
		 int queue_index, uint32_t swaped_in_len)
{
	sd_command_t *sd_cmd;
	struct sd_crc_sts_desc *desc;
	struct pending_jobs_t *pending_job;
	int ret;

	sd_cmd = (sd_command_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET);

	desc = &crc_desc[deq_op->feca_blk_id];
	sd_cmd->sd_axi_stat_addr_low =
		(uint32_t)desc->crc_stat[desc->crc_stat_idx];
	desc->crc_stat_idx = (desc->crc_stat_idx + 1) % MAX_CHANNEL_DEPTH;

	if (deq_op->harq_in_addr)
		process_in_harq(deq_op, queue_index);

	pending_job = &pending_jobs[queue_index];
	pending_job->num_retries = 0;
	pending_job->deq_op = deq_op;
	pending_job->dst_offset = 0;
	pending_job->base_addr = SWAP_32(deq_op->in_addr);
	pending_job->processing_len_left = swaped_in_len;
	pending_job->process_sd_pending_fn = process_sd_pending_multi_qdma;
	dma_sd_job_to_feca(deq_op);

#if VSPA_ENABLE
	pending_job->data_to_read = sd_input_circ_size;
	send_msg_to_vspa(pending_job->base_addr,
			pending_job->data_to_read | 0x80000000, 0,
			g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index]);
#else
	fill_decode_qdma_desc_multi_qdma(pending_job, queue_index);
	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
#endif

	pending_job->qdma_pending = 0;
	pending_job->dst_offset += pending_job->data_to_read;
	pending_job->processing_len_left -= pending_job->data_to_read;

#ifdef FECA_IPC_DEBUG
	feca_get_cb_reg(f_res.sd_chain, deq_op->feca_blk_id, deq_op->feca_job_type, &feca_sd_cb_regs);
#endif

	if (!deq_op->out_len)
		return 0;

	ret = process_sd_pending_multi_qdma(pending_job, queue_index);

	return ret;
}

void dma_ce_job_to_feca(chain_handle_t feca_chain,
		struct bbdev_ipc_dequeue_op *deq_op,
		int queue_index)
{
	BaseType_t feca_ce_cmd_addr;
	uint32_t cmd_size = deq_op->polar_cmd_size;
	ce_command_t *ce_cmd;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif
	UNUSED(queue_index);

	feca_ce_cmd_addr = (BaseType_t)feca_get_ch_axi_addr(feca_chain,
			feca_get_ch_id(feca_chain, CH_CMD, 0));

	ce_cmd = (ce_command_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET);

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->SrcDescFmt.StrideWay = 0;
	NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->dCmdListTable.LowAddrBase = SWAP_32(feca_ce_cmd_addr);
	NxpClt->sCmdListTable.LowAddrBase = SWAP_32((BaseType_t)ce_cmd);
	NxpClt->dCmdListTable.DataLen = cmd_size;
	NxpClt->sCmdListTable.DataLen = cmd_size;

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);

	/* No wait for CE QDMA job completion */
}

static void process_ce(struct bbdev_ipc_dequeue_op *deq_op, int queue_index)
{
	int status;
	uint32_t swap_data_to_read;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif

#ifdef FECA_IPC_DEBUG
	feca_get_cb_reg(f_res.ce_chain, deq_op->feca_blk_id, deq_op->feca_job_type, &feca_ce_cb_regs);
#endif

	dma_ce_job_to_feca(f_res.ce_chain, deq_op, queue_index);

	if (deq_op->feca_job_type == FECA_JOB_CE) {
		NxpQdmaCLT_t *NxpClt;
		NxpQdmaQueue_t *queue;
		/* Check status and clear the status bit. This is not required for FECA_JOB_CE_DCM as there
		 * is no output from CE
		 */
		while ((status = feca_get_job_status(f_res.ce_chain,
				(feca_job_t *)deq_op->feca_job_addr)) != FECA_SUCCESS) {}

		/* Preparing QDMA descriptor for L1 to L2 data copy */
		/* Incase CE maximum output size is 2048bytes
		 * (Maximum E size in command is 16K bits).
		 * So no need to use QDMA striding.
		 */
		swap_data_to_read = SWAP_32(deq_op->out_len);
		queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
		/* Update SER settings */
		queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
		/* TODO add check for queue full */
		NxpClt = &queue->NxpClt[queue->index];
		NxpClt->dCmdListTable.LowAddrBase = SWAP_32(deq_op->out_addr);
		NxpClt->sCmdListTable.LowAddrBase = feca_axi_addr_le[queue_index];
		NxpClt->dCmdListTable.DataLen = swap_data_to_read;
		NxpClt->sCmdListTable.DataLen = swap_data_to_read;

		NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
		/* No QDMA completion wait */
	}
}

static void dma_cd_llrs_to_host(struct bbdev_ipc_dequeue_op *deq_op,
				int queue_index)
{
	uint32_t data_len = deq_op->out_len;
	uint32_t swap_data_len = SWAP_32(deq_op->out_len);
	uint32_t job_type_idx;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif
	UNUSED(queue_index);

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	/* Set the Global QDMA descriptor */
	if (data_len > MAX_DST_SIZE) {
		/* SWAP_32((uint32_t)1 << 19) */
		NxpClt->SrcDescFmt.Cmd |= 0x800;;
		/* Stride size 2k and distance 0;sizes(b):bit = 2k:23, 1k:22, 512:21,
		   256:20, 128:19, 64:18 ... 1:12*/
		/* SWAP_32(BIT(23)) */
		NxpClt->SrcDescFmt.StrideWay = 0x8000;
	} else {
		NxpClt->SrcDescFmt.StrideWay = 0;
		NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;
	}

	job_type_idx = deq_op->feca_job_type - FECA_JOB_CD_DCM_ACK;
	NxpClt->dCmdListTable.LowAddrBase =
			SWAP_32(deq_op->out_addr);
	NxpClt->sCmdListTable.LowAddrBase =
			feca_cd_demux_input_axi_addr_le[job_type_idx][deq_op->feca_blk_id];
	NxpClt->dCmdListTable.DataLen = swap_data_len;
	NxpClt->sCmdListTable.DataLen = swap_data_len;

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
	/* No wait for QDMA completion */
}

void dma_cd_job_to_feca(chain_handle_t feca_chain,
		struct bbdev_ipc_dequeue_op *deq_op,
		int queue_index)
{
	BaseType_t feca_cd_cmd_addr;
	uint32_t cmd_size = deq_op->polar_cmd_size;
	cd_command_t *cd_cmd;
	NxpQdmaCLT_t *NxpClt;
	NxpQdmaQueue_t *queue;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif
	UNUSED(queue_index);

	switch(deq_op->feca_job_type) {
	case FECA_JOB_CD:
		feca_cd_cmd_addr = (BaseType_t)feca_get_ch_axi_addr(feca_chain,
				feca_get_ch_id(feca_chain, CH_CMD, 0));
		break;
	case FECA_JOB_CD_DCM_ACK:
		feca_cd_cmd_addr = (BaseType_t)feca_get_ch_axi_addr(feca_chain,
				(FECA_CD_ACK_CMD_CB_ID + deq_op->feca_blk_id));
		break;
	case FECA_JOB_CD_DCM_CS1:
		feca_cd_cmd_addr = (BaseType_t)feca_get_ch_axi_addr(feca_chain,
				(FECA_CD_CSI1_CMD_CB_ID + deq_op->feca_blk_id));
		break;
	case FECA_JOB_CD_DCM_CS2:
		feca_cd_cmd_addr = (BaseType_t)feca_get_ch_axi_addr(feca_chain,
				(FECA_CD_CSI2_CMD_CB_ID + deq_op->feca_blk_id));
		break;
	default:
		PRINTF("Invalid job type for CD\n\r");
		return;
	}

	cd_cmd = (cd_command_t *)(deq_op->feca_job_addr + FECA_CMD_OFFSET);

	queue = qDMA_get_clt_addr(NXP_DEFAULT_QDMA_QUEUE);
	/* Update SER settings */
	queue->DescHead[queue->index].Cfg2 &= 0xFFFEFFFF;
	/* TODO add check for queue full */
	NxpClt = &queue->NxpClt[queue->index];
	NxpClt->SrcDescFmt.StrideWay = 0;
	NxpClt->SrcDescFmt.Cmd &= 0xFFFFF7FF;

	NxpClt->dCmdListTable.LowAddrBase = SWAP_32(feca_cd_cmd_addr);
	NxpClt->sCmdListTable.LowAddrBase = SWAP_32((BaseType_t)cd_cmd);
	NxpClt->dCmdListTable.DataLen = cmd_size;
	NxpClt->sCmdListTable.DataLen = cmd_size;

	NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
	/* No wait for CD QDMA job completion */
}

static void process_cd(struct bbdev_ipc_dequeue_op *deq_op, int queue_index)
{
	int status;
#ifdef FECA_IPC_DEBUG
	uint32_t retries = 0;
#endif

	if (deq_op->op_flags & BBDEV_POLAR_DEQUEUE_LLRS) {
		dma_cd_llrs_to_host(deq_op, queue_index);
		return;
	}

#ifdef FECA_IPC_DEBUG
	feca_get_cb_reg(f_res.cd_chain, deq_op->feca_blk_id, deq_op->feca_job_type, &feca_cd_cb_regs);
#endif


	dma_cd_job_to_feca(f_res.cd_chain, deq_op, queue_index);

	if (deq_op->feca_job_type == FECA_JOB_CD) {
		fill_decode_qdma_desc(deq_op, queue_index);
		NxpCmdQueueEnqueueExt(NXP_DEFAULT_QDMA_QUEUE);
	}

	while ((status = feca_get_job_status(f_res.cd_chain,
			(feca_job_t *)deq_op->feca_job_addr)) != FECA_SUCCESS) {
#ifdef FECA_IPC_DEBUG
		if (retries == MAX_OP_RETRY * 100) {
			PRINTF("Failure point [%s, %d]\n", __func__, __LINE__);
			PRINTF("Job dump:\n\r");
			feca_job_dump(deq_op);
			PRINTF("Start FECA regs dump:\n\r");
			feca_print_cb_reg(&feca_cd_cb_regs);
			PRINTF("Current FECA regs dump:\n\r");
			feca_dump_cb_reg(f_res.cd_chain, deq_op->feca_blk_id,
					deq_op->feca_job_type);
			error_printed = 1;
		}
		retries++;
#endif
	}
}

void
process_pending_qdma_transactions(void)
{
	struct bbdev_ipc_dequeue_op *deq_op;
	int queue_index = 0, i, empty_pending_jobs;
	int ret = 0;

	while (1) {
		deq_op = pending_jobs[queue_index].deq_op;
		if (deq_op) {
			if (deq_op->feca_job_type == FECA_JOB_SE ||
					deq_op->feca_job_type == FECA_JOB_SE_DCM)
				ret = process_se_pending(&pending_jobs[queue_index],
							 queue_index);
			else
				ret = pending_jobs[queue_index].process_sd_pending_fn(
					&pending_jobs[queue_index], queue_index);
			if (!ret) {
                                send_and_free_op(deq_op, queue_index,
					g_queue_ids[((u8)ulMpicCurrentCore() * BBDEV_IPC_MAX_CORES) + queue_index], 0);
                                pending_jobs[queue_index].deq_op = NULL;
			}
		}
		queue_index = (queue_index + 1) % MAX_QUEUES_PER_CORE;
		empty_pending_jobs = 0;
		for (i = 0; i < MAX_QUEUES_PER_CORE; i++) {
			/* Check if all pending jobs are NULL */
			if (pending_jobs[queue_index].deq_op)
				empty_pending_jobs++;
		}

		if (empty_pending_jobs == MAX_QUEUES_PER_CORE)
			break;
	}

#ifdef FECA_IPC_DEBUG
	error_printed = 0;
#endif
}

static int
bbdev_feca_soft_reset_processing(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();
	int first_en_core = -1, total_en_cores = 0;
	int status, ret, i, j;

	process_pending_qdma_transactions();
	feca_init_done = 0;

	for (i = 0; i < BBDEV_IPC_MAX_CORES; i++) {
		if (bbdev_core_enabled[i]) {
			total_en_cores++;
			if (first_en_core == -1)
				first_en_core = i;
		}
	}

	/* Sync all enabled cores. */
	reset_sync_point2[core_id] = 0;
	reset_sync_point1[core_id] = 1;
	while (1) {
		j = 0;
		for (i = 0; i < BBDEV_IPC_MAX_CORES; i++)
			j += reset_sync_point1[i];
		/* Break once all cores have reset_sync_point1 set */
		if (j == total_en_cores)
			break;
	}

	/* Reset FECA and BBDEV from one core */
	if (core_id == first_en_core) {
		status = feca_dev_reset(f_res.device);
		if (status != FECA_SUCCESS) {
			log_err("feca reset failed \n");
			return -1;
		}

		ret = init_feca_channels(&f_res, first_en_core);
		if (ret) {
			log_err("feca channels intialization failed \n");
			return -1;
		}

		bbdev_ipc_soft_reset_complete(BBDEV_IPC_DEV_ID_0);
	}

	/* Again sync all enabled cores. Reuse bbdev_core_enabled. */
	reset_sync_point2[core_id] = 1;
	while (1) {
		j = 0;
		for (i = 0; i < BBDEV_IPC_MAX_CORES; i++)
			j += reset_sync_point2[i];
		/* Break once all cores have reset_sync_point2 set */
		if (j == total_en_cores)
			break;
	}
	reset_sync_point1[core_id] = 0;

	return 0;
}

static int bbdev_ipc_test(void)
{
	/*Buffer to be used by consumer message channels*/
	u8 core_id = (u8)ulMpicCurrentCore();
	int queue_id, num_queues_this_core = 0;
	struct bbdev_ipc_dequeue_op *deq_op;
	uint8_t cqm = NXP_QDMA_BCQMR_CQM_AUTO;
	struct dev_attr_t *attr;
	uint32_t op_type, feca_blk_id;
	int ret, i, queue_index = 0;
	int queue_ids[BBDEV_IPC_MAX_QUEUES];

	/* Wait for bbdev device ready.*/
	while (!bbdev_ipc_is_host_initialized(BBDEV_IPC_DEV_ID_0)) {}

	if (core_id != 4 && core_id != 5) {
		/* Initialize QDMA */
	        if (core_id == 0) {
			if (!QdmaInitNoIntr(core_id, cqm, 1,
		                     0x91111110, 0xfefd, NULL, NULL)) {
		                log_info("qDMA:Failed to qdmaa init on core: %d\r\n"
					 "This core can be used for RAW queues only",
					 core_id);
			}
		}
	        if (core_id == 1) {
			if (!QdmaInitNoIntr(core_id, cqm, 1,
		                     0xa2222221, 0xfbfd, NULL, NULL)) {
		                log_info("qDMA:Failed to qdma init on core: %d\r\n"
					 "This core can be used for RAW queues only",
					 core_id);
			}
		}
	        if (core_id == 2 || core_id == 3) {
			if (!QdmaInitNoIntr(core_id, cqm, 1,
		                      0, 0, NULL, NULL)) {
		                log_info("qDMA:Failed to qdma init on core: %d\r\n"
					 "This core can be used for RAW queues only",
					 core_id);
			}
		}
	}

#if VSPA_ENABLE
	/* Wait till all VSPA cores are booted */
	for (i = VSPA_CORE_0; i < VSPA_CORE_MAX; i++)
		while (!uiCheckVSPABoot(i));

	/* Initialize VSPA */
	ret = init_vspa();
	if (ret) {
		log_err("VSPA intialization failed \n");
		return -1;
	}
#endif

	attr = bbdev_ipc_get_dev_attr(BBDEV_IPC_DEV_ID_0);
	if (attr->num_feca_se_channels > 2) {
		se_input_circ_size = FECA_SE_MULTI_INPUT_MAX_SZ;
		se_output_circ_size = FECA_SE_MULTI_OUTPUT_MAX_SZ;
	} else {
		se_input_circ_size = FECA_SE_INPUT_MAX_SZ;
		se_output_circ_size = FECA_SE_OUTPUT_MAX_SZ;
	}

	/* By default Multi-QDMA is used, but can be configured to use
	 * single QDMA for enhanced performance. Using single QDMA can
	 * cause problem in some scenarios where FECA becomes slow in
	 * processing and QDMA overwrites previous unread data. So single
	 * QDMA should be used cautiously and only when all the scenarios
	 * are passing.
	 */
	if (!attr->use_feca_sd_single_qdma) {
		sd_input_circ_size = FECA_SD_MULTI_INPUT_MAX_SZ;
		sd_output_circ_size = FECA_SD_MULTI_OUTPUT_MAX_SZ;
	} else {
		use_feca_sd_single_qdma = 1;
		sd_input_circ_size = FECA_SD_INPUT_MAX_SZ;
		sd_output_circ_size = FECA_SD_OUTPUT_MAX_SZ;
	}

	ret = init_feca_chains(&f_res, GEUL_E200_MASTER_CORE);
	if (ret) {
		log_err("feca chains intialization failed \n");
		return -1;
	}

	ret = init_feca_channels(&f_res, GEUL_E200_MASTER_CORE);
	if (ret) {
		log_err("feca channels intialization failed \n");
		return -1;
	}

	/* Wait until core 0 has initialized all FECA channels */
	while (!feca_init_done);

	if (core_id != 4 && core_id != 5) {
	if (!RegisterCQueueCallback(qdma_completion_callback,
			(void *)NXP_DEFAULT_QDMA_QUEUE, NXP_DEFAULT_QDMA_QUEUE)) {
				log_err("RegisterCQueueCallback failed\n");
				return -1;
	}
	}
	for (i = 0; i < attr->num_queues; i++) {
		if (attr->qattr[i].core_id != core_id  ||
		    (attr->qattr[i].op_type == BBDEV_IPC_OP_LA12XX_VSPA))
			continue;

		queue_id = attr->qattr[i].queue_id;
		queue_index = num_queues_this_core;
		queue_ids[queue_index] = queue_id;
		g_queue_ids[(core_id * BBDEV_IPC_MAX_CORES) + queue_index] = queue_id;

		ret = bbdev_ipc_queue_configure(BBDEV_IPC_DEV_ID_0, queue_id);
		if (ret != IPC_SUCCESS) {
			log_err("queue configure failed for queue %d error %d \n",
				queue_id, ret);
			return ret;
		}
		op_type = attr->qattr[i].op_type;
		queues_op_type[queue_id] = op_type;
		feca_blk_id = attr->qattr[i].feca_blk_id;

		g_input_circ_size[queue_index] = attr->qattr[i].feca_input_circ_size;
		if (op_type == BBDEV_IPC_OP_LDPC_ENC) {
			init_qdma_encode_desc(&f_res, feca_blk_id, queue_index, 0, core_id);

			if (!g_input_circ_size[queue_index]) {
				g_input_circ_size[queue_index] = se_input_circ_size;
				bbdev_ipc_set_cb_size(BBDEV_IPC_DEV_ID_0, se_input_circ_size, i);
			}
		} else if (op_type == BBDEV_IPC_OP_LDPC_DEC) {
			init_qdma_decode_desc(&f_res, feca_blk_id, queue_index, 0, core_id);

			ret = init_sd_crc_stat(feca_blk_id);
			if (ret) {
				log_err("init_sd_crc_stat failed\n");
				return -1;
			}

			if (!g_input_circ_size[queue_index]) {
				g_input_circ_size[queue_index] = sd_input_circ_size;
				bbdev_ipc_set_cb_size(BBDEV_IPC_DEV_ID_0, sd_input_circ_size, i);
			}
		} else if (op_type == BBDEV_IPC_OP_POLAR_ENC) {
			init_qdma_encode_desc(&f_res, feca_blk_id, queue_index, 1, core_id);
		} else if (op_type == BBDEV_IPC_OP_POLAR_DEC) {
			init_qdma_decode_desc(&f_res, feca_blk_id, queue_index, 1, core_id);
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

	if (num_queues_this_core == 0) {
#ifdef BBDEV_DEFAULT_FUNC_CALL
		/* If no queue configured for this core, simply go in
		 * a while loop */
		while(1);
#else
		return 0;
#endif
	} else {
		bbdev_core_enabled[core_id] = 1;
	}

	queue_index = 0;
	do {
		/* Check if BBDEV FECA reset is requested by Host */
		if (bbdev_ipc_soft_reset_request(BBDEV_IPC_DEV_ID_0)) {
			ret = bbdev_feca_soft_reset_processing();
			if (ret) {
				log_err("bbdev_feca_soft_reset_processing failed \n\r");
				return -1;
			}
		}

		queue_index++;
		if (queue_index == num_queues_this_core)
			queue_index = 0;

		deq_op = pending_jobs[queue_index].deq_op;
		if (deq_op) {
			if (deq_op->feca_job_type == FECA_JOB_SE ||
					deq_op->feca_job_type == FECA_JOB_SE_DCM)
				ret = process_se_pending(&pending_jobs[queue_index],
							 queue_index);
			else
				ret = pending_jobs[queue_index].process_sd_pending_fn(
					&pending_jobs[queue_index], queue_index);
			if (!ret) {
				send_and_free_op(deq_op, queue_index,
						 queue_ids[queue_index], 0);
				pending_jobs[queue_index].deq_op = NULL;

			} else {
				continue;
			}
		}

		queue_id = queue_ids[queue_index];
		ret = bbdev_ipc_dequeue_ops(BBDEV_IPC_DEV_ID_0,
				queue_id, &deq_op, 1);
		if (ret < 1)
			continue;

		if (queues_op_type[queue_id] == BBDEV_IPC_OP_RAW) {
			struct bbdev_ipc_enqueue_op eop, *enq_op = NULL;
			if (deq_op->out_addr) {
				uint32_t *in_data, *out_data, len, i;

				in_data = (uint32_t *)SWAP_32(deq_op->in_addr);
				out_data = (uint32_t *)deq_op->out_addr;
				len = SWAP_32(deq_op->in_len) / sizeof(uint32_t);

				for (i = 0; i < len; i++)
					out_data[i] = SWAP_32(in_data[i]);

				eop.out_len = len * sizeof(uint32_t);
				enq_op = &eop;
			}
retry:
			ret = bbdev_ipc_enqueue_ops(BBDEV_IPC_DEV_ID_0,
						    queue_id, &enq_op, 1);
			if (!ret)
				goto retry;

			continue;
		}

#ifdef FECA_IPC_DEBUG
		log_info("#########################################\n");
		log_info("Packet received on Queue ID: %d\n", queue_id);
		feca_job_dump(deq_op);
#endif

		ret = 0;
		if (deq_op->feca_job_type == FECA_JOB_SE ||
				deq_op->feca_job_type == FECA_JOB_SE_DCM)
			ret = process_se_start(deq_op, queue_index);
		else if (deq_op->feca_job_type == FECA_JOB_SD ||
				deq_op->feca_job_type == FECA_JOB_SD_DCM) {
			uint32_t swaped_in_len = SWAP_32(deq_op->in_len);
			if (use_feca_sd_single_qdma == 1 ||
			    swaped_in_len < sd_input_circ_size)
				ret = process_sd_start(deq_op, queue_index);
			else
				ret = process_sd_start_multi_qdma(deq_op, queue_index, swaped_in_len);
		} else if (deq_op->feca_job_type == FECA_JOB_CD ||
				deq_op->feca_job_type == FECA_JOB_CD_DCM_ACK ||
				deq_op->feca_job_type == FECA_JOB_CD_DCM_CS1 ||
				deq_op->feca_job_type == FECA_JOB_CD_DCM_CS2)
			process_cd(deq_op, queue_index);
		else if (deq_op->feca_job_type == FECA_JOB_CE ||
				deq_op->feca_job_type == FECA_JOB_CE_DCM)
			process_ce(deq_op, queue_index);

		if (!ret) {
			send_and_free_op(deq_op, queue_index, queue_id, 0);
			pending_jobs[queue_index].deq_op = NULL;
		}
		else
			pending_jobs[queue_index].deq_op = deq_op;
	} while (1);

	return 0;
}

void vBbdevIpcTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	/*Start Rx and Tx on respective cores.*/
	if(bbdev_ipc_test()!= 0)
		RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	else
		SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);

	return;
}
