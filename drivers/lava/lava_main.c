// SPDX-License-Identifier: BSD-3-Clause
/*
Copyright 2022 NXP
*/

#include "lava_main.h"
#include "lava_qdma.h"
#include "geul_qdma.h"

#include "ppc.h"

user_if_t user_if;

uint32_t qcb_ptr;
uint32_t sgl_ptr;
uint32_t rng_seed;
uint32_t fram_offst;
uint32_t global_ts;

uint32_t qprep_p;
uint32_t qprep_c;

uint8_t qprep_d[QPREP_SIZE];

tp_stats_t tp_stats;

feca_ch_params_t qch_params[QCH_CH_NUM];

NxpQdmaCLT_t qcb_clts[QDMA_CMD_NUM] __attribute__((aligned(0x20))) __attribute__((section (".dmem.qdma")));

DescriptorFormat_t qcb_cmds[QDMA_CMD_NUM] __attribute__((aligned(QDMA_CMD_BUF_ALIGN))) __attribute__((section (".dmem.qdma")));
DescriptorFormat_t qcb_stat[QDMA_STS_NUM] __attribute__((aligned(QDMA_STS_BUF_ALIGN))) __attribute__((section (".shared.bss")));

ScatterGatherTableFormat_t qcb_sgl_s[QDMA_SGL_NUM + QDMA_SGL_MARGIN] __attribute__((aligned(0x20))) __attribute__((section (".dmem.qdma")));
ScatterGatherTableFormat_t qcb_sgl_d[QDMA_SGL_NUM + QDMA_SGL_MARGIN] __attribute__((aligned(0x20))) __attribute__((section (".dmem.qdma")));

uint8_t fq_mem_inp[MEM_INP_SZ * 2] __attribute__((aligned(FECA_DMA_ALIGNMENT))) __attribute__((section(".shared.bss")));
uint8_t fq_mem_out[MEM_OUT_SZ * 2] __attribute__((aligned(FECA_DMA_ALIGNMENT))) __attribute__((section(".shared.bss")));

feca_cd_command_t cdc_bulk[CDC_CB_SZ / sizeof(feca_cd_command_t)] __attribute__((section(".shared.bss"))) __attribute__((aligned(0x20)));
feca_ce_command_t cec_bulk[CEC_CB_SZ / sizeof(feca_ce_command_t)] __attribute__((section(".shared.bss"))) __attribute__((aligned(0x20)));
feca_sd_command_t sdc_bulk[SDC_CB_SZ / sizeof(feca_sd_command_t)] __attribute__((section(".shared.bss"))) __attribute__((aligned(0x20)));
feca_se_command_t sec_bulk[SEC_CB_SZ / sizeof(feca_se_command_t)] __attribute__((section(".shared.bss"))) __attribute__((aligned(0x20)));

const uint32_t ext_sgl_s = (uint32_t)qcb_sgl_s + 0x100000;
const uint32_t ext_sgl_d = (uint32_t)qcb_sgl_d + 0x100000;

const uint32_t cdc_template[] =
{
    0x03ff440a, 0x00000438, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};

const uint32_t cec_template[] =
{
    0x03ff240a, 0x00000438, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};

const uint32_t sdc_template[] =
{
    0x2020c071, 0x00000006, 0x04170010, 0x00482100, 0x000062b8, 0x000005e6, 0x000005e8, 0x00000000,
    0x5e485840, 0x3c5419e9, 0x00000000, 0x00000000, 0x0000dc35, 0x00000000, 0x00000000,
    0xffffffff, 0x003fffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05e60000, 0x11b20bcc, 0x1d7e1798, 0x00000000, 0x05e80000, 0x11b80bd0, 0x1d8817a0, 0x00000000
};

const uint32_t sec_template[] =
{
    0x0d364671, 0x04170010, 0x000062b8, 0x00002364, 0x00002370,
    0x5e485840, 0x3c5419e9, 0x00000000, 0x00000000, 0x0000dc35,
    0xffffffff, 0x003fffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05e60000, 0x11b20bcc, 0x1d7e1798, 0x00000000, 0x05e80000, 0x11b80bd0, 0x1d8817a0, 0x00000000
};

static uint32_t app_rng()
{
    if (!rng_seed) rng_seed = 0x7fffffff;
    rng_seed = (16807 * rng_seed) & 0x7fffffff;
    return rng_seed;
}

static inline void fast_memcpy32(void *dst, const void *src, const uint32_t szb)
{
    for (uint32_t i = 0; i < szb / 4; ++i)
        ((uint32_t *)dst)[i] = ((uint32_t *)src)[i];
}

static inline void fast_swap32(void *ptr, const uint32_t szb)
{
    for (uint32_t i = 0; i < szb / 4; ++i)
        ((uint32_t *)ptr)[i] = swap_uint32(((uint32_t *)ptr)[i]);
}

static void mem_init()
{
    for (uint32_t i = 0; i < sizeof(fq_mem_inp) / 4; ++i)
        ((uint32_t *)fq_mem_inp)[i] = app_rng();
}

static void qdma_init()
{
    qdma_block_map_t *block = &qdma_regs->block[0];
    qdma_queue_map_t *queue = &block->queue[0];

    iowrite32(0x0000b000, &queue->bcqmr);

    iowrite32((uint32_t)qcb_cmds + 0x100000, &queue->bcqepar);
    iowrite32((uint32_t)qcb_cmds + 0x100000, &queue->bcqdpar);

    iowrite32(0x0000b000, &block->bsqmr);

    iowrite32((uint32_t)qcb_stat, &block->bsqepar);
    iowrite32((uint32_t)qcb_stat, &block->bsqdpar);

    iowrite32(ioread32_fast(&block->bsqmr) | 0x80000000, &block->bsqmr);
    iowrite32(ioread32_fast(&queue->bcqmr) | 0x80000000, &queue->bcqmr);

    iowrite32(0x00000000, &qdma_regs->dmr);

    for (uint32_t i = 0; i < QDMA_CMD_NUM; ++i)
    {
        NxpQdmaCLT_t *clt = &qcb_clts[i];

        clt->CmdListTable.LowAddrBase = swap_uint32((uint32_t)&clt->SrcDescFmt + 0x100000);
        clt->CmdListTable.DataLen     = 0x20000000;

        clt->sCmdListTable.Cfg1 = 0x00000020; // QDMA_CLT_SG
        clt->dCmdListTable.Cfg1 = 0x000000a0; // QDMA_CLT_SG, QDMA_CLT_F

        clt->SrcDescFmt.Cmd = 0x00000008; // NS
        clt->DstDescFmt.Cmd = 0x00000008; // NS

        qcb_cmds[i].LowAddrBase = swap_uint32((uint32_t)&clt->CmdListTable + 0x100000);

        qcb_cmds[i].Cfg1 = 0x00000010; // QDMA_DF_LONG_FMT
      //qcb_cmds[i].Cfg2 = 0x00050000; // SO, SER
    }
}

static uint32_t qdma_prep()
{
    uint32_t sgl_pos = sgl_ptr;

    while (qprep_c != qprep_p)
    {
        uint32_t qch = qprep_d[qprep_c++]; qprep_c %= QPREP_SIZE;

        feca_ch_params_t *qcp = &qch_params[qch];

        uint32_t rm_size = qcp->rm_size;
        uint32_t ck_size = (qch < QCH_IF_ENA) ? qcp->th_size :
                (rm_size <= FECA_CBUF_SIZE) ? rm_size : FECA_CBUF_SIZE;

        qcp->cb_tgt = (qcp->cb_tgt + ck_size) % qcp->cb_cap;
        rm_size -= ck_size; ck_size = swap_uint32(ck_size);

        if (rm_size) {qprep_d[qprep_p++] = qch; qprep_p %= QPREP_SIZE;}

        qcb_sgl_s[sgl_pos].LowAddrBase = qcp->src_addr;
        qcb_sgl_d[sgl_pos].LowAddrBase = qcp->dst_addr;

        qcb_sgl_s[sgl_pos].DataLen = ck_size;
        qcb_sgl_d[sgl_pos].DataLen = ck_size;

        qcb_sgl_s[sgl_pos].Cfg = 0;
        qcb_sgl_d[sgl_pos].Cfg = 0;

        qcp->rm_size = rm_size;

        sgl_pos += 1; // do not wrap around yet
    }

    return sgl_pos;
}

static void qdma_issue(uint32_t tot_sz, uint32_t sgl_pos)
{
    /* Close SGLs, issue QDMA CD */

    NxpQdmaCLT_t *clt = &qcb_clts[qcb_ptr];

    tot_sz = swap_uint32(tot_sz);

    clt->sCmdListTable.DataLen = tot_sz;
    clt->dCmdListTable.DataLen = tot_sz;

    uint32_t sgl_off = sgl_ptr * sizeof(SourceDestinationFormat_t);

    clt->sCmdListTable.LowAddrBase = swap_uint32(ext_sgl_s + sgl_off);
    clt->dCmdListTable.LowAddrBase = swap_uint32(ext_sgl_d + sgl_off);

    qcb_sgl_s[sgl_pos - 1].Cfg = 0x00000080; // QDMA_SGT_F in LE
    qcb_sgl_d[sgl_pos - 1].Cfg = 0x00000080; // QDMA_SGT_F in LE

    iowrite32(0xc000b000, &qdma_regs->block[0].queue[0].bcqmr);

    sgl_ptr = (sgl_pos >= QDMA_SGL_NUM) ? 0 : sgl_pos;
    qcb_ptr = (qcb_ptr + 1) % QDMA_CMD_NUM;

    tp_stats.qd_nr += 1;
}

static void feca_cd_open()
{
    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_CMD_CB_ID].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(CDC_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_CMD_CB_ID].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_CD_CMD_CB_ID].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_IN_CB_ID].cb_start_addr);  fram_offst += FECA_FRAM_ALIGN(CDI_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_IN_CB_ID].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_CD_IN_CB_ID].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_OUT_CB_ID].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(CDO_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_OUT_CB_ID].cb_end_addr);
    iowrite32(0x80000100, &feca_regs->cb[FECA_CD_OUT_CB_ID].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_STS_CB_ID].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(STS_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CD_STS_CB_ID].cb_end_addr);
    iowrite32(0x80000010, &feca_regs->cb[FECA_CD_STS_CB_ID].cb_control);
}

static void feca_sd_open(uint8_t ch_id)
{
    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_CMD_CB_ID + ch_id].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(SDC_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_CMD_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_SD_CMD_CB_ID + ch_id].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_IN_CB_ID + ch_id].cb_start_addr);  fram_offst += FECA_FRAM_ALIGN(SDI_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_IN_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_SD_IN_CB_ID + ch_id].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_OUT_CB_ID + ch_id].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(SDO_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_OUT_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000800, &feca_regs->cb[FECA_SD_OUT_CB_ID + ch_id].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_STS_CB_ID + ch_id].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(STS_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SD_STS_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000010, &feca_regs->cb[FECA_SD_STS_CB_ID + ch_id].cb_control);
}

static void feca_ce_open()
{
    iowrite32(fram_offst, &feca_regs->cb[FECA_CE_CMD_CB_ID].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(CEC_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CE_CMD_CB_ID].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_CE_CMD_CB_ID].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_CE_IN_CB_ID].cb_start_addr);  fram_offst += FECA_FRAM_ALIGN(CEI_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CE_IN_CB_ID].cb_end_addr);
    iowrite32(0xa0000100, &feca_regs->cb[FECA_CE_IN_CB_ID].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_CE_OUT_CB_ID].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(CEO_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_CE_OUT_CB_ID].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_CE_OUT_CB_ID].cb_control);
}

static void feca_se_open(uint8_t ch_id)
{
    iowrite32(fram_offst, &feca_regs->cb[FECA_SE_CMD_CB_ID + ch_id].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(SEC_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SE_CMD_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_SE_CMD_CB_ID + ch_id].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_SE_IN_CB_ID + ch_id].cb_start_addr);  fram_offst += FECA_FRAM_ALIGN(SEI_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SE_IN_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000800, &feca_regs->cb[FECA_SE_IN_CB_ID + ch_id].cb_control);

    iowrite32(fram_offst, &feca_regs->cb[FECA_SE_OUT_CB_ID + ch_id].cb_start_addr); fram_offst += FECA_FRAM_ALIGN(SEO_CB_SZ);
    iowrite32(fram_offst, &feca_regs->cb[FECA_SE_OUT_CB_ID + ch_id].cb_end_addr);
    iowrite32(0x80000000, &feca_regs->cb[FECA_SE_OUT_CB_ID + ch_id].cb_control);
}

static void fq_cd_prep()
{
    uint32_t cdc_cap = CDC_CB_SZ / sizeof(feca_cd_command_t) / 2;

    if (cdc_cap * sizeof(feca_cd_command_t) > FECA_CBUF_SIZE)
        cdc_cap = FECA_CBUF_SIZE / sizeof(feca_cd_command_t);

    for (uint32_t i = 0; i < cdc_cap; ++i)
    {
        feca_cd_command_t *cdc = &cdc_bulk[i];

        fast_memcpy32(cdc, cdc_template, sizeof(feca_cd_command_t));

        cdc->cd_cfg1.complete_trig_en = ((i & 0x3f) == 0x3f || i == (cdc_cap - 1));
        cdc->cd_axi_data_addr_low = (uint32_t)fq_mem_out + FECA_DMA_ALIGN(app_rng() % MEM_OUT_SZ);

        if (cdc->cd_cfg1.complete_trig_en)
            cdc->cd_axi_stat_addr_low = (uint32_t)fq_mem_out + FECA_DMA_ALIGN(app_rng() % MEM_OUT_SZ);

        fast_swap32(cdc, sizeof(feca_cd_command_t));
    }

    feca_ch_params_t *cdc_qch = &qch_params[QCH_CD_CMD];

    cdc_qch->cb_idx  = FECA_CD_CMD_CB_ID;
    cdc_qch->cb_base = ioread32_fast(&feca_regs->cb[cdc_qch->cb_idx].cb_start_addr);
    cdc_qch->cb_cap  = ioread32_fast(&feca_regs->cb[cdc_qch->cb_idx].cb_end_addr) - cdc_qch->cb_base;
    cdc_qch->cb_mode = CB_MODE_PUSH;
    cdc_qch->th_size = cdc_cap * sizeof(feca_cd_command_t);

    uint32_t cdc_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + cdc_qch->cb_idx * FECA_CBUF_SIZE;

    cdc_qch->src_addr = swap_vint32(cdc_bulk);
    cdc_qch->dst_addr = swap_uint32(cdc_fram);

    feca_ch_params_t *cdi_qch = &qch_params[QCH_CD_INP];

    cdi_qch->cb_idx  = FECA_CD_IN_CB_ID;
    cdi_qch->cb_base = ioread32_fast(&feca_regs->cb[cdi_qch->cb_idx].cb_start_addr);
    cdi_qch->cb_cap  = ioread32_fast(&feca_regs->cb[cdi_qch->cb_idx].cb_end_addr) - cdi_qch->cb_base;
    cdi_qch->cb_mode = CB_MODE_PUSH;
    cdi_qch->th_size = cdi_qch->cb_cap / 2;

    uint32_t cdi_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + cdi_qch->cb_idx * FECA_CBUF_SIZE;

    cdi_qch->src_addr = swap_vint32(fq_mem_inp + FECA_DMA_ALIGN(app_rng() % MEM_INP_SZ));
    cdi_qch->dst_addr = swap_uint32(cdi_fram);
}

static void fq_sd_prep(uint8_t ch_id)
{
    uint32_t sdc_cap = SDC_CB_SZ / sizeof(feca_sd_command_t) / 2;

    if (sdc_cap * sizeof(feca_sd_command_t) > FECA_CBUF_SIZE)
        sdc_cap = FECA_CBUF_SIZE / sizeof(feca_sd_command_t);

    for (uint32_t i = 0; i < sdc_cap; ++i)
    {
        feca_sd_command_t *sdc = &sdc_bulk[i];

        fast_memcpy32(sdc, sdc_template, sizeof(feca_sd_command_t));
        sdc->sd_axi_data_addr_low = (uint32_t)fq_mem_out + FECA_DMA_ALIGN(app_rng() % MEM_OUT_SZ);
        sdc->sd_axi_stat_addr_low = (uint32_t)fq_mem_out + FECA_DMA_ALIGN(app_rng() % MEM_OUT_SZ);
        fast_swap32(sdc, sizeof(feca_sd_command_t));
    }

    feca_ch_params_t *sdc_qch = &qch_params[QCH_SD_CMD + ch_id];

    sdc_qch->cb_idx  = FECA_SD_CMD_CB_ID + ch_id;
    sdc_qch->cb_base = ioread32_fast(&feca_regs->cb[sdc_qch->cb_idx].cb_start_addr);
    sdc_qch->cb_cap  = ioread32_fast(&feca_regs->cb[sdc_qch->cb_idx].cb_end_addr) - sdc_qch->cb_base;
    sdc_qch->cb_mode = CB_MODE_PUSH;
    sdc_qch->th_size = sdc_cap * sizeof(feca_sd_command_t);

    uint32_t sdc_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + sdc_qch->cb_idx * FECA_CBUF_SIZE;

    sdc_qch->src_addr = swap_vint32(sdc_bulk);
    sdc_qch->dst_addr = swap_uint32(sdc_fram);

    feca_ch_params_t *sdi_qch = &qch_params[QCH_SD_INP + ch_id];

    sdi_qch->cb_idx  = FECA_SD_IN_CB_ID + ch_id;
    sdi_qch->cb_base = ioread32_fast(&feca_regs->cb[sdi_qch->cb_idx].cb_start_addr);
    sdi_qch->cb_cap  = ioread32_fast(&feca_regs->cb[sdi_qch->cb_idx].cb_end_addr) - sdi_qch->cb_base;
    sdi_qch->cb_mode = CB_MODE_PUSH;
    sdi_qch->th_size = sdi_qch->cb_cap / 2;

    uint32_t sdi_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + sdi_qch->cb_idx * FECA_CBUF_SIZE;

    sdi_qch->src_addr = swap_vint32(fq_mem_inp + FECA_DMA_ALIGN(app_rng() % MEM_INP_SZ));
    sdi_qch->dst_addr = swap_uint32(sdi_fram);
}

static void fq_ce_prep()
{
    uint32_t cec_cap = CEC_CB_SZ / sizeof(feca_ce_command_t) / 2;

    if (cec_cap * sizeof(feca_ce_command_t) > FECA_CBUF_SIZE)
        cec_cap = FECA_CBUF_SIZE / sizeof(feca_ce_command_t);

    for (uint32_t i = 0; i < cec_cap; ++i)
    {
        feca_ce_command_t *cec = &cec_bulk[i];

        fast_memcpy32(cec, cec_template, sizeof(feca_ce_command_t));
        cec->ce_axi_addr_low = (uint32_t)fq_mem_inp + FECA_DMA_ALIGN(app_rng() % MEM_INP_SZ);
        fast_swap32(cec, sizeof(feca_ce_command_t));
    }

    feca_ch_params_t *cec_qch = &qch_params[QCH_CE_CMD];

    cec_qch->cb_idx  = FECA_CE_CMD_CB_ID;
    cec_qch->cb_base = ioread32_fast(&feca_regs->cb[cec_qch->cb_idx].cb_start_addr);
    cec_qch->cb_cap  = ioread32_fast(&feca_regs->cb[cec_qch->cb_idx].cb_end_addr) - cec_qch->cb_base;
    cec_qch->cb_mode = CB_MODE_PUSH;
    cec_qch->th_size = cec_cap * sizeof(feca_ce_command_t);

    uint32_t cec_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + cec_qch->cb_idx * FECA_CBUF_SIZE;

    cec_qch->src_addr = swap_vint32(cec_bulk);
    cec_qch->dst_addr = swap_uint32(cec_fram);

    feca_ch_params_t *cei_qch = &qch_params[QCH_CE_INP];

    cei_qch->cb_idx  = FECA_CE_IN_CB_ID;
    cei_qch->cb_base = ioread32_fast(&feca_regs->cb[cei_qch->cb_idx].cb_start_addr);
    cei_qch->cb_cap  = ioread32_fast(&feca_regs->cb[cei_qch->cb_idx].cb_end_addr) - cei_qch->cb_base;
    cei_qch->cb_mode = CB_MODE_PUSH;
    cei_qch->th_size = cei_qch->cb_cap / 2;

    uint32_t cei_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + cei_qch->cb_idx * FECA_CBUF_SIZE;

    cei_qch->src_addr = swap_vint32(fq_mem_inp + FECA_DMA_ALIGN(app_rng() % MEM_INP_SZ));
    cei_qch->dst_addr = swap_uint32(cei_fram);

    feca_ch_params_t *ceo_qch = &qch_params[QCH_CE_OUT];

    ceo_qch->cb_idx  = FECA_CE_OUT_CB_ID;
    ceo_qch->cb_base = ioread32_fast(&feca_regs->cb[ceo_qch->cb_idx].cb_start_addr);
    ceo_qch->cb_cap  = ioread32_fast(&feca_regs->cb[ceo_qch->cb_idx].cb_end_addr) - ceo_qch->cb_base;
    ceo_qch->cb_mode = CB_MODE_PULL;
    ceo_qch->th_size = ceo_qch->cb_cap / 2;

    uint32_t ceo_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + ceo_qch->cb_idx * FECA_CBUF_SIZE;

    ceo_qch->src_addr = swap_uint32(ceo_fram);
    ceo_qch->dst_addr = swap_vint32(fq_mem_out + FECA_DMA_ALIGN(app_rng() % MEM_OUT_SZ));
}

static void fq_se_prep(uint8_t ch_id)
{
    uint32_t sec_cap = SEC_CB_SZ / sizeof(feca_se_command_t) / 2;

    if (sec_cap * sizeof(feca_se_command_t) > FECA_CBUF_SIZE)
        sec_cap = FECA_CBUF_SIZE / sizeof(feca_se_command_t);

    for (uint32_t i = 0; i < sec_cap; ++i)
    {
        feca_se_command_t *sec = &sec_bulk[i];

        fast_memcpy32(sec, sec_template, sizeof(feca_se_command_t));
        sec->se_axi_in_addr_low = (uint32_t)fq_mem_inp + FECA_DMA_ALIGN(app_rng() % MEM_INP_SZ);
        fast_swap32(sec, sizeof(feca_se_command_t));
    }

    feca_ch_params_t *sec_qch = &qch_params[QCH_SE_CMD + ch_id];

    sec_qch->cb_idx  = FECA_SE_CMD_CB_ID + ch_id;
    sec_qch->cb_base = ioread32_fast(&feca_regs->cb[sec_qch->cb_idx].cb_start_addr);
    sec_qch->cb_cap  = ioread32_fast(&feca_regs->cb[sec_qch->cb_idx].cb_end_addr) - sec_qch->cb_base;
    sec_qch->cb_mode = CB_MODE_PUSH;
    sec_qch->th_size = sec_cap * sizeof(feca_se_command_t);

    uint32_t sec_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + sec_qch->cb_idx * FECA_CBUF_SIZE;

    sec_qch->src_addr = swap_vint32(sec_bulk);
    sec_qch->dst_addr = swap_uint32(sec_fram);

    feca_ch_params_t *seo_qch = &qch_params[QCH_SE_OUT + ch_id];

    seo_qch->cb_idx  = FECA_SE_OUT_CB_ID + ch_id;
    seo_qch->cb_base = ioread32_fast(&feca_regs->cb[seo_qch->cb_idx].cb_start_addr);
    seo_qch->cb_cap  = ioread32_fast(&feca_regs->cb[seo_qch->cb_idx].cb_end_addr) - seo_qch->cb_base;
    seo_qch->cb_mode = CB_MODE_PULL;
    seo_qch->th_size = seo_qch->cb_cap / 2;

    uint32_t seo_fram = FECA_RAM_BASE + FECA_CBUF_OFFSET + seo_qch->cb_idx * FECA_CBUF_SIZE;

    seo_qch->src_addr = swap_uint32(seo_fram);
    seo_qch->dst_addr = swap_vint32(fq_mem_out + FECA_DMA_ALIGN(app_rng() % MEM_OUT_SZ));
}

static uint32_t fq_ch_ready(uint8_t qch)
{
    feca_ch_params_t *qcp = &qch_params[qch];

    uint32_t push = qcp->cb_mode == CB_MODE_PUSH;
    volatile feca_cb_regs_t *cbr = &feca_regs->cb[qcp->cb_idx];

    uint32_t nv_size = ioread32_fast(&cbr->cb_num_valid);
    uint32_t av_size = push ? qcp->cb_cap - nv_size : nv_size;

    if (av_size < qcp->th_size) return 0; // early exit

    uint32_t q_ptr = (push ? ioread32_fast(&cbr->cb_in_ptr)  : ioread32_fast(&cbr->cb_out_ptr)) - qcp->cb_base;
    uint32_t f_ptr = (push ? ioread32_fast(&cbr->cb_out_ptr) : ioread32_fast(&cbr->cb_in_ptr))  - qcp->cb_base;

    if (q_ptr != qcp->cb_tgt) return 0; // previous QDMA still in flight

    uint32_t av_spec = (f_ptr + qcp->cb_cap - q_ptr) % qcp->cb_cap;

    if (av_spec == 0) av_spec = av_size;
    if (av_spec < qcp->th_size) return 0;

    return av_spec;
}

static void fq_stats(user_if_t uif)
{
    uint32_t ts_l = mfpmr(PMR_PMC0);
    uint32_t ts_d = ts_l - global_ts;

    if (ts_d < PLAT_FREQ) return;

    for (uint32_t i = 0; i < 2; ++i)
    {
        uint32_t udf = ioread32_fast(&feca_regs->underflow[i]);
        if (udf) {PRINTF("UNDERFLOW[%u]=%08x\r\n", i, udf); while (1) {}}

        uint32_t ovf = ioread32_fast(&feca_regs->overflow[i]);
        if (ovf) {PRINTF("OVERFLOW[%u]=%08x\r\n", i, ovf); while (1) {}}
    }

    tp_stats.cd_tp = 0;
    tp_stats.sd_tp = 0;
    tp_stats.ce_tp = 0;
    tp_stats.se_tp = 0;

    uint32_t qd_tp = 0;

    for (uint32_t fch = 0; fch < QCH_IF_ENA; ++fch)
    {
        if (fch < QCH_CD_CMD + FECA_CD_NUM)
        {
            uint32_t job_size = ((feca_cd_command_t *)cdc_template)->cd_cfg1.K;
            tp_stats.cd_tp += qch_params[fch].tp_size / sizeof(feca_cd_command_t) * job_size;
        }
        else if (fch < QCH_SD_CMD + FECA_SD_NUM)
        {
            uint32_t job_size = ((feca_sd_command_t *)sdc_template)->sd_reserved;
            tp_stats.sd_tp += qch_params[fch].tp_size / sizeof(feca_sd_command_t) * job_size;
        }
        else if (fch < QCH_CE_CMD + FECA_CE_NUM)
        {
            uint32_t job_size = ((feca_ce_command_t *)cec_template)->ce_cfg1.K;
            tp_stats.ce_tp += qch_params[fch].tp_size / sizeof(feca_ce_command_t) * job_size;
        }
        else // qch < QCH_SE_CMD + FECA_SE_NUM
        {
            uint32_t job_size = ((feca_se_command_t *)sec_template)->se_axi_in_num_bytes;
            tp_stats.se_tp += qch_params[fch].tp_size / sizeof(feca_se_command_t) * job_size;
        }

        if (((1 << fch) & uif.fch_act) && !qch_params[fch].tp_size)
            {PRINTF("[ERROR] FCH%u is stuck\r\n", fch); while (1) {}}

        qd_tp += qch_params[fch].tp_size;
        qch_params[fch].tp_size = 0;
    }

    tp_stats.qd_tp = qd_tp;

    for (uint32_t qch = QCH_IF_ENA; qch < QCH_CH_NUM; ++qch)
    {
        tp_stats.qd_tp += qch_params[qch].tp_size;
        qch_params[qch].tp_size = 0;
    }

    if (uif.log_en)
        PRINTF("CD=%u SD=%u CE=%u SE=%u QD=%u QN=%u\r\n", // all units are Mbps
                tp_stats.cd_tp >> 20,
                tp_stats.sd_tp >> 17,
                tp_stats.ce_tp >> 20,
                tp_stats.se_tp >> 17,
     (uint32_t)(tp_stats.qd_tp >> 17),
                tp_stats.qd_nr);

    tp_stats.qd_nr = 0;

    global_ts = ts_l;
}

static void fq_feed_loop()
{
    while (1)
    {
        user_if_t uif = {.raw = swap_uint32(user_if.raw)};

        if (!uif.fch_act) {global_ts = mfpmr(PMR_PMC0); continue;}

        uint32_t qcbs_sz = 0;

        for (uint32_t qch = 0; qch < QCH_CH_NUM; ++qch)
        {
            uint32_t qch_cmp = qch;

            if (qch_cmp == QCH_CE_INP) qch_cmp  = QCH_CE_CMD;
            if (qch_cmp >= QCH_IF_ENA) qch_cmp -= QCH_IF_ENA;

            if (!((1 << qch_cmp) & uif.fch_act)) continue; // pair not enabled yet

            uint32_t av_size = fq_ch_ready(qch);
            if (!av_size) continue; // still crunching

            /* We have some available space at least as big as the threshold value */

            feca_ch_params_t *qcp = &qch_params[qch];

            uint32_t th_size = qcp->th_size;
            uint32_t rm_size = (qch < QCH_IF_ENA) ? (av_size / th_size) * th_size : th_size;

            qcp->rm_size  = rm_size;
            qcp->tp_size += rm_size;
            qcbs_sz      += rm_size;

            /* Enqueue for SGL preparation */
            qprep_d[qprep_p++] = qch; qprep_p %= QPREP_SIZE;
        }

        if (qcbs_sz) qdma_issue(qcbs_sz, qdma_prep());

        fq_stats(uif);
    }
}

int lava_feca_app()
{
    mem_init();
    qdma_init();

    feca_cd_open(); fq_cd_prep();

    for (uint8_t i = 0; i < FECA_SD_NUM; ++i)
        {feca_sd_open(i); fq_sd_prep(i);}

    feca_ce_open(); fq_ce_prep();

    for (uint8_t i = 0; i < FECA_SE_NUM; ++i)
        {feca_se_open(i); fq_se_prep(i);}

    user_if.log_en = 1;
    user_if.fch_act = (1 << QCH_IF_ENA) - 1;
    user_if.raw = swap_uint32(user_if.raw);

    fq_feed_loop();

    return 0;
}
