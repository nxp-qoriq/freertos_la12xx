// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2023 NXP
 */

#include "tc_feca_bbdev.h"
#include "tc_feca_bbdev_tv.h"
#ifdef GEUL_DEMO_FECA_BBDEV_TEST
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "debug_console.h"
#include "common.h"

#include "shared_ch_hw_param_du.h"
#include "feca_api.h"
#include "feca_main_lib.h"
#include "qdma.h"

typedef struct feca_bbdev_test_cb {
	dev_handle_t feca_dev;
	chain_handle_t ce_chain;
	chain_handle_t se_chain;
	chain_handle_t cd_chain;
	chain_handle_t sd_chain;
	ch_handle_t ce_ch[CE_CH_NUM];
	ch_handle_t se_ch[SE_CH_NUM];
	ch_handle_t cd_ch[CD_CH_NUM];
	ch_handle_t sd_ch[SD_CH_NUM];
	int feca_bbdev_initialized;
	BaseType_t feca_out_ptr;
	int output_len;
	BaseType_t feca_sts_ptr;
	volatile uint32_t *cb_out_ptr;
	volatile uint32_t *cb_in_ptr;
	int *e;
}feca_bbdev_test_cb_t;

static feca_bbdev_test_cb_t g_feca_bbdev_cb;

extern const unsigned char se_data_input_tv[26];
extern const unsigned char se_data_output_tv[256];
extern const unsigned char sd_data_input_tv[2048];
extern const unsigned char sd_data_output_tv[26];

#if DCM_ARM_POLAR_DECODER_POC_EN
#if DCM_ARM_POLAR_TEST_NR == 1
extern const unsigned char sd_dcm_tv_in[1372];
extern const unsigned char sd_dcm_tv_ref[64];
extern uint32_t sd_dcm_tv_cmd[102];
#elif DCM_ARM_POLAR_TEST_NR == 2
extern const unsigned char sd_dcm_tv_in[1800];
extern const unsigned char sd_dcm_tv_ref[40];
extern uint32_t sd_dcm_tv_cmd[102];
//extern const uint8_t cd_dcm_ACK_LLRs_tv[192];
//extern const uint8_t cd_dcm_CSI1_LLRs_tv[224];
//extern const uint8_t cd_dcm_CSI2_LLRs_tv[288];
//extern const uint8_t cd_dcm_ACK_tv_ref[22];
//extern const uint8_t cd_dcm_CSI1_tv_ref[23];
//extern const uint8_t cd_dcm_CSI2_tv_ref[23];
#endif /*  DCM_ARM_POLAR_TEST_NR */

/* Flag used for QDMA*/
static uint32_t flag;

#endif /* DCM_ARM_POLAR_DECODER_POC_EN */

extern struct bbdev_ipc_op_ldpc_enc bbdev_enc_tv;
extern struct bbdev_ipc_op_ldpc_dec bbdev_dec_tv;
extern uint32_t ce_tv_cmd[10];
extern const uint8_t ce_tv_ref[32];
extern const uint8_t ce_tv_in[4];
extern uint32_t cd_tv_cmd[9];
extern const uint8_t cd_tv_in[90];
extern const uint8_t cd_tv_ref[8];
extern const uint8_t cd_tv_sts[8];

#define FECA_BBDEV_SCAMBLER_BYPASS 1

/** 
 * These two pointers are used by FECA parameter maping APIs. 
 * These two variables required 8KB memory. APIs are allocating 
 * memory in data/bss section, which is limitted. So, we are 
 * allocating heap memory for these variables.
 **/

int *x1;
int *x2;

static inline uint32_t PTR_LO(const void *in)
{
	return (uint32_t) in;
}

static inline uint32_t PTR_HI(const void *in)
{
#if __WORDSIZE == 64
	return (uint32_t) in>>32;
#else
	UNUSED(in);
	return 0;
#endif
}

#ifdef TEST_FECA_BBDEV_DEBUG
/*TODO : This function need to be extended to support all ops */
static void L1C_FECA_bbdev_dump_cmd(struct bbdev_ipc_dequeue_op *tv)
{

	PRINTF("op_flags 	: %x\r\n", tv->ldpc_enc.op_flags);
	PRINTF("rv_index 	: %x\r\n", tv->ldpc_enc.rv_index);
	PRINTF("basegraph	: %x\r\n", tv->ldpc_enc.basegraph);
	PRINTF("z_c		: %x\r\n", tv->ldpc_enc.z_c);
	PRINTF("n_cb		: %x\r\n", tv->ldpc_enc.n_cb);
	PRINTF("q_m		: %x\r\n", tv->ldpc_enc.q_m);
	PRINTF("n_filler	: %x\r\n", tv->ldpc_enc.n_filler);
	PRINTF("code_block_mode	: %x\r\n", tv->ldpc_enc.code_block_mode);
	PRINTF("ea		: %x\r\n", tv->ldpc_enc.tb_params.ea);
	PRINTF("eb		: %x\r\n", tv->ldpc_enc.tb_params.eb);
	PRINTF("c 		: %x\r\n", tv->ldpc_enc.tb_params.c);
	PRINTF("r 		: %x\r\n", tv->ldpc_enc.tb_params.r);
	PRINTF("cab		: %x\r\n", tv->ldpc_enc.tb_params.cab);
}
#endif

#if DCM_ARM_POLAR_DECODER_POC_EN
#if DCM_POLAR_DECODER_DEBUG
static void FECA_DCM_dump_cmd( struct bbdev_ipc_dequeue_op *op )
{
	int i;
	sd_dcm_command_t *sd_dcm_command;
	sd_dcm_command = &op->feca_job.command_chain_t.sd_dcm_command_ch_obj;

	PRINTF("\r\nDump the DCM command\r\n");
	PRINTF("set_index = %d\r\n", sd_dcm_command->sd_cfg1.set_index);
	PRINTF("base_graph2 = %d\r\n",sd_dcm_command->sd_cfg1.base_graph2);
	PRINTF("lifting_index = %d\r\n",sd_dcm_command->sd_cfg1.lifting_index);
	PRINTF("one_code_block = %d\r\n",sd_dcm_command->sd_cfg1.one_code_block);
	PRINTF("data_control_mux = %d\r\n",sd_dcm_command->sd_cfg1.data_control_mux);
	PRINTF("tb_24_bit_crc = %d\r\n",sd_dcm_command->sd_cfg1.tb_24_bit_crc);
	PRINTF("remove_tb_crc) = %d\r\n",sd_dcm_command->sd_cfg1.remove_tb_crc);
	PRINTF("min_num_iter = %d\r\n",sd_dcm_command->sd_cfg1.min_num_iterations);
	PRINTF("max_num_iter = %d\r\n",sd_dcm_command->sd_cfg1.max_num_iterations);

	PRINTF("mod_order = %d\r\n",sd_dcm_command->sd_cfg2.mod_order);
	PRINTF("harq_en = %d\r\n",sd_dcm_command->sd_cfg2.harq_en);
	PRINTF("compact_harq = %d\r\n",sd_dcm_command->sd_cfg2.compact_harq);
	PRINTF("complete_trig_en = %d\r\n",sd_dcm_command->sd_cfg2.complete_trig_en);
	PRINTF("send_msi = %d\r\n",sd_dcm_command->sd_cfg2.send_msi);

	PRINTF("e_floor_thresh = %d\r\n",sd_dcm_command->sd_sizes1.e_floor_thresh);
	PRINTF("num_output_bytes = %d\r\n",sd_dcm_command->sd_sizes1.num_output_bytes);

	PRINTF("bits_per_cb = %d\r\n",sd_dcm_command->sd_sizes2.bits_per_cb);
	PRINTF("num_filler_bits = %d\r\n",sd_dcm_command->sd_sizes2.num_filler_bits);

	PRINTF("sd_circ_buf = 0x%x\r\n",sd_dcm_command->sd_circ_buf);
	PRINTF("sd_floor_num_input_bytes = %d\r\n",sd_dcm_command->sd_floor_num_input_bytes);
	PRINTF("sd_ceiling_num_input_bytes = %d\r\n",sd_dcm_command->sd_ceiling_num_input_bytes);
	PRINTF("sd_hram_base = 0x%x\r\n",sd_dcm_command->sd_hram_base);
	PRINTF("sd_sc_x1_init = 0x%x\r\n",sd_dcm_command->sd_sc_x1_init);
	PRINTF("sd_sc_x2_init = 0x%x\r\n",sd_dcm_command->sd_sc_x2_init);
	PRINTF("sd_axi_data_addr_low = 0x%x\r\n",sd_dcm_command->sd_axi_data_addr_low);
	PRINTF("sd_axi_data_addr_high = 0x%x\r\n",sd_dcm_command->sd_axi_data_addr_high);
	PRINTF("sd_axi_data_num_bytes = %d\r\n",sd_dcm_command->sd_axi_data_num_bytes);
	PRINTF("sd_axi_stat_addr_low = 0x%x\r\n",sd_dcm_command->sd_axi_stat_addr_low);
	PRINTF("sd_axi_stat_addr_high = 0x%x\r\n",sd_dcm_command->sd_axi_stat_addr_high);

	for( i = 0; i < 8; i++ )
	{
		PRINTF("sd_cb_mask[%d] = 0x%x\r\n", i, sd_dcm_command->sd_cb_mask[i]);
	}

	for( i = 0; i < 4; i++ )
	{
		PRINTF("sd_di_start_ofst_ceiling[%d] = 0x%x\r\n", i, sd_dcm_command->sd_di_start_ofst_ceiling[i]);
		PRINTF("sd_di_start_ofst_floor[%d] = 0x%x\r\n", i, sd_dcm_command->sd_di_start_ofst_floor[i]);
	}

	PRINTF("sd_llrs_per_re = %d\r\n",sd_dcm_command->sd_llrs_per_re);

	for( i = 0; i < 14; i++ )
	{
		PRINTF("[%d].sd_n_re_ack_re = 0x%x\r\n", i, sd_dcm_command->demux[i].sd_n_re_ack_re);
		PRINTF("[%d].sd_n_csi1_re_n_csi2_re = 0x%x\r\n", i, sd_dcm_command->demux[i].sd_n_csi1_re_n_csi2_re);
		PRINTF("[%d].sd_n_dlsch_re_d_ack = 0x%x\r\n", i, sd_dcm_command->demux[i].sd_n_dlsch_re_d_ack);
		PRINTF("[%d].sd_d_csi1_d_csi2 = 0x%x\r\n", i, sd_dcm_command->demux[i].sd_d_csi1_d_csi2);
		PRINTF("[%d].sd_d_ack2_ack2_re = 0x%x\r\n", i, sd_dcm_command->demux[i].sd_d_ack2_ack2_re);
	}
}
#endif /* DCM_POLAR_DECODER_DEBUG */

#if DCM_POLAR_DECODER_DEBUG
static void vCallback( void *param, uint32_t LowAddrBase_val )
#else
static void vCallback(void *param, __attribute__((unused))uint32_t LowAddrBase_val)
#endif
{
	int *p = param;
	*p = *p + 1;
#if DCM_POLAR_DECODER_DEBUG
	log_info("LowAddrBase_val %x\r\n",SWAP_32(LowAddrBase_val));
#endif
}

static uint8_t isDdrDcmDecodingDone( feca_ch_type_t ch_type )
{
	BaseType_t dst_addr;
	mod_mem_region_t *ddr_addr_m;
	uint32_t used_size = DCM_SCRATCH_BUFFER_OFFSET - sizeof(ddr_dcm_channel);
	u32 uiCurrentCore = ulMpicCurrentCore();
	uint8_t channel_ddr;
	uint8_t is_decoding = 0;

	if ((ch_type < CH_IN_DCM_ACK) || (ch_type > CH_IN_DCM_CSI2))
	{
		log_err("DCM-QDMA: Wrong channel type\r\n");
		return -1;
	}

	ddr_addr_m = (mod_mem_region_t *) bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
	dst_addr = ddr_addr_m->addr_v + used_size + uiCurrentCore * 0x100000;
	channel_ddr = ioread8((volatile uint8_t*)(dst_addr));

	switch (ch_type)
	{
		case CH_IN_DCM_ACK:
			is_decoding = (channel_ddr & DCM_POLAR_DECODER_ACK);
		break;

		case CH_IN_DCM_CSI1:
			is_decoding = (channel_ddr & DCM_POLAR_DECODER_CSI1);
		break;

		case CH_IN_DCM_CSI2:
			is_decoding = (channel_ddr & DCM_POLAR_DECODER_CSI2);
		break;

		default:
			log_err("DCM-QDMA: Wrong channel type\r\n");
			return -1;
	}
	return !is_decoding;
}

static int DdrDcmDecodingUpdate( feca_ch_type_t ch_type, uint32_t tb_id )
{
	BaseType_t dst_addr;
	mod_mem_region_t *ddr_addr_m;
	uint32_t used_size = DCM_SCRATCH_BUFFER_OFFSET - sizeof(ddr_dcm_channel);;
	u32 uiCurrentCore = ulMpicCurrentCore();
	ddr_dcm_channel arm_polar_decoder;
	uint8_t channel_ddr = 0;

	if ((ch_type < CH_IN_DCM_ACK) || (ch_type > CH_IN_DCM_CSI2))
	{
		log_err("DCM-QDMA: Wrong channel type\r\n");
		return -1;
	}

	ddr_addr_m = (mod_mem_region_t *) bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
	dst_addr = ddr_addr_m->addr_v + used_size + uiCurrentCore * 0x100000;
	channel_ddr = ioread8((volatile uint8_t*)(dst_addr));

	switch (ch_type)
	{
		case CH_IN_DCM_ACK:
			arm_polar_decoder.channel = (channel_ddr | DCM_POLAR_DECODER_ACK);
			arm_polar_decoder.tb_id[0] = tb_id;
			arm_polar_decoder.A[0] = DCM_PAYLOAD_SIZE_ACK;
			arm_polar_decoder.E[0] = DCM_LLRS_SIZE_ACK;
		break;

		case CH_IN_DCM_CSI1:
			arm_polar_decoder.channel = (channel_ddr | DCM_POLAR_DECODER_CSI1);
			arm_polar_decoder.tb_id[1] = tb_id;
			arm_polar_decoder.A[1] = DCM_PAYLOAD_SIZE_CSI1;
			arm_polar_decoder.E[1] = DCM_LLRS_SIZE_CSI1;
		break;

		case CH_IN_DCM_CSI2:
			arm_polar_decoder.channel = (channel_ddr | DCM_POLAR_DECODER_CSI2);
			arm_polar_decoder.tb_id[2] = tb_id;
			arm_polar_decoder.A[2] = DCM_PAYLOAD_SIZE_CSI2;
			arm_polar_decoder.E[2] = DCM_LLRS_SIZE_CSI2;
		break;

		default:
			log_err("DCM-QDMA: Wrong channel type\r\n");
			return -1;
	}

	memcpy((void * restrict)dst_addr, &arm_polar_decoder, sizeof(ddr_dcm_channel));
	return 0;
}

static int vFecaQdmaDcmLlrs( chain_handle_t chain, struct bbdev_ipc_dequeue_op *op, feca_ch_type_t ch_type )
{
	BaseType_t src_addr;
	BaseType_t dst_addr;
	size_t len;
	uint8_t cqm = NXP_QDMA_BCQMR_CQM_AUTO;
	uint32_t loop_count = LOOP_COUNT;
	uint32_t i;
	static int iIsQdmaInitialized;
	uint32_t used_size = DCM_SCRATCH_BUFFER_OFFSET;
	struct feca_chain *chain_cd = (struct feca_chain *)chain;

	if ((ch_type < CH_IN_DCM_ACK) || (ch_type > CH_IN_DCM_CSI2))
	{
		log_err("DCM-QDMA: Wrong channel type\r\n");
		return -1;
	}

	if (chain_cd->chain_type != FECA_CD_CHAIN)
	{
		log_err("DCM-QDMA: Wrong chain type\r\n");
		return -1;
	}

	used_size += (SCRATCH_BUFFER_LLRS_SIZE * (ch_type - CH_IN_DCM_ACK));

	mod_mem_region_t *ddr_addr_m;
	u32 uiCurrentCore = ulMpicCurrentCore();

	len = feca_dcm_ch_in_valid_bytes(chain, ch_type, op->feca_blk_id);

	if (len > SCRATCH_BUFFER_LLRS_SIZE)
	{
		log_err("qDMA:To many LLRs in CBM[%d] to be transfered\r\n",
				feca_get_ch_id(chain, ch_type, op->feca_blk_id /*tb_num*/));
		return -1;
	}
//	log_info("len to be transfered  = %d\r\n", len);
	src_addr = (BaseType_t)feca_get_ch_axi_addr(chain, feca_get_ch_id(chain, ch_type, op->feca_blk_id /*tb_num*/));

	ddr_addr_m = (mod_mem_region_t *) bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
//	log_info(" ddr_addr_m %x, size %x \r\n", ddr_addr_m->addr_v, (uint32_t)ddr_addr_m->size);

	dst_addr = ddr_addr_m->addr_v + used_size + uiCurrentCore * 0x100000;
//	log_info("ddr_addr = 0x%x \r\n", dst_addr);

	if (!iIsQdmaInitialized) {
		if (!QdmaInit(uiCurrentCore, cqm, NXP_QDMA_QUEUE_NUM_MAX,
				0, 0, NULL, NULL))
		{
			log_err("qDMA:Failed to qdma init\r\n");
			return -1;
		}
		for (i = 0; i < NXP_QDMA_QUEUE_NUM_MAX ; i++)
			if (!RegisterCQueueCallback(vCallback, &flag, i))
			{
				log_err("qDMA:Failed to register callback\r\n");
				return -1;
			}
		iIsQdmaInitialized = 1;
	}

	flag = 0;

	DmaUSFFillIssue(dst_addr,
			src_addr,
			len,
			0,
			0);

	log_info("qDMA:Waiting transfer completion of LLRs\r\n");

	while ((flag != 1) && (loop_count > 0))
		loop_count--;

	if (flag != 1) {
		log_err("qDMA:Time-out in transmitting job\r\n");
		return -1;
	}

	vL1DCacheInvLine((uint32_t)dst_addr, len);

#if DCM_POLAR_DECODER_DEBUG
	PRINTF("Read data from DDR\r\n");
	char *tempdstaddr = (char *)dst_addr;
	for (i = 0; i < len; i++) {
		PRINTF("%d. -  %x \r\n", i, *tempdstaddr);
		tempdstaddr++;
	}
#endif
	return 0;

}
#endif /* DCM_ARM_POLAR_DECODER_POC_EN */

int vFecaBBDevDispatchJob(feca_bbdev_test_cb_t *cb, struct bbdev_ipc_dequeue_op *op, feca_job_t *feca_job,
		chain_handle_t chain, uint8_t use_feca_in_dma, uint8_t check_sts)
{
	status_t stat;
	int32_t timeout;

	if (use_feca_in_dma) {
		PRINTF ("%s: DMA mode operations not supported..switching to IO mode\r\n", __func__);
		use_feca_in_dma = 0;
	} 

	cb->cb_out_ptr = feca_get_ch_axi_addr(chain, feca_get_ch_id(chain, CH_OUT, op->feca_blk_id /*tb_num*/));
	if(!cb->cb_out_ptr) {
		pr_err("Can't fetch correct AXI address for OUT buffer");
		return -1;
	}

	cb->cb_in_ptr =  feca_get_ch_axi_addr(chain, feca_get_ch_id(chain, CH_IN, op->feca_blk_id /*tb_num*/));
	if(!cb->cb_in_ptr) {
		pr_err("Can't fetch correct AXI address for IN buffer");
		return -1;
	}

	//feca_job_dispatch(chain, &op->feca_job);

	if (!use_feca_in_dma) {
		uint8_t *input_ptr = (uint8_t *)op->in_addr;
		uint32_t input_size = op->in_len;
		uint32_t data_size_w = input_size >> 2;
		uint32_t data_size_b = input_size & 0x3;
		uint32_t i;

		/* Write 4-byte words of input data */
		//pr_debug("%s: Copy input buffer. w size[%d] b_size %d\n", __func__,data_size_w * 4, data_size_b);
		for (i = 0; i < data_size_w; ++i, input_ptr += 4) {
			iowrite32be_fast(*(uint32_t *)input_ptr, cb->cb_in_ptr);
		//	fsl_print("%x: ", *(uint32_t *)input_ptr);
		}

		/* Write remainder bytes */
		for (i = 0; i < data_size_b; ++i, input_ptr += 1) {
			iowrite8(*input_ptr, (volatile uint8_t *)cb->cb_in_ptr);
			//fsl_print("%x: ", *input_ptr);
		}
		core_memory_barrier();

		//            tman_get_timestamp(&tparam->ts_din_done);
	}
	
        //pr_debug("CHECK : Job status %d\n", feca_get_job_status(chain, &op->feca_job));
	stat = feca_job_dispatch(chain, feca_job);
	if (stat != FECA_SUCCESS)
		PRINTF("feca_job_dispatch  ===>Failed\r\n");

	if (check_sts) {
		timeout = 100;
		while ((stat = feca_get_job_status(chain, feca_job)) != FECA_SUCCESS)
		{
			if (!timeout--)
				break;
		}
	}
	else
		stat = FECA_SUCCESS;

	if (stat != FECA_SUCCESS)
	{
		PRINTF("FECA job failed .............!");
		return -1;
	}

	return 0;	
}

int vFecaBBDevPrepareSDJob(feca_bbdev_test_cb_t *cb, struct bbdev_ipc_dequeue_op *op,
			   feca_job_t *feca_job, void *op_data)
{
	int *e; /* e: array of code block lengths */
	int32_t  tbs_valid = -1, rc = 0, ii, jj;	
	int32_t set_index, base_graph2, lifting_index, mod_order, harq_buf_sz;
	int32_t tb_24_bit_crc, num_code_blocks, num_output_bytes, e_floor_thresh;
	uint32_t *codeblock_mask, bits_per_cb, axi_data_num_bytes, num_filler_bits, one_code_block;
	int ofst_floor[8], ofst_ceiling[8];
	struct bbdev_ipc_op_ldpc_dec *ldpc_dec = (struct bbdev_ipc_op_ldpc_dec *)op_data;
		
#ifdef TEST_FECA_BBDEV_DEBUG
	L1C_FECA_bbdev_dump_cmd(op);
#endif

	e = cb->e;

	memset(feca_job, 0, sizeof(*feca_job));

	/** TODO : right now codeblock_mask is set all 1 statically. 
	 *	building mask dynamically need to be enabled after UT.
	 **/
	codeblock_mask = (uint32_t *)&feca_job->command_chain_t.sd_command_ch_obj.sd_cb_mask[0];
	memset (codeblock_mask, 0, 32);	
	for (ii = ldpc_dec->tb_params.r, jj = 0; ii < ldpc_dec->tb_params.c; ii ++) {
		codeblock_mask[ii/32] |= (1 << (ii % 32));
		if (ii < ldpc_dec->tb_params.cab)
			e[jj++] = ldpc_dec->tb_params.ea;
 		else
			e[jj++] = ldpc_dec->tb_params.eb;
	}

	sch_decode_hw_param_du( ldpc_dec->basegraph,
			ldpc_dec->q_m,
			(int *)e,
			ldpc_dec->rv_index,
			op->out_len * 8, /* A: it has to be sum of SG lengths */
			/** q, n_ID, n_RNTI are scrammbler inputs and set them to zeros **/
			0, /* q */
			4, /* n_ID */
			65295, /* n_RTNI */ 
			FECA_BBDEV_SCAMBLER_BYPASS, /* scrambler_bypass */
			ldpc_dec->n_cb,
			1, /* crc removal */
			(int *)&harq_buf_sz,	
			(int *)&tbs_valid,
			(int *)&num_code_blocks,
			(int *)codeblock_mask, 
			/* output parameters */
			(int *)&set_index,
			(int *)&base_graph2,
			(int *)&lifting_index,
			(int *)&mod_order,
			(int *)&tb_24_bit_crc,
			(int *)&one_code_block,
			(int *)&e_floor_thresh,
			(int *)&num_output_bytes,
			(int *)&bits_per_cb,
			(int *)&num_filler_bits,
			(int *)&feca_job->command_chain_t.sd_command_ch_obj.sd_sc_x1_init,
			(int *)&feca_job->command_chain_t.sd_command_ch_obj.sd_sc_x2_init,
			(int *)&feca_job->command_chain_t.sd_command_ch_obj.sd_floor_num_input_bytes,
			(int *)&feca_job->command_chain_t.sd_command_ch_obj.sd_ceiling_num_input_bytes,
			(int *)&ofst_floor[0],
			(int *)&ofst_ceiling[0],
			(int *)&feca_job->command_chain_t.sd_command_ch_obj.sd_circ_buf,
			(int *)&axi_data_num_bytes);

			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[0] = 
				((uint16_t)ofst_floor[1] << 16) | (uint16_t)ofst_floor[0];
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[1] = 
				((uint16_t)ofst_floor[3] << 16) | (uint16_t)ofst_floor[2];
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[2] = 
				((uint16_t)ofst_floor[5] << 16) | (uint16_t)ofst_floor[4];
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[3] = 
				((uint16_t)ofst_floor[7] << 16) | (uint16_t)ofst_floor[6];

			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[0] = 
				((uint16_t)ofst_ceiling[1] << 16) | (uint16_t)ofst_ceiling[0];
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[1] = 
				((uint16_t)ofst_ceiling[3] << 16) | (uint16_t)ofst_ceiling[2];
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[2] = 
				((uint16_t)ofst_ceiling[5] << 16) | (uint16_t)ofst_ceiling[4];
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[3] = 
				((uint16_t)ofst_ceiling[7] << 16) | (uint16_t)ofst_ceiling[6];

#ifdef TEST_FECA_BBDEV_DEBUG			
	PRINTF("INPUTS \r\n");

	PRINTF("base_graph2_input= %d\r\nQ_m = %d\r\ne[0]:e[1]=%d:%d\r\nrv_id=%d\r\nA= %d\r\nq=%d\r\nn_ID=%d\r\nn_RTNI=%d\r\nscrambler_bypass=%d\r\nN_cb=%d\r\n codeblock_mask[]=%x:%x:%x:%x:%x:%x:%x:%x\r\n", ldpc_dec->basegraph, ldpc_dec->q_m, e[0], e[1], ldpc_dec->rv_index, op->sgSrcTableHead[0].DataLen * 8, 0, 4, 65295, FECA_BBDEV_SCAMBLER_BYPASS, ldpc_dec->n_cb, codeblock_mask[0],codeblock_mask[1],codeblock_mask[2],codeblock_mask[3],codeblock_mask[4],codeblock_mask[5],codeblock_mask[6],codeblock_mask[7]);

	PRINTF("OUTPUT \r\n");
	PRINTF(" set_index=%d\r\n base_graph2=%d\r\nlifting_index=%d\r\nmod_order=%d\r\ntb_24_bit_crc=%d\r\nnum_code_blocks=%d\r\none_code_block:%d\r\nnum_output_bytes=%d\r\n_floor_thresh=%d\r\nse_floor_num_output_bits=%d\r\nse_ceiling_num_output_bits=%d\r\nse_sc_x1_init=%d\r\nse_sc_x2_init=%d\r\nse_di_start_ofst_floor=%d:%d:%d:%d\r\nse_di_start_ofst_ceiling=%d:%d:%d:%d\r\nse_circ_buf=%d\r\naxi_data_len:%d\r\n",
			set_index,
			base_graph2,
			lifting_index,
			mod_order,
			tb_24_bit_crc,
			num_code_blocks,
			one_code_block,
			num_output_bytes,
			e_floor_thresh,
			feca_job->command_chain_t.sd_command_ch_obj.sd_floor_num_input_bytes,
			feca_job->command_chain_t.sd_command_ch_obj.sd_ceiling_num_input_bytes,
			feca_job->command_chain_t.sd_command_ch_obj.sd_sc_x1_init,
			feca_job->command_chain_t.sd_command_ch_obj.sd_sc_x2_init,
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[0],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[1],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[2],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_floor[3],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[0],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[1],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[2],
			feca_job->command_chain_t.sd_command_ch_obj.sd_di_start_ofst_ceiling[3],
			feca_job->command_chain_t.sd_command_ch_obj.sd_circ_buf,
			axi_data_num_bytes);

#endif

	feca_job->command_chain_t.sd_command_ch_obj.sd_cfg1.raw_sd_cfg1 = 
				set_index | (base_graph2 << 3) | (lifting_index << 4) | 
				(one_code_block << 7) | (tb_24_bit_crc << 14) | (1 << 15) |
				(ldpc_dec->iter_count << 16) | (ldpc_dec->iter_max << 24); 
	
	feca_job->command_chain_t.sd_command_ch_obj.sd_cfg2.raw_sd_cfg2 = 
				mod_order | (1 << 12); /* b'12 to enable complete trigger */

	feca_job->command_chain_t.sd_command_ch_obj.sd_sizes1.raw_sd_sizes1 = 
					(num_output_bytes << 16) | (e_floor_thresh);
	feca_job->command_chain_t.sd_command_ch_obj.sd_sizes2.raw_sd_sizes2 = 
					(num_filler_bits << 16) | (bits_per_cb);


#ifdef TEST_FECA_BBDEV_DEBUG
	{
		uint32_t *dump_ptr = (uint32_t *)&feca_job->command_chain_t.sd_command_ch_obj;
		int kk;

		memcpy(dump_ptr, sd_t2_cmd, sizeof(feca_job->command_chain_t.sd_command_ch_obj));
		PRINTF("\r\n COMMAND \r\n ");
		for (kk = 0; kk < sizeof(feca_job->command_chain_t.sd_command_ch_obj)/4; )
		{
			PRINTF("%08x ", dump_ptr[kk]);
			kk++;
			if (!(kk%4))
				PRINTF("\r\n");
		}
	}
#endif

	feca_job->command_chain_t.sd_command_ch_obj.sd_axi_data_addr_low = PTR_LO((void *)op->out_addr);
	feca_job->command_chain_t.sd_command_ch_obj.sd_axi_data_addr_high = PTR_HI((void *)op->out_addr);
	feca_job->command_chain_t.sd_command_ch_obj.sd_axi_data_num_bytes = op->out_len; 
	feca_job->command_chain_t.sd_command_ch_obj.sd_axi_stat_addr_low = PTR_LO((void *)cb->feca_sts_ptr);	
	feca_job->command_chain_t.sd_command_ch_obj.sd_axi_stat_addr_high = PTR_HI((void *)cb->feca_sts_ptr);
	feca_job->job_type = FECA_JOB_SD;
	feca_job->t_blk_id = 0;

	return rc;
}

int vFecaBBDevPrepareSEJob(feca_bbdev_test_cb_t *cb, struct bbdev_ipc_dequeue_op *op,
			   feca_job_t *feca_job, void *op_data)
{
	BaseType_t input_addr_lo, input_addr_hi;
	int *e; /* e: array of code block lengths */
	int32_t  tbs_valid = -1, rc = 0, ii, jj;	
	int32_t set_index, base_graph2, lifting_index, mod_order;
	int32_t tb_24_bit_crc, num_code_blocks, num_input_bytes, e_floor_thresh;
	uint32_t *codeblock_mask;
	int ofst_floor[8], ofst_ceiling[8];
	struct bbdev_ipc_op_ldpc_enc *ldpc_enc = (struct bbdev_ipc_op_ldpc_enc *)op_data;
		
	input_addr_lo = op->in_addr;
	input_addr_hi = 0;

#ifdef TEST_FECA_BBDEV_DEBUG
	L1C_FECA_bbdev_dump_cmd(op);
#endif

	e = cb->e;
	memset(e, 0, ldpc_enc->tb_params.c * sizeof(int32_t));

	memset(feca_job, 0, sizeof(*feca_job));

	/** TODO : right now codeblock_mask is set all 1 statically. 
	 *	building mask dynamically need to be enabled after UT.
	 **/
	codeblock_mask = (uint32_t *)&feca_job->command_chain_t.se_command_ch_obj.se_cb_mask[0];
	memset (codeblock_mask, 0xff, 32);	
	for (ii = ldpc_enc->tb_params.r, jj = 0; ii < ldpc_enc->tb_params.c; ii ++) {
		//codeblock_mask[ii/32] |= (1 << (ii % 32));
		if (ii < ldpc_enc->tb_params.cab)
			e[jj++] = ldpc_enc->tb_params.ea;
 		else
			e[jj++] = ldpc_enc->tb_params.eb;
	}

	sch_encode_hw_param_du( ldpc_enc->basegraph,
			ldpc_enc->q_m,
			(int *)e,
			ldpc_enc->rv_index,
			op->in_len * 8, /* A: it has to be sum of SG lengths */
			/** q, n_ID, n_RNTI are scrammbler inputs and set them to zeros **/
			0, /* q */
			4, /* n_ID */
			65295, /* n_RTNI */ 
			FECA_BBDEV_SCAMBLER_BYPASS, /* scrambler_bypass */
			ldpc_enc->n_cb,
			(int *)codeblock_mask, 
			(int *)&tbs_valid,
			/* output parameters */
			(int *)&set_index,
			(int *)&base_graph2,
			(int *)&lifting_index,
			(int *)&mod_order,
			(int *)&tb_24_bit_crc,
			(int *)&num_code_blocks,
			(int *)&num_input_bytes,
			(int *)&e_floor_thresh,
			(int *)&feca_job->command_chain_t.se_command_ch_obj.se_floor_num_output_bits,
			(int *)&feca_job->command_chain_t.se_command_ch_obj.se_ceiling_num_output_bits,
			(int *)&feca_job->command_chain_t.se_command_ch_obj.se_sc_x1_init,
			(int *)&feca_job->command_chain_t.se_command_ch_obj.se_sc_x2_init,
			(int *)&ofst_floor[0],
			(int *)&ofst_ceiling[0],
			(int *)&feca_job->command_chain_t.se_command_ch_obj.se_circ_buf);

			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[0] = 
				((uint16_t)ofst_floor[1] << 16) | (uint16_t)ofst_floor[0];
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[1] = 
				((uint16_t)ofst_floor[3] << 16) | (uint16_t)ofst_floor[2];
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[2] = 
				((uint16_t)ofst_floor[5] << 16) | (uint16_t)ofst_floor[4];
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[3] = 
				((uint16_t)ofst_floor[7] << 16) | (uint16_t)ofst_floor[6];

			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[0] = 
				((uint16_t)ofst_ceiling[1] << 16) | (uint16_t)ofst_ceiling[0];
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[1] = 
				((uint16_t)ofst_ceiling[3] << 16) | (uint16_t)ofst_ceiling[2];
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[2] = 
				((uint16_t)ofst_ceiling[5] << 16) | (uint16_t)ofst_ceiling[4];
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[3] = 
				((uint16_t)ofst_ceiling[7] << 16) | (uint16_t)ofst_ceiling[6];

#ifdef TEST_FECA_BBDEV_DEBUG			
	PRINTF("INPUTS \r\n");

	PRINTF("base_graph2_input= %d\r\nQ_m = %d\r\ne[0]:e[1]=%d:%d\r\nrv_id=%d\r\nA= %d\r\nq=%d\r\nn_ID=%d\r\nn_RTNI=%d\r\nscrambler_bypass=%d\r\nN_cb=%d\r\n codeblock_mask[]=%x:%x:%x:%x:%x:%x:%x:%x\r\n", ldpc_enc->basegraph, ldpc_enc->q_m, e[0], e[1], ldpc_enc->rv_index, op->sgSrcTableHead[0].DataLen * 8, 0, 4, 65295, FECA_BBDEV_SCAMBLER_BYPASS, ldpc_enc->n_cb, codeblock_mask[0],codeblock_mask[1],codeblock_mask[2],codeblock_mask[3],codeblock_mask[4],codeblock_mask[5],codeblock_mask[6],codeblock_mask[7]);

	PRINTF("OUTPUT \r\n");
	PRINTF(" set_index=%d\r\n base_graph2=%d\r\nlifting_index=%d\r\nmod_order=%d\r\ntb_24_bit_crc=%d\r\nnum_code_blocks=%d\r\nnum_input_bytes=%d\r\n_floor_thresh=%d\r\nse_floor_num_output_bits=%d\r\nse_ceiling_num_output_bits=%d\r\nse_sc_x1_init=%d\r\nse_sc_x2_init=%d\r\nse_di_start_ofst_floor=%d:%d:%d:%d\r\nse_di_start_ofst_ceiling=%d:%d:%d:%d\r\nse_circ_buf=%d\r\n",
			set_index,
			base_graph2,
			lifting_index,
			mod_order,
			tb_24_bit_crc,
			num_code_blocks,
			num_input_bytes,
			e_floor_thresh,
			feca_job->command_chain_t.se_command_ch_obj.se_floor_num_output_bits,
			feca_job->command_chain_t.se_command_ch_obj.se_ceiling_num_output_bits,
			feca_job->command_chain_t.se_command_ch_obj.se_sc_x1_init,
			feca_job->command_chain_t.se_command_ch_obj.se_sc_x2_init,
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[0],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[1],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[2],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_floor[3],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[0],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[1],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[2],
			feca_job->command_chain_t.se_command_ch_obj.se_di_start_ofst_ceiling[3],
			feca_job->command_chain_t.se_command_ch_obj.se_circ_buf);

#endif

	feca_job->command_chain_t.se_command_ch_obj.se_cfg1.raw_se_cfg1 = 
				set_index | (base_graph2 << 3) | (lifting_index << 4) | 
				(mod_order << 8) | (1 << 12) /*complete_trig_en*/ | 
				(tb_24_bit_crc << 14) | (num_code_blocks << 16); 



	feca_job->command_chain_t.se_command_ch_obj.se_sizes1.raw_se_sizes1 = 
					(num_input_bytes << 16) | (e_floor_thresh);

#ifdef TEST_FECA_BBDEV_DEBUG
	{
		uint32_t *dump_ptr = (uint32_t *)&feca_job->command_chain_t.se_command_ch_obj;
		int kk;

		PRINTF("\r\n COMMAND \r\n ");
		for (kk = 0; kk < sizeof(feca_job->command_chain_t.se_command_ch_obj)/4; )
		{
			PRINTF("%08x ", dump_ptr[kk]);
			kk++;
			if (!(kk%4))
				PRINTF("\r\n");
		}
	}
#endif

	feca_job->command_chain_t.se_command_ch_obj.se_axi_in_addr_low = input_addr_lo;	
	feca_job->command_chain_t.se_command_ch_obj.se_axi_in_addr_high = input_addr_hi;
	feca_job->command_chain_t.se_command_ch_obj.se_axi_in_num_bytes = 0; 
	feca_job->job_type = FECA_JOB_SE;
	feca_job->t_blk_id = 0;

	return rc;
}

static int vFecaBBDevPrepareCDJob(feca_bbdev_test_cb_t *cb, feca_job_t *feca_job)
{
	uint32_t *command;
		
	command = cd_tv_cmd;
	// complete_trig_en bit also enables/disables CRC output
	command[0] = (command[0] | (1 << 12));
	command[3] = PTR_LO((void *)cb->feca_out_ptr);
	command[4] = PTR_HI((void *)cb->feca_out_ptr);
	command[5] = PTR_LO((void *)cb->feca_sts_ptr);
	command[6] = PTR_HI((void *)cb->feca_sts_ptr);
	memset(feca_job, 0, sizeof(*feca_job));
	feca_job->job_type = FECA_JOB_CD;//TBD if this param is needed
	feca_job->t_blk_id = 0;
	memcpy(&feca_job->command_chain_t.cd_command_ch_obj, command, sizeof(cd_tv_cmd));

	return 0;
}

static int vFecaBBDevPrepareCEJob(feca_job_t *feca_job)
{
	uint32_t *command;
		
	command = ce_tv_cmd;
	// complete_trig_en bit also enables/disables CRC output
	command[0] = (command[0] | (1 << 12));
	command[4] = PTR_LO(ce_tv_in);
	command[5] = PTR_HI(ce_tv_in);
	memset(feca_job, 0, sizeof(*feca_job));
	feca_job->job_type = FECA_JOB_CE;//TBD if this param is needed
	feca_job->t_blk_id = 0;
	memcpy(&feca_job->command_chain_t.ce_command_ch_obj, command, sizeof(ce_tv_cmd));

	return 0;
}
#if DCM_ARM_POLAR_DECODER_POC_EN
static int vFecaBBDevPrepareSD_DCMJob(feca_bbdev_test_cb_t *cb, struct bbdev_ipc_dequeue_op *op,
				      feca_job_t *feca_job)
{
	uint32_t *command;

	command = sd_dcm_tv_cmd;

	memset(feca_job, 0, sizeof(*feca_job));
	memcpy(&feca_job->command_chain_t.sd_dcm_command_ch_obj, command, sizeof(sd_dcm_command_t));

	feca_job->command_chain_t.sd_dcm_command_ch_obj.sd_axi_data_addr_low = PTR_LO((void *)op->out_addr);
	feca_job->command_chain_t.sd_dcm_command_ch_obj.sd_axi_data_addr_high = PTR_HI((void *)op->out_addr);
	feca_job->command_chain_t.sd_dcm_command_ch_obj.sd_axi_data_num_bytes = op->out_len;
	feca_job->command_chain_t.sd_dcm_command_ch_obj.sd_axi_stat_addr_low = PTR_LO((void *)cb->feca_sts_ptr);
	feca_job->command_chain_t.sd_dcm_command_ch_obj.sd_axi_stat_addr_high = PTR_HI((void *)cb->feca_sts_ptr);

	feca_job->job_type = FECA_JOB_SD_DCM;//TBD if this param is needed
	feca_job->t_blk_id = 1;
#if DCM_POLAR_DECODER_DEBUG
	FECA_DCM_dump_cmd(op);
#endif
	return 0;
}
#endif /* DCM_ARM_POLAR_DECODER_POC_EN */

static int L1C_FECA_bbdev_test(struct feca_bbdev_test_cb *cb, int test)
{
	struct bbdev_ipc_dequeue_op *bbdev_op;
	feca_job_t *feca_job;
	uint32_t ii;
	int timeout = 0;

	bbdev_op =  fsl_malloc(sizeof (struct bbdev_ipc_dequeue_op), 0x10);
	if (!bbdev_op) {
		PRINTF("%s:%d: failed allocate memory \r\n", __func__, __LINE__);
		return -1;
	}

	feca_job =  fsl_malloc(sizeof (feca_job_t), 0x10);
	if (!feca_job) {
		PRINTF("%s:%d: failed allocate memory \r\n", __func__, __LINE__);
		return -1;
	}

	switch (test)
	{
		case FECA_TEST_CD:
			bbdev_op->in_addr = PTR_LO((void *)cd_tv_in);
			bbdev_op->in_len = sizeof (cd_tv_in);

			bbdev_op->out_addr = (uint32_t)g_feca_bbdev_cb.feca_out_ptr;
			bbdev_op->out_len = sizeof (cd_tv_ref);
			memset((void *)bbdev_op->out_addr, 0,  bbdev_op->out_len);	
			
			if (!vFecaBBDevPrepareCDJob(&g_feca_bbdev_cb, feca_job)) {
				uint8_t *out_data = (uint8_t *)bbdev_op->out_addr;
				uint32_t out_len = bbdev_op->out_len;

				if (!vFecaBBDevDispatchJob(&g_feca_bbdev_cb, bbdev_op, feca_job,
							cb->cd_chain, 0, 1)) {

					for ( ii = 0; ii < out_len; ii++){
						if ( ioread8((volatile uint8_t *)(out_data + ii)) !=  
								cd_tv_ref[ii]) {
							PRINTF ("Failed to match vector data %d: %x:%x\r\n",								ii, out_data[ii], cd_tv_ref[ii]);
							break;		
						}
					}

					if (ii == out_len)
						PRINTF("\r\n!!!CD Test vector successfully matched !!!\r\n");
				}
				else
					PRINTF("%s: Job execution failed \r\n", __func__);
			}	
			break;

		case FECA_TEST_CE:
			bbdev_op->in_addr = PTR_LO((void *)ce_tv_in);
			bbdev_op->in_len = sizeof (ce_tv_in);
			bbdev_op->out_addr = (uint32_t)g_feca_bbdev_cb.feca_out_ptr;
			bbdev_op->out_len = sizeof (ce_tv_ref);
			memset((void *)bbdev_op->out_addr, 0,  bbdev_op->out_len);	
			if (!vFecaBBDevPrepareCEJob(feca_job)) {
				uint8_t *out_data = (uint8_t *)bbdev_op->out_addr;
				uint32_t out_len = bbdev_op->out_len;

				if (!vFecaBBDevDispatchJob(&g_feca_bbdev_cb, bbdev_op, feca_job,
							cb->ce_chain, 0, 1)) {

					uint32_t ref_sz_w = out_len / 4;

					//pr_info("%s:ref_sz %d ref_sz_w %d\n", __func__, out_len, ref_sz_w);
					for (ii = 0; ii < ref_sz_w; ++ii) {
						/* Copy from CB to Local Memory */
						*(uint32_t *)(&out_data[ii<<2]) = ioread32be((const volatile uint32_t *)(cb->cb_out_ptr));
						//out_data[ii] = ioread32be((const volatile uint32_t *)(cb->cb_out_ptr + ii));
						//		pr_info("\r\n0x%08x ", out_data[ii]);
					}

					for ( ii = 0; ii < out_len; ii++){
						if ( out_data[ii] !=  ce_tv_ref[ii]) {
							PRINTF ("Failed to match vector data %d: %x:%x\r\n",								ii, out_data[ii], ce_tv_ref[ii]);
							break;		
						}
					}

					if (ii == out_len)
						PRINTF("\r\n!!!CE Test vector successfully matched !!!\r\n");
				}
				else
					PRINTF("%s: Job execution failed \r\n", __func__);

			}
			break;

		case FECA_TEST_SD:
			bbdev_op->in_addr = PTR_LO((void *)sd_data_input_tv);
			bbdev_op->in_len = sizeof (sd_data_input_tv);

			bbdev_op->out_addr = (uint32_t)g_feca_bbdev_cb.feca_out_ptr;
			bbdev_op->out_len = sizeof (sd_data_output_tv);
			memset((void *)bbdev_op->out_addr, 0,  bbdev_op->out_len);	

			if (!vFecaBBDevPrepareSDJob(&g_feca_bbdev_cb, bbdev_op, feca_job, &bbdev_dec_tv)) {
				uint8_t *out_data = (uint8_t *)bbdev_op->out_addr;
				uint32_t out_len = bbdev_op->out_len;
				if (!vFecaBBDevDispatchJob(&g_feca_bbdev_cb, bbdev_op, feca_job,
							cb->sd_chain, 0, 1)) {


					for ( ii = 0; ii < out_len; ii++){
						if ( ioread8((volatile uint8_t *)(out_data + ii)) !=  sd_data_output_tv[ii]) {
							PRINTF ("Failed to match vector data %d: %x:%x\r\n",								ii, out_data[ii], sd_data_output_tv[ii]);
							break;		
						}
					}

					if (ii == out_len)
						PRINTF("\r\n!!!SD Test vector successfully matched !!!\r\n");
				}

			}
			else
				PRINTF("%s: Job execution failed \r\n", __func__);
			break;


		case FECA_TEST_SE:
			bbdev_op->in_addr = PTR_LO((void *)se_data_input_tv);
			bbdev_op->in_len = sizeof (se_data_input_tv);


			//			bbdev_op->sgDstTableHead[0].LowAddrBase = PTR_LO((void *)g_feca_bbdev_cb.feca_out_ptr);
			//			bbdev_op->sgDstTableHead[0].HighAddrBase = PTR_HI((void *)g_feca_bbdev_cb.feca_out_ptr);
			//			bbdev_op->sgDstTableHead[0].DataLen = sizeof (se_data_output_tv);
			bbdev_op->out_addr = (uint32_t)g_feca_bbdev_cb.feca_out_ptr;
			bbdev_op->out_len = sizeof (se_data_output_tv);

			if (!vFecaBBDevPrepareSEJob(&g_feca_bbdev_cb, bbdev_op, feca_job, &bbdev_enc_tv)) {
				uint8_t *out_data = (uint8_t *)bbdev_op->out_addr;
				uint32_t out_len = bbdev_op->out_len;
				if (!vFecaBBDevDispatchJob(&g_feca_bbdev_cb, bbdev_op, feca_job,
							cb->se_chain, 0, 1)) {

					uint32_t ref_sz_w = out_len / 4;

					//	pr_info("%s:ref_sz %d ref_sz_w %d\n", __func__, out_len, ref_sz_w);
					for (ii = 0; ii < ref_sz_w; ++ii) {
						/* Copy from CB to Local Memory */
						*(uint32_t *)(&out_data[ii<<2]) = ioread32be((const volatile uint32_t *)(cb->cb_out_ptr));
						//out_data[ii] = ioread32be((const volatile uint32_t *)(cb->cb_out_ptr + ii));
						//		pr_info("\r\n0x%08x ", out_data[ii]);
					}

					for ( ii = 0; ii < out_len; ii++){
						if ( out_data[ii] !=  se_data_output_tv[ii]) {
							PRINTF ("Failed to match vector data %d: %x:%x\r\n",								ii, out_data[ii], se_data_output_tv[ii]);
							break;		
						}
					}
					
					if (ii == out_len)
						PRINTF("\r\n!!!SE Test vector successfully matched !!!\r\n");
				}

			}
			else
				PRINTF("%s: Job execution failed \r\n", __func__);

			break;
#if DCM_ARM_POLAR_DECODER_POC_EN
		case FECA_TEST_DCM_ARM:

			for (int channel_dcm = CH_IN_DCM_ACK; channel_dcm <= CH_IN_DCM_CSI2; channel_dcm++)
			{
				timeout = 10000;
				uint32_t cb_valid_bytes, arm_decoding_is_done;
				while(--timeout)
				{
					cb_valid_bytes = feca_dcm_ch_in_valid_bytes(cb->cd_chain, channel_dcm,
											feca_job->t_blk_id);
					arm_decoding_is_done = isDdrDcmDecodingDone(channel_dcm);
					if(((int)cb_valid_bytes == -1) || ((int)arm_decoding_is_done == -1))
					{
						fsl_free(bbdev_op);
						fsl_free(feca_job);
						return -1;
					} else if ((cb_valid_bytes == 0) && arm_decoding_is_done)
						break;
				}
				if (timeout == 0)
				{
					log_err("\r\nARM Polar decoding is still pending - TIMEOUT\r\n");
					fsl_free(bbdev_op);
					fsl_free(feca_job);
					return -1;
				}
			}
			bbdev_op->in_addr = PTR_LO((void *)sd_dcm_tv_in);
			bbdev_op->in_len = sizeof (sd_dcm_tv_in);

			bbdev_op->out_addr = (uint32_t)g_feca_bbdev_cb.feca_out_ptr;
			bbdev_op->out_len = sizeof (sd_dcm_tv_ref);
			memset((void *)bbdev_op->out_addr, 0,  bbdev_op->out_len);

			if (!vFecaBBDevPrepareSD_DCMJob(&g_feca_bbdev_cb, bbdev_op, feca_job))
			{
				uint8_t *out_data = (uint8_t *)bbdev_op->out_addr;
				uint32_t out_len = bbdev_op->out_len;
				if (!vFecaBBDevDispatchJob(&g_feca_bbdev_cb, bbdev_op, feca_job, cb->sd_chain, 0, 1))
				{
					for ( ii = 0; ii < out_len; ii++)
					{
						if ( ioread8((volatile uint8_t *)(out_data + ii)) !=  sd_dcm_tv_ref[ii])
						{
							PRINTF ("Failed to match vector data %d: %x:%x\r\n", 
									ii, out_data[ii], sd_dcm_tv_ref[ii]);
							break;
						}
					}
					if (ii == out_len)
						PRINTF("\r\n!!!SD-DCM LDPC Test vector successfully matched !!!\r\n");
					else
						PRINTF("\r\n!!!SD-DCM LDPC failed decoding!!!\r\n");
				}

			} else {
				PRINTF("%s: Job execution failed \r\n", __func__);
			}

			/* QDMA de-multiplxed LLRs in DDR to be decoded by ARM Polar Decoder*/
			for (int channel_dcm = CH_IN_DCM_ACK; channel_dcm <= CH_IN_DCM_CSI2; channel_dcm++)
				if(feca_dcm_ch_in_valid_bytes(cb->cd_chain, channel_dcm, bbdev_op->feca_blk_id))
				{
					if(vFecaQdmaDcmLlrs(cb->cd_chain, bbdev_op, channel_dcm))
						break;
					DdrDcmDecodingUpdate(channel_dcm, bbdev_op->feca_blk_id);
				}
			break;
#endif
		default:
			PRINTF("%s:%d: Invalid  test\r\n", __func__, __LINE__);
	}

	fsl_free(bbdev_op);
	fsl_free(feca_job);

	return 0;
}

void vFecaBBDevTestCE()
{
	u32 core_id = ulMpicCurrentCore();

	if (core_id == 0) {
		vFecaBBDevInit();
		L1C_FECA_bbdev_test(&g_feca_bbdev_cb, FECA_TEST_CE);
		SET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	}
	else
		RESET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	return;
}

void vFecaBBDevTestCD()
{
	u32 core_id = ulMpicCurrentCore();

	if (core_id == 0) {
		vFecaBBDevInit();
		L1C_FECA_bbdev_test(&g_feca_bbdev_cb, FECA_TEST_CD);
		SET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	}
	else
		RESET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	return;
}

void vFecaBBDevTestSE()
{
	u32 core_id = ulMpicCurrentCore();

	if (core_id == 0) {
		vFecaBBDevInit();
		L1C_FECA_bbdev_test(&g_feca_bbdev_cb, FECA_TEST_SE);
		SET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	}
	else
		RESET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);

	return;
}

void vFecaBBDevTestSD()
{
	u32 core_id = ulMpicCurrentCore();

	if (core_id == 0) {
		vFecaBBDevInit();
		L1C_FECA_bbdev_test(&g_feca_bbdev_cb, FECA_TEST_SD);
		SET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	}
	else
		RESET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);

	return;
}

#if DCM_ARM_POLAR_DECODER_POC_EN
void vFecaBBDevTestDCM_ARM()
{
	u32 core_id = ulMpicCurrentCore();

	if (core_id == 0) {
		vFecaBBDevInit();
		L1C_FECA_bbdev_test(&g_feca_bbdev_cb, FECA_TEST_DCM_ARM);
		SET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	} else {
		RESET_TEST_STATUS(core_id, GEUL_DEMO_FECA_BBDEV_TEST_STATUS);
	}
	return;
}
#endif /* DCM_ARM_POLAR_DECODER_POC_EN */

int vFecaBBDevInit()
{
	int ii;
	chain_param_t chain_param;
	ch_param_t ch_param;
	feca_bbdev_test_cb_t *cb = &g_feca_bbdev_cb;

	if (g_feca_bbdev_cb.feca_bbdev_initialized) {
//		PRINTF("%s: Initialization done\r\n", __func__);
		goto done;
	}

	/* Allocate to memory for x1 and x2 used by FECA LIB */

#ifndef FECA_BBDEV_SCAMBLER_BYPASS
	x1 = pvGeulMalloc(100000);
	ASSERT_COND(x1 !=NULL);

	PRINTF("%s:%d\r\n", __func__, __LINE__);
	x2 = pvGeulMalloc(100000);
	ASSERT_COND(x1 !=NULL);
#endif 

	cb->feca_dev = feca_dev_open(FECA_DEV_NAME, NULL);

	ASSERT_COND(cb->feca_dev !=NULL);
	
	//pr_info("****** FECA CONTROL DECODE CHAIN INIT ******\r\n");
	/* Open CD chain */
	chain_param.type = FECA_CD_CHAIN;
	chain_param.irq_mask = 0;
	chain_param.dcm_irq_mask = 0;

	cb->cd_chain = feca_chain_open(cb->feca_dev, chain_param, NULL);
	ASSERT_COND(cb->cd_chain!=NULL);

	/* Open all 4 channel of CD */
	ch_param.ch_size		= sizeof(cd_command_t);
	ch_param.dma_disable	= 0;
	ch_param.ext_dma		= 0;
	ch_param.type		= CH_CMD;
	ch_param.xfer_size		= 0;
	ch_param.tb_num		= 0;
	cb->cd_ch[CH_CMD] = feca_ch_open(cb->cd_chain, ch_param);
	ASSERT_COND(cb->cd_ch[CH_CMD] !=NULL);

	ch_param.ch_size	= FECA_CD_INPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_IN;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	cb->cd_ch[CH_IN] = feca_ch_open(cb->cd_chain, ch_param);
	ASSERT_COND(cb->cd_ch[CH_IN] !=NULL);

	ch_param.ch_size	 = FECA_CD_OUTPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_OUT;
	ch_param.xfer_size	 = 0x100;
	ch_param.tb_num		 = 0;
	cb->cd_ch[CH_OUT] = feca_ch_open(cb->cd_chain, ch_param);
	ASSERT_COND(cb->cd_ch[CH_OUT] !=NULL);

	ch_param.ch_size	 = FECA_CD_STATUS_MAX_SZ;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_CRC_OUT;
	ch_param.xfer_size	 = 0x8;
	ch_param.tb_num		 = 0;
	cb->cd_ch[CH_CRC_OUT] = feca_ch_open(cb->cd_chain, ch_param);
	ASSERT_COND(cb->cd_ch[CH_CRC_OUT] !=NULL);

	/* Shared decode init */
	//pr_info("****** FECA SHARED DECODE INIT ******\r\n");
	/* Open SD chain */
	chain_param.type = FECA_SD_CHAIN;
	chain_param.irq_mask = 0;
	cb->sd_chain = feca_chain_open(cb->feca_dev, chain_param, NULL);
	ASSERT_COND(cb->sd_chain!=NULL);

	//for (ii = TB_0; ii < TB_2; ii++) { 
	for (ii = TB_0; ii < TB_1; ii++) { 
		/* Open all 4 channel of SD */
		ch_param.ch_size	 = sizeof(sd_command_t);
		ch_param.tb_num		 = ii;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_CMD;
		ch_param.xfer_size	 = 0;
		cb->sd_ch[(CH_CMD * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_CMD * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size	 = FECA_SD_INPUT_MAX_SZ;
		ch_param.tb_num		 = ii;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_IN;
		ch_param.xfer_size	 = 0;
		cb->sd_ch[(CH_IN * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_IN * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size	 = FECA_SD_OUTPUT_MAX_SZ;
		ch_param.tb_num		 = ii;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_OUT;
		ch_param.xfer_size	 = 0xC00;
		cb->sd_ch[(CH_OUT * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_OUT * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size	 = FECA_SD_STATUS_MAX_SZ;
		ch_param.tb_num		 = ii;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_CRC_OUT;
		ch_param.xfer_size	 = 0x40;
		cb->sd_ch[(CH_CRC_OUT * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_CRC_OUT * TB_MAX ) + ii] !=NULL);
	}
	
	/* Shared encode init */
	//pr_info("****** FECA SHARED ENCODE INIT ******\r\n");
	/* Open SE chain */
	chain_param.type = FECA_SE_CHAIN;
	chain_param.irq_mask = 0;
	cb->se_chain = feca_chain_open(cb->feca_dev, chain_param, NULL);
	ASSERT_COND(cb->se_chain !=NULL);

	/* FIXME : fill ch_param */
	//for (ii = TB_0; ii < TB_2; ii++) { 
	for (ii = TB_0; ii < TB_1; ii++) { 
		/* Open all 3 channel of SE */
		ch_param.ch_size	 = sizeof(se_command_t);
		ch_param.tb_num		 = ii;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_CMD;
		ch_param.xfer_size	 = 0;
		cb->se_ch[(CH_CMD * TB_MAX ) + ii] = feca_ch_open(cb->se_chain, ch_param);
		ASSERT_COND(cb->se_ch[(CH_CMD * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size	 = FECA_SE_INPUT_MAX_SZ;
		ch_param.tb_num		 = ii;
		ch_param.dma_disable	 = 1;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_IN;
		ch_param.xfer_size	 = 0xc00;
		cb->se_ch[(CH_IN * TB_MAX ) + ii] = feca_ch_open(cb->se_chain, ch_param);
		ASSERT_COND(cb->se_ch[(CH_IN * TB_MAX ) + ii] !=NULL);


		ch_param.ch_size	 = FECA_SE_OUTPUT_MAX_SZ;
		ch_param.tb_num		 = ii;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_OUT;
		ch_param.xfer_size	 = 0x0;
		cb->se_ch[(CH_OUT * TB_MAX ) + ii] = feca_ch_open(cb->se_chain, ch_param);
		ASSERT_COND(cb->se_ch[(CH_OUT * TB_MAX ) + ii] !=NULL);
	}
	
	/* Control encode init */
	//pr_info("****** FECA CONTROL ENCODE INIT ******\r\n");
	/* Open CE chain */
	chain_param.type = FECA_CE_CHAIN;
	chain_param.irq_mask = 0;
	cb->ce_chain = feca_chain_open(cb->feca_dev, chain_param, NULL);
	ASSERT_COND(cb->ce_chain!=NULL);

	/* Open all 3 channel of CE */
	ch_param.ch_size	 = sizeof(ce_command_t);
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_CMD;
	ch_param.xfer_size	 = 0;
	ch_param.tb_num		 = 0;
	cb->ce_ch[CH_CMD] = feca_ch_open(cb->ce_chain, ch_param);
	ASSERT_COND(cb->ce_ch[CH_CMD] !=NULL);

	ch_param.ch_size	 = FECA_CE_INPUT_MAX_SZ;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_IN;
	ch_param.xfer_size	 = 0x100;
	ch_param.tb_num		 = 0;
	cb->ce_ch[CH_IN] = feca_ch_open(cb->ce_chain, ch_param);
	ASSERT_COND(cb->ce_ch[CH_IN] !=NULL);

	ch_param.ch_size	 = FECA_CE_OUTPUT_MAX_SZ;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_OUT;
	ch_param.xfer_size	 = 0;
	ch_param.tb_num		 = 0;
	cb->ce_ch[CH_OUT] = feca_ch_open(cb->ce_chain, ch_param);
	ASSERT_COND(cb->ce_ch[CH_OUT] !=NULL);

#if DCM_ARM_POLAR_DECODER_POC_EN
	//pr_info("****** FECA SHARED DECODE CHAIN TEST (DCM) ******\n");
	/* SD chain 0 is already open fot non-DCM*/
	for (ii = TB_1; ii < TB_2; ii++) {
		/* Open all 4 channel of SD */
		ch_param.ch_size        = sizeof(sd_dcm_command_t);   //0x400
		ch_param.tb_num         = ii;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_CMD;
		ch_param.xfer_size      = 0;
		cb->sd_ch[(CH_CMD * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		//ASSERT_COND(cb->sd_chain[(CH_CMD * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size        = FECA_SD_INPUT_MAX_SZ; //0x1000
		ch_param.tb_num         = ii;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_IN;
		ch_param.xfer_size      = 0;
		cb->sd_ch[(CH_IN * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_IN * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size        = FECA_SD_OUTPUT_MAX_SZ; //0x1000
		ch_param.tb_num         = ii;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_OUT;
		ch_param.xfer_size      = 0x40;
		cb->sd_ch[(CH_OUT * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_OUT * TB_MAX ) + ii] !=NULL);

		ch_param.ch_size        = FECA_SD_STATUS_MAX_SZ;
		ch_param.tb_num         = ii;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_CRC_OUT;
		ch_param.xfer_size      = 0x4;
		cb->sd_ch[(CH_CRC_OUT * TB_MAX ) + ii] = feca_ch_open(cb->sd_chain, ch_param);
		ASSERT_COND(cb->sd_ch[(CH_CRC_OUT * TB_MAX ) + ii] !=NULL);
	}
	//pr_info("****** FECA CONTROL DECODE CHAIN TEST (DCM) ******\n");
	/* CD chain 0 is already open for non-DCM*/

	for (ii = TB_1; ii < TB_2; ii++) {
		ch_param.ch_size        = FECA_CD_DCM_INPUT_MAX_SIZE;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_CMD_DCM_ACK;
		ch_param.xfer_size      = 0;
		ch_param.tb_num         = ii;
		cb->cd_ch[CH_CMD_DCM_ACK + ii] = feca_ch_open(cb->cd_chain, ch_param);
		ASSERT_COND(cb->cd_ch[CH_CMD_DCM_ACK + ii] !=NULL);

		ch_param.ch_size        = FECA_CD_DCM_INPUT_MAX_SIZE;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_CMD_DCM_CSI1;
		ch_param.xfer_size      = 0;
		ch_param.tb_num         = ii;
		cb->cd_ch[CH_CMD_DCM_ACK + TB_MAX + ii] = feca_ch_open(cb->cd_chain, ch_param);
		ASSERT_COND(cb->cd_ch[CH_CMD_DCM_ACK + TB_MAX + ii ] !=NULL);

		ch_param.ch_size        = FECA_CD_DCM_INPUT_MAX_SIZE;
		ch_param.dma_disable    = 0;
		ch_param.ext_dma        = 0;
		ch_param.type           = CH_CMD_DCM_CSI2;
		ch_param.xfer_size      = 0;
		ch_param.tb_num         = ii;
		cb->cd_ch[CH_CMD_DCM_ACK + (2 * TB_MAX) + ii] = feca_ch_open(cb->cd_chain, ch_param);
		ASSERT_COND(cb->cd_ch[CH_CMD_DCM_ACK + (2 * TB_MAX) + ii] !=NULL);
	}

	// In DCM mode, there is no output through Status CB
	ch_param.ch_size        = FECA_CD_OUTPUT_MAX_SZ;
	ch_param.dma_disable    = 0;
	ch_param.ext_dma        = 0;
	ch_param.type           = CH_OUT;
	ch_param.xfer_size      = 0x9;
	ch_param.tb_num         = 0;
	cb->cd_ch[CH_OUT] = feca_ch_open(cb->cd_chain, ch_param);
	ASSERT_COND(cb->cd_ch[CH_OUT] !=NULL);

#endif /* DCM_ARM_POLAR_DECODER_POC_EN */

	/* Allocating output buffer to mimic BBDev command */
	cb->feca_out_ptr = (BaseType_t)pvGeulMalloc(sizeof (se_data_output_tv) + 0x10);
	ASSERT_COND((void *)cb->feca_out_ptr !=NULL);

	cb->feca_sts_ptr = (BaseType_t)pvGeulMalloc(0x20);
	ASSERT_COND((void *)cb->feca_sts_ptr !=NULL);

	cb->e = fsl_malloc(256, 4); /* For 64 blocks */
	ASSERT_COND(cb->e !=NULL);

	//feca_get_ch_id()
	cb->feca_bbdev_initialized = 1;

done:

	return 0;
}

void vFecaBBDevFree(void)
{
	return;
}
#endif //GEUL_DEMO_FECA_BBDEV_TEST
