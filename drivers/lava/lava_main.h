// SPDX-License-Identifier: BSD-3-Clause
/*
Copyright 2022 NXP
*/

#ifndef __LAVA_MAIN_H__
#define __LAVA_MAIN_H__

#include "lava_feca.h"
#include "ppu_intrinsics.h"

#define MEM_INP_SZ 0x10000
#define MEM_OUT_SZ 0x10000

#define QDMA_SGL_MARGIN 128
#define QPREP_SIZE QDMA_SGL_MARGIN

enum {
    QCH_CD_CMD = 0,
    QCH_SD_CMD = QCH_CD_CMD + FECA_CD_NUM,
    QCH_CE_CMD = QCH_SD_CMD + FECA_SD_NUM,
    QCH_SE_CMD = QCH_CE_CMD + FECA_CE_NUM,
    QCH_IF_ENA = QCH_SE_CMD + FECA_SE_NUM,

    QCH_CD_INP = QCH_IF_ENA,
    QCH_SD_INP = QCH_CD_INP + FECA_CD_NUM,
    QCH_CE_OUT = QCH_SD_INP + FECA_SD_NUM,
    QCH_SE_OUT = QCH_CE_OUT + FECA_CE_NUM,
    QCH_CE_INP = QCH_SE_OUT + FECA_SE_NUM,
    QCH_CH_NUM = QCH_CE_INP + FECA_CE_NUM
};

enum {
    CB_MODE_PUSH,
    CB_MODE_PULL,
};

typedef union
{
    struct {
        volatile uint32_t log_en  : 1;
        volatile uint32_t fch_act : 31;
    };

    volatile uint32_t raw;
} user_if_t;

typedef struct
{
    uint32_t cd_tp;
    uint32_t sd_tp;
    uint32_t ce_tp;
    uint32_t se_tp;
    uint32_t qd_nr;
    uint64_t qd_tp;
} tp_stats_t;

typedef struct
{
    uint32_t cb_idx;
    uint32_t cb_base;
    uint32_t cb_cap;
    uint32_t cb_mode;
    uint32_t cb_tgt;
    uint32_t th_size;
    uint32_t rm_size;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t tp_size;
} feca_ch_params_t;

static inline void iowrite32(uint32_t val, volatile uint32_t *addr)
{
    __stwbrx(addr, val);
    __asm__("msync");
}

static inline uint32_t ioread32_fast(const volatile uint32_t *addr)
{
    return (uint32_t)__lwbrx(addr);
}

static inline uint32_t swap_uint32(uint32_t val)
{
    return (uint32_t)__lwbrx(&val);
}

static inline int32_t swap_vint32(void *val)
{
    return (int32_t)__lwbrx(&val);
}

#endif /* __LAVA_MAIN_H__ */
