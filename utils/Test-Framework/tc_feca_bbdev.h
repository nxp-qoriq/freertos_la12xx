// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef __GEUL_FECA_BBDEV_TEST_H__
#define __GEUL_FECA_BBDEV_TEST_H__
#include "test_framework.h"

#ifdef GEUL_DEMO_FECA_BBDEV_TEST
#include <geul_bbdev_ipc.h>
#define FECA_CD_NUM 1
#define FECA_CE_NUM 1
#define FECA_SD_NUM 8
#define FECA_SE_NUM 8

/* TODO FECA limits according to Test Vector sizes */
#define FECA_CD_INPUT_MAX_SZ  0x00a0
#define FECA_CD_OUTPUT_MAX_SZ 0x0020
#define FECA_CD_STATUS_MAX_SZ 0x0008
#define FECA_CD_DCM_INPUT_MAX_SIZE 0x4000

#define FECA_CE_INPUT_MAX_SZ  0x0080
#define FECA_CE_OUTPUT_MAX_SZ 0x0080

#define FECA_SD_INPUT_MAX_SZ  0x6600
#define FECA_SD_OUTPUT_MAX_SZ 0x0c00
#define FECA_SD_STATUS_MAX_SZ 0x0008

#define FECA_SE_INPUT_MAX_SZ  0x0c00
#define FECA_SE_OUTPUT_MAX_SZ 0x0d00

#if DCM_ARM_POLAR_DECODER_POC_EN
/*** Defines used for ARM Polar Decoder DCM PoC *****/
/*DCM ARM Polar Test Numbers
 * 1 - Only ACK
 * 2 - ACK + CSI1 + CSI2
 */
#define DCM_ARM_POLAR_TEST_NR   2
/*Must be kept in sync with test vectors*/
#if (DCM_ARM_POLAR_TEST_NR == 1)
#define DCM_LLRS_SIZE_ACK	765
#define DCM_LLRS_SIZE_CSI1	0
#define DCM_LLRS_SIZE_CSI2	0
#define DCM_PAYLOAD_SIZE_ACK	57
#define DCM_PAYLOAD_SIZE_CSI1	0
#define DCM_PAYLOAD_SIZE_CSI2	0
#else
#define DCM_LLRS_SIZE_ACK	192
#define DCM_LLRS_SIZE_CSI1	224
#define DCM_LLRS_SIZE_CSI2	288
#define DCM_PAYLOAD_SIZE_ACK	(22 * 8)
#define DCM_PAYLOAD_SIZE_CSI1	(23 * 8)
#define DCM_PAYLOAD_SIZE_CSI2	(23 * 8)
#endif /* DCM_ARM_POLAR_TEST_NR */

/* DCM Debug */
#define DCM_POLAR_DECODER_DEBUG 0

#define DCM_POLAR_DECODER_ACK	0X01
#define DCM_POLAR_DECODER_CSI1	0X02
#define DCM_POLAR_DECODER_CSI2	0X04
#define DCM_POLAR_DECODER_MASK	(DCM_POLAR_DECODER_ACK | \
                                DCM_POLAR_DECODER_CSI1 | \
                                DCM_POLAR_DECODER_CSI2 |)

#define LOOP_COUNT 0x1000000
#define SCRATCH_BUFFER_LLRS_SIZE	0X4000   //The maximum number of LLRs for 1 channel
#define SCRATCH_BUFFER_PAGE_SIZE	0X1000
#define SCRATCH_BUFFER_USED_MEMORY	((SCRATCH_BUFFER_LLRS_SIZE * 3) + SCRATCH_BUFFER_PAGE_SIZE)
/*Use the final memory of SCRATCH_MODEM_SHARE area */
#define DCM_SCRATCH_BUFFER_OFFSET (get_modem_share_area_size() - SCRATCH_BUFFER_USED_MEMORY)

/* Struct used to comunciate to ARM Poalr decoder the pending decoding and parameters*/
typedef struct __attribute__ ((packed)){
	uint8_t channel;
	uint8_t tb_id[3];
        uint16_t A[3];
        uint16_t E[3];
}ddr_dcm_channel;
#endif /* DCM_ARM_POLAR_DECODER_POC_EN */

typedef enum
{
    /* Must be kept in sync to the array definitions in test_vectors.c */

    FECA_TEST_CD,
    FECA_TEST_CE,
    FECA_TEST_SD,
    FECA_TEST_SE,
    FECA_TEST_DCM_ARM,
    FECA_TEST_NUM
} feca_test_id_t;

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
    CHCK_STS  // do FECA output status check (applies for CD, SD)
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


int vFecaBBDevInit(void);
void vFecaBBDevFree(void);
int vFecaBBDevPrepareDispatchJob(struct bbdev_ipc_dequeue_op *tv, uint8_t use_feca_dma);
void vFecaBBDevTestSE();
void vFecaBBDevTestSD();
void vFecaBBDevTestCE();
void vFecaBBDevTestCD();
#if DCM_ARM_POLAR_DECODER_POC_EN
void vFecaBBDevTestDCM_ARM();
#endif
#endif /* GEUL_DEMO_FECA_BBDEV_TEST */
#endif /* __TEST_FECA_BBDEV_TEST_H */
