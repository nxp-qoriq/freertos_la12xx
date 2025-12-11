// SPDX-License-Identifier: BSD-3-Clause
/*
Copyright 2022 NXP
*/

#ifndef __LAVA_QDMA_H__
#define __LAVA_QDMA_H__

#define QDMA_BLOCK_NUM 4
#define QDMA_QUEUE_NUM 8

#define QDMA_CMD_NUM 64
#define QDMA_SGL_NUM 256
#define QDMA_STS_NUM 64

#define QDMA_CMD_BUF_ALIGN (QDMA_CMD_NUM * sizeof(DescriptorFormat_t))
#define QDMA_STS_BUF_ALIGN (QDMA_STS_NUM * sizeof(DescriptorFormat_t))

typedef struct
{
    volatile uint32_t bcqmr;
    uint8_t reserved0[0x10];
    volatile uint32_t bcqdpar;
    uint8_t reserved1[0x4];
    volatile uint32_t bcqepar;
    uint8_t reserved2[0xe0];
} qdma_queue_map_t;

typedef struct
{
    qdma_queue_map_t queue[QDMA_QUEUE_NUM];
    volatile uint32_t bsqmr;
    uint8_t reserved0[0x10];
    volatile uint32_t bsqdpar;
    uint8_t reserved1[0x4];
    volatile uint32_t bsqepar;
    uint8_t reserved2[0x7e0];
} qdma_block_map_t;

typedef struct
{
    volatile uint32_t dmr;
    uint8_t reserved0[0x2ffc];
    qdma_block_map_t block[QDMA_BLOCK_NUM];
} qdma_ip_regs_t;

static qdma_ip_regs_t * const qdma_regs = (qdma_ip_regs_t *)0xFA2C0000;

#endif /* __LAVA_QDMA_H__ */
