// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2023 NXP
 */

#include "FreeRTOS.h"
#include "gul_host_if.h"
#include "bbdev_ipc.h"
#include "sync.h"
#include "qdma.h"
#include "soc.h"

#define IPC_MAX_DEVICES			1

/* These MAX queues are defined and configurable
 * in CMakeList in FreeRTOS
 */
#define MSG_CH_MAX_BUFS			(MAX_CHANNEL_DEPTH)
#define QUEUE_MEMPOOL_SIZE		(MAX_MSG_SIZE * MSG_CH_MAX_BUFS)
#define MODEM_IPC_APP_MEMPOOL_SIZE	(QUEUE_MEMPOOL_SIZE * BBDEV_IPC_MAX_QUEUES)

#define IPC_DEBUG 0

#define UNUSED(x)	(void)(x)

ipc_t ipc_handle[IPC_MAX_DEVICES];
volatile struct gul_hif* hif;

struct md_priv {
	uint32_t pi[BBDEV_IPC_MAX_QUEUES];
	uint32_t ci[BBDEV_IPC_MAX_QUEUES];
	struct bbdev_ipc_raw_op_t *op[MAX_CHANNEL_DEPTH];
};

struct md_priv md_priv_queue;

struct gul_hif ipc_hif_area __attribute__ ((section (".hif.start"))) __attribute__ ((aligned (64)));
ipc_metadata_t ipc_md_area __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
uint8_t ipc_mempool_area[MODEM_IPC_APP_MEMPOOL_SIZE] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
volatile uint8_t core_ready[BBDEV_IPC_MAX_CORES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));
uint32_t ci_ctx[BBDEV_IPC_MAX_QUEUES]  __attribute__ ((section (".hif"))) __attribute__ ((aligned (64))) = {0xff};

struct dev_attr_t dev_attr;

/** IPC memory pool */
typedef struct ipc_mem_pool {
	uint64_t mod_phys;      /**< modem address of the buffer */
	uint64_t modem_cptr;    /**< Points to the start of the freespace in mem pool */
	uint32_t size;  /**< size of the memory pool */
} __attribute__((packed)) ipc_mem_pool_t;

ipc_mem_pool_t ipc_mem_pool[BBDEV_IPC_MAX_QUEUES] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

static void
ipc_m2h_32(uint32_t val, volatile uint32_t * addr)
{
	out_le32(addr, val);
}

static inline int
is_bd_ring_full(uint32_t ci, uint32_t pi, uint32_t ring_size)
{
	if (((pi + 1) % ring_size) == ci)
		return 1; /* Ring is Full */

	return 0;
}

static inline int
is_bd_ring_empty(uint32_t ci, uint32_t pi)
{
	if (ci == pi)
		return 1; /* No more Buffer */
	return 0;
}


uint16_t bbdev_ipc_dequeue_ops(uint8_t dev_id, uint16_t queue_id,
		struct bbdev_ipc_dequeue_op **ops, uint16_t num_ops)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch = &(ipc_instance->ch_list[queue_id]);
	ipc_br_md_t *md = &(ch->md);
	uint32_t pi;
	uint16_t i;

	for (i = 0; i < num_ops; i++) {
		pi = md->pi;
		if (ci_ctx[queue_id] == 0xff)
			ci_ctx[queue_id] = md->ci;

		if (ci_ctx[queue_id] == md->ring_size)
			ci_ctx[queue_id] = 0;
		if (is_bd_ring_empty((ci_ctx[queue_id]), pi))
			break;

#if IPC_DEBUG
		pr_debug("%s pi: %d, ci: %d, ring size: %d\r\n",
			__func__, pi, ci_ctx[queue_id], md->ring_size);
#endif

		ipc_bd_t *bdr = ch->bd_m;
		ipc_bd_t *bd = &bdr[ci_ctx[queue_id]];
		ops[i] = (struct bbdev_ipc_dequeue_op *)(bd->modem_ptr +
			PEBM_BASE_ADDR);
		ci_ctx[queue_id]++;
	}

	return i;
}

int bbdev_ipc_enqueue_raw_op(uint16_t dev_id, uint16_t queue_id,
				  struct bbdev_ipc_raw_op_t *op)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch = &(ipc_instance->ch_list[queue_id]);
	ipc_br_md_t *md = &(ch->md);
	host_ipc_params_t *hp = (host_ipc_params_t *)ch->host_ipc_params;
	mod_mem_region_t *huge_page;
	struct bbdev_ipc_raw_op_t *host_mem;
	uint32_t pi, ci, ring_size;
	ipc_bd_t *bdr;
	ipc_bd_t *bd;

	huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);

	/**
	 * In case of confirmation mode, local consumer index is incremented
	 * after receiving the output data(dequeue). Hence, before enqueuing the
	 * raw operation, we need to compare this local consumer index and
	 * producer index to check if bd ring is full.
	 * But, in case of non confirmation mode, since we will not receive the
	 * output data(dequeue function will not be called), local consumer
	 * index will not be updated. Hence, to check if bd ring is full, we
	 * will rely on the shared consumer index, which will be incrememnted by
	 * other side after consuming the packet.
	 */
	if (ch->conf_enable)
		ci = md_priv_queue.ci[queue_id];
	else
		ci = md->ci;
	pi = md_priv_queue.pi[queue_id];
	ring_size = md->ring_size;

	if (is_bd_ring_full(ci, pi, ring_size))
		return IPC_CH_FULL;

#if IPC_DEBUG
	pr_debug("%s enter: pi: %d, pi_flag: %d, ring size: %d\r\n", __func__,
		 pi, ring_size);
#endif

	bdr = ch->bd_h;
	bd = &bdr[pi];

	host_mem = (struct bbdev_ipc_raw_op_t *)(bd->modem_ptr +
			huge_page->addr_v);
	if (op->out_addr)
		host_mem->out_addr = op->out_addr - PEBM_BASE_ADDR;
	else
		host_mem->out_addr = 0;
	host_mem->out_len = op->out_len;
	host_mem->in_addr = op->in_addr - PEBM_BASE_ADDR;
	host_mem->in_len = op->in_len;

	md_priv_queue.op[pi] = op;

	/* Move Producer Index forward */
	pi++;
	/* Reset PI, if wrapping */
	if (pi == ring_size)
		pi = 0;
	md_priv_queue.pi[queue_id] = pi;

	/* Wait for all updates and then update PI */
	sync_dmb();
	hp->pi = pi;

#if IPC_DEBUG
	pr_debug("%s exit: pi: %d, pi_flag: %d, ring size: %d\r\n", __func__,
		 pi, ring_size);
#endif

	return IPC_SUCCESS;
}

struct bbdev_ipc_raw_op_t *bbdev_ipc_dequeue_raw_op(uint16_t dev_id,
		uint16_t queue_id)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch = &(ipc_instance->ch_list[queue_id]);
	ipc_br_md_t *md = &(ch->md);
	uint32_t ci, pi, temp_ci;
	struct bbdev_ipc_raw_op_t *op;
	uint32_t md_priv_op_index;

	if (ch->is_host_to_modem) {
		ci = md_priv_queue.ci[queue_id];
		pi = md->pi;

		if (is_bd_ring_empty(ci, pi))
			return NULL;

#if IPC_DEBUG
		pr_debug("%s enter: pi: %d, ci: %d, ring size: %d\r\n",
			 __func__, pi, ci, md->ring_size);
#endif

		ipc_bd_t *bdr = ch->bd_m;
		ipc_bd_t *bd = &bdr[ci];

		op = (struct bbdev_ipc_raw_op_t *)
			(bd->modem_ptr + PEBM_BASE_ADDR);

#if IPC_DEBUG
		pr_debug("%s exit: pi: %d, ci: %d, ring size: %d\r\n",
			 __func__, pi, ci, md->ring_size);
#endif
	} else {
		temp_ci = md->ci;
		ci = md_priv_queue.ci[queue_id];
		if (temp_ci == ci)
			return NULL;
		pi = md_priv_queue.pi[queue_id];

#if IPC_DEBUG
		pr_debug("%s enter: pi: %d, ci: %d, ring size: %d\r\n",
			 __func__, pi, ci, md->ring_size);
#endif

		ipc_bd_t *bdr = ch->bd_m;
		ipc_bd_t *bd = &bdr[ci];

		op = (struct bbdev_ipc_raw_op_t *)
			(bd->modem_ptr + PEBM_BASE_ADDR);
		md_priv_queue.op[ci]->status = op->status;
		md_priv_queue.op[ci]->out_len = op->out_len;
		md_priv_op_index = ci;

		/* Move Consumer Index forward */
		ci++;
		/* Reset CI, if wrapping */
		if (ci == md->ring_size)
			ci = 0;
		md_priv_queue.ci[queue_id] = ci;

#if IPC_DEBUG
		pr_debug("%s exit: pi: %d, ci: %d, ring size: %d\r\n",
			 __func__, pi, ci, md->ring_size);
#endif
		op = md_priv_queue.op[md_priv_op_index];
	}

	return op;
}

uint16_t bbdev_ipc_consume_raw_op(uint16_t dev_id, uint16_t queue_id,
				  struct bbdev_ipc_raw_op_t *op)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch = &(ipc_instance->ch_list[queue_id]);
	ipc_br_md_t *md = &(ch->md);
	host_ipc_params_t *hp = (host_ipc_params_t *)ch->host_ipc_params;
	mod_mem_region_t *huge_page;
	struct bbdev_ipc_raw_op_t *host_mem;
	ipc_bd_t *bdr;
	ipc_bd_t *bd;
	uint32_t ci;

	huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
	ci = md_priv_queue.ci[queue_id];

#if IPC_DEBUG
	pr_debug("%s enter: ci: %d, ring size: %d\r\n",
		__func__, ci, md->ring_size);
#endif

	bdr = ch->bd_h;
	bd = &bdr[ci];

	host_mem = (struct bbdev_ipc_raw_op_t *)(bd->modem_ptr +
		huge_page->addr_v);
	host_mem->status = op->status;
	host_mem->out_len = op->out_len;

	/* Move Consumer Index forward */
	ci++;
	/* Reset CI, if wrapping */
	if (ci == md->ring_size)
		ci = 0;
	md_priv_queue.ci[queue_id] = ci;

	/* Wait for all updates and then update PI */
	sync_dmb();
	hp->ci = ci;

#if IPC_DEBUG
	pr_debug("%s exit: ci: %d, ring size: %d\r\n",
		__func__, ci, md->ring_size);
#endif

	return 0;
}

uint16_t bbdev_ipc_enqueue_ops(uint8_t dev_id, uint16_t queue_id,
		struct bbdev_ipc_enqueue_op **ops, uint16_t num_ops)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch = &(ipc_instance->ch_list[queue_id]);
	ipc_br_md_t *md = &(ch->md);
	host_ipc_params_t *hp = (host_ipc_params_t *)ch->host_ipc_params;
	mod_mem_region_t *huge_page;
	struct bbdev_ipc_enqueue_op *host_mem;
	ipc_bd_t *bdr;
	ipc_bd_t *bd;
	uint32_t ci;
	uint16_t i;

	huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);

	for (i = 0; i < num_ops; i++) {
		ci = md->ci;

#if IPC_DEBUG
		pr_debug("%s enter: ci: %d, ring size: %d\r\n",
			__func__, ci, md->ring_size);
#endif

		if (ops[i]) {
			bdr = ch->bd_h;
			bd = &bdr[ci];

			host_mem = (struct bbdev_ipc_enqueue_op *)(bd->modem_ptr +
				huge_page->addr_v);
			host_mem->status = ops[i]->status;
			host_mem->crc_stat_addr = ops[i]->crc_stat_addr;
			host_mem->out_len = ops[i]->out_len;
		}

		/* Move Consumer Index forward */
		ci++;
		/* Reset CI, if wrapping */
		if (ci == md->ring_size)
			ci = 0;
		md->ci = ci;

		/* Wait for all updates and then update PI */
		sync_dmb();
		hp->ci = ci;

#if IPC_DEBUG
		pr_debug("%s exit: ci: %d, ring size: %d\r\n",
			__func__, ci, md->ring_size);
#endif
	}

	return i;
}

int bbdev_ipc_is_host_initialized(uint8_t dev_id)
{
	UNUSED(dev_id);

	/* TODO: Try reducing to one check for LIB */
	/* Wait for Host LIB and APP ready */
	if (!CHK_HIF_HOST_RDY(bsp_get_hif(), HIF_HOST_READY_IPC_LIB)) {
#if IPC_DEBUG
		pr_debug("%s: HOST LIB not ready.",__func__);
#endif
		return 0;
	}
	if(!CHK_HIF_HOST_RDY(bsp_get_hif(), HIF_HOST_READY_IPC_APP)) {
#if IPC_DEBUG
		pr_debug("%s: HOST APP not ready.",__func__);
#endif
		return 0;
	}

	return 1;
}

void bbdev_ipc_signal_ready(uint8_t dev_id)
{
	uint8_t core_id = ulMpicCurrentCore();
	uint8_t i, j;
	uint8_t num_cores = get_soc_numcores();

	UNUSED(dev_id);

	/* Wait till all cores are initialized */
	core_ready[core_id] = 1;

	while (1) {
		j = 0;
		for (i = 0; i < num_cores; i++)
			j += core_ready[i];
		/* Break once all cores have been initialized for BBDEV */
		if (j == num_cores)
			break;
	}

	if (core_id == 0)
		SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);
}

struct dev_attr_t *
bbdev_ipc_get_dev_attr(uint8_t dev_id)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch;
	int i;

	dev_attr.num_feca_se_channels = 0;
	dev_attr.num_feca_sd_channels = 0;
	dev_attr.num_feca_ce_channels = 0;
	dev_attr.num_feca_cd_channels = 0;
	dev_attr.num_queues = 0;

	dev_attr.use_feca_sd_single_qdma = ipc_instance->feca_sd_single_qdma;
	for (i = 0; i < BBDEV_IPC_MAX_QUEUES; i++) {
		ch = &(ipc_instance->ch_list[i]);
		if (ch->op_type) {
			if (ch->op_type == BBDEV_IPC_OP_LDPC_ENC)
				dev_attr.num_feca_se_channels++;
			if (ch->op_type == BBDEV_IPC_OP_LDPC_DEC)
				dev_attr.num_feca_sd_channels++;
			if (ch->op_type == BBDEV_IPC_OP_POLAR_ENC)
				dev_attr.num_feca_ce_channels++;
			if (ch->op_type == BBDEV_IPC_OP_POLAR_DEC)
				dev_attr.num_feca_cd_channels++;

			/* Configured queues are in sequence.
			 * So it is safe to assume i = num_queues at
			 * when ch->op_type is valid.
			 */
			dev_attr.num_queues++;
			dev_attr.qattr[i].queue_id = i;
			dev_attr.qattr[i].op_type = ch->op_type;
			dev_attr.qattr[i].depth = ch->depth;
			dev_attr.qattr[i].feca_blk_id = ch->feca_blk_id;
			dev_attr.qattr[i].feca_input_circ_size =
				ch->feca_input_circ_size;
			dev_attr.qattr[i].core_id = ch->la12xx_core_id;
		}
	}

	return &dev_attr;
}

void
bbdev_ipc_set_cb_size(uint8_t dev_id, uint32_t size, uint32_t queue)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch = &(ipc_instance->ch_list[queue]);

	ch->feca_input_circ_size = size;
}

int
bbdev_ipc_soft_reset_request(uint8_t dev_id)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];

	return ipc_instance->feca_soft_reset;
}

void
bbdev_ipc_soft_reset_complete(uint8_t dev_id)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	ipc_ch_t *ch;
	int i;

	/* Reset md priv queue */
	for (i = 0; i < BBDEV_IPC_MAX_QUEUES; i++) {
		md_priv_queue.pi[i] = 0;
		md_priv_queue.ci[i] = 0;
	}

	for (i = 0; i < MAX_CHANNEL_DEPTH; i++)
		md_priv_queue.op[i] = NULL;

	/* Reset Channel */
	for(i = 0; i < BBDEV_IPC_MAX_QUEUES; i++) {
		ch = &(ipc_instance->ch_list[i]);
		ch->md.pi = 0;
		ch->md.ci = 0;
		ci_ctx[i] = 0xff;
	}

	ipc_instance->feca_soft_reset = 0;
}

static phys_addr_t
bbdev_ipc_malloc(ipc_mem_pool_t* pool, uint32_t size, uint32_t align, int *err)
{
	uint32_t waste = 0;
	uint32_t align_off = 0;

	/* Calculate hole due to alignment requirement */
	if (align) {
		align_off = (uint32_t)(pool->modem_cptr % align);
		waste = align - align_off;
	}

	/* Is enough memory available in mempool? */
	if ((pool->modem_cptr + waste + size) >
	    (pool->mod_phys + pool->size)) {
		*err = IPC_MEM_INVALID;
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
	*err = IPC_SUCCESS;

	return addr;
}

int
bbdev_ipc_queue_configure(uint8_t dev_id, uint16_t queue_id)
{
	ipc_instance_t *ipc_instance = ipc_handle[dev_id];
	uint32_t depth, i;
	uint32_t msg_size = MAX_MSG_SIZE;
	host_ipc_params_t *hp;
	ipc_ch_t *ch;
	phys_addr_t mem;
	int code;

	while (!(in_le32(&(ipc_instance->initialized)))) { }

	ch = &(ipc_instance->ch_list[queue_id]);
	hp = (host_ipc_params_t *)ch->host_ipc_params;

	ipc_m2h_32(queue_id, &(ch->ch_id));
	depth = ch->depth;

	if (depth > MAX_CHANNEL_DEPTH) {
		pr_err("depth: %d is more than MAX_CHANNEL_DEPTH: %d\n",
			depth, MAX_CHANNEL_DEPTH);
		return IPC_INPUT_INVALID;
	}

#if IPC_DEBUG
	pr_debug("%s: queue: %d, depth: %d, type: %d, msg size: %d\r\n",
		 __func__, queue_id, depth, msg_size);
#endif

	ch->md.ring_size = depth;
	ch->md.pi = 0;
	ch->md.ci = 0;

	ipc_m2h_32(msg_size, &(ch->md.msg_size));

	/* We have 64 bytes of buf_entry, followed by remaining memory for
	 * the data. So for e.g. if msg_size is 256, data which can be stored
	 * is 192 bytes.
	 */
	mem = bbdev_ipc_malloc(&(ipc_mem_pool[queue_id]),
			 depth * msg_size, 0, &code);
	if (!mem)
		return code;

	for (i = 0; i < depth; i++) {
		ch->bd_m[i].modem_ptr = mem - PEBM_BASE_ADDR;
		ipc_m2h_32(ch->bd_m[i].modem_ptr,
			&(hp->bd_m_modem_ptr[i]));
		mem += msg_size;
	}

	return IPC_SUCCESS;
}

int
bbdev_ipc_init(uint8_t dev_id, uint8_t core_id)
{
	gul_mod_priv_t* priv = pGulModPriv;
	ipc_instance_t *instance;
	uint16_t i;

	/* Initialize memory for IPC metadata */
	priv->ipc_md = &ipc_md_area;

	/* Initialize per core globals.*/
	hif = bsp_get_hif();
	ipc_handle[dev_id] = &(priv->ipc_md->instance_list[dev_id]);

	if (core_id == GEUL_E200_MASTER_CORE) {
		log_info("%s: IPC_MD: 0x%x size:0x%x offset:0x%x\n\r",
			__func__, priv->ipc_md, (uint32_t)sizeof(ipc_metadata_t),
			((uint32_t)priv->ipc_md - (uint32_t) PEBM_BASE_ADDR));

		/* All one time init done by Core-0 for now.*/
		memset((void *)priv->ipc_md, 0x0, sizeof(ipc_metadata_t));

		/* update IPC Geul signature */
		ipc_m2h_32(0xA5A5A5A5, &priv->ipc_md->ipc_geul_signature);

		/* Update IPC md offset and size in HIF */
		ipc_m2h_32((uint32_t)priv->ipc_md - PEBM_BASE_ADDR,
			&(priv->pHif->ipc_regs.ipc_mdata_offset));
		ipc_m2h_32((uint32_t)sizeof(ipc_metadata_t),
			&(priv->pHif->ipc_regs.ipc_mdata_size));
		sync_dmb();

		instance = &(priv->ipc_md->instance_list[dev_id]);
		for (i = 0; i < BBDEV_IPC_MAX_QUEUES; i++) {
			/* Create mem pools.*/
			/* Get contiguous block for IPC Rx buffers. */
			ipc_mem_pool[i].mod_phys = (uint32_t)ipc_mempool_area +
					(i * QUEUE_MEMPOOL_SIZE);
			ipc_mem_pool[i].modem_cptr = ipc_mem_pool[i].mod_phys;
			ipc_mem_pool[i].size = QUEUE_MEMPOOL_SIZE;
		}

		/* Initialize the IPC channel id in metadata */
		for(i = 0; i < BBDEV_IPC_MAX_QUEUES; i++)
			ipc_m2h_32(i, &instance->ch_list[i].ch_id);

		ipc_m2h_32(dev_id, &(instance->instance_id));
		ipc_m2h_32(1, &(instance->initialized));

		ipc_m2h_32(MAX_LDPC_ENC_FECA_QUEUES, &(instance->max_ldpc_enc_feca_queues));
		ipc_m2h_32(MAX_LDPC_DEC_FECA_QUEUES, &(instance->max_ldpc_dec_feca_queues));
		ipc_m2h_32(MAX_POLAR_ENC_FECA_QUEUES, &(instance->max_polar_enc_feca_queues));
		ipc_m2h_32(MAX_POLAR_DEC_FECA_QUEUES, &(instance->max_polar_dec_feca_queues));
		ipc_m2h_32(MAX_RAW_QUEUES, &(instance->max_raw_queues));
		ipc_m2h_32(MAX_CHANNEL_DEPTH, &(instance->max_channel_depth));

		/* TODO: Try reducing to one set for LIB */
		/* Set modem ready bit */
		SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_LIB);

		/* Check if MAX_MSG_SIZE is more then bbdev_ipc_dequeue_op */
		if (MAX_MSG_SIZE < sizeof(struct bbdev_ipc_dequeue_op) ||
		    MAX_MSG_SIZE < sizeof(struct bbdev_ipc_enqueue_op) ||
		    MAX_MSG_SIZE < sizeof(struct bbdev_ipc_raw_op_t)) {
			pr_err("MAX_MSG_SIZE: %d is less.\n\r"
				"bbdev_ipc_dequeue_op size: %d\n\r"
				"bbdev_ipc_enqueue_op size: %d\n\r"
				"bbdev_ipc_raw_op size: %d\n\r",
				MAX_MSG_SIZE, sizeof(struct bbdev_ipc_dequeue_op),
				sizeof(struct bbdev_ipc_enqueue_op),
				sizeof(struct bbdev_ipc_raw_op_t));
			return IPC_MEM_INVALID;
		}

		PRINTF("%s modem ready - 0x%x\r\n", __func__, in_le32(&hif->mod_ready));
	}

	log_info("%s:IPC Lib init done.\n\r",__func__);

	return IPC_SUCCESS;
}
