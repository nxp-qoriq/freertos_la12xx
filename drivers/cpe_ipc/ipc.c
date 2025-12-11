// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 * 
 */

#include "FreeRTOS.h"
#include "gul_host_if.h"
#include "geul_cpe_ipc.h"
#include "geul_cpe_ipc_api.h"
#include "ipc.h"
#include "sync.h"
#include "geul_qdma.h"
#include "qdma.h"
#ifdef CONFIG_L1C_ENABLE
#include "l1ca_fwk.h"
#endif

#define IPC_NUM_OF_MEM_POOLS 			1
#define IPC_VALIDATE_API_INPUT			0
#define BUF_START_ALIGN				64
#define MAX_PTR_BUF_COUNT			64

#define IPC_DEBUG 0

struct gul_hif ipc_hif_area __attribute__ ((section (".hif.start"))) __attribute__ ((aligned (64)));

ipc_metadata_t ipc_md_area __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

uint8_t ipc_mempool_area[MODEM_IPC_APP_MEMPOOL_SIZE]
					  __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

ipc_t ipc_handle;
struct gul_ipc_stats ipc_stats;
ipc_mem_pool_t *ipc_mem_pool[IPC_NUM_OF_MEM_POOLS];
volatile struct gul_hif* hif;

/* IPC event mode locals */
uint32_t     		ipc_msi_to_chan[HOST_MSI_MAX_IRQ_COUNT];	/* Reverse lookup from msi->channel.*/
ipc_chan_callback 	ipc_callback_arr[IPC_MAX_CHANNEL_COUNT];	/* User callbacks. */
uint32_t		ipc_last_assigned_irq __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

uint32_t ipc_h2m_32(volatile uint32_t * addr)
{
	uint32_t retval = in_le32(addr);
	return retval;
}

void ipc_m2h_32(uint32_t val, volatile uint32_t * addr)
{
	out_le32(addr, val);
}

static inline void ipc_fill_errorcode(int *err, int code)
{
	if (err)
		*err = code;
}

static inline void ipc_update_modem_stats(int code)
{

	switch(code) {
			case IPC_INPUT_INVALID:
				ipc_m2h_32(++ipc_stats.err_input_invalid, &(hif->stats.m_ipc_stats.err_input_invalid));
				break;
			case IPC_INSTANCE_INVALID:
				ipc_m2h_32(++ipc_stats.err_instance_invalid, &(hif->stats.m_ipc_stats.err_instance_invalid));
				break;
			case IPC_MEM_INVALID:
				ipc_m2h_32(++ipc_stats.err_mem_invalid, &(hif->stats.m_ipc_stats.err_mem_invalid));
				break;
		}
}

static inline void ipc_update_channel_stats(int code, uint32_t ch_id)
{
	switch(code) {
			case IPC_INPUT_INVALID:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_input_invalid, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_input_invalid));
				break;
			case IPC_CH_INVALID:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_channel_invalid, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_channel_invalid));
				break;
			case IPC_MEM_INVALID:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_mem_invalid, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_mem_invalid));
				break;
			case IPC_CH_FULL:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_channel_full, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_channel_full));
				break;
			case IPC_CH_EMPTY:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_channel_empty, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_channel_empty));
				break;
			case IPC_BL_EMPTY:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_buf_list_empty, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_buf_list_empty));
				break;
			case IPC_BL_FULL:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_buf_list_full, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_buf_list_full));
				break;
			case IPC_NOT_IMPLEMENTED:
				ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].err_not_implemented, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].err_not_implemented));

	}
	return;
}

static void ipc_update_channel_rxtx_stats(uint32_t ch_id, uint32_t length, uint8_t tx_rx)
{
	ipc_stats.ipc_ch_stats[ch_id].total_msg_length+=length;
	switch(tx_rx) {
		case RX_STATS:
			ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].num_of_msg_recved, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].num_of_msg_recved));
			break;
		case TX_STATS:
			ipc_m2h_32(++ipc_stats.ipc_ch_stats[ch_id].num_of_msg_sent, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].num_of_msg_sent));
			break;
	}
	ipc_m2h_32(ipc_stats.ipc_ch_stats[ch_id].total_msg_length, &(hif->stats.m_ipc_stats.ipc_ch_stats[ch_id].total_msg_length));
	return;
}

static inline
int ipc_is_bd_ring_empty(uint32_t ci, uint32_t ci_flag,
			 uint32_t pi, uint32_t pi_flag)
{
	if (ci == pi) {
		/* If PI & CI flags are same, means Ring is Empty */
		if (ci_flag == pi_flag) {
#if IPC_DEBUG
			pr_debug("Ring Empty: ci %d ci_flag %d, pi %d pi_flag %d\n",
							ci, ci_flag, pi, pi_flag);
#endif
			return 1; /* No more Data */
		}
	}
	return 0;
}

static inline
int ipc_is_bd_ring_full(uint32_t ci, uint32_t ci_flag,
			uint32_t pi, uint32_t pi_flag)
{
	if (pi == ci) {
		/* If PI & CI flags are not same, mean consumer is pending
		   And Ring is Full */
		if (pi_flag != ci_flag) {
#if IPC_DEBUG
			pr_debug("BD Ring Full: ci %d ci_flag %d, pi %d pi_flag %d\n",
							ci, ci_flag, pi, pi_flag);
#endif
			return 1; /* Ring is Full */
		}
	}
	return 0;
}

static inline
int ipc_is_bl_full(uint32_t ci, uint32_t ci_flag,
		   uint32_t pi, uint32_t pi_flag)
{
	if (ci == pi) {
		if (pi_flag == ci_flag)
			return 1; /* List is Full */
	}
	return 0;
}

static inline
int ipc_is_bl_empty(uint32_t ci, uint32_t ci_flag,
		    uint32_t pi, uint32_t pi_flag)
{
	if (ci == pi) {
		/* If PI & CI flags are not same, means Ring has no free buffer */
		if (ci_flag != pi_flag) {
#if IPC_DEBUG
			pr_debug("BL Empty: ci %d ci_flag %d, pi %d pi_flag %d\n",
							ci, ci_flag, pi, pi_flag);
#endif
			return 1; /* No more empty buffer */
		}
	}
	return 0;
}

static phys_addr_t ipc_malloc(ipc_mem_pool_t* pool, uint32_t size, uint32_t align, int *err)
{
#if IPC_VALIDATE_API_INPUT
	/* Validate input */
	if (!pool || !size || !pool->mod_phys) {
		ipc_fill_errorcode(err, IPC_INPUT_INVALID);
		return 0;
	}
#endif
	uint32_t waste = 0;
	uint32_t align_off = 0;

	/* Calculate hole due to alignment requirement */
	if (align) {
		align_off = (uint32_t)(pool->modem_cptr % align);
		waste = align - align_off;
	}

	/* Is enough memory available in mempool? */
	if ((pool->modem_cptr + waste + size) >= (pool->mod_phys + pool->size)) {
		ipc_fill_errorcode(err, IPC_MEM_INVALID);
		pr_err("mempool don't have enough memory\r\n");
		return 0;
	}

	/* Update memory pointers in mempool after allocation */
	pool->modem_cptr += waste;
	phys_addr_t addr = (phys_addr_t) pool->modem_cptr;
	pool->modem_cptr += size;

	/* Zero out allocated memory */
	memset((void *)addr, 0, size);

	/* Fill errocode */
	ipc_fill_errorcode(err, IPC_SUCCESS);

	return addr;
}

static inline void ipc_mark_channel_as_configured(uint32_t channel_id, ipc_instance_t *instance)
{
	/* Read mask */
	ipc_bitmask_t mask = ipc_h2m_32(&(instance->cfgmask[channel_id / bitcount(ipc_bitmask_t)]));

	/* Set channel specific bit */
	mask |= (uint32_t)1 << (channel_id % bitcount(mask));

	/* Write mask */
	ipc_m2h_32(mask, &(instance->cfgmask[channel_id / bitcount(ipc_bitmask_t)]));
}

int ipc_is_channel_configured(uint32_t channel_id, ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;

#if IPC_VALIDATE_API_INPUT
	/* Validate channel id */
	if (!ipc_instance || channel_id >= IPC_MAX_CHANNEL_COUNT)
		return IPC_CH_INVALID;
#endif
	/* Read mask */
	ipc_bitmask_t mask = ipc_h2m_32(&(ipc_instance->cfgmask[channel_id / bitcount(ipc_bitmask_t)]));

	/* !! to return either 0 or 1 */
	return !!(mask & ((uint32_t)1 << (channel_id % bitcount(mask))));
}

/* list array size must be IPC_BITMASK_ARRAY_SIZE */
int ipc_get_list_of_configured_channel(ipc_bitmask_t list[], ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;

#if IPC_VALIDATE_API_INPUT
	/* Validate instance*/
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized)))
		return IPC_INSTANCE_INVALID;
#endif
	/* Fill masks from metadata to the argument list */
	unsigned int i;
	for (i = 0; i < IPC_BITMASK_ARRAY_SIZE; i++)
		list[i] = ipc_h2m_32(&(ipc_instance->cfgmask[i]));

	return 0;
}

int ipc_init(gul_mod_priv_t * priv, uint8_t core_id)
{
	/* Initialize memory for IPC metadata */
	priv->ipc_md = &ipc_md_area;
	log_info("%s: IPC_MD: 0x%x size:0x%x offset:0x%x\r\n", __func__, priv->ipc_md, (uint32_t)sizeof(ipc_metadata_t), ((uint32_t)priv->ipc_md - (uint32_t) PEBM_BASE_ADDR));

	/* Initialize per core globals */

	hif 		= bsp_get_hif();
	ipc_handle	= &(priv->ipc_md->instance_list[IPC1_INSTANCE_ID]);
	memset(&ipc_stats, 0, sizeof(struct gul_ipc_stats));

	if (core_id == GEUL_E200_MASTER_CORE)
	{
		int error;

		/* All one time init done by Core-0 for now.*/
		memset((void *)priv->ipc_md, 0x0, sizeof(ipc_metadata_t));

		/* update IPC Geul signature */
		ipc_m2h_32(0xA5A5A5A5, &priv->ipc_md->ipc_geul_signature);

		/* Update IPC md offset and size in HIF */
		ipc_m2h_32((uint32_t)priv->ipc_md - PEBM_BASE_ADDR, &(priv->pHif->ipc_regs.ipc_mdata_offset));
		ipc_m2h_32((uint32_t)sizeof(ipc_metadata_t), &(priv->pHif->ipc_regs.ipc_mdata_size));
		sync_dmb();

		/*Create mem pools.*/
		/* Get contiguous block for IPC Rx buffers. */
		ipc_mem_pool[0]	= pvGeulMalloc(sizeof(ipc_mem_pool_t));

		if( !ipc_mem_pool[0]) {
			log_err("%s:Cannot get memory for pool handle\r\n",__func__);
			return IPC_MALLOC_FAIL;
		}

		ipc_mem_pool[0]->mod_phys	= (uint32_t)ipc_mempool_area;
		ipc_mem_pool[0]->modem_cptr	= ipc_mem_pool[0]->mod_phys;
		ipc_mem_pool[0]->size		= MODEM_IPC_APP_MEMPOOL_SIZE;

		log_info("%s: IPC MEM POOL: 0x%x size:0x%x\r\n", __func__, (uint32_t)ipc_mempool_area, MODEM_IPC_APP_MEMPOOL_SIZE);

		/* Initialize IPC IRQ value */
		ipc_last_assigned_irq = IPC_CHANNEL_IRQ1;

		/* Initialize IPC subsystem and mark handle as initialized */
		if (ipc_modem_init(IPC1_INSTANCE_ID, ipc_mem_pool, &error) == NULL) {
			log_err("\r\n %s(%d) ipc_modem_init failed\r\n", __FUNCTION__,__LINE__);
			return error;
		}
	}

	log_info("%s:IPC Lib init done.\r\n",__func__);

	return IPC_SUCCESS;

}

ipc_t ipc_modem_init(uint32_t instance_id, ipc_mem_pool_t* mem_pools[], int *err)
{
	uint32_t i;
	int code = 0;

#if IPC_VALIDATE_API_INPUT
	if (instance_id >= IPC_MAX_INSTANCE_COUNT) {
		code = IPC_INSTANCE_INVALID;
		ipc_fill_errorcode(err, code);
		ipc_update_modem_stats(code);
		return NULL;
	}

	if (!mem_pools) {
		code = IPC_INPUT_INVALID;
		ipc_fill_errorcode(err, code);
		ipc_update_modem_stats(code);
		return NULL;
	}

	for (i = 0; i < IPC_MAX_MEMPOOL_COUNT; i++)
		if (!mem_pools[i]) {
			code = IPC_MEM_INVALID;
			ipc_fill_errorcode(err, code);
			ipc_update_modem_stats(code);
			return NULL;
		}
#endif

	ipc_metadata_t *md = bsp_get_ipc_md();
	ipc_instance_t *instance = &(md->instance_list[instance_id]);

	/* Return error if instance is already initialized */
	if (ipc_h2m_32(&(instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_fill_errorcode(err, code);
		ipc_update_modem_stats(code);
		return NULL;
	}

	for (i = 0; i < IPC_MAX_MEMPOOL_COUNT; i++)
		memcpy(&(instance->mem_pool[i]), mem_pools[i], sizeof (ipc_mem_pool_t));

	/* Initialize the IPC channel id in metadata */
	for(i = 0; i < IPC_MAX_CHANNEL_COUNT; i++)
	{
		ipc_m2h_32(i, &instance->ch_list[i].ch_id);
	}

	ipc_m2h_32(instance_id, &(instance->instance_id));
	ipc_m2h_32(1, &(instance->initialized));

	/* Set modem ready bit */
	SET_HIF_MOD_RDY(hif, HIF_MOD_READY_IPC_LIB);
	//vL1DCacheInvLine((uint32_t)hif, sizeof(*hif));
	PRINTF("%s modem ready - 0x%x\r\n", __func__, in_le32(&hif->mod_ready));
	ipc_fill_errorcode(err, IPC_SUCCESS);

	return instance;
}

int ipc_configure_channel(uint32_t channel_id,
			  uint32_t depth,
			  ipc_ch_type_t channel_type,
			  uint32_t msg_size,
			  uint8_t en_event,
			  ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t i;
	phys_addr_t mem;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}

	/* Supported modes: 0 - Poll, 1 - Event. NAPI unsupported.*/
	if (en_event > 1) {
		code = IPC_INPUT_INVALID;
		return code;
	}

#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (ipc_is_channel_configured(channel_id, instance)) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

#if IPC_DEBUG
	pr_debug("%s: channel: %d, depth: %d, type: %d, msg size: %d\r\n",
		 __func__, channel_id, depth, channel_type, msg_size);
#endif
	if (channel_type == IPC_CH_MSG) {

		ipc_m2h_32(channel_type, (volatile uint32_t *)&(ch->ch_type));
		ipc_m2h_32(channel_id, &(ch->ch_id));

		ipc_m2h_32(depth, &(ch->br_msg_desc.md.ring_size));
		ch->br_msg_desc.md.pi = 0;
		ch->br_msg_desc.md.ci = 0;
		ipc_m2h_32(msg_size, &(ch->br_msg_desc.md.msg_size));

		mem = ipc_malloc(&(ipc_instance->mem_pool[IPC_MODEM_BUF_ALLOC_POOL]),
				     depth * msg_size, BUF_START_ALIGN, &code);
		if (!mem) {
			code = IPC_MEM_INVALID;
			ipc_update_channel_stats(code, channel_id);
			return code;
		}

		for (i = 0; i < depth; i++) {
			ipc_m2h_32((uint32_t)((uint32_t)mem - (uint32_t)PEBM_BASE_ADDR),
					&(ch->br_msg_desc.bd[i].modem_ptr));
			mem += msg_size;
		}

	} else if (channel_type == IPC_CH_PTR) {

		ipc_m2h_32(channel_type, (volatile uint32_t *)&(ch->ch_type));
		ipc_m2h_32(channel_id, &(ch->ch_id));
		/* Fill msg */
		ipc_m2h_32(depth, &(ch->br_msg_desc.md.ring_size));
		ch->br_msg_desc.md.pi = 0;
		ch->br_msg_desc.md.ci = 0;
		ipc_m2h_32(sizeof(ipc_sh_buf_t), &(ch->br_msg_desc.md.msg_size));

		mem = ipc_malloc(&(ipc_instance->mem_pool[IPC_MODEM_BUF_ALLOC_POOL]),
				     depth * sizeof(ipc_sh_buf_t), 0, &code);
		if (!mem) {
			code = IPC_MEM_INVALID;
			ipc_update_channel_stats(code, channel_id);
			return code;
		}

		for (i = 0; i < depth; i++) {
			ipc_m2h_32((uint32_t)((uint32_t)mem - (uint32_t)PEBM_BASE_ADDR),
					&(ch->br_msg_desc.bd[i].modem_ptr));
			mem += sizeof(ipc_sh_buf_t);
		}

		/* Fill bl */
		ipc_m2h_32(MAX_PTR_BUF_COUNT, &(ch->br_bl_desc.md.ring_size));
		ch->br_bl_desc.md.pi = 0;
		ch->br_bl_desc.md.ci = 0;
		/* Buffer size should be 4288 Bytes - QDMA Aligned */
		ipc_m2h_32(msg_size, &(ch->br_bl_desc.md.msg_size));

		depth = MAX_PTR_BUF_COUNT;
		mem = ipc_malloc(&(ipc_instance->mem_pool[IPC_MODEM_BUF_ALLOC_POOL]),
				     depth * msg_size, BUF_START_ALIGN, &code);
		if (!mem) {
			code = IPC_MEM_INVALID;
			ipc_update_channel_stats(code, channel_id);
			return code;
		}

		for (i = 0; i < depth; i++) {
			/* Initialize QDMA descriptor contents */
			if (!QdmaInitCltLongCdSG((BaseType_t) mem)) {
				pr_debug("QDMA buffer intialization failed\n");
				code = IPC_MEM_INVALID;
				ipc_update_channel_stats(code, channel_id);
				return code;
			}
			ipc_m2h_32((uint32_t)((uint32_t)mem - (uint32_t)PEBM_BASE_ADDR),
					&(ch->br_bl_desc.bd[i].mod_phys));
			ipc_m2h_32(msg_size, &(ch->br_bl_desc.bd[i].buf_size));
			mem += msg_size;
		}
		ch->bl_initialized = 1;

	} else {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ipc_mark_channel_as_configured(channel_id, ipc_instance);

	/*
	 * There are multiple ways to configure interrupt based channel:
	 * 1) Either this API needs to be configured with en_event parameter.
	 * 2) Host can set msi_valid property of channel.
	 * 0 = Poll mode, 1 = Interrupt
	 */
	if (en_event)
		ipc_m2h_32(en_event, &(ch->msi_valid));

	return IPC_SUCCESS;
}

static int32_t ipc_irq_to_channel(uint32_t irq)
{
	if ((irq >= IPC_CHANNEL_IRQ1) && (irq < HOST_MSI_MAX_IRQ_COUNT)) {
		/*Difference between start of IRQ and channel values.*/
		return (int32_t)ipc_msi_to_chan[irq];
	} else {
		/*Invalid value. */
		return IPC_INPUT_INVALID;
	}
}

static bool_t ipc_channel_int_handler(uint32_t msi_irq, void *pvDevData)
{
	uint32_t ulMSIOffset = 0x10;
	uint32_t irq_num     = msi_irq - MSI_INTR_START;
	int   channel_id     = ipc_irq_to_channel(irq_num);

	(void)pvDevData;

	if (channel_id == IPC_INPUT_INVALID)
		return false;

	ipc_callback_arr[channel_id].func(ipc_callback_arr[channel_id].data);
	/* receive msg from interrupt, read will also clear interrupt */
	mpic_in32(MPIC_REGS_MSIR0 + irq_num * ulMSIOffset);
	return true;
}

static uint32_t ipc_get_free_irq(void)
{
	if (ipc_last_assigned_irq == IPC_CHANNEL_IRQ3)
		return IPC_CHANNEL_IRQ3;
	else
		return ipc_last_assigned_irq++;
}

int ipc_register_channel_callback(uint32_t channel_id, void (*func)(ipc_t), ipc_t data, ipc_t ch_handle)
{
	int32_t retval;
	uint32_t irq_number;

	/* Get the MSI IRQ number from channel data. */
	irq_number = ipc_h2m_32(&(((ipc_ch_t *)ch_handle)->msi_value));
	if (!irq_number) {
		irq_number = ipc_get_free_irq();
		/*Set the MSI IRQ number in channel data. */
		ipc_m2h_32(irq_number, &(((ipc_ch_t *)ch_handle)->msi_value));
		ipc_m2h_32(1, &(((ipc_ch_t *)ch_handle)->msi_valid));
	}

	fsl_print("IRQ register %d  For CH %d\r\n", irq_number, channel_id);
	if (irq_number == HOST_MSI_MAX_IRQ_COUNT)
		return IPC_INPUT_INVALID;

	retval = lRegisterIrq((uint32_t)(MSI_INTR_START + irq_number), ipc_channel_int_handler, 0);

	if (retval != 1) {
		log_err("IPC: IRQ register error:%d\r\n", irq_number);
		return IPC_EVENTFD_FAIL;
	}

	/*Associate IRQ number with channel, to be used during IRQ handler. */
	ipc_msi_to_chan[irq_number] = channel_id;
	/*Register user callbacks. */
	ipc_callback_arr[channel_id].func = func;
	ipc_callback_arr[channel_id].data = data;

	bMpicEnable((u32)DEVICE_SHARE_MESSAGE, (u32)irq_number);

	return IPC_SUCCESS;
}

/*
 * Host should init free buffer list
 * So not implemented on modem as of now
 */
int ipc_init_ptr_buf_list(uint32_t channel_id,
			  uint32_t depth, uint32_t size, ipc_t instance)
{
	UNUSED(channel_id);
	UNUSED(depth);
	UNUSED(size);
	UNUSED(instance);

	return IPC_NOT_IMPLEMENTED;
}

ipc_sh_buf_t* ipc_get_buf(uint32_t channel_id, ipc_t instance, int *err)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;
	uint32_t ring_size;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_fill_errorcode(err, code);
		ipc_update_modem_stats(code);
		return NULL;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		ipc_fill_errorcode(err, code);
		return NULL;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance) ||
	    ipc_h2m_32((volatile uint32_t *)&(ch->ch_type)) != IPC_CH_PTR || !ipc_h2m_32(&(ch->bl_initialized))) {
		code = IPC_CH_INVALID;
		ipc_fill_errorcode(err, code);
		ipc_update_channel_stats(code, channel_id);
		return NULL;
	}
#endif

	ipc_sh_buf_t *bd = ch->br_bl_desc.bd;
	ipc_br_md_t *md = &(ch->br_bl_desc.md);

	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);

	if (ipc_is_bl_empty(ci, ci_flag, pi, pi_flag)) {
		code = IPC_BL_EMPTY;
		ipc_fill_errorcode(err, code);
		ipc_update_channel_stats(code, channel_id);
		return NULL;
	}

	ipc_sh_buf_t* buf = &bd[ci];
	ci++;
	ring_size = ipc_h2m_32(&(md->ring_size));
	/* Flip the CI flag, if wrapping */
	if (ring_size == ci) {
		ci = 0;
		ci_flag = ci_flag ? 0 : 1;
	}
	if (ci_flag)
		IPC_SET_PI_FLAG(ci);
	else
		IPC_RESET_PI_FLAG(ci);
	ipc_m2h_32(ci, &(md->ci));
#if IPC_DEBUG
	pr_debug("%s: ci %d ci_flag %d, pi %d pi_flag %d\r\n",
			__func__, ci, ci_flag, pi, pi_flag);
#endif

	ipc_fill_errorcode(err, IPC_SUCCESS);

	return buf;
}

/*
 * As per current use case/design where PTR channel is used for both RX TB
 * from modem to host and TX TB from host to modem through shared buffer,
 * This API will be called from modem side.
 *
 */
int ipc_put_buf(uint32_t channel_id, ipc_sh_buf_t *buf_to_free, ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi, ring_size;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance) ||
	    ipc_h2m_32((volatile uint32_t *)&(ch->ch_type)) != IPC_CH_PTR || !ipc_h2m_32(&(ch->bl_initialized))) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_bl_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);
	if (ipc_is_bl_full(ci, ci_flag, pi, pi_flag)) {
		code = IPC_BL_FULL;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ring_size = ipc_h2m_32(&(md->ring_size));
	/* Copy back to ipc_sh_buf_t */
	memcpy(&ch->br_bl_desc.bd[pi], (void *)buf_to_free, sizeof(ipc_sh_buf_t));
	pi++;
	/* Flip the CI flag, if wrapping */
	if (ring_size == pi) {
		pi = 0;
		pi_flag = pi_flag ? 0 : 1;
	}
	if (pi_flag)
		IPC_SET_PI_FLAG(pi);
	else
		IPC_RESET_PI_FLAG(pi);

	ipc_m2h_32(pi, &(md->pi));
#if IPC_DEBUG
	pr_debug("%s: ci %d ci_flag %d, pi %d pi_flag %d\r\n", __func__,
			ci, ci_flag, pi, pi_flag);
#endif

	return IPC_SUCCESS;
}

int ipc_send_ptr(uint32_t channel_id,
		 ipc_sh_buf_t *buf,
		 ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t msg_len = 0;
	uint32_t ring_size;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;

#if IPC_VALIDATE_API_INPUT
	if (!buf || (ipc_h2m_32(&buf->host_virt_l) == 0)) {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance) ||
	    ipc_h2m_32((volatile uint32_t *)&(ch->ch_type)) != IPC_CH_PTR ||
	    !ipc_h2m_32(&(ch->bl_initialized))) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);
	if (ipc_is_bd_ring_full(ci, ci_flag, pi, pi_flag)) {
		code = IPC_CH_FULL;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ring_size = ipc_h2m_32(&(md->ring_size));
#if IPC_DEBUG
	pr_debug("%s enter: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, ci, pi_flag, ci_flag, ring_size);
#endif

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[pi];
	mod_mem_region_t *huge_page =  bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);

#ifdef CONFIG_L1C_ENABLE
	FAST_MEMCPY32((void *)(ipc_h2m_32(&(bd->modem_ptr)) + huge_page->addr_v), buf, sizeof (ipc_sh_buf_t));
#else
	memcpy((void *)(ipc_h2m_32(&(bd->modem_ptr)) + huge_page->addr_v), buf, sizeof (ipc_sh_buf_t));
#endif
	ipc_m2h_32(sizeof (ipc_sh_buf_t), &(bd->len));
	bd->crc = 0;

	pi++;
	/* Flip the PI flag, if wrapping */
	if (ring_size == pi) {
		pi = 0;
		pi_flag = pi_flag ? 0 : 1;
	}
	/* PI FLAG is updated in MSB*/
	if (pi_flag)
		IPC_SET_PI_FLAG(pi);
	else
		IPC_RESET_PI_FLAG(pi);
	/*
	 * Send specific interrupt
	 */
	uint32_t m_msi_value = ipc_h2m_32(&(ch->msi_value));
	uint32_t m_msi_valid = ipc_h2m_32(&(ch->msi_valid));

#if IPC_DEBUG
	pr_info("%s: MSI value %d  msi_valid %d channel_id %d \n\r", __func__,
		m_msi_value,m_msi_valid, channel_id);
#endif
	/*m_msi_valid value 2 implies NAPI enabled event based channel*/
	if (m_msi_valid == 2)
		ipc_m2h_32(0, &(ch->msi_valid));

	/* Wait for all updates and then update PI and raise interrupt */
	sync_dmb();
	ipc_m2h_32(pi, &(md->pi));

#if IPC_DEBUG
	pr_debug("%s exit: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n", __func__,
		 pi, ipc_h2m_32(&(md->ci)), ipc_h2m_32(&(md->pi_flag)), ipc_h2m_32(&(md->ci_flag)), ring_size);
#endif

	msg_len = ipc_h2m_32(&(buf->data_size));

	/*m_msi_valid value 2 implies NAPI enabled event based channel*/
	if (m_msi_valid == 2) {
		out_le32(bsp_get_mod_priv()->msi_info[m_msi_value].addr, bsp_get_mod_priv()->msi_info[m_msi_value].data);
	/*m_msi_valid value 1 implies NAPI disabled event based channel*/
	} else if (m_msi_valid == 1) {
		out_le32(bsp_get_mod_priv()->msi_info[m_msi_value].addr, bsp_get_mod_priv()->msi_info[m_msi_value].data);
	}

	ipc_update_channel_rxtx_stats(channel_id, msg_len, TX_STATS);
	return IPC_SUCCESS;
}

/*
 * Not to be implemented as of now.
 */
int ipc_get_prod_buf_ptr(uint32_t channel_id, void **buf_ptr, ipc_t instance)
{
	UNUSED(channel_id);
	UNUSED(buf_ptr);
	UNUSED(instance);

	return IPC_NOT_IMPLEMENTED;
}

int ipc_send_msg(uint32_t channel_id,
		 void *src,
		 uint32_t len,
		 ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ring_size;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;

#if IPC_VALIDATE_API_INPUT
	if (!src || !len) {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance)){
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);

	if (len > ipc_h2m_32(&(md->msg_size))) {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	if (ipc_is_bd_ring_full(ci, ci_flag, pi, pi_flag)) {
		code = IPC_CH_FULL;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ring_size = ipc_h2m_32(&(md->ring_size));

#if IPC_DEBUG
	pr_debug("%s enter: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, ipc_h2m_32(&(md->ci)), pi_flag, ipc_h2m_32(&(md->ci_flag)), ring_size);
#endif

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[pi];
	mod_mem_region_t *huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);

#ifdef CONFIG_L1C_ENABLE
	FAST_MEMCPY32((void *)(ipc_h2m_32(&(bd->modem_ptr)) + huge_page->addr_v), src, len);
#else
	memcpy((void *)(ipc_h2m_32(&(bd->modem_ptr)) + huge_page->addr_v), src, len);
#endif
	ipc_m2h_32((uint32_t)len, &(bd->len));

	pi++;
	/* Flip the PI flag, if wrapping */
	if (ring_size == pi) {
		pi = 0;
		pi_flag = pi_flag ? 0 : 1;
	}
	if (pi_flag)
		IPC_SET_PI_FLAG(pi);
	else
		IPC_RESET_PI_FLAG(pi);
	/*
	 * Send specific interrupt
	 */
	uint32_t m_msi_value = ipc_h2m_32(&(ch->msi_value));
	uint32_t m_msi_valid = ipc_h2m_32(&(ch->msi_valid));

#if IPC_DEBUG
	pr_info("%s: MSI value %d  msi_valid %d channel_id %d \n\r", __func__,
		m_msi_value,m_msi_valid, channel_id);
#endif
	/*m_msi_valid value 2 implies NAPI enabled event based channel*/
	if (m_msi_valid == 2)
		ipc_m2h_32(0, &(ch->msi_valid));

	/* Wait for all updates and then update PI and raise interrupt */
	sync_dmb();
	ipc_m2h_32(pi, &(md->pi));

#if IPC_DEBUG
	pr_debug("%s exit: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, ci, pi_flag, ci_flag, ring_size);
#endif


	/*m_msi_valid value 2 implies NAPI enabled event based channel*/
	if (m_msi_valid == 2) {
		out_le32(bsp_get_mod_priv()->msi_info[m_msi_value].addr, bsp_get_mod_priv()->msi_info[m_msi_value].data);
	}
	/*m_msi_valid value 1 implies NAPI disabled event based channel*/
	else if (m_msi_valid == 1) {
		out_le32(bsp_get_mod_priv()->msi_info[m_msi_value].addr, bsp_get_mod_priv()->msi_info[m_msi_value].data);
	}

	ipc_update_channel_rxtx_stats(channel_id, len, TX_STATS);
	return IPC_SUCCESS;
}


int ipc_recv_ptr(uint32_t channel_id, void *dst, ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ring_size;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;
	struct ipc_sh_buf *sh;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance)) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);

	if (ipc_is_bd_ring_empty(ci, ci_flag, pi, pi_flag)) {
		code = IPC_CH_EMPTY;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ring_size = ipc_h2m_32(&(md->ring_size));
#if IPC_DEBUG
	pr_debug("%s enter: ci: %d, pi: %d, ci_flag: %d, pi_flag: %d, ring size: %d\r\n",
		__func__, ci, ipc_h2m_32(&(md->pi)), ci_flag, ipc_h2m_32(&(md->pi_flag)), ring_size);
#endif

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[ci];

	sh = (struct ipc_sh_buf *)(ipc_h2m_32(&(bd->modem_ptr)) + (uint32_t)PEBM_BASE_ADDR);
	memcpy(dst, (void *)sh, sizeof(ipc_sh_buf_t));

	ci++;
	/* Flip the PI flag, if wrapping */
	if (ring_size == ci) {
		ci = 0;
		ci_flag = ci_flag ? 0 : 1;
	}
	if (ci_flag)
		IPC_SET_CI_FLAG(ci);
	else
		IPC_RESET_CI_FLAG(ci);

	sync_dmb();
	ipc_m2h_32(ci, &(md->ci));

#if IPC_DEBUG
	pr_debug("%s exit: ci: %d, pi: %d, ci_flag: %d, pi_flag: %d\r\n",
			__func__, ci, pi, ci_flag, pi_flag);
#endif

	ipc_update_channel_rxtx_stats(channel_id,
			ipc_h2m_32(&(sh->data_size)), RX_STATS);

	return IPC_SUCCESS;
}

int ipc_recv_msg(uint32_t channel_id, void *dst,
		 uint32_t *len, ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi, ring_size;

#if IPC_VALIDATE_API_INPUT
	if (!dst || !len) {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance)) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);
	if (ipc_is_bd_ring_empty(ci, ci_flag, pi, pi_flag)) {
		code = IPC_CH_EMPTY;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ring_size = ipc_h2m_32(&(md->ring_size));

#if IPC_DEBUG
	pr_debug("%s enter: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, ci, pi_flag, ci_flag, ring_size);
#endif

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[ci];
	ci++;
	/* Flip the CI flag, if wrapping */
	if (ring_size == ci) {
		ci = 0;
		ci_flag = ci_flag ? 0 : 1;
	}
	if (ci_flag)
		IPC_SET_CI_FLAG(ci);
	else
		IPC_RESET_CI_FLAG(ci);

	ipc_m2h_32(ci, &(md->ci));

	uint32_t msg_len = ipc_h2m_32(&(bd->len));
	if (msg_len > ipc_h2m_32(&(md->msg_size))) {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

        memcpy(dst, (void *)(ipc_h2m_32(&(bd->modem_ptr)) + (uint32_t)PEBM_BASE_ADDR), msg_len);

	*len = msg_len;

#if IPC_DEBUG
	pr_debug("%s exit: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, ipc_h2m_32(&(md->pi)), ci, ipc_h2m_32(&(md->pi_flag)), ci_flag, ring_size);
#endif
	ipc_update_channel_rxtx_stats(channel_id, msg_len, RX_STATS);

	return 0;
}

int ipc_recv_msg_ptr(uint32_t channel_id, void **dst_buffer,
		     uint32_t *len, ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;

#if IPC_VALIDATE_API_INPUT
	if (!dst_buffer || !len)
		return IPC_INPUT_INVALID;

	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized)))
		return IPC_INSTANCE_INVALID;

	if (channel_id >= IPC_MAX_CHANNEL_COUNT)
		return IPC_CH_INVALID;
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance))
		return IPC_CH_INVALID;
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);
	if (ipc_is_bd_ring_empty(ci, ci_flag, pi, pi_flag))
		return IPC_CH_EMPTY;

#if IPC_DEBUG
	uint32_t ring_size = ipc_h2m_32(&(md->ring_size));
	pr_debug("%s pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n", __func__,
		 pi, ci, pi_flag, ci_flag, ring_size);
#endif

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[ci];

	*dst_buffer = (void *)(ipc_h2m_32(&(bd->modem_ptr)) + (uint32_t)PEBM_BASE_ADDR);
	*len = ipc_h2m_32(&(bd->len));

	ipc_update_channel_rxtx_stats(channel_id, *len, RX_STATS);
	return IPC_SUCCESS;
}

int ipc_set_produced_status(uint32_t channel_id, ipc_t instance)
{
	UNUSED(channel_id);
	UNUSED(instance);

	return IPC_NOT_IMPLEMENTED;
}

int ipc_set_consumed_status(uint32_t channel_id, ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	uint32_t ring_size;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized)))
		return IPC_INSTANCE_INVALID;

	if (channel_id >= IPC_MAX_CHANNEL_COUNT)
		return IPC_CH_INVALID;
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance))
		return IPC_CH_INVALID;
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);
	if (ipc_is_bd_ring_empty(ci, ci_flag, pi, pi_flag))
		return IPC_CH_EMPTY;

	ring_size = ipc_h2m_32(&(md->ring_size));

#if IPC_DEBUG
	pr_debug("%s enter: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, ci, pi_flag, ci_flag, ring_size);
#endif

	ci++;
	/* Flip the CI flag, if wrapping */
	if (ring_size == ci) {
		ci = 0;
		ci_flag = ci_flag ? 0 : 1;
	}
	if (ci_flag)
		IPC_SET_CI_FLAG(ci);
	else
		IPC_RESET_CI_FLAG(ci);

	ipc_m2h_32(ci, &(md->ci));
#if IPC_DEBUG
	pr_debug("%s exit: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, ci, pi_flag, ci_flag, ring_size);
#endif

	return IPC_SUCCESS;
}

int ipc_get_msg_ptr(uint32_t channel_id,
		ipc_t instance,
		void **dst_buffer)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ci, ci_flag, pi, pi_flag;
	uint32_t tmp_ci, tmp_pi;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance)) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	tmp_ci = ipc_h2m_32(&(md->ci));

	ci = IPC_GET_CI_INDEX(tmp_ci);
	ci_flag = IPC_GET_CI_FLAG(tmp_ci);
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);
	if (ipc_is_bd_ring_full(ci, ci_flag, pi, pi_flag)) {
		code = IPC_CH_FULL;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[pi];
	mod_mem_region_t *huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
	*dst_buffer =  (void *)(ipc_h2m_32(&(bd->modem_ptr)) + huge_page->addr_v);

	return IPC_SUCCESS;
}

int ipc_send_msg_ptr(uint32_t channel_id,
		uint32_t len,
		ipc_t instance)
{
	ipc_instance_t *ipc_instance = instance;
	int code = IPC_SUCCESS;
	uint32_t ring_size;
	uint32_t pi, pi_flag, tmp_pi;

#if IPC_VALIDATE_API_INPUT
	if (!ipc_instance || !ipc_h2m_32(&(ipc_instance->initialized))) {
		code = IPC_INSTANCE_INVALID;
		ipc_update_modem_stats(code);
		return code;
	}

	if (channel_id >= IPC_MAX_CHANNEL_COUNT) {
		code = IPC_CH_INVALID;
		return code;
	}
#endif

	ipc_ch_t *ch = &(ipc_instance->ch_list[channel_id]);

#if IPC_VALIDATE_API_INPUT
	if (!ipc_is_channel_configured(channel_id, instance)) {
		code = IPC_CH_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}
#endif

	ipc_br_md_t *md = &(ch->br_msg_desc.md);
	tmp_pi = ipc_h2m_32(&(md->pi));
	pi = IPC_GET_PI_INDEX(tmp_pi);
	pi_flag = IPC_GET_PI_FLAG(tmp_pi);

	if (!len || len > ipc_h2m_32(&(md->msg_size))) {
		code = IPC_INPUT_INVALID;
		ipc_update_channel_stats(code, channel_id);
		return code;
	}

	ring_size = ipc_h2m_32(&(md->ring_size));
#if IPC_DEBUG
	pr_debug("%s enter: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, IPC_GET_CI_INDEX(ipc_h2m_32(&(md->ci))), pi_flag,
		IPC_GET_CI_FLAG(ipc_h2m_32(&(md->ci))), ring_size);
#endif

	ipc_bd_t *bdr = ch->br_msg_desc.bd;
	ipc_bd_t *bd = &bdr[pi];
	ipc_m2h_32((uint32_t)len, &(bd->len));
	/* Move Producer Index forward */
	pi++;
	/* Flip the PI flag, if wrapping */
	if (ring_size == pi) {
		pi = 0;
		pi_flag = pi_flag ? 0 : 1;
	}
	/* PI FLAG is updated in MSB*/
	if (pi_flag)
		IPC_SET_PI_FLAG(pi);
	else
		IPC_RESET_PI_FLAG(pi);
	/*
	 * Send specific interrupt
	 */
	uint32_t m_msi_value = ipc_h2m_32(&(ch->msi_value));
	uint32_t m_msi_valid = ipc_h2m_32(&(ch->msi_valid));

#if IPC_DEBUG
	pr_info("%s: MSI value %d  msi_valid %d channel_id %d \n\r", __func__,
		m_msi_value, m_msi_valid, channel_id);
#endif
	if (m_msi_valid == 2)
		ipc_m2h_32(0, &(ch->msi_valid));

	/* Wait for all updates and then update PI */
	 /* sync_dmb(); */
	ipc_m2h_32(pi, &(md->pi));
#if IPC_DEBUG
	pr_debug("%s exit: pi: %d, ci: %d, pi_flag: %d, ci_flag: %d, ring size: %d\r\n",
		__func__, pi, IPC_GET_CI_INDEX(ipc_h2m_32(&(md->ci))), pi_flag,
		IPC_GET_CI_FLAG(ipc_h2m_32(&(md->ci))), ring_size);
#endif

	/*m_msi_valid value 2 implies NAPI enabled event based channel*/
	if (m_msi_valid == 2) {
		out_le32(bsp_get_mod_priv()->msi_info[m_msi_value].addr, bsp_get_mod_priv()->msi_info[m_msi_value].data);
	} else if (m_msi_valid == 1) {
		/*m_msi_valid value 1 implies NAPI disabled event based channel*/
		out_le32(bsp_get_mod_priv()->msi_info[m_msi_value].addr, bsp_get_mod_priv()->msi_info[m_msi_value].data);
	}

	ipc_update_channel_rxtx_stats(channel_id, len, TX_STATS);
	return IPC_SUCCESS;
}

/* TODO: Implement below API for Geul */
int ipc_chk_recv_status(uint64_t *bmask, ipc_t instance)
{
	UNUSED(bmask);
	UNUSED(instance);

	return IPC_NOT_IMPLEMENTED;
}
