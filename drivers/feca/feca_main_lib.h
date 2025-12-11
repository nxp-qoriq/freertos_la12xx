// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef __FECA_MAIN_LIB_H__
#define __FECA_MAIN_LIB_H__

#include "tman_inline.h"

#include "feca_api.h"

#define FECA_DEV_NAME	"FECA_5G"

#define FECA_CB_AXI_OFFSET 0x01800000
#define FECA_CB_AXI_SIZE   0x00001000

/* Circular buffer control register */
#define CIRC_BUFF_DMA_DISABLE		29
#define CIRC_BUFF_EXT_DMA		30
#define CIRC_BUFF_ENABLE		31
#define CIRC_BUFF_TRANS_SIZE		0xFFFF

/* INTERRUPTS and their RESET values MACROS*/
#define FECA_INTR_DISABLE			0x0
#define FECA_SW_RESET				(1 << 0)

/* Memory Addressing of FECA */
#define FECA_HRAM_START			(FECA_RAM_BASE_ADDR + 0x0)
#define FECA_HRAM_SIZE			0x600000
#define FECA_FRAM_START			(FECA_RAM_BASE_ADDR + 0x01000000)
#define FECA_FRAM_SIZE			0xC0000

#define FECA_INVAL_ADDR			0xffffffff

/* Memory Alignment */
#define FECA_FRAM_ALIGNMENT 0x80
#define FECA_DMA_ALIGNMENT  0x10 // any FECA DMA address must be aligned to 16 bytes

/* Command Complete Interrupt Enable Masks*/
#define CMD_COMPLETE_SD_EN_MASK		0xFF
#define CMD_COMPLETE_SE_EN_MASK		0xFF00
#define CMD_COMPLETE_CD_EN_MASK		(0x1 << 16)
#define CMD_COMPLETE_CE_EN_MASK		(0x1 << 17)
#define CMD_COMPLETE_CD_ACK_EN_MASK	0xFF
#define CMD_COMPLETE_CD_CSI1_EN_MASK	0xFF00
#define CMD_COMPLETE_CD_CSI2_EN_MASK	0xFF0000
#define CMD_COMPLETE_CD_DCM_MASK	(CMD_COMPLETE_CD_ACK_EN_MASK | \
					CMD_COMPLETE_CD_CSI1_EN_MASK | \
					CMD_COMPLETE_CD_CSI2_EN_MASK)

/* Command Complete Interrupt Enable Bits*/
#define CMD_COMPLETE_SD_EN_BIT		0
#define CMD_COMPLETE_SE_EN_BIT		8
#define CMD_COMPLETE_CD_EN_BIT		16
#define CMD_COMPLETE_CE_EN_BIT		17
#define CMD_COMPLETE_CD_ACK_EN_BIT	0
#define CMD_COMPLETE_CD_CSI1_EN_BIT	8
#define CMD_COMPLETE_CD_CSI2_EN_BIT	16

#define	ST_NAME_MAX			32

typedef enum {
	FECA_5G = 0,
	FECA_DEV_MAX
	//FECA_WIFI
}feca_dev_id;

/* For representing the device current state */
typedef enum {
	FECA_STATE_UNINITIALIZED,
	FECA_STATE_INIT,
        FECA_STATE_OPEN,
        FECA_STATE_XFER,
        FECA_STATE_RESET
} feca_state_t;

/* For representing the device current state */
typedef enum {
	CHAIN_STATE_UNINITIALIZED,
	CHAIN_STATE_INIT,
	CHAIN_STATE_OPEN,
	CHAIN_STATE_XFER,
	CHAIN_STATE_RESET
} chain_state_t;

/* FOr channel current state representation */
typedef enum {
	CHANNEL_INITIALIZING_STATE,
        CHANNEL_OPERATIONAL_STATE,
        CHANNEL_BUSY_STATE,
        CHANNEL_ABORT_STATE,
        CHANNEL_XFER_STATE,
} feca_channel_state_t;

/* Channel Transfer Mode */
typedef enum {
        CH_AXI_SLAVE_TO_CB_CNTR = 1,
        CH_AXI_SLAVE_TO_CNTR_DEC,
        CH_CNTR_DEC_TO_AXI_MSTR,
        CH_AXI_SLAVE_TO_SH_DEC,
        CH_SH_DEC_TO_AXI_MSTR,
        CH_AXI_MSTR_TO_CNTR_ENC,
        CH_CNTR_ENC_AXI_SLAVE,
        CH_AXI_MSTR_TO_SH_ENC,
        CH_SH_ENC_TO_AXI_SLAVE,
} ch_trans_mod_t;

/* Memory Type */
typedef enum {
	FECA_FRAM = 0,
	FECA_HRAM,
	FECA_AXI_SLAVE,
	FECA_MEM_MAX
}feca_mem_t;

/* Memory to allocate for CD ACK/CSI1/CSI2 Circular Commands Buffer */
#define FECA_CD_DEMUX_CIRC_CMD_SIZE	0x400

/* Circular Buffer IP Registers */
struct circ_regs {
        volatile uint32_t circ_start;
        volatile uint32_t circ_end;
        volatile uint32_t circ_control;
        volatile uint32_t circ_in;
        volatile uint32_t circ_out;
        volatile uint32_t circ_num_valid;
        volatile uint32_t reserved[2];
}__attribute__((packed));

/* MSI IP Registers */
struct feca_msi_regs {
        volatile uint32_t msi_addr_low;
        volatile uint32_t msi_addr_high;
        volatile uint32_t msi_data;
}__attribute__((packed));

/* FECA Device IP Registers */
struct feca_ip_regs {
        volatile uint32_t control_status;
        volatile uint32_t underflow0;
        volatile uint32_t underflow1;
        volatile uint32_t underflow2;
        volatile uint32_t overflow0;
        volatile uint32_t overflow1;
        volatile uint32_t overflow2;
        volatile uint32_t cmd_complete_status;
        volatile uint32_t cmd_complete_irqen;
	volatile uint32_t dcm_complete_status;
	volatile uint32_t dcm_complete_irqen;
	uint32_t reserved0[21];
        volatile struct feca_msi_regs msi_regs[TB_MAX];
	uint32_t reserved1[8];
        struct circ_regs circ_buff[CIRC_BUF_NUM_MAX];
}__attribute__((packed));

/*
 * Struct for FECA Memories i.e. FRAM, HRAM
 * start	:- Start address of memory
 * end		:- End Memory address
 * current	:- Current address of memory (allocation is available)
 * size		:- size of the memory
 */
typedef struct {
	uint32_t start;
	uint32_t end;
	uint32_t current;
	uint32_t rmng_size;
} __attribute__((packed)) feca_resource_t;

/*
 * Structure for Channel Information
 * base			:- FRAM Base address linked with channel
 * size			:- Size on FRAM Memory
 * axi_addr		:- AXI bus address
 * trans_mode	:- transfer mode of channel
 * ch_type		:- Channel Type
 */
typedef struct {
	uint32_t base;
	uint32_t size;
	uint32_t axi_addr;
	ch_trans_mod_t trans_mode;
	feca_ch_type_t ch_type;
}__attribute__((packed)) ch_info_t;

/* Device Level Stats */
struct feca_dev_stats {
	uint32_t tx_dma;
	//TBD
} __attribute__((packed));

/* FECA Device Specific structure
 * feca_dev_name			:- device name
 * id_num					:- feca device id (required in case of multiple FECA instances)
 * dev_state				:- feca device current state
 * feca_dev_error_handler	:- Error handler for FECA Device
 * regs						:- FECA IP registers map
 * chain_count				:- Active chains (bit masking)
 * feca_resource			:- FECA Memory Allocation Information and its type (i.e. FRAM, HRAM)
 * feca_dev_chains			:- Chains handled by current FECA device
 * feca_stats				:- Device txn stats counter // TBD
 */
typedef struct feca_device{
        char feca_dev_name[ST_NAME_MAX];
        int id_num;
        feca_state_t dev_state;
        feca_dev_cb_t feca_dev_error_handler;
        struct feca_ip_regs *regs;
        feca_resource_t feca_resource[FECA_MEM_MAX];
        struct feca_chain *feca_dev_chains[FECA_CHAIN_MAX];
        struct feca_dev_stats feca_stats;
}__attribute__((packed)) feca_device_t;

/* FECA Chain structure will be representing the Encoding and decoding direction
 * chain_type	:-	Type of current chain
 * chain_error	:- 	Chain Specific Errors
 * Feca_channels:-	continuous channel allocation for the current chain
 * queue		:-	pointer to chain specific Job Queue
 * chain_handler:-  chain level handler
 */
struct feca_chain{
        chain_type_t chain_type;
        chain_state_t state;
        feca_error_t chain_error;
        struct feca_channel *feca_channels;
        feca_chain_cb_t chain_handler;
        feca_device_t *parent_feca_dev;
} __attribute__((packed));

/* Channel Specific Structure
 * id 			:- Channel Id of existing channel
 * channel_status	:- Channel Current Status
 * channel_info		:- Channel specific information
 * ch_regs		:- Channel Specific registers mapping
 * dispatch_cb 		:- Pointer to the call back function associated with channel
 * parent_chain		:- Parent chain pointer (channel is associated)
 * feca_ch_stats	:- Stats of txns happening on channel
 */
struct feca_channel {
        uint32_t id;
        feca_channel_state_t state;
        ch_info_t channel_info;
        volatile struct circ_regs *ch_regs;
        feca_chann_cb_t chann_handler;
        struct feca_chain *parent_chain;
        struct feca_channel_stats *feca_ch_stats; // keep track of transactions : send , completed , failed
        // Tracker for producer and consumer packet count
        /* Parameters added for FECA_FPGA */
} __attribute__((packed));

int geul_feca_init(feca_dev_id id);
void geul_feca_deinit(void);
#endif
