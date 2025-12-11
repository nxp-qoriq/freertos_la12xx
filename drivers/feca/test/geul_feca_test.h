// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef __GEUL_FECA_TEST_H__
#define __GEUL_FECA_TEST_H__

#define DCM_MODE_TEST 0

/* TODO FECA limits according to Test Vector sizes */
#define FECA_CD_INPUT_MAX_SZ  0x00a0
#define FECA_CD_OUTPUT_MAX_SZ 0x0020
#define FECA_CD_STATUS_MAX_SZ 0x0008

#define FECA_CE_INPUT_MAX_SZ  0x0080
#define FECA_CE_OUTPUT_MAX_SZ 0x0080

#define FECA_SD_INPUT_MAX_SZ  0x6600
#define FECA_SD_OUTPUT_MAX_SZ 0x0c00
#define FECA_SD_STATUS_MAX_SZ 0x0008

#define FECA_SE_INPUT_MAX_SZ  0x0c00
#define FECA_SE_OUTPUT_MAX_SZ 0x0d00

#if DCM_MODE_TEST
#define FECA_CD_TESTS_NUM 1
#define FECA_CE_TESTS_NUM 4
#define FECA_SD_TESTS_NUM 1
#define FECA_SE_TESTS_NUM 1
#else
#define FECA_CD_TESTS_NUM 1
#define FECA_CE_TESTS_NUM 1
#define FECA_SD_TESTS_NUM 1
#define FECA_SE_TESTS_NUM 2
#endif
//#define FECA_WSRAM_START_OFF 0x10
#define FECA_MAX_TESTS_NUM  10 //0x100
#define FECA_TEST_MARK 0xFFAAFFBB

#define FECA_NUM_CB_IDS 64

typedef enum
{
    /* Must be kept in sync to the array definitions in test_vectors.c */

    FECA_TEST_CD  = 0,
    FECA_TEST_CE  = FECA_TEST_CD + FECA_CD_TESTS_NUM,
    FECA_TEST_SD  = FECA_TEST_CE + FECA_CE_TESTS_NUM,
    FECA_TEST_SE  = FECA_TEST_SD + FECA_SD_TESTS_NUM,
    FECA_TEST_NUM = FECA_TEST_SE + FECA_SE_TESTS_NUM
} feca_test_id_t;

typedef struct
{
    volatile uint32_t cb_start_addr;
    volatile uint32_t cb_end_addr;
    volatile uint32_t cb_control;
    volatile uint32_t cb_in_ptr;
    volatile uint32_t cb_out_ptr;
    volatile uint32_t cb_num_valid;
    volatile uint32_t cb_reserved[2];
} feca_cb_regs_t;

typedef struct
{
    volatile uint32_t addr_low;
    volatile uint32_t addr_high;
    volatile uint32_t addr_data;
} feca_msi_regs_t;

typedef struct
{
    uint32_t raw_data[38];
} feca_cd_job_t;

typedef struct
{
    uint32_t raw_data[38];
} feca_ce_job_t;

typedef struct
{
    uint32_t raw_data[31];
} feca_sd_job_t;

typedef struct
{
    uint32_t raw_data[26];
} feca_se_job_t;

enum
{
    NO_FECA_DMA, // do not use FECA DMA for input (CE, SE) or output (CD, SD)
    IN_FECA_DMA, // use FECA DMA for input (CE, SE)
    OUT_FECA_DMA // use FECA DMA for output (CD, SD)
};

enum
{
    NO_QDMA, // use QDMA as input/output to FECA (unless FECA DMA setting applies)
    IN_QDMA, // do not use QDMA as input to FECA
    OUT_QDMA // do not use QDMA as output from FECA
};

enum
{
    SKIP_STS, // do not do FECA output status check
    JOB_STS,   // Job status
    CHCK_STS  // do FECA output status check (applies for CD, SD)
};

enum
{
    VALIDATE_SKIP, // skip validation for current test (keep track, validate later)
    VALIDATE_PREV, // validate current and all previously 'VALIDATE_SKIP' tests
    VALIDATE_PERF  // same as 'VALIDATE_PREV', but use 'performance testing' mode
};

enum
{
    NO_QDMA_IN_WAIT, // do not wait for QDMA input to finish (assumes
                     // a single test is run, useful for performance testing)
    DO_QDMA_IN_WAIT  // wait for QDMA input to finish (normal behavior)
};

enum
{
    DATA_IN_AFTER_COMMAND, // input data after FECA command
    COMMAND_AFTER_DATA_IN  // input FECA command after data (used for measurements)
};

enum
{
    FECA_TO_WSRAM,
    WSRAM_TO_FECA
};

typedef struct
{
    uint32_t feca_out_cbidx;
    uint32_t feca_sts_cbidx;

    volatile uint32_t *feca_out_mark;
    volatile uint32_t *feca_sts_mark;

    uint32_t *cb_out_ptr;
    uint8_t *feca_out_ptr;
    uint8_t *feca_sts_ptr;

    uint64_t ts_cmd_in;
    uint64_t ts_din_start;
    uint64_t ts_din_done;
    uint64_t ts_job_done;
    uint64_t ts_out_done;
    uint64_t ts_sts_done;

    uint32_t out_waited;
    uint32_t sts_waited;

    uint32_t feca_tid;

    uint8_t use_feca_dma;
    uint8_t use_qdma;
    uint8_t check_status;
} feca_test_params_t;

//void build_apps_array(struct sys_module_desc *apps);

int do_feca_validation(void);

#endif /* __TEST_FECA_FPGA_H */
