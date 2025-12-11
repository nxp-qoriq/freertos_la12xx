// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022 NXP
 */

#include "geul_bsp_init.h"
#include "feca_internal.h"
#include "feca_lib_sl.h"

#define FECA_CMD_COMPLETE_TRIG_FLAG	0x00001000

#define FECA_VALIDATION	0
#if FECA_VALIDATION
int do_feca_validation(void);
#endif
#define CHECK_FLOW(arg) ({				\
				uint32_t count = 0x01, pos = 0x0; \
				while(!(count & ioread32((const volatile unsigned long *)arg))){ \
					count <<= 1; \
					pos++;	\
					if(pos > 32) \
						break; \
				}\
				if(pos < 32) \
					iowrite32((0x01 << pos),(volatile uint32_t *) arg); \
			})

struct feca_channel *channel_alloc(int count)
{
	struct feca_channel *channels = NULL;
	
	channels = feca_alloc_mem(sizeof(struct feca_channel) * count);
	return channels;
}

void *feca_ch_handler(struct feca_chain *chain, feca_ch_events_t event, void *cookiee)
{

	UNUSED(cookiee);
	/* For Overflow and Underflow Handling */
	struct feca_device *feca_dev = chain->parent_feca_dev;

	PRINTF("FECA CHAIN %d handler called with event %d\n",chain->chain_type, event);	
	switch (event) {
	
	case UNDERFLOW0:
			CHECK_FLOW(feca_dev->regs->underflow0);
			break;

	case UNDERFLOW1:
			CHECK_FLOW(feca_dev->regs->underflow1);
			break;

	case UNDERFLOW2:
			CHECK_FLOW(feca_dev->regs->underflow2);
			break;
	case OVERFLOW0:
			CHECK_FLOW(feca_dev->regs->overflow0);
			break;
	case OVERFLOW1:
			CHECK_FLOW(feca_dev->regs->overflow1);
			break;
	case OVERFLOW2:
			CHECK_FLOW(feca_dev->regs->overflow2);
			break;
	}
	return NULL;
}

struct feca_channel *feca_channel_init(struct feca_chain *chain)
{
	struct feca_channel *channels = NULL, *dcm_channels;
	uint8_t i = 0, j = 0;
	chain_type_t chain_type = chain->chain_type;
	
	switch(chain_type){
	
	case FECA_CD_CHAIN:
		channels = channel_alloc(CD_CH_NUM);
		if(!channels) {
			pr_err("feca_channel DS Allocation failed \n");
			return NULL;
		}

		for(i = CH_CMD; i <= CH_CRC_OUT; i++) {
			(channels + i)->id = feca_channel_ids.cd_ch_ids[i][0];
			(channels + i)->state = CHANNEL_INITIALIZING_STATE;
			(channels + i)->parent_chain = chain;
			(channels + i)->ch_regs = chain->parent_feca_dev->regs->circ_buff + ((channels + i)->id); // CB registers
		}

		dcm_channels = &channels[CH_CMD_DCM_ACK];
		for(i = CH_CMD_DCM_ACK; i <= CH_IN_DCM_CSI2; i++)
		{
			int idx = i - CH_CMD_DCM_ACK;
			for(j = 0; j < TB_MAX; j++)
			{
				(dcm_channels + (idx * TB_MAX) + j)->id = feca_channel_ids.cd_ch_ids[i][j];
				(dcm_channels + (idx * TB_MAX) + j)->state = CHANNEL_INITIALIZING_STATE;
				(dcm_channels + (idx * TB_MAX) + j)->parent_chain = chain;
				(dcm_channels + (idx * TB_MAX) + j)->ch_regs = (chain->parent_feca_dev->regs->circ_buff +
						((dcm_channels + (idx * TB_MAX) + j)->id));
			}
		}

		break;

	case FECA_SD_CHAIN:
		channels = channel_alloc(SD_CH_NUM);
		if(!channels)
			return NULL;

		for(i = CH_CMD; i <= CH_CRC_OUT; i++) 
		{
			for(j = 0; j < TB_MAX; j++) 
			{
				(channels + (i * TB_MAX) + j)->id = feca_channel_ids.sd_ch_ids[i][j];
				(channels + (i * TB_MAX) + j)->state = CHANNEL_INITIALIZING_STATE;
				(channels + (i * TB_MAX) + j)->parent_chain = chain;
				(channels + (i * TB_MAX) + j)->ch_regs = (chain->parent_feca_dev->regs->circ_buff +
						((channels + (i * TB_MAX) + j)->id));
			}
		}
		
		break;
		
	case FECA_CE_CHAIN:
		channels = channel_alloc(CE_CH_NUM);
		if(!channels)
			return NULL;

		for(i = CH_CMD; i <= CH_OUT; i++) {
			(channels + i)->id = feca_channel_ids.ce_ch_ids[i];
			(channels + i)->state = CHANNEL_INITIALIZING_STATE;
			(channels + i)->parent_chain = chain;
			(channels + i)->ch_regs = (chain->parent_feca_dev->regs->circ_buff + ((channels + i)->id));
		}

		break;

	case FECA_SE_CHAIN:
		channels = channel_alloc(SE_CH_NUM);
		if(!channels)
			return NULL;

		for(i = CH_CMD; i <= CH_OUT; i++) {
			for(j = 0; j < TB_MAX; j++) {
				(channels + (i * TB_MAX) + j)->id = feca_channel_ids.se_ch_ids[i][j];
				(channels + (i * TB_MAX) + j)->state = CHANNEL_INITIALIZING_STATE;
				(channels + (i * TB_MAX) + j)->parent_chain = chain;
				(channels + (i * TB_MAX) + j)->ch_regs = (chain->parent_feca_dev->regs->circ_buff +
						((channels + (i * TB_MAX) + j)->id));
			}
		}

		break;

	case FECA_CHAIN_MAX:
		return NULL;
	}
	return channels;
}

dev_handle_t feca_dev_open(char *feca_dev_name, feca_dev_cb_t feca_dev_error_handler)
{
	feca_dev_id id;
	int32_t ret = -1;
	feca_device_t *feca_dev;

	for(id = FECA_5G; id < FECA_DEV_MAX; id++)
	{
		feca_dev = bsp_get_feca_dev(id);
		if(NULL == feca_dev)
			break;

		ret = strcmp(feca_dev_name, feca_dev->feca_dev_name);
		if(0 == ret)
			break;
	}
	/* Device exist with feca_dev_name */
	if(0 == ret)
	{
		if(FECA_STATE_OPEN <= feca_dev->dev_state)
		{
			pr_err("%s: DEV already open\n",__func__);
			return feca_dev;
		}
		else
		{
			/* Update error handler in feca_dev*/
			feca_dev->feca_dev_error_handler = feca_dev_error_handler;
	
			/* Update feca dev state */
			feca_dev->dev_state = FECA_STATE_OPEN;
			return feca_dev;
		}
	}
	pr_err("%s: DEV not found [%s]\n",__func__, feca_dev_name);
	return NULL;
}

status_t feca_dev_reset(dev_handle_t dev)
{
	uint64_t tmp_start = 0, tmp_end = 0;
	struct feca_device *feca_dev = (struct feca_device *)dev;
	struct feca_ip_regs *feca_regs;
	uint32_t reg;
	int i;
	
	/*Checking for the valid feca_dev*/
	if(!feca_dev)
		return FECA_NO_DEV;
	
	feca_dev->dev_state = FECA_STATE_RESET;
	feca_regs = feca_dev->regs;
	
	/*Disableing the device Interrupts */
	feca_regs->cmd_complete_irqen = FECA_INTR_DISABLE;

	/* Initiate reset via Control reg. */
	reg = FECA_SW_RESET;
	iowrite32(reg, &feca_dev->regs->control_status);

	/*wait for 32 clock cycles to be complete */
	tman_get_timestamp(&tmp_start);
	tman_get_timestamp(&tmp_end);
	while((tmp_end - tmp_start) < 32) {
		tman_get_timestamp(&tmp_end);
		}

	reg = ioread32(&feca_dev->regs->control_status);
	reg &= ~FECA_SW_RESET;
	iowrite32(reg, &feca_dev->regs->control_status);

	/* Reset the FECA FRAM allocation */
	feca_dev->feca_resource[FECA_FRAM].start = FECA_FRAM_START;
	feca_dev->feca_resource[FECA_FRAM].rmng_size = FECA_FRAM_SIZE;
	feca_dev->feca_resource[FECA_FRAM].end = FECA_FRAM_START + FECA_FRAM_SIZE;
	feca_dev->feca_resource[FECA_FRAM].current = FECA_FRAM_START;

	feca_dev->dev_state = FECA_STATE_INIT;
	for (i = 0; i < FECA_CHAIN_MAX; i++)
		feca_dev->feca_dev_chains[i]->state = CHAIN_STATE_INIT;

	/*Set state*/
	feca_dev->dev_state = FECA_STATE_OPEN;

	return FECA_SUCCESS;
}

status_t feca_dev_close(dev_handle_t dev )
{
	struct feca_device *feca_dev = (struct feca_device *)dev;
	struct feca_ip_regs *feca_regs;

	/*Checking for the valid feca_dev*/
	if(!feca_dev)
		return FECA_NO_DEV;

	feca_dev->dev_state = FECA_STATE_INIT;
	feca_regs = feca_dev->regs;

	/*Disableing the device Interrupts */
	feca_regs->cmd_complete_irqen = FECA_INTR_DISABLE;

	/*Clear the chains and channels via sub-routine*/

	/*Wait for the completion of the current transaction to be complete*/

	/* Unmap the IP Registers */
	feca_dev->regs = NULL;

	/*Destroy feca_device and setting it to NULL*/
	feca_mem_free((void *)feca_dev);
	feca_dev = NULL;

	return FECA_SUCCESS;
}

chain_handle_t feca_chain_open(dev_handle_t dev, chain_param_t chain_param,
							   feca_chain_cb_t feca_chain_error_handler)
{
	feca_device_t *feca_dev = (feca_device_t *)dev;
	struct feca_chain *feca_dev_chain = NULL;
	uint32_t reg, dcm_reg;

	/* Checking for the valid feca_dev */
	if(NULL == feca_dev) {
		pr_err("%s: Not a Valid feca_device\n", __func__);
		return NULL;
	}

	/* Validate chain type */
	if(FECA_CHAIN_MAX <= chain_param.type)
	{
		pr_err("%s: Invalid chain type [%d]\n", __func__, chain_param.type);
		return NULL;
	}

	feca_dev_chain = feca_dev->feca_dev_chains[chain_param.type];

	if(CHAIN_STATE_OPEN <= feca_dev_chain->state)
	{
		pr_err("%s: Chain already open. state[%d]\n", __func__, feca_dev_chain->state);
		return NULL;
	}

	feca_dev_chain->chain_handler = feca_chain_error_handler;

#ifndef ABERDEEN
	/* Enable the Chain Specific Interrupts */
	reg = ioread32(&feca_dev->regs->cmd_complete_irqen);
	switch(feca_dev_chain->chain_type) {
	case FECA_CD_CHAIN:
		if(chain_param.irq_mask & 0x1)
		{
			reg |= CMD_COMPLETE_CD_EN_MASK;
			iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		}

		if(chain_param.dcm_irq_mask)
		{
			dcm_reg = ioread32(&feca_dev->regs->dcm_complete_irqen);
			dcm_reg |= (chain_param.dcm_irq_mask << CMD_COMPLETE_CD_ACK_EN_BIT);
			dcm_reg |= (chain_param.dcm_irq_mask << CMD_COMPLETE_CD_CSI1_EN_BIT);
			dcm_reg |= (chain_param.dcm_irq_mask << CMD_COMPLETE_CD_CSI2_EN_BIT);
			iowrite32(dcm_reg, &feca_dev->regs->dcm_complete_irqen);
		}
		break;
		
	case FECA_SD_CHAIN:
		if(chain_param.irq_mask)
		{
			reg |= (chain_param.irq_mask << CMD_COMPLETE_SD_EN_BIT);
			iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		}
		break;
		
	case FECA_CE_CHAIN:
		if(chain_param.irq_mask & 0x1)
		{
			reg |= CMD_COMPLETE_CE_EN_MASK;
			iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		}
		break;

	case FECA_SE_CHAIN:
		if(chain_param.irq_mask)
		{
			reg |= (chain_param.irq_mask << CMD_COMPLETE_SE_EN_BIT);
			iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		}
		break;

	case FECA_CHAIN_MAX:
		return NULL;
	}
#endif 

	feca_dev_chain->state = CHAIN_STATE_OPEN;
	feca_dev->dev_state = FECA_STATE_XFER;

	return feca_dev_chain;
}

status_t feca_chain_close(chain_handle_t chain)
{
	struct feca_chain *feca_dev_chain = (struct feca_chain *) chain;
	struct feca_device *feca_dev = feca_dev_chain->parent_feca_dev;
	
#ifndef ABERDEEN
	/*Disable Interrupts and wait for the current transaction to be complete*/
	uint32_t reg, dcm_reg;
	reg = ioread32(&feca_dev->regs->cmd_complete_irqen);
	dcm_reg = ioread32(&feca_dev->regs->dcm_complete_irqen);
	switch(feca_dev_chain->chain_type) {
	case FECA_CD_CHAIN:
		reg &= ~(CMD_COMPLETE_CD_EN_MASK);
		dcm_reg &= ~(CMD_COMPLETE_CD_DCM_MASK);
		iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		iowrite32(dcm_reg, &feca_dev->regs->dcm_complete_irqen);
		break;
		
	case FECA_SD_CHAIN:
		reg &= ~(CMD_COMPLETE_SD_EN_MASK);
		iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		break;
		
	case FECA_CE_CHAIN:
		reg &= ~(CMD_COMPLETE_CE_EN_MASK);
		iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		break;
		
	case FECA_SE_CHAIN:
		reg &= ~(CMD_COMPLETE_SE_EN_MASK);
		iowrite32(reg, &feca_dev->regs->cmd_complete_irqen);
		break;

	case FECA_CHAIN_MAX:
		return FECA_SUCCESS;
	}
#endif
	/*Call channel sub-routine to clean channels and destroy their objects*/
	
	/* Destroy Channel DS */
	feca_mem_free(feca_dev_chain->feca_channels);
	
	/*Clean the Queue Object present in chain queue*/
	//feca_mem_free(feca_dev_chain->queue->jobs_head);
	//feca_mem_free(feca_dev_chain->queue);
	
	/* Remove chain handler */
	feca_dev_chain->chain_handler = NULL;
	
	/* Destroy chain object */
	feca_mem_free(chain);
	chain = NULL;
	
	return FECA_SUCCESS;
}

uint32_t mem_allocator(struct feca_channel *channel, feca_mem_t mem_type, uint32_t size)
{
	feca_device_t *feca_dev =  channel->parent_chain->parent_feca_dev;
	feca_resource_t *mem;
	uint32_t addr;
	
	/* Validate device handle */
	if(!feca_dev)
		return FECA_INVAL_ADDR;
	
	mem = &feca_dev->feca_resource[mem_type];
	if(mem->rmng_size < size)
	{
		pr_info("NO Memory available. mem_type[%d]\n", mem_type);
		return FECA_INVAL_ADDR;
	}

	addr = mem->current;
	mem->current += size;
	mem->rmng_size -= size;
#if FECA_DEBUG
	pr_debug("%s: addr[%x], size[%x]\n", __func__, addr, size);
#endif
	return addr;
}

ch_handle_t feca_ch_open(chain_handle_t chain, ch_param_t ch_param)
{
	struct feca_chain *feca_dev_chain = (struct feca_chain *) chain;
	struct feca_channel *channel;
	chain_type_t chain_type;
	uint32_t fram_addr, reg;

	/* Validate chain handle */
	if(NULL == feca_dev_chain)
	{
		pr_err("%s: Chain handle is invalid\n", __func__);
		return NULL;
	}

	/* Validate channel type and get the valid channel data structure
	 * based to valid channel number in passed chain
	 */
	chain_type = feca_dev_chain->chain_type;
	switch (chain_type) {
	case FECA_CE_CHAIN:
		if((INVALID_CH >= ch_param.type) || (CH_OUT < ch_param.type))
		{
			pr_err("%s: Invalid channel type [%d]\n", __func__, ch_param.type);
			return NULL;
		}
		channel = feca_dev_chain->feca_channels + ch_param.type;
	break;
	case FECA_SE_CHAIN:
		if((INVALID_CH >= ch_param.type) || (CH_CRC_OUT < ch_param.type))
		{
			pr_err("%s: Invalid channel type [%d]\n", __func__, ch_param.type);
			return NULL;
		}
		channel = feca_dev_chain->feca_channels + (ch_param.type * TB_MAX) + ch_param.tb_num;
	break;
	case FECA_SD_CHAIN:
		if((INVALID_CH >= ch_param.type) || (CH_CRC_OUT < ch_param.type))
		{
			pr_err("%s: Invalid channel type [%d]\n", __func__, ch_param.type);
			return NULL;
		}
		channel = feca_dev_chain->feca_channels + (ch_param.type * TB_MAX) + ch_param.tb_num;
	break;
	case FECA_CD_CHAIN:
		if((INVALID_CH >= ch_param.type) || (CH_CMD_DCM_CSI2 < ch_param.type))
		{
			pr_err("%s: Invalid channel type [%d]\n", __func__, ch_param.type);
			return NULL;
		}
		if (ch_param.type <= CH_CRC_OUT)
			channel = feca_dev_chain->feca_channels + ch_param.type;
		else /* DCM */
			channel = feca_dev_chain->feca_channels + CH_CMD_DCM_ACK +
				(((ch_param.type - CH_CMD_DCM_ACK) * TB_MAX) + ch_param.tb_num);
	break;
	default:
		return NULL;
	}

	channel->channel_info.ch_type = ch_param.type;
	channel->chann_handler = (feca_chann_cb_t) feca_ch_handler;

	if (ch_param.type >= CH_CMD_DCM_ACK) {

		volatile struct circ_regs *ch_regs;
		uint32_t dcm_in_ch_id;

		/* For ACK/CSI1/CSI2 allocate command circular buffers
		 * (for ch#63-ch#86)by a predefined memory size and use the size
		 * provided as input to the API as the memory to allocate for
		 * ACK/CSI1/CSI2 data in circular buffers (ch#87-ch#110).
		 */
		/* Allocate memory from FRAM */
		fram_addr = mem_allocator(channel, FECA_FRAM,
				FECA_CD_DEMUX_CIRC_CMD_SIZE);
		if (FECA_INVAL_ADDR == fram_addr)
			return NULL;
		/* Initializing the circular buffer start/end reg */
		iowrite32(fram_addr, &channel->ch_regs->circ_start);
		iowrite32((fram_addr + FECA_CD_DEMUX_CIRC_CMD_SIZE),
				&channel->ch_regs->circ_end);

		/* Update circulare buffer control reg */
		reg = ch_param.xfer_size;
		reg |= (ch_param.dma_disable << CIRC_BUFF_DMA_DISABLE);
		reg |= (ch_param.ext_dma << CIRC_BUFF_EXT_DMA);
		reg |= (1 << CIRC_BUFF_ENABLE);
		iowrite32(reg, &channel->ch_regs->circ_control);
#if FECA_DEBUG
		PRINTF("%s: ch[%d] circ_control [%x], start[%x] end[%x]\n", __func__, channel->id, ioread32(&channel->ch_regs->circ_control),
			ioread32(&channel->ch_regs->circ_start), ioread32(&channel->ch_regs->circ_end));
#endif
		/* Configure equivalent DCM input CB implicitly */
		dcm_in_ch_id = FECA_CD_ACK_IN_CB_ID + (channel->id - FECA_CD_ACK_CMD_CB_ID);
		fram_addr = (uint32_t)mem_allocator(channel, FECA_FRAM, ch_param.ch_size);
		/* Initializing the circular buffer start/end reg */
		ch_regs = feca_dev_chain->parent_feca_dev->regs->circ_buff + dcm_in_ch_id; // CB registers
		iowrite32(fram_addr, &ch_regs->circ_start);
		iowrite32((fram_addr + ch_param.ch_size), &ch_regs->circ_end);

		/* Update circulare buffer control reg */
		reg = 0;
		reg |= (1 << CIRC_BUFF_ENABLE);
		iowrite32(reg, &ch_regs->circ_control);
#if FECA_DEBUG
		PRINTF("%s: DCM-internal ch[%d] circ_control [%x], start[%x] end[%x]\n", __func__, dcm_in_ch_id, ioread32(&ch_regs->circ_control),
			ioread32(&ch_regs->circ_start), ioread32(&ch_regs->circ_end));
#endif
	} else {
		/* TODO: Is there any limit on size */
		/* Attaching FRAM Memory to Channels */
		ch_param.ch_size = ALIGN_UP(ch_param.ch_size , FECA_FRAM_ALIGNMENT);

		fram_addr = mem_allocator(channel, FECA_FRAM, ch_param.ch_size);
		if (FECA_INVAL_ADDR == fram_addr)
			return NULL;
		/* Initializing the circular buffer start/end reg */
		iowrite32(fram_addr, &channel->ch_regs->circ_start);
		iowrite32((fram_addr + ch_param.ch_size), &channel->ch_regs->circ_end);

		/* Update circulare buffer control reg */
		reg = ch_param.xfer_size;
		reg |= (ch_param.dma_disable << CIRC_BUFF_DMA_DISABLE);
		reg |= (ch_param.ext_dma << CIRC_BUFF_EXT_DMA);
		reg |= (1 << CIRC_BUFF_ENABLE);
		iowrite32(reg, &channel->ch_regs->circ_control);
	}
#if FECA_DEBUG
	pr_debug("%s: circ_control [%x], start[%x] end[%x]\n", __func__, ioread32(&channel->ch_regs->circ_control),
			ioread32(&channel->ch_regs->circ_start), ioread32(&channel->ch_regs->circ_end));
#endif

	return channel;
}

status_t feca_ch_close(ch_handle_t ch, feca_ch_type_t feca_ch_type)
{
	int i=0;
	struct feca_channel *channels = (struct feca_channel *)ch;
	struct feca_chain *feca_dev_chain = channels->parent_chain;

	/*Disable interrupt and wait for current transaction to be completed*/

	/*Clear bit representing error bits*/

	/*Flush data present in channel via sub-routine*/

	/*Free the memory of "SIZE" associated with channel from FRAM.*/
	if(feca_dev_chain->chain_type == FECA_SD_CHAIN || 
			feca_dev_chain->chain_type == FECA_SE_CHAIN) {
		for(i=0; i < TB_MAX; i++) {
			((channels + i) + feca_ch_type)->channel_info.ch_type = INVALID_CH;
			((channels + i) + feca_ch_type)->chann_handler = NULL;
		}
	} else {
		(channels + feca_ch_type)->channel_info.ch_type = INVALID_CH;
		(channels + feca_ch_type)->chann_handler = NULL;
	}
	/* Need to add code in allocator to free FRAM*/

	/*Enable Interrupts*/

	
	return FECA_SUCCESS;	
}

uint32_t *feca_get_ch_axi_addr(chain_handle_t chain_handle, uint32_t ch_id)
{
	feca_device_t *dev = ((struct feca_chain *)chain_handle)->parent_feca_dev;

	uint32_t axi_start = dev->feca_resource[FECA_AXI_SLAVE].start;
	return (uint32_t *)(axi_start + FECA_CB_AXI_OFFSET + (ch_id * FECA_CB_AXI_SIZE));
}

uint32_t *feca_get_harq_axi_addr(chain_handle_t chain_handle)
{
	feca_device_t *dev = ((struct feca_chain *)chain_handle)->parent_feca_dev;

	uint32_t axi_start = dev->feca_resource[FECA_AXI_SLAVE].start;
	return (uint32_t *)(axi_start);
}

uint32_t feca_get_ch_id(chain_handle_t chain_handle, feca_ch_type_t ch_type, uint32_t tb_num)
{
	struct feca_chain *chain = (struct feca_chain *)chain_handle;
#if FECA_DEBUG
	pr_debug("%s: chain_type[%d], ch_type[%d], TB[%d]]n", __func__, chain->chain_type,
			ch_type, tb_num);
#endif
	if(((FECA_CD_CHAIN == chain->chain_type) && (ch_type < CH_CMD_DCM_ACK)) || 
	    (FECA_CE_CHAIN == chain->chain_type))
		return (chain->feca_channels[ch_type].id);
	else if ((FECA_CD_CHAIN == chain->chain_type) && (ch_type >= CH_CMD_DCM_ACK))
		return (chain->feca_channels
			[CH_CMD_DCM_ACK + ((ch_type - CH_CMD_DCM_ACK) * TB_MAX) + tb_num].id);
	else
		return (chain->feca_channels[(ch_type * TB_MAX) + tb_num].id);
}

void feca_dump_cb_reg(chain_handle_t *chain_handle, uint32_t tb_num, job_type_t job_type)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain_handle;
	uint32_t cmd, in, out, crc = 0;
	uint32_t dcm = 0 ;

	/* To remove unused variable warning */
	(void) cmd;
	(void) in;
	(void) out;
	(void) crc;

	switch(feca_chain->chain_type)
	{
		/* SACHIN TODO */
		case FECA_CD_CHAIN:
			/* CE doesn't have CRC out status */
			crc = CH_CRC_OUT;
			/* fallthrough */
		case FECA_CE_CHAIN:	
			if (job_type == FECA_JOB_CD_DCM_ACK) {
				cmd = CH_CMD_DCM_ACK + tb_num;
				dcm = CH_CMD_DCM_ACK + ((CH_IN_DCM_ACK - CH_CMD_DCM_ACK) * TB_MAX) + tb_num;
			} else if (job_type == FECA_JOB_CD_DCM_CS1) {
				cmd = CH_CMD_DCM_ACK + TB_MAX + tb_num;
				dcm = CH_CMD_DCM_ACK + ((CH_IN_DCM_CSI1 - CH_CMD_DCM_ACK) * TB_MAX) + tb_num;
			} else if (job_type == FECA_JOB_CD_DCM_CS2) {
				cmd = CH_CMD_DCM_ACK + (2 * TB_MAX) + tb_num;
				dcm = CH_CMD_DCM_ACK + ((CH_IN_DCM_CSI2 - CH_CMD_DCM_ACK) * TB_MAX) + tb_num;
			} else
				cmd = CH_CMD;
			in  = CH_IN;
			out = CH_OUT;
		break;
		case FECA_SD_CHAIN:
			/* SE doesn't have CRC out status */
			crc = (CH_CRC_OUT * TB_MAX) + tb_num;
			/* fallthrough */
		case FECA_SE_CHAIN:
			cmd = (CH_CMD * TB_MAX) + tb_num;
			in = (CH_IN * TB_MAX) + tb_num;
			out = (CH_OUT * TB_MAX) + tb_num;
		break;

		default:
			return;
	}

	if(crc){
	pr_debug("cmd_id[%d], in_id[%d], out_id[%d], crc_id[%d]\r\n", cmd, in, out, crc);
	pr_debug("cmd_chid[%d], in_chid[%d], out_chid[%d], crc_chid[%d]\r\n",
			feca_chain->feca_channels[cmd].id,
			dcm ? feca_chain->feca_channels[dcm].id : feca_chain->feca_channels[in].id,
			feca_chain->feca_channels[out].id, feca_chain->feca_channels[crc].id);
	} else {
	pr_debug("cmd_id[%d], in_id[%d], out_id[%d]\r\n", cmd, in, out);
        pr_debug("cmd_chid[%d], in_chid[%d], out_chid[%d]\r\n",
                        feca_chain->feca_channels[cmd].id,
                        dcm ? feca_chain->feca_channels[dcm].id : feca_chain->feca_channels[in].id,
                        feca_chain->feca_channels[out].id);

	}
	pr_debug("cmd_cb_start = %x\r\n", ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_start));
	pr_debug("cmd_cb_end = %x\r\n", ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_end));
	pr_debug("cmd_cb_ctrl = %x\r\n", ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_control));
	pr_debug("cmd_cb_in = %x\r\n", ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_in));
	pr_debug("cmd_cb_out = %x\r\n", ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_out));
	pr_debug("cmd_cb_valid = %x\r\n", ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_num_valid));


	if (job_type >= FECA_JOB_CD_DCM_ACK && job_type <= FECA_JOB_CD_DCM_CS2) {
	pr_debug("dcm_ip_cb_start = %x\r\n", ioread32(&feca_chain->feca_channels[dcm].ch_regs->circ_start));
	pr_debug("dcm_ip_cb_end = %x\r\n", ioread32(&feca_chain->feca_channels[dcm].ch_regs->circ_end));
	pr_debug("dcm_ip_cb_ctrl = %x\r\n", ioread32(&feca_chain->feca_channels[dcm].ch_regs->circ_control));
	pr_debug("dcm_ip_cb_in = %x\r\n", ioread32(&feca_chain->feca_channels[dcm].ch_regs->circ_in));
	pr_debug("dcm_ip_cb_out = %x\r\n", ioread32(&feca_chain->feca_channels[dcm].ch_regs->circ_out));
	pr_debug("dcm_ip_cb_valid = %x\r\n", ioread32(&feca_chain->feca_channels[dcm].ch_regs->circ_num_valid));
	} else {
	pr_debug("ip_cb_start = %x\r\n", ioread32(&feca_chain->feca_channels[in].ch_regs->circ_start));
	pr_debug("ip_cb_end = %x\r\n", ioread32(&feca_chain->feca_channels[in].ch_regs->circ_end));
	pr_debug("ip_cb_ctrl = %x\r\n", ioread32(&feca_chain->feca_channels[in].ch_regs->circ_control));
	pr_debug("ip_cb_in = %x\r\n", ioread32(&feca_chain->feca_channels[in].ch_regs->circ_in));
	pr_debug("ip_cb_out = %x\r\n", ioread32(&feca_chain->feca_channels[in].ch_regs->circ_out));
	pr_debug("ip_cb_valid = %x\r\n", ioread32(&feca_chain->feca_channels[in].ch_regs->circ_num_valid));
	}

	pr_debug("out_cb_start = %x\r\n", ioread32(&feca_chain->feca_channels[out].ch_regs->circ_start));
	pr_debug("out_cb_end = %x\r\n", ioread32(&feca_chain->feca_channels[out].ch_regs->circ_end));
	pr_debug("out_cb_ctrl = %x\r\n", ioread32(&feca_chain->feca_channels[out].ch_regs->circ_control));
	pr_debug("out_cb_in = %x\r\n", ioread32(&feca_chain->feca_channels[out].ch_regs->circ_in));
	pr_debug("out_cb_out = %x\r\n", ioread32(&feca_chain->feca_channels[out].ch_regs->circ_out));
	pr_debug("out_cb_valid = %x\r\n", ioread32(&feca_chain->feca_channels[out].ch_regs->circ_num_valid));
	
	if((FECA_CD_CHAIN == feca_chain->chain_type) || (FECA_SD_CHAIN == feca_chain->chain_type))
	{
		pr_debug("crc_cb_start = %x\r\n", ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_start));
		pr_debug("crc_cb_end = %x\r\n", ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_end));
		pr_debug("crc_cb_ctrl = %x\r\n", ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_control));
		pr_debug("crc_cb_in = %x\r\n", ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_in));
		pr_debug("crc_cb_out = %x\r\n", ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_out));
		pr_debug("crc_cb_valid = %x\r\n", ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_num_valid));
	}
}

void feca_get_cb_reg(chain_handle_t *chain_handle, uint32_t tb_num, job_type_t job_type,
		struct feca_circ_buf_regs *feca_cb_regs)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain_handle;
	uint32_t cmd, in, out, crc;
	volatile struct circ_regs *ch_regs = NULL;
	uint32_t dcm_in_id = 0 ;

	switch(feca_chain->chain_type)
	{
		/* SACHIN TODO */
		case FECA_CD_CHAIN:
		case FECA_CE_CHAIN:
			if (job_type == FECA_JOB_CD_DCM_ACK) {
				cmd = CH_CMD_DCM_ACK + tb_num;
				dcm_in_id = (FECA_CD_ACK_IN_CB_ID + tb_num);
				ch_regs = feca_chain->parent_feca_dev->regs->circ_buff + dcm_in_id;
			} else if (job_type == FECA_JOB_CD_DCM_CS1) {
				cmd = CH_CMD_DCM_ACK + TB_MAX + tb_num;
				dcm_in_id = (FECA_CD_CSI1_IN_CB_ID + tb_num);
				ch_regs = feca_chain->parent_feca_dev->regs->circ_buff + dcm_in_id;
			} else if (job_type == FECA_JOB_CD_DCM_CS2) {
				cmd = CH_CMD_DCM_ACK + (2 * TB_MAX) + tb_num;
				dcm_in_id = (FECA_CD_CSI2_IN_CB_ID + tb_num);
				ch_regs = feca_chain->parent_feca_dev->regs->circ_buff + dcm_in_id;
			} else
				cmd = CH_CMD;
			in  = CH_IN;
			out = CH_OUT;
			crc = CH_CRC_OUT;
		break;
		case FECA_SD_CHAIN:
		case FECA_SE_CHAIN:
			cmd = (CH_CMD * TB_MAX) + tb_num;
			in 	= (CH_IN * TB_MAX) + tb_num;
			out = (CH_OUT * TB_MAX) + tb_num;
			crc = (CH_CRC_OUT * TB_MAX) + tb_num;
		break;

		default:
			return;
	}

	feca_cb_regs->cmd_id = cmd;
	feca_cb_regs->in_id = in;
	feca_cb_regs->out_id = out;
	feca_cb_regs->crc_id = crc;
	feca_cb_regs->cmd_chid = feca_chain->feca_channels[cmd].id;
	feca_cb_regs->in_chid = dcm_in_id ? dcm_in_id : feca_chain->feca_channels[in].id;
	feca_cb_regs->out_chid = feca_chain->feca_channels[out].id;
	feca_cb_regs->crc_chid = feca_chain->feca_channels[crc].id;

	feca_cb_regs->cmd_cb_start = ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_start);
	feca_cb_regs->cmd_cb_end = ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_end);
	feca_cb_regs->cmd_cb_ctrl = ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_control);
	feca_cb_regs->cmd_cb_in = ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_in);
	feca_cb_regs->cmd_cb_out = ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_out);
	feca_cb_regs->cmd_cb_valid = ioread32(&feca_chain->feca_channels[cmd].ch_regs->circ_num_valid);

	if (job_type >= FECA_JOB_CD_DCM_ACK && job_type <= FECA_JOB_CD_DCM_CS2) {
		feca_cb_regs->in_cb_start = ioread32(&ch_regs->circ_start);
		feca_cb_regs->in_cb_end = ioread32(&ch_regs->circ_end);
		feca_cb_regs->in_cb_ctrl = ioread32(&ch_regs->circ_control);
		feca_cb_regs->in_cb_in = ioread32(&ch_regs->circ_in);
		feca_cb_regs->in_cb_out = ioread32(&ch_regs->circ_out);
		feca_cb_regs->in_cb_valid = ioread32(&ch_regs->circ_num_valid);
	} else {
		feca_cb_regs->in_cb_start = ioread32(&feca_chain->feca_channels[in].ch_regs->circ_start);
		feca_cb_regs->in_cb_end = ioread32(&feca_chain->feca_channels[in].ch_regs->circ_end);
		feca_cb_regs->in_cb_ctrl = ioread32(&feca_chain->feca_channels[in].ch_regs->circ_control);
		feca_cb_regs->in_cb_in = ioread32(&feca_chain->feca_channels[in].ch_regs->circ_in);
		feca_cb_regs->in_cb_out = ioread32(&feca_chain->feca_channels[in].ch_regs->circ_out);
		feca_cb_regs->in_cb_valid = ioread32(&feca_chain->feca_channels[in].ch_regs->circ_num_valid);
	}

	feca_cb_regs->out_cb_start = ioread32(&feca_chain->feca_channels[out].ch_regs->circ_start);
	feca_cb_regs->out_cb_end = ioread32(&feca_chain->feca_channels[out].ch_regs->circ_end);
	feca_cb_regs->out_cb_ctrl = ioread32(&feca_chain->feca_channels[out].ch_regs->circ_control);
	feca_cb_regs->out_cb_in = ioread32(&feca_chain->feca_channels[out].ch_regs->circ_in);
	feca_cb_regs->out_cb_out = ioread32(&feca_chain->feca_channels[out].ch_regs->circ_out);
	feca_cb_regs->out_cb_valid = ioread32(&feca_chain->feca_channels[out].ch_regs->circ_num_valid);

	if((FECA_CD_CHAIN == feca_chain->chain_type) || (FECA_SD_CHAIN == feca_chain->chain_type))
	{
		feca_cb_regs->crc_cb_start = ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_start);
		feca_cb_regs->crc_cb_end = ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_end);
		feca_cb_regs->crc_cb_ctrl = ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_control);
		feca_cb_regs->crc_cb_in = ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_in);
		feca_cb_regs->crc_cb_out = ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_out);
		feca_cb_regs->crc_cb_valid = ioread32(&feca_chain->feca_channels[crc].ch_regs->circ_num_valid);
	}
}

void feca_print_cb_reg(struct feca_circ_buf_regs *feca_cb_regs)
{
	pr_debug("cmd_id[%d], in_id[%d], out_id[%d], crc_id[%d]\n",
		feca_cb_regs->cmd_id, feca_cb_regs->in_id, feca_cb_regs->out_id, feca_cb_regs->crc_id);
	pr_debug("cmd_chid[%d], in_chid[%d], out_chid[%d], crc_chid[%d]\n",
			feca_cb_regs->cmd_chid,	feca_cb_regs->in_chid,
			feca_cb_regs->out_chid, feca_cb_regs->crc_chid);

	pr_debug("cmd_cb_start = %x\n", feca_cb_regs->cmd_cb_start);
	pr_debug("cmd_cb_end = %x\n", feca_cb_regs->cmd_cb_end);
	pr_debug("cmd_cb_ctrl = %x\n", feca_cb_regs->cmd_cb_ctrl);
	pr_debug("cmd_cb_in = %x\n", feca_cb_regs->cmd_cb_in);
	pr_debug("cmd_cb_out = %x\n", feca_cb_regs->cmd_cb_out);
	pr_debug("cmd_cb_valid = %x\n", feca_cb_regs->cmd_cb_valid);

	pr_debug("ip_cb_start = %x\n", feca_cb_regs->in_cb_start);
	pr_debug("ip_cb_end = %x\n", feca_cb_regs->in_cb_end);
	pr_debug("ip_cb_ctrl = %x\n", feca_cb_regs->in_cb_ctrl);
	pr_debug("ip_cb_in = %x\n", feca_cb_regs->in_cb_in);
	pr_debug("ip_cb_out = %x\n", feca_cb_regs->in_cb_out);
	pr_debug("ip_cb_valid = %x\n", feca_cb_regs->in_cb_valid);

	pr_debug("out_cb_start = %x\n", feca_cb_regs->out_cb_start);
	pr_debug("out_cb_end = %x\n", feca_cb_regs->out_cb_end);
	pr_debug("out_cb_ctrl = %x\n", feca_cb_regs->out_cb_ctrl);
	pr_debug("out_cb_in = %x\n", feca_cb_regs->out_cb_in);
	pr_debug("out_cb_out = %x\n", feca_cb_regs->out_cb_out);
	pr_debug("out_cb_valid = %x\n", feca_cb_regs->out_cb_valid);
}

status_t feca_job_dispatch(chain_handle_t chain, feca_job_t *feca_job)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain;
	uint32_t ch_id = 0, cmd_size, i;
	uint32_t *dstn_addr;

	switch(feca_job->job_type)
	{
		case FECA_JOB_CD:
		{
			cd_command_t *cd_cmd;

			cd_cmd = &(feca_job->command_chain_t.cd_command_ch_obj);
			ch_id = feca_get_ch_id(feca_chain, CH_CMD, 0);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, ch_id);
#if FECA_DEBUG
			PRINTF("---- CMD Desc addr 0x%X\n", dstn_addr);
#endif

			/* fz_lut size depends on pe_n parameter */
			cmd_size  = (sizeof(*cd_cmd) - sizeof(cd_cmd->cd_fz_lut)) >> 2;
			cmd_size += (1 << cd_cmd->cd_cfg1.pd_n) / 32;

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)cd_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)cd_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_CE:
		case FECA_JOB_CE_DCM:
		{
			ce_command_t *ce_cmd;

			ce_cmd = &(feca_job->command_chain_t.ce_command_ch_obj);
			ch_id = feca_get_ch_id(feca_chain, CH_CMD, 0);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, ch_id);

			/* fz_lut size depends on pe_n parameter */
			cmd_size  = (sizeof(*ce_cmd) - sizeof(ce_cmd->ce_fz_lut)) >> 2;
			cmd_size += (1 << ce_cmd->ce_cfg1.pe_n) / 32;

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)ce_cmd) + i), dstn_addr);
#if FECA_DEBUG
//				PRINTF("0x%X\n", *(((uint32_t *)ce_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_SD:
		{
			sd_command_t *sd_cmd;

			sd_cmd = &(feca_job->command_chain_t.sd_command_ch_obj);
			ch_id = feca_get_ch_id(feca_chain, CH_CMD, feca_job->t_blk_id);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, ch_id);
			cmd_size  = (sizeof(*sd_cmd) >> 2);

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)sd_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)sd_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_SE:
		{
			se_command_t *se_cmd;

			se_cmd = &(feca_job->command_chain_t.se_command_ch_obj);
			ch_id = feca_get_ch_id(feca_chain, CH_CMD, feca_job->t_blk_id);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, ch_id);
			cmd_size  = (sizeof(*se_cmd) >> 2);

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)se_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)se_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_CD_DCM_ACK:
		{
			cd_command_t *cd_cmd;

			cd_cmd = &(feca_job->command_chain_t.cd_command_ch_obj);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, (FECA_CD_ACK_CMD_CB_ID + feca_job->t_blk_id));
#if FECA_DEBUG
			PRINTF("---- DCM CMD Desc addr 0x%X\n", dstn_addr);
#endif

			/* fz_lut size depends on pe_n parameter */
			cmd_size  = (sizeof(*cd_cmd) - sizeof(cd_cmd->cd_fz_lut)) >> 2;
			cmd_size += (1 << cd_cmd->cd_cfg1.pd_n) / 32;

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)cd_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)cd_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_CD_DCM_CS1:
		{
			cd_command_t *cd_cmd;

			cd_cmd = &(feca_job->command_chain_t.cd_command_ch_obj);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, (FECA_CD_CSI1_CMD_CB_ID + feca_job->t_blk_id));
#if FECA_DEBUG
			PRINTF("---- DCM CMD Desc addr 0x%X\n", dstn_addr);
#endif

			/* fz_lut size depends on pe_n parameter */
			cmd_size  = (sizeof(*cd_cmd) - sizeof(cd_cmd->cd_fz_lut)) >> 2;
			cmd_size += (1 << cd_cmd->cd_cfg1.pd_n) / 32;

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)cd_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)cd_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_CD_DCM_CS2:
		{
			cd_command_t *cd_cmd;

			cd_cmd = &(feca_job->command_chain_t.cd_command_ch_obj);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, (FECA_CD_CSI2_CMD_CB_ID + feca_job->t_blk_id));
#if FECA_DEBUG
			PRINTF("---- DCM CMD Desc addr 0x%X\n", dstn_addr);
#endif

			/* fz_lut size depends on pe_n parameter */
			cmd_size  = (sizeof(*cd_cmd) - sizeof(cd_cmd->cd_fz_lut)) >> 2;
			cmd_size += (1 << cd_cmd->cd_cfg1.pd_n) / 32;

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)cd_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)cd_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_SD_DCM:
		{
			sd_dcm_command_t *sd_dcm_cmd;

			sd_dcm_cmd = &(feca_job->command_chain_t.sd_dcm_command_ch_obj);
			ch_id = feca_get_ch_id(feca_chain, CH_CMD, feca_job->t_blk_id);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, ch_id);
			cmd_size  = (sizeof(*sd_dcm_cmd) >> 2);

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)sd_dcm_cmd) + i), dstn_addr);
#if FECA_DEBUG
				PRINTF("0x%X\n", *(((uint32_t *)sd_dcm_cmd) + i));
#endif
			}
		}
		break;
		case FECA_JOB_SE_DCM:
		{
			se_dcm_command_t *se_dcm_cmd;

			se_dcm_cmd = &(feca_job->command_chain_t.se_dcm_command_ch_obj);
			ch_id = feca_get_ch_id(feca_chain, CH_CMD, feca_job->t_blk_id);
			dstn_addr = feca_get_ch_axi_addr(feca_chain, ch_id);
			cmd_size  = (sizeof(*se_dcm_cmd) >> 2);

			/* Write command to AXI salve */
			for(i = 0; i < cmd_size; i++) {
				iowrite32(*(((uint32_t *)se_dcm_cmd) + i), dstn_addr);
#if FECA_DEBUG
	//			PRINTF("0x%X\n", *(((uint32_t *)se_dcm_cmd) + i));
#endif
			}
		}
		break;
		default:
			return FECA_NO_DEV;
		break;
	}

#if FECA_DEBUG
	pr_debug("%s: cmd_ch_id[%d], cmd_ch_addr[%x], cmd_size[%d]\n", __func__, ch_id, dstn_addr, cmd_size);
#endif
	return FECA_SUCCESS;
}

status_t feca_get_sd_job_status(chain_handle_t chain, feca_job_t *feca_job)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain;
	uint32_t bit_num = 0;
	volatile uint32_t *status = 0;

#if FECA_DEBUG
	uint32_t cd_cfg1 = SWAP_32(feca_job->command_chain_t.cd_command_ch_obj.cd_cfg1.raw_cd_cfg1);
	uint32_t sd_cfg2 = SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_cfg2.raw_sd_cfg2);

	if((!(cd_cfg1 & FECA_CMD_COMPLETE_TRIG_FLAG) && (FECA_JOB_SD != feca_job->job_type &&
		FECA_JOB_SD_DCM != feca_job->job_type))	|| ((FECA_JOB_SD == feca_job->job_type ||
		FECA_JOB_SD_DCM == feca_job->job_type) && !(sd_cfg2 & FECA_CMD_COMPLETE_TRIG_FLAG)))
	{
		pr_err("%s: complete_trig_en is not set in job[%d]\n",
				__func__, feca_job->job_type);
		return FECA_ERROR;
	}
#endif

	feca_device_t *feca_dev = feca_chain->parent_feca_dev;
	bit_num = 1 << (CMD_COMPLETE_SD_EN_BIT + feca_job->t_blk_id);
	status = &feca_dev->regs->cmd_complete_status;

	if (ioread32(status) & bit_num) {
		iowrite32(bit_num, status); // clear
		return FECA_SUCCESS;
	} else
		return FECA_JOB_NOT_COMPLETE;
}

status_t feca_get_job_status(chain_handle_t chain, feca_job_t *feca_job)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain;
	uint32_t bit_num = 0;
	volatile uint32_t *status = 0;

#if FECA_DEBUG
	uint32_t cd_cfg1 = SWAP_32(feca_job->command_chain_t.cd_command_ch_obj.cd_cfg1.raw_cd_cfg1);
	uint32_t sd_cfg2 = SWAP_32(feca_job->command_chain_t.sd_command_ch_obj.sd_cfg2.raw_sd_cfg2);

	if((!(cd_cfg1 & FECA_CMD_COMPLETE_TRIG_FLAG) && (FECA_JOB_SD != feca_job->job_type &&
		FECA_JOB_SD_DCM != feca_job->job_type))	|| ((FECA_JOB_SD == feca_job->job_type ||
		FECA_JOB_SD_DCM == feca_job->job_type) && !(sd_cfg2 & FECA_CMD_COMPLETE_TRIG_FLAG)))
	{
		pr_err("%s: complete_trig_en is not set in job[%d]\n",
				__func__, feca_job->job_type);
		return FECA_ERROR;
	}
#endif

	feca_device_t *feca_dev = feca_chain->parent_feca_dev;
	switch(feca_job->job_type)
	{
		case FECA_JOB_CD:
			bit_num = 1 << CMD_COMPLETE_CD_EN_BIT;
			status = &feca_dev->regs->cmd_complete_status;
			break;
		case FECA_JOB_SD:
		case FECA_JOB_SD_DCM:
			bit_num = 1 << (CMD_COMPLETE_SD_EN_BIT + feca_job->t_blk_id);
			status = &feca_dev->regs->cmd_complete_status;
			break;
		case FECA_JOB_CE:
		case FECA_JOB_CE_DCM:
			bit_num = 1 << CMD_COMPLETE_CE_EN_BIT;
			status = &feca_dev->regs->cmd_complete_status;
			break;
		case FECA_JOB_SE:
		case FECA_JOB_SE_DCM:
			bit_num = 1 << (CMD_COMPLETE_SE_EN_BIT + feca_job->t_blk_id);
			status = &feca_dev->regs->cmd_complete_status;
			break;
		case FECA_JOB_CD_DCM_ACK:
			bit_num = 1 << (CMD_COMPLETE_CD_ACK_EN_BIT + feca_job->t_blk_id);
			status = &feca_dev->regs->dcm_complete_status;
			break;
		case FECA_JOB_CD_DCM_CS1:
			bit_num = 1 << (CMD_COMPLETE_CD_CSI1_EN_BIT + feca_job->t_blk_id);
			status = &feca_dev->regs->dcm_complete_status;
			break;
		case FECA_JOB_CD_DCM_CS2:
			bit_num = 1 << (CMD_COMPLETE_CD_CSI2_EN_BIT + feca_job->t_blk_id);
			status = &feca_dev->regs->dcm_complete_status;
			break;
		default:
			return FECA_INV_JOB_TYPE;
	}
#if FECA_DEBUG_CB
	pr_debug("-- FECA ctrl status LE 0x%X\n", ioread32(status));
	feca_dump_cb_reg(chain, feca_job->t_blk_id, feca_job->job_type);
#endif
	if (ioread32(status) & bit_num) {
		iowrite32(bit_num, status); // clear
		return FECA_SUCCESS;
	} else
		return FECA_JOB_NOT_COMPLETE;
}

uint32_t feca_ch_out_valid_bytes(chain_handle_t chain, uint32_t tb_num)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain;

	return ioread32(&feca_chain->feca_channels[CH_OUT * TB_MAX + tb_num].ch_regs->circ_num_valid);
}

uint32_t feca_ch_in_valid_bytes(chain_handle_t chain, uint32_t tb_num)
{
	struct feca_chain * feca_chain = (struct feca_chain *)chain;

	return ioread32(&feca_chain->feca_channels[CH_IN * TB_MAX + tb_num].ch_regs->circ_num_valid);
}

uint32_t feca_dcm_ch_in_valid_bytes(chain_handle_t chain, feca_ch_type_t ch_type, uint32_t tb_num)
{
        struct feca_chain * feca_chain = (struct feca_chain *)chain;

	if((feca_chain->chain_type != FECA_CD_CHAIN) || (ch_type <= CH_CMD_DCM_CSI2) || (ch_type > CH_IN_DCM_CSI2))
	{
		pr_err("%s: Wrong chain or input channel type for DCM mode \n", __func__);
		return -1;
	}

        return ioread32(&feca_chain->feca_channels
			[CH_CMD_DCM_ACK + ((ch_type - CH_CMD_DCM_ACK) * TB_MAX) + tb_num].
			ch_regs->circ_num_valid);
}

static int geul_feca_dev_init(feca_dev_id id)
{
	int err = 0;
	feca_device_t *feca_dev = NULL;
	mod_mem_region_t *mem_reg = NULL;

	/* For allocating memory to feca_dev*/
	feca_dev = (feca_device_t *) feca_alloc_mem(sizeof(feca_device_t));
	if(NULL == feca_dev) {
		pr_err("%s: Can't allocate Memory for feca_device \n", __func__);
		return -1;
	}

	/* Update device name and id */
	strcpy(feca_dev->feca_dev_name, FECA_DEV_NAME);
	feca_dev->id_num = id;
#if FECA_DEBUG
	pr_debug("%s: feca dev name [%s], id[%d]\n", __func__, feca_dev->feca_dev_name, id);
#endif
	/* Fetch IP register and update it in feca_dev */
	mem_reg = bsp_feca_get_mem_region(MOD_MEM_FECA_APB_SLAVE);
	if(NULL == mem_reg){
		pr_err("%s: Didn't get APB CCSR\n", __func__);
		return -1;
	}
	feca_dev->regs = (struct feca_ip_regs *) mem_reg->addr_v;

	/* Fetch AXI slave and update it in feca_dev */
	mem_reg = bsp_feca_get_mem_region(MOD_MEM_FECA_AXI_SLAVE);
	if(NULL == mem_reg){
		pr_err("%s: Didn't get AXI slave mem\n", __func__);
		return -1;
	}

	/* Update AXI slave mem state in feca_dev */
	feca_dev->feca_resource[FECA_AXI_SLAVE].start = mem_reg->addr_v;
	feca_dev->feca_resource[FECA_AXI_SLAVE].rmng_size = (uint32_t)mem_reg->size;
	feca_dev->feca_resource[FECA_AXI_SLAVE].end = mem_reg->addr_v + (uint32_t)mem_reg->size;
	feca_dev->feca_resource[FECA_AXI_SLAVE].current = mem_reg->addr_v;

	/* Update FRAM state in feca_dev */
	feca_dev->feca_resource[FECA_FRAM].start = FECA_FRAM_START;
	feca_dev->feca_resource[FECA_FRAM].rmng_size = FECA_FRAM_SIZE;
	feca_dev->feca_resource[FECA_FRAM].end = FECA_FRAM_START + FECA_FRAM_SIZE;
	feca_dev->feca_resource[FECA_FRAM].current = FECA_FRAM_START;

	/* Update HRAM state in feca_dev */
	feca_dev->feca_resource[FECA_HRAM].start = FECA_HRAM_START;
	feca_dev->feca_resource[FECA_HRAM].rmng_size = FECA_HRAM_SIZE;
	feca_dev->feca_resource[FECA_HRAM].end = FECA_HRAM_START + FECA_HRAM_SIZE;
	feca_dev->feca_resource[FECA_HRAM].current = FECA_HRAM_START;

	err = bsp_update_feca_dev(id, feca_dev);
	if(err < 0)
	{
		pr_err("%s: FECA id is incorrect. Id[%d]\n", __func__, id);
		return -1;
	}

	return err;
}

static int geul_feca_chain_init(feca_dev_id id)
{
	int err = 0;
	uint8_t type;
	feca_device_t *feca_dev;
	struct feca_chain *chain = NULL;
	struct feca_channel *channels = NULL;

	feca_dev = bsp_get_feca_dev(id);

	/* Checking for the valid feca_dev */
	if(!feca_dev) {
		pr_err("%s: Not a Valid feca_device\n", __func__);
		return -1;
	}

	for(type = FECA_CD_CHAIN; type < FECA_CHAIN_MAX; type++)
	{
		/*Allocating memory for chain*/
		chain = (struct feca_chain *)feca_alloc_mem(sizeof(struct feca_chain));
		if(!chain){
			pr_err("%s: Can't allocate memory for Chain\n", __func__);
			return -1;
		}

		/* Updating parameter in chain */
		chain->chain_type = (chain_type_t) type;
		chain->parent_feca_dev  = feca_dev;
		/*Memory allocation for channel and it's valid Ids and state initialization*/
		channels = feca_channel_init(chain);
		if(!channels)
			return -1;

		/* Attaching channels with chain */
		chain->feca_channels = channels;

		/* Updating chain in feca_dev */
		feca_dev->feca_dev_chains[type] = chain;
	}

	return err;
}

int geul_feca_init(feca_dev_id id)
{
	int err = 0, i ;

	err = geul_feca_dev_init(id);
	if(err < 0)
	{
		pr_err("%s: FECA dev init fail. err[%d]\n", __func__, err);
		return err;
	}

	feca_dev_reset(bsp_get_feca_dev(id));
	for (i=0; i< 10000000; i++);

	err = geul_feca_chain_init(id);
	if(err < 0)
	{
		pr_err("%s: FECA chain init fail. err[%d]\n", __func__, err);
		return err;
	}

#if FECA_VALIDATION
	do_feca_validation();
#endif
	return err;
}

void geul_feca_deinit(void)
{

}
