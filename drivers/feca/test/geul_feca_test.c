// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "../feca_api.h"
#include "../fsl_dbg.h"
#include "../tman_inline.h"

#include "geul_feca_test.h"


/* Test Parameter database for validation of previous tests */
feca_test_params_t test_params[FECA_MAX_TESTS_NUM];

uint32_t test_params_crt_idx = 0;
uint32_t test_params_prv_idx = 0;


int feca_cb_out_size_acc[FECA_NUM_CB_IDS] = {0};
int feca_cb_sts_size_acc[FECA_NUM_CB_IDS] = {0};


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


static int feca_job_validate(chain_type_t chain_type,
                                const uint8_t *ref,
                                uint32_t ref_sz,
                                uint8_t *feca_out_ptr,
                                uint32_t *cb_out_ptr)
{
    if (chain_type == FECA_SD_CHAIN || chain_type == FECA_CD_CHAIN)
    {
        unsigned int i;

        for (i = 0; i < ref_sz; ++i)
        {
            uint8_t byte = ioread8(feca_out_ptr + i);
            if (ref[i] != byte) {
                pr_err("Mistmatch at[%d] addr 0x%llx -> Expected 0x%x got 0x%x\n", i, feca_out_ptr+i, ref[i], byte);
                break;
            }
        }

        if (i < ref_sz)
        {
           pr_err("%s: Chain[%d] -- Validation Fail\n", __func__, chain_type);
           return i+1; // test failed
        }
        else
		pr_info("%s:  Chain[%d] Validation pass ... %d Bytes matched\n",
						__func__, chain_type, ref_sz);
    }
    else
    {
        unsigned int i;
	uint32_t *out_ptr = (uint32_t *)feca_out_ptr;
        /* When reading output from FECA AXI Slave we are constrained to 32-bit access sizes */

        uint32_t ref_sz_w = ref_sz / 4;
        const uint32_t *ref_w = (const uint32_t *)ref;


#if FECA_DEBUG
        pr_info("%s:ref_sz %d ref_sz_w %d\n", __func__, ref_sz, ref_sz_w);
#endif
        for (i = 0; i < ref_sz_w; ++i) {
	    /* Copy from CB to Local Memory */
	    out_ptr[i] = ioread32be((const volatile uint32_t *)cb_out_ptr);
	    if (ref_w[i] != out_ptr[i]) {
                pr_err("ref[%d] = 0x%X not matched with out 0x%X\n", i, ref_w[i], out_ptr[i]);
                break;
            }
#if 0
	    if (i  < 20 || i > 800)
		pr_info("ref[%d]: 0x%X == 0x%X\n", i, ref_w[i], out_ptr[i]);
#endif
        }

        if (i < ref_sz_w) {
           pr_err("%s: Chain[%d] -- Validation Fail\n", __func__, chain_type);
            return i*4+1; // test failed
        }

        pr_info("%s:  Chain[%d] Validation pass ... %d Bytes matched\n",
						__func__, chain_type, ref_sz);
    }

    return 0;
}


static int app_feca_validate(chain_type_t chain_type, uint8_t validation_type)
{
    extern const uint8_t *test_refs[];
    extern const uint8_t *test_stss[];

    extern const uint32_t test_ref_sizes[];
    extern const uint32_t test_sts_sizes[];

    uint32_t i;
    int err;

    /* Validate current and all previously validation skipped tests */
    for (i = test_params_prv_idx; i < test_params_crt_idx; ++i)
    {
        feca_test_params_t *tparam = &test_params[i];

        tparam->out_waited = 0;
        tparam->sts_waited = 0;

        tparam->ts_job_done = 0;
        tparam->ts_out_done = 0;
        tparam->ts_sts_done = 0;

        if (tparam->feca_out_mark) // wait for data to arrive in WSRAM
        {
            //while (*tparam->feca_out_mark == FECA_TEST_MARK)
            while (*tparam->feca_out_mark == FECA_TEST_MARK)
                tparam->out_waited += 1;

            tman_get_timestamp(&tparam->ts_out_done);
        }
        else // wait for data to accumulate in FECA CB
        {
            //const volatile uint32_t *feca_addr = &feca_regs->cb[tparam->feca_out_cbidx].cb_num_valid;
            //int wait_size;

            if (validation_type == VALIDATE_PREV)
            {
                /* Wait for FECA to produce exactly the expected number of bytes
                 * (the FECA output is read after each VALIDATE_PREV validation)
                 */
                //wait_size = (int)test_ref_sizes[tparam->feca_tid];
            }
            else // validation_type == VALIDATE_PERF
            {
                /* Wait for FECA to produce accumulated number of bytes
                 * (the FECA output is NOT read after each VALIDATE_PERF validation
                 * and so the output accumulates)
                 */
                feca_cb_out_size_acc[tparam->feca_out_cbidx] += (int)test_ref_sizes[tparam->feca_tid];
                //wait_size = feca_cb_out_size_acc[tparam->feca_out_cbidx];
            }

            /* Measurements */

            //while ((int)ioread32(feca_addr) < wait_size)
                tparam->out_waited += 1;

            if (tparam->out_waited)
                tman_get_timestamp(&tparam->ts_job_done);
        }

        if (validation_type == VALIDATE_PREV)
        {
            err = feca_job_validate(chain_type,
                                    test_refs[tparam->feca_tid],
                                    test_ref_sizes[tparam->feca_tid],
                                    tparam->feca_out_ptr,
                                    tparam->cb_out_ptr);

            if (!tparam->feca_out_mark)
                tman_get_timestamp(&tparam->ts_out_done);

            if (err)
                return err;
        }
        // else validation is done at the end for all jobs at once to prevent ruining the measurements

        /* Same logic for status */

        if ((tparam->check_status == CHCK_STS) && test_sts_sizes[tparam->feca_tid])
        {
            if (tparam->feca_sts_mark)
            {
                while (*tparam->feca_sts_mark == FECA_TEST_MARK)
                    tparam->sts_waited += 1;

                tman_get_timestamp(&tparam->ts_sts_done);
            }
            else
            {
                //const volatile uint32_t *feca_addr = &feca_regs->cb[tparam->feca_sts_cbidx].cb_num_valid;
                //int wait_size;

                if (validation_type == VALIDATE_PREV) {
                    //wait_size = (int)test_sts_sizes[tparam->feca_tid];
                }
                else {
                    feca_cb_sts_size_acc[tparam->feca_sts_cbidx] += (int)test_sts_sizes[tparam->feca_tid];
                    //wait_size = feca_cb_sts_size_acc[tparam->feca_sts_cbidx];
                }

                //while((int)ioread32(feca_addr) < wait_size)
                    tparam->sts_waited += 1; // most likely this remains 0
            }

            if (validation_type == VALIDATE_PREV)
            {
                err = feca_job_validate(chain_type,
                                        test_stss[tparam->feca_tid],
                                        test_sts_sizes[tparam->feca_tid],
                                        tparam->feca_sts_ptr,
                                        tparam->cb_out_ptr);

                if (!tparam->feca_sts_mark)
                    tman_get_timestamp(&tparam->ts_sts_done);

                if (err)
                    return err;
            }
            // else validation is done at the end for all jobs at once
        }
    }

    /* VALIDATE_PERF validation is done at the end for all jobs at once */

    if (validation_type == VALIDATE_PERF)
    {
        for (i = test_params_prv_idx; i < test_params_crt_idx; ++i)
        {
            feca_test_params_t *tparam = &test_params[i];

            err = feca_job_validate(chain_type,
                                    test_refs[tparam->feca_tid],
                                    test_ref_sizes[tparam->feca_tid],
                                    tparam->feca_out_ptr,
				    tparam->cb_out_ptr);

            if (!tparam->feca_out_mark)
                tman_get_timestamp(&tparam->ts_out_done);

            feca_cb_out_size_acc[tparam->feca_out_cbidx] = 0;

            if (err)
                return err;

            if ((tparam->check_status == CHCK_STS) && test_sts_sizes[tparam->feca_tid])
            {
                err = feca_job_validate(chain_type,
                                        test_stss[tparam->feca_tid],
                                        test_sts_sizes[tparam->feca_tid],
                                        tparam->feca_sts_ptr,
                                        tparam->cb_out_ptr);

                if (!tparam->feca_sts_mark)
                    tman_get_timestamp(&tparam->ts_sts_done);

                feca_cb_sts_size_acc[tparam->feca_sts_cbidx] = 0;

                if (err)
                    return err;
            }
        }
    }

    return 0;
}

static void prepare_job_and_dispatch(
                              chain_handle_t chain,
                              chain_type_t chain_type,
                              uint32_t tb_num,
                              uint32_t *command,
                              const uint8_t *input_ptr,
                              uint32_t input_size,
                              void *output_ptr,
                              void *status_ptr,
                              //uint32_t output_off,
                              //uint32_t status_off,
                              uint8_t use_feca_dma,
                              uint8_t use_qdma,
                              uint8_t get_status,
                              uint8_t wait_qdma_in,
                              uint8_t input_order,
                              uint8_t dcm_mode
                              )
{
    feca_test_params_t *tparam = &test_params[test_params_crt_idx];
    volatile uint32_t *feca_inp_ptr;
    feca_job_t feca_job;
    uint32_t i;
    status_t status;

    switch (chain_type)
    {
    case FECA_CD_CHAIN:
        // complete_trig_en bit also enables/disables CRC output
        command[0] = get_status ? (command[0] | (1 << 12)) : (command[0] & ~(1 << 12));
        command[3] = PTR_LO(output_ptr);
        command[4] = PTR_HI(output_ptr);
        command[5] = PTR_LO(status_ptr);
        command[6] = PTR_HI(status_ptr);

	if (dcm_mode)
	    feca_job.job_type = dcm_mode;
	else
	    feca_job.job_type = FECA_JOB_CD;
        feca_job.t_blk_id = tb_num;
        memcpy(&feca_job.command_chain_t.cd_command_ch_obj, command, sizeof(cd_command_t));
        break;

    case FECA_CE_CHAIN:
        // complete_trig_en bit also enables/disables CRC output
        command[0] = get_status ? (command[0] | (1 << 12)) : (command[0] & ~(1 << 12));
        command[4] = PTR_LO(input_ptr);
        command[5] = PTR_HI(input_ptr);
        feca_job.job_type = dcm_mode ? FECA_JOB_CE_DCM : FECA_JOB_CE;
	feca_job.t_blk_id = tb_num;
	memcpy(&feca_job.command_chain_t.ce_command_ch_obj, command, sizeof(ce_command_t));
        break;

    case FECA_SD_CHAIN:
        command[1] |= (1 << 12); // complete_trig_en
        command[10] = PTR_LO(output_ptr);
        command[11] = PTR_HI(output_ptr);
        command[12] = (use_feca_dma == OUT_FECA_DMA) ? command[12] : 0;
        command[13] = PTR_LO(status_ptr);
        command[14] = PTR_HI(status_ptr);
        feca_job.t_blk_id = tb_num; // TBD if this param is needed
	if (dcm_mode) {
	    feca_job.job_type = FECA_JOB_SD_DCM;
	    memcpy(&feca_job.command_chain_t.sd_dcm_command_ch_obj, command, sizeof(sd_dcm_command_t));
	} else {
	    feca_job.job_type = FECA_JOB_SD;
	    memcpy(&feca_job.command_chain_t.sd_command_ch_obj, command, sizeof(sd_command_t));
	}
        break;

    case FECA_SE_CHAIN:
        // complete_trig_en bit also enables/disables CRC output
        command[0] = get_status ? (command[0] | (1 << 12)) : (command[0] & ~(1 << 12));
        //command[0] = (command[0] | (1 << 12));
        command[7] = PTR_LO(input_ptr);
        command[8] = PTR_HI(input_ptr);
        command[9] = (use_feca_dma == IN_FECA_DMA) ? input_size : 0;
        feca_job.t_blk_id = tb_num; // TBD if this param is needed
        memcpy(&feca_job.command_chain_t.se_command_ch_obj, command, sizeof(se_command_t));
	if (dcm_mode) {
	    feca_job.job_type = FECA_JOB_SE_DCM;
	    memcpy(&feca_job.command_chain_t.se_dcm_command_ch_obj, command, sizeof(se_dcm_command_t));
	} else {
	    feca_job.job_type = FECA_JOB_SE;
	    memcpy(&feca_job.command_chain_t.se_command_ch_obj, command, sizeof(se_command_t));
	}
        break;

    default:
        break;
    }
    feca_inp_ptr = (volatile uint32_t *)feca_get_ch_axi_addr(chain, feca_get_ch_id(chain, CH_IN, tb_num));
//#if FECA_DEBUG
    pr_debug("%s: out_ptr[%x_%x], crc_ptr[%x_%x], in_ptr[%x_%x]\n", __func__, PTR_LO(output_ptr), PTR_HI(output_ptr),
                PTR_LO(status_ptr), PTR_HI(status_ptr), PTR_LO(input_ptr), PTR_HI(input_ptr));
    pr_debug("%s: IN[%x]\n", __func__, feca_inp_ptr);
//#endif

    if (input_order == DATA_IN_AFTER_COMMAND)
    {
        status = feca_job_dispatch(chain, &feca_job); //?? is job_t linked in the command queue as it is, or is copied. As now  feca_job is allocated on stack
        if(status != FECA_SUCCESS)
        {
            //TBD
        }
        tman_get_timestamp(&tparam->ts_cmd_in);
    }

    if (use_feca_dma != IN_FECA_DMA)
    {
        tman_get_timestamp(&tparam->ts_din_start);

        if (use_qdma == IN_QDMA)
        {
            /* TODO QDMA input start */

            if (wait_qdma_in) {
                ; // TODO QDMA input wait
                tman_get_timestamp(&tparam->ts_din_done);
            }
            else {
                tparam->ts_din_done = 0;
            }
        }
        else
        {
            /* Input data would normally be written by the FECA DMA or VSPA DMA or QDMA.
             * Here we write it by hand using e200 stores, the data is already placed
             * in arrays with the correct endianness (big). */

            uint32_t data_size_w = input_size >> 2;
            uint32_t data_size_b = input_size & 0x3;
#if 1
            /* Write 4-byte words of input data */
            pr_debug("%s: Copy input buffer. w size[%d] b_size %d\n", __func__,data_size_w * 4, data_size_b);
            for (i = 0; i < data_size_w; ++i, input_ptr += 4) {
                iowrite32be_fast(*(uint32_t *)input_ptr, feca_inp_ptr);
                //iowrite32(*(uint32_t *)input_ptr, feca_inp_ptr++);
               //fsl_print("%x: ", *(uint32_t *)input_ptr);
            }

            /* Write remainder bytes */
            for (i = 0; i < data_size_b; ++i, input_ptr += 1) {
                iowrite8(*input_ptr, (volatile uint8_t *)feca_inp_ptr);
                //fsl_print("%x: ", *input_ptr);
            }
		//fsl_print("\n");
#else
		memcpy(feca_inp_ptr, input_ptr, input_size);
#endif 
            core_memory_barrier();

            tman_get_timestamp(&tparam->ts_din_done);
        }
    }
    else // FECA will grab the input by itself
    {
        tparam->ts_din_start = 0;
        tparam->ts_din_done  = 0;
    }

    if (input_order == COMMAND_AFTER_DATA_IN)
    {
        //pr_debug("CHECK : Job status %d\n", feca_get_job_status(chain, &feca_job));
        status = feca_job_dispatch(chain, &feca_job); //?? is job_t linked in the command queue as it is, or is copied. As now  feca_job is allocated on stack
        if(status != FECA_SUCCESS)
        {
            //TBD
        }
        tman_get_timestamp(&tparam->ts_cmd_in);
    }
    if(JOB_STS <= get_status)
    {
	uint32_t timeout = 10;
        /* Wait for job completion */
        pr_debug("%s: Waiting for Job complete...\n", __func__);
        /* TODO: Enable this once CMD_COMPLETE_STATUS reg works*/
        while(FECA_SUCCESS != feca_get_job_status(chain, &feca_job)) {
            timeout--;
            if(!timeout)
                break;
        }
         if(!timeout) {
            pr_debug("%s: Job complete failed\n", __func__);
           // for (;;) ;
        } else
            pr_debug("%s: Job complete successful timeout = %d\n", __func__, timeout);
       //     for (i = 0; i < 8; i++) {
         //       fsl_print("%x: ", ioread8((volatile uint8_t *)(output_ptr+ i)));
           // }
	//	fsl_print("\n");
    }
}


/******************************************************************************/
/* @function app_feca_test
 *
 * @brief Perform a single FECA test, open FECA channel if necessary
 *
 * @param ch_type      FECA Chain Type (CD/CE/SD/SE)
 * @param ch_id        FECA Channel Id
 * @param ch_size      FECA Channel Size (0 means 'do not open channel')
 * @param test_id      Test index to run from the test database
 * @param use_feca_dma Use FECA DMA for input (CE, SE) or output (CD, SD).
 *                     If false then the e200 core will use load/store instructions instead
 * @param use_qdma     Use QDMA for FECA input/output. Applicable only if e200 load/store
 *                     instructions would be used otherwise
 * @param check_status Check FECA status. Applicable only for CD and SD
 * @param val_type     Validation type (see VALIDATE_* enum definitions)
 * @param dcm_mode     Whether DCM mode is used
 */
/******************************************************************************/
static int app_feca_test(chain_handle_t chain,
                         chain_type_t chain_type,
                         uint32_t   tb_num,
                         uint32_t test_id,
                         uint8_t use_feca_dma,
                         uint8_t use_qdma,
                         uint8_t check_status,
                         uint8_t val_type,
			 uint8_t dcm_mode)
{
    feca_test_params_t *tparam = &test_params[test_params_crt_idx];

    extern uint32_t *test_cmds[];

    extern const uint8_t *test_inputs[];

    extern const uint32_t test_inp_sizes[];
    extern const uint32_t test_ref_sizes[];
    extern const uint32_t test_sts_sizes[];

    /* Virtual pointers to FECA output and status */
    uint8_t *feca_out_ptr = NULL;
    uint32_t *cb_out_ptr = NULL;
    uint8_t *feca_sts_ptr = NULL;

    /* Memory address to poll for FECA job completion */
    volatile uint32_t *feca_out_mark = NULL;
    volatile uint32_t *feca_sts_mark = NULL;

    uint32_t feca_tid = 0;

    switch (chain_type)
    {
    case FECA_CD_CHAIN:
        ASSERT_COND(test_id < FECA_CD_TESTS_NUM);
        feca_tid = FECA_TEST_CD + test_id;
        break;

    case FECA_CE_CHAIN:
        ASSERT_COND(test_id < FECA_CE_TESTS_NUM);
        feca_tid = FECA_TEST_CE + test_id;
        break;

    case FECA_SD_CHAIN:
        ASSERT_COND(test_id < FECA_SD_TESTS_NUM);
        feca_tid = FECA_TEST_SD + test_id;
        break;

    case FECA_SE_CHAIN:
        ASSERT_COND(test_id < FECA_SE_TESTS_NUM);
        feca_tid = FECA_TEST_SE + test_id;
        break;

    default:
        return 0;
    }

	PRINTF("feca_tid = %d \n", feca_tid);

    //if (chain_type == FECA_CD_CHAIN || chain_type == FECA_SD_CHAIN)
    if (use_feca_dma == OUT_FECA_DMA || use_qdma == OUT_QDMA)
    {
	    int i;
        /* Either the FECA output DMA or the QDMA will be used, so we allocate some space */
	/* FSL malloc woks for SRAM allocation which is not DMAble so use PEB malloc */ 
        feca_out_ptr = (uint8_t *)pvGeulMalloc(test_ref_sizes[feca_tid]);
       // feca_out_ptr = (uint8_t *)0xE031A800; //FECA_DMA_DATA_PEBM
	/* TODO Dummy writes */
	for (i = 0; i < 8; i++) {
		*(feca_out_ptr + i) = 0x5A;
		//PRINTF("0x%X:",*(feca_out_ptr + i));
	}
	PRINTF("\n");

        feca_out_mark = (volatile uint32_t*)(feca_out_ptr + ((test_ref_sizes[feca_tid] - 1) & ~0x3));
        *feca_out_mark = FECA_TEST_MARK;
	PRINTF("--Test ID: %d feca_out_ptr 0x%X feca_out_mark 0x%X size %d\n",feca_tid, feca_out_ptr, feca_out_mark, test_ref_sizes[feca_tid]);

    } else {

        feca_out_ptr = (uint8_t *)pvGeulMalloc(test_ref_sizes[feca_tid]);
        if(!feca_out_ptr) {
		pr_err("Can't get memory for OUT buffer");
        }
        cb_out_ptr = (uint32_t *)feca_get_ch_axi_addr(chain, feca_get_ch_id(chain, CH_OUT, tb_num));
        if(!cb_out_ptr) {
		pr_err("Can't fetch correct AXI address for OUT buffer");
        }
	PRINTF("--Test ID: %d feca_out_ptr 0x%X cb_out_ptr 0x%X size %d\n",feca_tid, feca_out_ptr, cb_out_ptr, test_ref_sizes[feca_tid]);
    }

    if ((check_status == CHCK_STS) && test_sts_sizes[feca_tid])
    {
        if (use_feca_dma == OUT_FECA_DMA) // || use_qdma == OUT_QDMA) the QDMA is never used for status
        {
            //feca_sts_ptr = fsl_malloc(test_sts_sizes[feca_tid], 0x10);
            feca_sts_ptr = (uint8_t *)pvGeulMalloc(test_sts_sizes[feca_tid]);
            feca_sts_mark = (volatile uint32_t*)(feca_sts_ptr + ((test_sts_sizes[feca_tid] - 1) & ~0x3));
           *feca_sts_mark = FECA_TEST_MARK;
	PRINTF("--feca_sts_ptr 0x%X feca_sts_mark 0x%X\n",feca_sts_ptr, feca_sts_mark);
        }
        // else the status is read & validated directly from the FECA CBs
    }
    // else the job is CE or SE or status check is not desired

    /* Save the test parameters for later validation */
    tparam->feca_tid = feca_tid;
    tparam->feca_out_mark  = feca_out_mark;
    tparam->feca_sts_mark  = feca_sts_mark;
    tparam->cb_out_ptr	= cb_out_ptr;
    tparam->feca_out_ptr   = feca_out_ptr;
    tparam->feca_sts_ptr   = feca_sts_ptr;
    tparam->use_qdma       = use_qdma;
    tparam->check_status   = check_status;

    prepare_job_and_dispatch( chain,
                              chain_type,
                              tb_num,
                              test_cmds[feca_tid],
                              test_inputs[feca_tid],
                              test_inp_sizes[feca_tid],
                              feca_out_ptr, feca_sts_ptr,
                              use_feca_dma, use_qdma, check_status,
                              DO_QDMA_IN_WAIT, COMMAND_AFTER_DATA_IN,
			      dcm_mode);

    test_params_crt_idx += 1;
    ASSERT_COND(test_params_crt_idx <= FECA_MAX_TESTS_NUM);

    if (val_type != VALIDATE_SKIP)
    {
        int err = app_feca_validate(chain_type, val_type);
        test_params_prv_idx = test_params_crt_idx;
        return err;
    }
    return 0;
}



/******************************************************************************/
int do_feca_validation(void)
{
    int err = 0;
    dev_handle_t device;
    chain_handle_t cd_chain, ce_chain, sd_chain, se_chain;
    ch_handle_t cd_ch0[CD_CH_NUM], sd_ch0[SD_CH_NUM], se_ch0[SE_CH_NUM], ce_ch0[CE_CH_NUM];
    chain_param_t 	chain_param;
    ch_param_t		ch_param;
    uint8_t j=0;

    /* See the following SharePoint location to learn about the different FECA issues discovered through the current application:
     * https://nxp1-my.sharepoint.com/:x:/g/personal/andrei_enescu_nxp_com/EWjgalkkL11Gpql-8ObpoJMBJfX0IwUGDDPrfXH3dxaWpA?e=ggoy44
     */

    /* FECA Channel number should be > 0 only for the first test for a particular (ch_type, ch_id) combination.
     * Due to xlsx issue #1, DO_FDMA can only be used once for a channel. After that, the e200 core must operate using load/store.
     * The QDMA input/output is in progress.
     * Due to xlsx issue #11, FECA can only do a limited number of jobs for each type, then freezes
     * Due to FECA CD issues, CHCK_STS can only be used for a limited number of jobs, then it should be skipped
     * Due to other issues the FECA SD/SE can only be used in start-stop mode (input-wait-validate). In pipeline mode the PCI switch crashes.
     * Due to xlsx issue #7 we can't validate more than one job
     */

    device =  feca_dev_open("FECA_5G", NULL);

    /*maybe here we need to  configure some input/output base eg
    PCIE_FECA_AXI_MASTER + PCIE_BAR1_ADDRESS
    PCIE_FECA_AXI_SLAVE_V*/

    ASSERT_COND(device!=NULL);

#if !DCM_MODE_TEST
    pr_info("****** FECA CONTROL DECODE CHAIN TEST ******\n");
    /* Open CD chain */
    chain_param.type = FECA_CD_CHAIN;
    chain_param.irq_mask = 0;
    chain_param.dcm_irq_mask = 0;

    cd_chain = feca_chain_open(device, chain_param, NULL);
    ASSERT_COND(cd_chain!=NULL);

    /* Open all 4 channel of CD */
    ch_param.ch_size		= sizeof(cd_command_t);
    ch_param.dma_disable	= 0;
    ch_param.ext_dma		= 0;
    ch_param.type		= CH_CMD;
    ch_param.xfer_size		= 0;
    ch_param.tb_num		= 0;
    cd_ch0[CH_CMD] = feca_ch_open(cd_chain, ch_param);
    ASSERT_COND(cd_ch0[CH_CMD] !=NULL);

	ch_param.ch_size	= FECA_CD_INPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_IN;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	cd_ch0[CH_IN] = feca_ch_open(cd_chain, ch_param);
	ASSERT_COND(cd_ch0[CH_IN] !=NULL);

	ch_param.ch_size	 = FECA_CD_OUTPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_OUT;
	ch_param.xfer_size	 = 0x100;
	ch_param.tb_num		 = 0;
	cd_ch0[CH_OUT] = feca_ch_open(cd_chain, ch_param);
	ASSERT_COND(cd_ch0[CH_OUT] !=NULL);

	ch_param.ch_size	 = FECA_CD_STATUS_MAX_SZ;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_CRC_OUT;
	ch_param.xfer_size	 = 0x8;
	ch_param.tb_num		 = 0;
	cd_ch0[CH_CRC_OUT] = feca_ch_open(cd_chain, ch_param);
	ASSERT_COND(cd_ch0[CH_CRC_OUT] !=NULL);
#if 1
	if (!err) err = app_feca_test(cd_chain, FECA_CD_CHAIN, ch_param.tb_num, 0,  OUT_FECA_DMA, NO_QDMA, CHCK_STS, VALIDATE_PERF, 0);
	pr_info("******* FECA Control Decode %s **************\n", err ? "FAILED" : "PASSED");
#endif
#if 0
    status = feca_ch_reset(cd_ch0);
    ASSERT_COND(status == FECA_SUCCESS);

    fsl_print("FECA Control Decode %s\n", err ? "FAILED" : "PASSED");
    status = feca_chain_reset(device);
    ASSERT_COND(status == FECA_SUCCESS);
    status = feca_chain_close(device);
    ASSERT_COND(status == FECA_SUCCESS);
#endif
    pr_info("****** FECA SHARED DECODE CHAIN TEST ******\n");
    /* Open SD chain */
	chain_param.type = FECA_SD_CHAIN;
	chain_param.irq_mask = 0;
	sd_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(sd_chain!=NULL);

	for (j = TB_0; j < TB_1; j++) {
		/* Open all 4 channel of SD */
		ch_param.ch_size	 = sizeof(sd_command_t);
		ch_param.tb_num		 = j;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_CMD;
		ch_param.xfer_size	 = 0;
		sd_ch0[(CH_CMD * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_CMD * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	 = FECA_SD_INPUT_MAX_SZ;
		ch_param.tb_num		 = j;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_IN;
		ch_param.xfer_size	 = 0;
		sd_ch0[(CH_IN * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_IN * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	 = FECA_SD_OUTPUT_MAX_SZ;
		ch_param.tb_num		 = j;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_OUT;
		ch_param.xfer_size	 = 0xC00;
		sd_ch0[(CH_OUT * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_OUT * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	 = FECA_SD_STATUS_MAX_SZ;
		ch_param.tb_num		 = j;
		ch_param.dma_disable = 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_CRC_OUT;
		ch_param.xfer_size	 = 0x40;
		sd_ch0[(CH_CRC_OUT * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_CRC_OUT * TB_MAX ) + j] !=NULL);
#if 1
		if (!err) err = app_feca_test(sd_chain, FECA_SD_CHAIN, ch_param.tb_num, 0, OUT_FECA_DMA, NO_QDMA, CHCK_STS, VALIDATE_PERF, 0);
		pr_info("******* FECA Shared Decode  %s **************\n", err ? "FAILED" : "PASSED");
#endif
	}

#if 0
    status = feca_ch_reset(se_ch0);
    ASSERT_COND(status == FECA_SUCCESS);

    fsl_print("FECA Shared Decode %s\n", err ? "FAILED" : "PASSED");
    status = feca_chain_reset(device);
    ASSERT_COND(status == FECA_SUCCESS);
    status = feca_chain_close(device);
    ASSERT_COND(status == FECA_SUCCESS);
#endif
    pr_info("****** FECA SHARED ENCODE CHAIN TEST ******\n");
    /* Open SE chain */
	chain_param.type = FECA_SE_CHAIN;
	chain_param.irq_mask = 0;
	se_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(se_chain!=NULL);

	for (j = TB_0; j < TB_1; j++) {
		/* Open all 3 channel of SE */
		ch_param.ch_size	 = sizeof(se_command_t);
		ch_param.tb_num		 = j;
		ch_param.dma_disable= 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_CMD;
		ch_param.xfer_size	 = 0;
		se_ch0[(CH_CMD * TB_MAX ) + j] = feca_ch_open(se_chain, ch_param);
		ASSERT_COND(se_ch0[(CH_CMD * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	 = FECA_SE_INPUT_MAX_SZ;
		ch_param.tb_num		 = j;
#if 1 /* Enable When need to use FDMA */
		ch_param.dma_disable	 = 0;
#else
		ch_param.dma_disable	 = 1;
#endif
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_IN;
		ch_param.xfer_size	 = 0xc00;
		se_ch0[(CH_IN * TB_MAX ) + j] = feca_ch_open(se_chain, ch_param);
		ASSERT_COND(se_ch0[(CH_IN * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	 = FECA_SE_OUTPUT_MAX_SZ;
		ch_param.tb_num		 = j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	 = 0;
		ch_param.type		 = CH_OUT;
		//ch_param.xfer_size	 = 0xCC0;
		ch_param.xfer_size	 = 0x0;
		se_ch0[(CH_OUT * TB_MAX ) + j] = feca_ch_open(se_chain, ch_param);
		ASSERT_COND(se_ch0[(CH_OUT * TB_MAX ) + j] !=NULL);

#if 1 /* Enable When need to use FDMA */
	err = app_feca_test(se_chain, FECA_SE_CHAIN, ch_param.tb_num, 0, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_PERF, 0);
	pr_info("******* FECA Shared Encode %s **************\n", err ? "FAILED" : "PASSED");
#endif
#if 0
	err = app_feca_test(se_chain, FECA_SE_CHAIN, ch_param.tb_num, 0, NO_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_PERF, 0);
	pr_info("******* FECA Shared Encode w/o FDMA [TV-1] %s **************\n", err ? "FAILED" : "PASSED");
	err = app_feca_test(se_chain, FECA_SE_CHAIN, ch_param.tb_num, 1, NO_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_PERF, 0);
	pr_info("******* FECA Shared Encode w/o FDMA [TV-2] %s **************\n", err ? "FAILED" : "PASSED");
#endif
	}
#if 0
    status = feca_ch_reset(sd_ch0);
    ASSERT_COND(status == FECA_SUCCESS);

    status = feca_ch_reset(sd_chain);
    ASSERT_COND(status == FECA_SUCCESS);
    fsl_print("FECA Shared Encode %s\n", err ? "FAILED" : "PASSED");
    status = feca_chain_reset(device);
    ASSERT_COND(status == FECA_SUCCESS);
    status = feca_chain_close(device);
    ASSERT_COND(status == FECA_SUCCESS);
#endif
	pr_info("****** FECA CONTROL ENCODE CHAIN TEST ******\n");
	/* Open CE chain */
	chain_param.type = FECA_CE_CHAIN;
	chain_param.irq_mask = 0;
	ce_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(ce_chain!=NULL);

	/* Open all 3 channel of CE */
	ch_param.ch_size	 = sizeof(ce_command_t);
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_CMD;
	ch_param.xfer_size	 = 0;
	ch_param.tb_num		 = 0;
	ce_ch0[CH_CMD] = feca_ch_open(ce_chain, ch_param);
	ASSERT_COND(ce_ch0[CH_CMD] !=NULL);

	ch_param.ch_size	 = FECA_CE_INPUT_MAX_SZ;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_IN;
	ch_param.xfer_size	 = 0xF;
	ch_param.tb_num		 = 0;
	ce_ch0[CH_IN] = feca_ch_open(ce_chain, ch_param);
	ASSERT_COND(ce_ch0[CH_IN] !=NULL);

	ch_param.ch_size	 = FECA_CE_OUTPUT_MAX_SZ;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_OUT;
	ch_param.xfer_size	 = 0;
	ch_param.tb_num		 = 0;
	ce_ch0[CH_OUT] = feca_ch_open(ce_chain, ch_param);
	ASSERT_COND(ce_ch0[CH_OUT] !=NULL);

#if 1
	if (!err) err = app_feca_test(ce_chain, FECA_CE_CHAIN, ch_param.tb_num, 0, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_PERF, 0);
	pr_info("******* FECA Control Encode %s **************\n", err ? "FAILED" : "PASSED");
#endif
#if 0
	status = feca_ch_reset(ce_ch0);
	ASSERT_COND(status == FECA_SUCCESS);

	fsl_print("FECA Control Encode %s\n", err ? "FAILED" : "PASSED");
	status = feca_chain_reset(device);
	ASSERT_COND(status == FECA_SUCCESS);
	status = feca_chain_close(device);
	ASSERT_COND(status == FECA_SUCCESS);

    iowrite32((uint32_t)err, (volatile uint32_t *)wsram_feca_mem_v);

    status = feca_dev_reset(device);
    ASSERT_COND(status == FECA_SUCCESS);
    status = feca_dev_close(device);
    ASSERT_COND(status == FECA_SUCCESS);
#endif

#else			/********  DCM MODE Testing ********/

	pr_info("****** FECA CONTROL ENCODE TEST (DCM) ******\n");
	/* Open CE chain */
	chain_param.type = FECA_CE_CHAIN;
	chain_param.irq_mask = 0;
	ce_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(ce_chain!=NULL);

	/* Open all 3 channel of CE */
	ch_param.ch_size	 = sizeof(ce_command_t);
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_CMD;
	ch_param.xfer_size	 = 0;
	ch_param.tb_num		 = 0;
	ce_ch0[CH_CMD] = feca_ch_open(ce_chain, ch_param);
	ASSERT_COND(ce_ch0[CH_CMD] !=NULL);

	ch_param.ch_size	 = 0x98;
	ch_param.dma_disable = 0;
	ch_param.ext_dma	 = 0;
	ch_param.type		 = CH_IN;
	ch_param.xfer_size	 = 0xF;
	ch_param.tb_num		 = 0;
	ce_ch0[CH_IN] = feca_ch_open(ce_chain, ch_param);
	ASSERT_COND(ce_ch0[CH_IN] !=NULL);

    pr_info("****** FECA SHARED ENCODE TEST (DCM) ******\n");
    /* Open SE chain */
	chain_param.type = FECA_SE_CHAIN;
	chain_param.irq_mask = 0;
	se_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(se_chain!=NULL);

	for (j = TB_0; j < TB_1; j++) {
		/* Open all 3 channel of SE */
		ch_param.ch_size	= sizeof(se_dcm_command_t);
		ch_param.tb_num		= j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_CMD;
		ch_param.xfer_size	= 0;
		se_ch0[(CH_CMD * TB_MAX ) + j] = feca_ch_open(se_chain, ch_param);
		ASSERT_COND(se_ch0[(CH_CMD * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	= 16384;
		ch_param.tb_num		= j;
#if 1 /* Enable When need to use FDMA */
		ch_param.dma_disable	= 0;
#else
		ch_param.dma_disable	 = 1;
#endif
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_IN;
		ch_param.xfer_size	= 0x1000;
		se_ch0[(CH_IN * TB_MAX ) + j] = feca_ch_open(se_chain, ch_param);
		ASSERT_COND(se_ch0[(CH_IN * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	= 27648;
		ch_param.tb_num		= j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_OUT;
		//ch_param.xfer_size	= 0xCC0;
		ch_param.xfer_size	= 0x0;
		se_ch0[(CH_OUT * TB_MAX ) + j] = feca_ch_open(se_chain, ch_param);
		ASSERT_COND(se_ch0[(CH_OUT * TB_MAX ) + j] !=NULL);

#if 1
	err = app_feca_test(ce_chain, FECA_CE_CHAIN, ch_param.tb_num, 0, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_SKIP, 1);
	pr_info("******* FECA Control DCM Encode(1) %s **************\n", err ? "FAILED" : "PASSED");
	err = app_feca_test(ce_chain, FECA_CE_CHAIN, ch_param.tb_num, 1, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_SKIP, 1);
	pr_info("******* FECA Control DCM Encode(2) %s **************\n", err ? "FAILED" : "PASSED");
	err = app_feca_test(ce_chain, FECA_CE_CHAIN, ch_param.tb_num, 2, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_SKIP, 1);
	pr_info("******* FECA Control DCM Encode(3) %s **************\n", err ? "FAILED" : "PASSED");
	err = app_feca_test(ce_chain, FECA_CE_CHAIN, ch_param.tb_num, 3, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_SKIP, 1);
	pr_info("******* FECA Control DCM Encode(4) %s **************\n", err ? "FAILED" : "PASSED");
	err = app_feca_test(se_chain, FECA_SE_CHAIN, ch_param.tb_num, 0, IN_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_PERF, 1);
	pr_info("******* FECA Shared DCM Encode --> %s **************\n", err ? "FAILED" : "PASSED");
#endif
	}

	pr_info("****** FECA SHARED DECODE CHAIN TEST (DCM) ******\n");
	/* Open SD chain */
	chain_param.type = FECA_SD_CHAIN;
	chain_param.irq_mask = 0;
	sd_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(sd_chain!=NULL);

	for (j = TB_0; j < TB_1; j++) {
		/* Open all 4 channel of SD */
		ch_param.ch_size	= 0x400;
		ch_param.tb_num		= j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_CMD;
		ch_param.xfer_size	= 0;
		sd_ch0[(CH_CMD * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_CMD * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	= 0x1000;
		ch_param.tb_num		= j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_IN;
		ch_param.xfer_size	= 0;
		sd_ch0[(CH_IN * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_IN * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	= 0x1000;
		ch_param.tb_num		= j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_OUT;
		ch_param.xfer_size	= 0x40;
		sd_ch0[(CH_OUT * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_OUT * TB_MAX ) + j] !=NULL);

		ch_param.ch_size	= FECA_SD_STATUS_MAX_SZ;
		ch_param.tb_num		= j;
		ch_param.dma_disable	= 0;
		ch_param.ext_dma	= 0;
		ch_param.type		= CH_CRC_OUT;
		ch_param.xfer_size	= 0x4;
		sd_ch0[(CH_CRC_OUT * TB_MAX ) + j] = feca_ch_open(sd_chain, ch_param);
		ASSERT_COND(sd_ch0[(CH_CRC_OUT * TB_MAX ) + j] !=NULL);
	}

	pr_info("****** FECA CONTROL DECODE CHAIN TEST (DCM) ******\n");
	/* Open CD chain */
	chain_param.type = FECA_CD_CHAIN;
	chain_param.irq_mask = 0;
	chain_param.dcm_irq_mask = 0;
	cd_chain = feca_chain_open(device, chain_param, NULL);
	ASSERT_COND(cd_chain!=NULL);

	for (j = TB_0; j < TB_1; j++) {
	ch_param.ch_size	= 0x100;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD_DCM_ACK;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= j;
	cd_ch0[CH_CMD_DCM_ACK] = feca_ch_open(cd_chain, ch_param);
	ASSERT_COND(cd_ch0[CH_CMD_DCM_ACK] !=NULL);
#if 0
	ch_param.ch_size	= 0x100;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD_DCM_CSI1;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= j;
	cd_ch0[CH_CMD_DCM_CSI1] = feca_ch_open(cd_chain, ch_param);

	ASSERT_COND(cd_ch0[CH_CMD_DCM_CSI1] !=NULL);
	ch_param.ch_size	= 0x100;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_CMD_DCM_CSI2;
	ch_param.xfer_size	= 0;
	ch_param.tb_num		= 0;
	cd_ch0[CH_CMD_DCM_CSI2] = feca_ch_open(cd_chain, ch_param);
	ASSERT_COND(cd_ch0[CH_CMD_DCM_CSI2] !=NULL);
#endif
	}
	ch_param.ch_size	= FECA_CD_OUTPUT_MAX_SZ;
	ch_param.dma_disable	= 0;
	ch_param.ext_dma	= 0;
	ch_param.type		= CH_OUT;
	ch_param.xfer_size	= 0x9;
	ch_param.tb_num		= 0;
	cd_ch0[CH_OUT] = feca_ch_open(cd_chain, ch_param);
	ASSERT_COND(cd_ch0[CH_OUT] !=NULL);

#if 1
	err = app_feca_test(sd_chain, FECA_SD_CHAIN, TB_0, 0, OUT_FECA_DMA, NO_QDMA, CHCK_STS, VALIDATE_PERF, FECA_JOB_SD_DCM);
	pr_info("******* FECA Shared Decode (DCM)  %s **************\n", err ? "FAILED" : "PASSED");
	err = app_feca_test(cd_chain, FECA_CD_CHAIN, TB_0, 0,  OUT_FECA_DMA, NO_QDMA, JOB_STS, VALIDATE_PERF, FECA_JOB_CD_DCM_ACK);
	pr_info("******* FECA Control Decode (DCM-ACK) %s **************\n", err ? "FAILED" : "PASSED");
#endif

#endif /* DCM */
    return 0;
}
