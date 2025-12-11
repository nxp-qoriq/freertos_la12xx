// SPDX-License-Identifier: BSD-3-Clause
/*
Copyright 2022 NXP
*/

#ifndef __LAVA_FECA_H__
#define __LAVA_FECA_H__

#include "stdint.h"

#define FECA_RAM_BASE 0xE5000000

#define FECA_FRAM_SZ 0x000C0000
#define FECA_HRAM_SZ 0x00600000

#define FECA_HRAM_OFFSET 0x00000000
#define FECA_FRAM_OFFSET 0x01000000
#define FECA_CBUF_OFFSET 0x01800000
#define FECA_CBUF_SIZE   0x00001000

#define FECA_FRAM_ALIGNMENT 0x80
#define FECA_DMA_ALIGNMENT  0x10

#define FECA_FRAM_ALIGN(SIZE) (((uint32_t)(SIZE) + FECA_FRAM_ALIGNMENT - 1) & (~(FECA_FRAM_ALIGNMENT - 1)))
#define FECA_DMA_ALIGN(SIZE)  (((uint32_t)(SIZE) + FECA_DMA_ALIGNMENT  - 1) & (~(FECA_DMA_ALIGNMENT  - 1)))

#define FECA_CD_IRQ_NUM 8

#define FECA_CD_NUM 1
#define FECA_CE_NUM 1
#define FECA_SD_NUM 8
#define FECA_SE_NUM 8

#define CDC_CB_SZ 0x5c00
#define CDI_CB_SZ 0x28000
#define CDO_CB_SZ FECA_CBUF_SIZE

#define SDC_CB_SZ 0x0200
#define SDI_CB_SZ 0xc000
#define SDO_CB_SZ FECA_CBUF_SIZE

#define CEC_CB_SZ 0x1c00
#define CEI_CB_SZ FECA_CBUF_SIZE
#define CEO_CB_SZ 0x1600

#define SEC_CB_SZ 0x0200
#define SEI_CB_SZ FECA_CBUF_SIZE
#define SEO_CB_SZ 0x3500

#define STS_CB_SZ 0x100

enum
{
    FECA_CD_CMD_CB_ID,
    FECA_SD_CMD_CB_ID = FECA_CD_CMD_CB_ID + FECA_CD_NUM,
    FECA_CE_CMD_CB_ID = FECA_SD_CMD_CB_ID + FECA_SD_NUM,
    FECA_SE_CMD_CB_ID = FECA_CE_CMD_CB_ID + FECA_CE_NUM,

    FECA_CD_IN_CB_ID  = FECA_SE_CMD_CB_ID + FECA_SE_NUM,
    FECA_CD_OUT_CB_ID = FECA_CD_IN_CB_ID  + FECA_CD_NUM,
    FECA_CD_STS_CB_ID = FECA_CD_OUT_CB_ID + FECA_CD_NUM,

    FECA_SD_IN_CB_ID  = FECA_CD_STS_CB_ID + FECA_CD_NUM,
    FECA_SD_OUT_CB_ID = FECA_SD_IN_CB_ID  + FECA_SD_NUM,
    FECA_SD_STS_CB_ID = FECA_SD_OUT_CB_ID + FECA_SD_NUM,

    FECA_CE_IN_CB_ID  = FECA_SD_STS_CB_ID + FECA_SD_NUM,
    FECA_CE_OUT_CB_ID = FECA_CE_IN_CB_ID  + FECA_CE_NUM,

    FECA_SE_IN_CB_ID  = FECA_CE_OUT_CB_ID + FECA_CE_NUM,
    FECA_SE_OUT_CB_ID = FECA_SE_IN_CB_ID  + FECA_SE_NUM,

    FECA_CD_ACK_CMD_CB_ID  = FECA_SE_OUT_CB_ID      + FECA_SE_NUM,
    FECA_CD_CSI1_CMD_CB_ID = FECA_CD_ACK_CMD_CB_ID  + FECA_SD_NUM,
    FECA_CD_CSI2_CMD_CB_ID = FECA_CD_CSI1_CMD_CB_ID + FECA_SD_NUM,

    FECA_CD_ACK_IN_CB_ID  = FECA_CD_CSI2_CMD_CB_ID + FECA_SD_NUM,
    FECA_CD_CSI1_IN_CB_ID = FECA_CD_ACK_IN_CB_ID   + FECA_SD_NUM,
    FECA_CD_CSI2_IN_CB_ID = FECA_CD_CSI1_IN_CB_ID  + FECA_SD_NUM,

    FECA_NUM_CB_IDS = FECA_CD_CSI2_IN_CB_ID + FECA_SD_NUM // should be equal to 111
};

typedef union
{
    struct
    {
        uint32_t                  : 9;
        uint32_t dma_err_cb       : 7;
        uint32_t                  : 5;
        uint32_t err_irqen        : 1;
        uint32_t                  : 1;
        uint32_t dma_err          : 1;
        uint32_t shared_enc_busy  : 1;
        uint32_t control_enc_busy : 1;
        uint32_t shared_dec_busy  : 1;
        uint32_t control_dec_busy : 1;
        uint32_t                  : 2;
        uint32_t mode_11ad        : 1;
        uint32_t sw_reset         : 1;
    };

    uint32_t raw;
} feca_control_status_t;

typedef union
{
    struct
    {
        uint32_t cg : FECA_CD_IRQ_NUM;
        uint32_t rv : 6;
        uint32_t ce : FECA_CE_NUM;
        uint32_t cd : FECA_CD_NUM;
        uint32_t se : FECA_SE_NUM;
        uint32_t sd : FECA_SD_NUM;
    };

    uint32_t raw;
} feca_cb_complete_t;

typedef union
{
    struct
    {
        uint32_t : 32 - 3 * FECA_SD_NUM;
        uint32_t csi2 : FECA_SD_NUM;
        uint32_t csi1 : FECA_SD_NUM;
        uint32_t ack  : FECA_SD_NUM;
    };

    uint32_t raw;
} feca_dcm_complete_t;

typedef struct
{
    uint16_t n_re;
    uint16_t n_ack_re;
    uint16_t n_csi1_re;
    uint16_t n_csi2_re;
    uint16_t n_ulsch_re;
    uint16_t d_ack;
    uint16_t d_csi1;
    uint16_t d_csi2;
    uint16_t n_ack2_re;
    uint16_t d_ack2;
} feca_dcm_info_t;

typedef struct
{
    volatile uint32_t addr_low;
    volatile uint32_t addr_high;
    volatile uint32_t addr_data;
} feca_msi_regs_t;

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

#define FECA_OVF_UDF_NUM_REGS ((FECA_CD_ACK_IN_CB_ID + 1 /* one bit gap after FECA_SE_OUT_CB_ID */ + 31) / 32)

typedef struct
{
    volatile feca_control_status_t control_status;

    volatile uint32_t underflow[FECA_OVF_UDF_NUM_REGS];
    volatile uint32_t  overflow[FECA_OVF_UDF_NUM_REGS];

    volatile feca_cb_complete_t cmd_complete_status;
    volatile feca_cb_complete_t cmd_complete_irqen;

    volatile feca_dcm_complete_t dcm_complete_status;
    volatile feca_dcm_complete_t dcm_complete_irqen;

    volatile feca_msi_regs_t msi[FECA_SD_NUM] __attribute__((aligned(0x80)));

    volatile feca_cb_regs_t cb[FECA_NUM_CB_IDS] __attribute__((aligned(0x80)));
} feca_ip_regs_t;

static feca_ip_regs_t * const feca_regs = (feca_ip_regs_t *)0xF9040000;

typedef union
{
    struct
    {
        uint32_t                     : 1;
        uint32_t irq_sel             : 3;
        uint32_t                     : 2;
        uint32_t K                   : 10;
        uint32_t                     : 1;
        uint32_t output_deint_bypass : 1;
        uint32_t input_deint_bypass  : 1;
        uint32_t complete_trig_en    : 1;
        uint32_t crc_type            : 3;
        uint32_t pc_en               : 1;
        uint32_t                     : 2;
        uint32_t rm_mode             : 2;
        uint32_t pd_n                : 4;
    };

    uint32_t raw;
} feca_cd_cfg1_t;

typedef union
{
    struct
    {
        uint32_t            : 2;
        uint32_t E_truncate : 14;
        uint32_t            : 2;
        uint32_t E          : 14;
    };

    uint32_t raw;
} feca_cd_cfg2_t;

typedef union
{
    struct
    {
        uint32_t           : 2;
        uint32_t pc_index2 : 10;
        uint32_t pc_index1 : 10;
        uint32_t pc_index0 : 10;
    };

    uint32_t raw;
} feca_cd_pe_indices_t;

typedef struct
{
    feca_cd_cfg1_t cd_cfg1;
    feca_cd_cfg2_t cd_cfg2;

    feca_cd_pe_indices_t cd_pe_indices;

    uint32_t cd_axi_data_addr_low;
    uint32_t cd_axi_data_addr_high;
    uint32_t cd_axi_stat_addr_low;
    uint32_t cd_axi_stat_addr_high;

    uint32_t cd_fz_lut[32];
} feca_cd_command_t;

typedef union
{
    struct
    {
        uint32_t                   : 6;
        uint32_t K                 : 10;
        uint32_t                   : 1;
        uint32_t output_int_bypass : 1;
        uint32_t input_int_bypass  : 1;
        uint32_t complete_trig_en  : 1;
        uint32_t crc_type          : 3;
        uint32_t pc_en             : 1;
        uint32_t dst_sel           : 2;
        uint32_t rm_mode           : 2;
        uint32_t pe_n              : 4;
    };

    uint32_t raw;
} feca_ce_cfg1_t;

typedef union
{
    struct
    {
        uint32_t crc_rnti : 16;
        uint32_t          : 2;
        uint32_t E        : 14;
    };

    uint32_t raw;
} feca_ce_cfg2_t;

typedef union
{
    struct
    {
        uint32_t                 : 23;
        uint32_t block_concat_en : 1;
        uint32_t out_pad_bytes   : 8;
    };

    uint32_t raw;
} feca_ce_cfg3_t;

typedef union
{
    struct
    {
        uint32_t           : 2;
        uint32_t pc_index2 : 10;
        uint32_t pc_index1 : 10;
        uint32_t pc_index0 : 10;
    };

    uint32_t raw;
} feca_ce_pe_indices_t;

typedef struct
{
    feca_ce_cfg1_t ce_cfg1;
    feca_ce_cfg2_t ce_cfg2;
    feca_ce_cfg3_t ce_cfg3;

    feca_ce_pe_indices_t ce_pe_indices;

    uint32_t ce_axi_addr_low;
    uint32_t ce_axi_addr_high;

    uint32_t ce_fz_lut[32];
} feca_ce_command_t;

typedef union
{
    struct
    {
        uint32_t max_num_iterations : 8;
        uint32_t min_num_iterations : 8;
        uint32_t remove_tb_crc      : 1;
        uint32_t tb_24_bit_crc      : 1;
        uint32_t                    : 5;
        uint32_t data_control_mux   : 1;
        uint32_t one_code_block     : 1;
        uint32_t lifting_index      : 3;
        uint32_t base_graph2        : 1;
        uint32_t set_index          : 3;
    };

    uint32_t raw;
} feca_sd_cfg1_t;

typedef union
{
    struct
    {
        uint32_t                  : 15;
        uint32_t send_msi         : 1;
        uint32_t                  : 3;
        uint32_t complete_trig_en : 1;
        uint32_t                  : 6;
        uint32_t compact_harq     : 1;
        uint32_t harq_en          : 1;
        uint32_t mod_order        : 4;
    };

    uint32_t raw;
} feca_sd_cfg2_t;

typedef union
{
    struct
    {
        uint32_t                  : 5;
        uint32_t num_output_bytes : 11;
        uint32_t                  : 8;
        uint32_t e_floor_thresh   : 8;
    };

    uint32_t raw;
} feca_sd_sizes1_t;

typedef union
{
    struct
    {
        uint32_t                 : 2;
        uint32_t num_filler_bits : 14;
        uint32_t                 : 2;
        uint32_t bits_per_cb     : 14;
    };

    uint32_t raw;
} feca_sd_sizes2_t;

typedef union
{
    struct
    {
        uint32_t max_req_bytes : 16;
        uint32_t               : 7;
        uint32_t pack          : 1;
        uint32_t               : 2;
        uint32_t llrs_per_re   : 6;
    };

    uint32_t raw;
} feca_sd_bits_per_re_t;

typedef struct
{
    feca_sd_cfg1_t sd_cfg1;
    feca_sd_cfg2_t sd_cfg2;

    feca_sd_sizes1_t sd_sizes1;
    feca_sd_sizes2_t sd_sizes2;

    uint32_t sd_circ_buf;

    uint32_t sd_floor_num_input_bytes;
    uint32_t sd_ceiling_num_input_bytes;

    uint32_t sd_hram_base;

    uint32_t sd_sc_x1_init;
    uint32_t sd_sc_x2_init;

    uint32_t sd_axi_data_addr_low;
    uint32_t sd_axi_data_addr_high;

    uint32_t sd_reserved;

    uint32_t sd_axi_stat_addr_low;
    uint32_t sd_axi_stat_addr_high;

    uint32_t sd_cb_mask[8];

    uint16_t sd_di_start_ofst_floor[8];
    uint16_t sd_di_start_ofst_ceiling[8];
} feca_sd_command_t;

typedef union
{
    struct
    {
        uint32_t out_pad_bytes    : 8;
        uint32_t num_code_blocks  : 8;
        uint32_t                  : 1;
        uint32_t tb_24_bit_crc    : 1;
        uint32_t                  : 1;
        uint32_t complete_trig_en : 1;
        uint32_t mod_order        : 4;
        uint32_t data_control_mux : 1;
        uint32_t lifting_index    : 3;
        uint32_t base_graph2      : 1;
        uint32_t set_index        : 3;
    };

    uint32_t raw;
} feca_se_cfg1_t;

typedef union
{
    struct
    {
        uint32_t                 : 5;
        uint32_t num_input_bytes : 11;
        uint32_t                 : 8;
        uint32_t e_floor_thresh  : 8;
    };

    uint32_t raw;
} feca_se_sizes1_t;

typedef union
{
    struct
    {
        uint32_t             : 23;
        uint32_t p_ack       : 1;
        uint32_t             : 2;
        uint32_t bits_per_re : 6;
    };

    uint32_t raw;
} feca_se_bits_per_re_t;

typedef struct
{
    feca_se_cfg1_t se_cfg1;

    feca_se_sizes1_t se_sizes1;

    uint32_t se_circ_buf;

    uint32_t se_floor_num_output_bits;
    uint32_t se_ceiling_num_output_bits;

    uint32_t se_sc_x1_init;
    uint32_t se_sc_x2_init;

    uint32_t se_axi_in_addr_low;
    uint32_t se_axi_in_addr_high;

    uint32_t se_axi_in_num_bytes;

    uint32_t se_cb_mask[8];

    uint16_t se_di_start_ofst_floor[8];
    uint16_t se_di_start_ofst_ceiling[8];
} feca_se_command_t;

#endif /* __LAVA_FECA_H__ */
