// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#ifndef __FECA_API_H__
#define __FECA_API_H__

/**
 * @file        feca_api.h
 * @brief       FECA related APIs.
 * @addtogroup  FECA_API
 * @{
 */

#include "fsl_dbg.h"
#include "fsl_malloc.h"

#include <geul_feca.h>

#define FECA_DEBUG	0
#define FECA_DEBUG_CB	0

typedef void *dev_handle_t;
typedef void *chain_handle_t;
typedef void *ch_handle_t;

#define bool	_Bool
#define CD_CH_NUM_NON_DCM	4
/**< Number of Control Decode channels - Non DCM Mode*/
#define CD_CH_NUM_DCM	48
/**< Number of Control Decode channels - DCM Mode*/

#define CD_CH_NUM	(CD_CH_NUM_NON_DCM + CD_CH_NUM_DCM)
/**< Number of Control Decode channels */
#define SD_CH_NUM	32
/**< Number of Shared Decode channels */
#define CE_CH_NUM	3
/**< Number of Control Encode channels */
#define SE_CH_NUM	24
/**< Number of Shared Encode channels */

/** Types of FECA errors */
typedef enum {
	FECA_SUCCESS = 0, /**< No Error */
	FECA_NO_DEV = 1, /**< Invalid Device handle */
	FECA_NO_CHAIN, /**< Invalid Chain handle */
	FECA_INV_JOB_TYPE, /**< Invalid FECA job type */
	FECA_JOB_DISPATCH_FAILED, /**< Job is not dispatched */
	FECA_JOB_NOT_COMPLETE, /**< Job is not completed */
	FECA_ERROR /**< Indicates Error/Failure */
	//TBD all errors
} feca_error_t;

typedef feca_error_t status_t;

/* Events For Device handler */
typedef enum {
	SW_RESET,
	CD_BUSY,
	SD_BUSY,
	CE_BUSY,
	SE_BUSY,
	DMA_RESET,
	DMA_SW_RESET_COMPLETE
	//MSI Related Events
	//TBD
}feca_dev_events_t;

/* Events for chain handler */
typedef enum {
	CMD_COMPLETE_SD = 1,
	CMD_COMPLETE_SE,
	CMD_COMPLETE_CD,
	CMD_COMPLETE_CE
	//TBD
}feca_chain_events_t;

/* Events for Channel handler */
typedef enum {
	UNDERFLOW0,
	UNDERFLOW1,
	UNDERFLOW2,
	OVERFLOW0,
	OVERFLOW1,
	OVERFLOW2
}feca_ch_events_t;

/** FECA Channel Types */
typedef enum {
	INVALID_CH = -1, /**< Invalid channel */
	CH_CMD = 0, /**< Command channel */
	CH_IN = 1, /**< Input channel */
	CH_OUT, /**< Output channel */
	CH_CRC_OUT, /**< CRC output channel */
	CH_CMD_DCM_ACK, /**< Command DCM ACK channel */
	CH_CMD_DCM_CSI1,/**< Command DCM CSI1 channel */
	CH_CMD_DCM_CSI2,/**< Command DCM CSI2 channel */
	CH_IN_DCM_ACK, /**< Input DCM ACK channel */
	CH_IN_DCM_CSI1,/**< Input DCM CSI1 channel */
	CH_IN_DCM_CSI2 /**< Input DCM CSI2 channel */
}feca_ch_type_t;

/** Total of eight Transport blocks are supported for concurrent processing.
 * (TB_0, TB_1 ... TB_7)
 */
typedef enum {
	TB_0,
	TB_1,
	TB_2,
	TB_3,
	TB_4,
	TB_5,
	TB_6,
	TB_7,
	TB_MAX
}tb_num;

/* FECA IP Registers Specific MACROS */
#define FECA_CD_NUM 1
#define FECA_CE_NUM 1
#define FECA_SD_NUM 8
#define FECA_SE_NUM 8
enum
{
    FECA_CD_CMD_CB_ID = 0,
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

    CIRC_BUF_NUM_MAX = FECA_CD_CSI2_IN_CB_ID + FECA_SD_NUM // should equal 111
};

/** Chain configuration parameters */
typedef struct chain_param {
	chain_type_t type;
	/**< Type of Chain (CD, SD, CE or SE) */
	uint8_t irq_mask;
	/**< Enables/Disables interrupt on command completion */
	uint8_t dcm_irq_mask;
	/**< DCM (ACK + CSI1 + CSI2) enables/disables interrupt on command completion */
}chain_param_t;

/** Channel configuration parameters */
typedef struct ch_param {
	feca_ch_type_t	type;
	/**< Type of channel. (CH_CMD, CH_IN etc.) */
	uint32_t	tb_num;
	/**< Transport block id */
	uint32_t	ch_size;
	/**< Size of channel circular buffer */
	uint16_t	xfer_size;
	/**< DMA transfer size. The size in bytes of
	 * the chunk of data that will be transferred.
	 */
	bool		dma_disable;
	/**< When set, FECA internal DMA will be disabled */
	bool		ext_dma;
	/**< External DMA. When set, the circular buffer
	 * takes data from an external DMA (qDMA)
	 */
}ch_param_t;

/** Callback function for device Error handling*/
typedef void *(*feca_dev_cb_t) (dev_handle_t, feca_dev_events_t, void *);

/** Callback Function for Chain Error Handling */
typedef void *(*feca_chain_cb_t) (chain_handle_t, feca_chain_events_t, void *);

/** Callback function associated with the channel */
typedef void *(*feca_chann_cb_t) (ch_handle_t, feca_ch_events_t, void *);

/* FECA Device Functions */
/**
 * Create the FECA top-level Device object
 *
 * @param feca_dev_name FECA
 * Device name ("FECA-5G", "FECA-WIGIG")
 * @param feca_dev_error_handler
 * FECA Device error handler provided by user
 * @return
 * Handle to newly created object
 */
dev_handle_t feca_dev_open(char *feca_dev_name, feca_dev_cb_t feca_dev_error_handler);

/**
 * Resets the FECA Device
 *
 * @param dev
 * FECA Device handle (obtained with the feca_dev_open call)
 * @return
 * Status describing the outcome of the reset operation
 */
status_t feca_dev_reset(dev_handle_t dev);

/**
 * Destroys/Closes the FECA Device
 * @param dev
 * FECA Device handle (obtained with the feca_dev_open call)
 * @return
 * Status describing the outcome of the close operation
 */
status_t feca_dev_close(dev_handle_t dev);

/* FECA Chain Functions */
/**
 * Creates a FECA Chain object
 *
 * @param dev
 * FECA Device handle (obtained with the feca_dev_open call)
 * @param chain_param
 * FECA Chain parameters, such as chain type("CD", "CE", "SD", "SE"), and so on
 * @param feca_chain_error_handler
 * FECA Chain error handler provided by user
 * @return
 * Handle to newly created object or NULL pointer in case of error.
 */
chain_handle_t feca_chain_open(dev_handle_t dev, chain_param_t chain_param,
				feca_chain_cb_t feca_chain_error_handler);

/**
 * Destroys/Closes the given FECA Chain
 *
 * @param chain
 * FECA Chain handle (obtained with the feca_chain_open call)
 * @return
 * Status describing the outcome of the close operation
 */
status_t feca_chain_close(chain_handle_t chain);

/* FECA Channel Functions */
/**
 * Opens a FECA Channel for a given chain
 *
 * @param chain
 * FECA Chain handle (obtained with the feca_chain_open call)
 * @param ch_param
 * FECA Channel parameters structure of type ch_param_t
 * @return
 * Handle to newly created object or NUll in case of error.
 */
ch_handle_t feca_ch_open(chain_handle_t chain, ch_param_t ch_param);

/**
 * Destroys/Closes the FECA Channel
 * @note The API is not fully supported yet.
 *
 * @param ch
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param feca_ch
 * FECA channel type
 * @return
 * Status describing the outcome of the reset operation
 */
status_t feca_ch_close(ch_handle_t ch, feca_ch_type_t feca_ch);

/**
 * Dispatches a FECA Job
 * @note This function will not wait for the job completion.
 * It just submits the job and return.
 *
 * @param chain
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param feca_job
 * FECA Job Dispatch parameters structure that contains details of job
 * @return
 * FECA_SUCCESS or Failure in case of error.
 */
status_t feca_job_dispatch(chain_handle_t chain, feca_job_t *feca_job);

/**
 * Get status of a given FECA Job
 * @note This function is not blocking
 *
 * @param chain
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param feca_job
 * FECA Job Dispatch parameters structure that contains details of job
 * @return
 * FECA_SUCCESS if job is completed, FECA_JOB_NOT_COMPLETE if job is
 * still pending or Failure in case of error.
 */
status_t feca_get_job_status(chain_handle_t chain, feca_job_t *feca_job);

/**
 * Get status of a given FECA SD Job
 * @note This function is not blocking and provides lower latency
 * for checking SD job status than feca_get_job_status() API
 *
 * @param chain
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param feca_job
 * FECA Job Dispatch parameters structure that contains details of job
 * @return
 * FECA_SUCCESS if job is completed, FECA_JOB_NOT_COMPLETE if job is
 * still pending or Failure in case of error.
 */
status_t feca_get_sd_job_status(chain_handle_t chain, feca_job_t *feca_job);

/**
 * Get Valid number of valid bytes for OUT channel
 * @note This function is not blocking
 *
 * @param chain
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param tb_num
 * Transport block id
 * @return
 * Number of valid bytes in the output channel
 */
uint32_t feca_ch_out_valid_bytes(chain_handle_t chain, uint32_t tb_num);

/**
 * Get Valid number of valid bytes for IN channel
 * @note This function is not blocking
 *
 * @param chain
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param tb_num
 * Transport block id
 * @return
 * Number of valid bytes in the input channel
 */
uint32_t feca_ch_in_valid_bytes(chain_handle_t chain, uint32_t tb_num);

/**
 * Get the number of valid bytes for DCM IN channels (CB 87 - 111)
 * @note This function is not blocking
 *
 * @param chain
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param ch_type
 * FECA Channel type
 * @param tb_num
 * Transport block id
 * @return
 * Number of valid bytes in the input DCM channel
 */
uint32_t feca_dcm_ch_in_valid_bytes(chain_handle_t chain, feca_ch_type_t ch_type, uint32_t tb_num);

/**
 * Get address of Circular  buffer for a given channel.
 * Using this address, Input data can be written or Output data can be read.
 *
 * @param chain_handle
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param ch_id
 * FECA Channel Id (obtained from feca_get_ch_id call)
 * @return
 * A 32-bit pointer to circular buffer
 */
uint32_t *feca_get_ch_axi_addr(chain_handle_t chain_handle, uint32_t ch_id);

/**
 * Get address of HARQ AXI slave address.
 *
 * @param chain_handle
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @return
 * A 32-bit pointer to HARQ AXI slave address
 */
uint32_t *feca_get_harq_axi_addr(chain_handle_t chain_handle);

/**
 * Dump the FECA circular buffer registers
 *
 * @param chain_handle
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param tb_num
 * Transport block id
 * @param ch_type
 * Type of the FECA channel
 */
void feca_dump_cb_reg(chain_handle_t *chain_handle, uint32_t tb_num,
		job_type_t ch_type);


struct feca_circ_buf_regs {
	uint32_t cmd_id;
	uint32_t in_id;
	uint32_t out_id;
	uint32_t crc_id;
	uint32_t cmd_chid;
	uint32_t in_chid;
	uint32_t out_chid;
	uint32_t crc_chid;
	uint32_t cmd_cb_start;
	uint32_t cmd_cb_end;
	uint32_t cmd_cb_ctrl;
	uint32_t cmd_cb_in;
	uint32_t cmd_cb_out;
	uint32_t cmd_cb_valid;
	uint32_t in_cb_start;
	uint32_t in_cb_end;
	uint32_t in_cb_ctrl;
	uint32_t in_cb_in;
	uint32_t in_cb_out;
	uint32_t in_cb_valid;
	uint32_t out_cb_start;
	uint32_t out_cb_end;
	uint32_t out_cb_ctrl;
	uint32_t out_cb_in;
	uint32_t out_cb_out;
	uint32_t out_cb_valid;
	uint32_t crc_cb_start;
	uint32_t crc_cb_end;
	uint32_t crc_cb_ctrl;
	uint32_t crc_cb_in;
	uint32_t crc_cb_out;
	uint32_t crc_cb_valid;
};

void feca_get_cb_reg(chain_handle_t *chain_handle,
		uint32_t tb_num, job_type_t ch_type,
		struct feca_circ_buf_regs *feca_cb_regs);

void feca_print_cb_reg(struct feca_circ_buf_regs *feca_cb_regs);

/**
 * Get Channel Id
 *
 * @param chain_handle
 * FECA Channel handle (obtained with the feca_ch_open call)
 * @param ch_type
 * FECA Channel type
 * @param tb_num
 * Transport block id
 * @return
 * A 32-bit Channel Id
 */
uint32_t feca_get_ch_id(chain_handle_t chain_handle, feca_ch_type_t ch_type, uint32_t tb_num);

/** @} */
#endif /* __FECA_HEADER_H */
