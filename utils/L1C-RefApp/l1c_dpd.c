/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

#include "FreeRTOS.h"
#include "task.h"
#include "spinlock_api.h"
#include <debug_console.h>
#include "mpic.h"
#include "geul_avi.h"
#include "geul_avi_ds.h"
#include "semphr.h"
#include "Time.h"
#include "pmux.h"
#include "gpio.h"
#include "ppc.h"
#include "ppu_intrinsics.h"
#include "l1c_defs.h"
#include "l1c_freq.h"
#include "l1c_fwk_tasks.h"
#include "l1c_vspa_agent.h"
#include "l1c_time_agent.h"
#include "l1c_time_proc.h"
#include "l1c_vspa_proc.h"
#include "l1c_dma.h"
#include "l1c_dpd.h"
#include "l1c_debug.h"
#include "dcs_plat_config.h"

#define DPDH_TX_CORE 0
#define DPDH_DPD_CORE 4
#define DPDH_SRX_LS_CORE 2
#define DPDH_SRX_HS_CORE 6
#define DPDH_SRX_HS_EXTRA_CORE 7

#define DPDH_VSPA_DMA_CHANNEL 30
#define DPDH_VSPA_DMA_CHAN_MASK (1 << DPDH_VSPA_DMA_CHANNEL)

#define DPDH_TICK_ADVANCE_491  ((d.srx_interface_id > LS_DCS1_IF1)?TBGEN2_50_US:TBGEN1_50_US)
#define DPDH_TICK_ADVANCE_245 (((d.srx_interface_id > LS_DCS1_IF1)?TBGEN2_50_US:TBGEN1_50_US) * 2)
#define DPDH_TICK_ADVANCE_122 (((d.srx_interface_id > LS_DCS1_IF1)?TBGEN2_50_US:TBGEN1_50_US) * 4)

#define HOST_NOT_L1C_CAPTURE_READY 0x43210000

#define DPDH_SEGMENT_SIZE 4096

#define DPDH_BUFFER_SIZE 0x2800000

#define DEBUG_DUMP_VSPA_MSGS 0x0001

dpd_vars_t d = {
	.tx_address = TX_SIGNAL_BUFFER,
	.dpd_dump_address = DPD_SIGNAL_BUFFER,
	.rx_address = SRX_SIGNAL_BUFFER,
	.signal_segment_count = 4,
	.training_segment_count = 4,
	.interface_id = 0,
	.srx_interface_id = ~0, // default to invalid to copy tx interface
	// to save memory, assumption made here that we'll never use more than half of the steps for DPDH
	.srx_trx_allow = &trx_allow[MAX_TDD_SEQUENCE_STEPS / 2],
	.advance = 8,
	.repeat = 999,
	.segment_size = DPDH_SEGMENT_SIZE,
	.dpd_sample_rate = SAMPLE_RATE_491,
	.tx_dcs_sample_rate = 0,
	.rx_dcs_sample_rate = 0,
	.ifft_sample_rate = SAMPLE_RATE_491,
	.fft_sample_rate = SAMPLE_RATE_491,
	.dpd_running_mode = DPD_CONTINOUS,
	.dpd_seq_remaining = 0
};

signal_config_t vspa_signal_msg_tx;
signal_config_t vspa_signal_msg_dpd;
signal_config_t vspa_signal_msg_srx;
coeff_update_msg_t vspa_coeff_update_msg;

vuint32 *axiq_timer_ctrl = NULL;
vuint32 *srx_axiq_timer_ctrl = NULL;
vuint32 *srx_hs0_lp_timer_ctrl = NULL;
vuint32 *srx_hs1_lp_timer_ctrl = NULL;
rf_ctrl_tbgen_signal_t *rf_fem_ctrl_ptr = NULL;
rf_ctrl_tbgen_signal_t *rf_fem_tdd_ctrl_ptr = NULL;

int irq_c = 0;
extern uint32_t tbgen1_div_B0;
extern uint32_t tbgen2_div_B0;
extern void send_host_notification(uint32_t msg_id, uint32_t data, uint32_t data2, uint32_t data3);

bool_t vspa_dma_irq_handler(uint32_t ulIrqNo, void *pdata)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	TaskHandle_t xTaskHandle = pdata;

	ulIrqNo -= INTERNAL_INTR_START; // adapt to MPIC range

	if (d.srx_interface_id > LS_DCS1_IF1)
	{
		vspa_set_dmareg_irq_stat(DPDH_SRX_HS_CORE, DPDH_VSPA_DMA_CHAN_MASK);
		vspa_set_dmareg_irq_stat(DPDH_SRX_HS_EXTRA_CORE, DPDH_VSPA_DMA_CHAN_MASK);
	} else {
		vspa_set_dmareg_irq_stat(DPDH_SRX_LS_CORE, DPDH_VSPA_DMA_CHAN_MASK);
	}

	irq_c++;

	xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR(xTaskHandle, 1, eSetBits, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	return 0;
}

void l1c_register_vspa_irq(void * pdata, uint8_t core, uint32_t irq_no)
{
	int ret;

	ret = lRegisterIrq(INTERNAL_INTR_START + irq_no, vspa_dma_irq_handler, pdata);

	if (ret < 0)
	{
		PRINTF("IRQ register error!\r\n");
	}
	else
	{
		bMpicEnable(DEVICE_INTERNAL, irq_no);
		vspa_set_irqen(core , 1 << 4 /* irq_dma_cmp */);
	}
}

void dbg_vspa_signal_msg_dump(uint8_t core, signal_config_t *msg)
{
#ifdef DEBUG_DUMP_VSPA_MSGS
if (e200_print_mask_get(DEBUG_DUMP_VSPA_MSGS)) {
	PRINTF("Sending to VSPA %u Signal msg:\r\n", core);
	PRINTF(" fetch_address %#x\r\n", swap_uint32(msg->fetch_address));
	PRINTF(" dump_address %#x\r\n", swap_uint32(msg->dump_address));
	PRINTF(" signal_segment_count %#x\r\n", swap_uint32(msg->signal_segment_count));
	PRINTF(" training_segment_count %#x\r\n", swap_uint32(msg->training_segment_count));
	PRINTF(" axiq_path %u\r\n", swap_uint32(msg->axiq_path));
	PRINTF(" repeat %u\r\n", swap_uint32(msg->repeat));
	PRINTF(" segment_size %u\r\n", swap_uint32(msg->segment_size));
	PRINTF(" dpd_sample_rate %u\r\n", swap_uint32(msg->dpd_sample_rate));
	PRINTF(" tx_dcs_sample_rate %u\r\n", swap_uint32(msg->tx_dcs_sample_rate));
	PRINTF(" rx_dcs_sample_rate %u\r\n", swap_uint32(msg->rx_dcs_sample_rate));
	PRINTF(" ifft_sample_rate %u\r\n", swap_uint32(msg->ifft_sample_rate));
	PRINTF(" fft_sample_rate %u\r\n", swap_uint32(msg->fft_sample_rate));
}
#else
	(void) core;
	(void) msg;
#endif /* DEBUG_DUMP_VSPA_MSGS */
}

inline void l1c_vspa_signal_msg_send(uint8_t core, signal_config_t *msg)
{
	l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_SIGNAL_CONFIG, msg, sizeof(signal_config_t));
	dbg_vspa_signal_msg_dump(core, msg);
}

void dbg_vspa_coeff_update_dump(uint8_t core, coeff_update_msg_t *msg)
{
#ifdef DEBUG_DUMP_VSPA_MSGS
if (e200_print_mask_get(DEBUG_DUMP_VSPA_MSGS)) {
	PRINTF("Sending to VSPA %u coeff type %u update msg from addr %#u\r\n",
		core, swap_uint32(msg->coeff_type), swap_uint32(msg->memory_address));
}
#else
	(void) core;
	(void) msg;
#endif /* DEBUG_DUMP_VSPA_MSGS */
}

inline void l1c_vspa_coeff_update_msg_send(uint8_t core, coeff_update_msg_t *msg)
{
	if (l1c_vspa_get_runtime_overlay(core) < 0)
		return;
	l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_COEFF_UPDATE, msg, sizeof(coeff_update_msg_t));
	dbg_vspa_coeff_update_dump(core, msg);
}

/* TODO: To be removed */
void vL1CDPDConfig(l1c_config_dpd_t *cfg)
{
	e200_trace(E200_TRACE_MSG_DPD_CONFIG, E200_TRACE_PARAM_TRACK);
	d.interface_id = (cfg->dcs << 1) | (cfg->interface);
	if (d.srx_interface_id > 5)
		d.srx_interface_id = d.interface_id;
	d.signal_segment_count = cfg->signal_segment_cnt;
	d.training_segment_count = cfg->training_segment_cnt;
	d.repeat = cfg->repeat;

	d.dpd_sample_rate = cfg->dpd_sample_rate;
	d.ifft_sample_rate = cfg->ifft_sample_rate;
	d.fft_sample_rate = cfg->fft_sample_rate;

	vPortFree(cfg);
}

void vL1CDPDConfigAdvance(uint32_t cnt)
{
	d.advance = cnt;
}

void vL1CDPDConfigRepeat(uint32_t cnt)
{
	d.repeat = cnt;
}

void vL1CDPDConfigTrainingSeg(uint32_t cnt)
{
	d.training_segment_count = cnt;
}

void vL1CDPDConfigSignalSeg(uint32_t cnt)
{
	d.signal_segment_count = cnt;
}

void vL1CDPDConfigTx(uint8_t tx)
{
	d.interface_id = tx;
}

void vL1CDPDConfigSRx(uint8_t srx)
{
	d.srx_interface_id = srx;

	if(d.srx_interface_id < HS_DCS_IF0 )
		d.fft_sample_rate = SAMPLE_RATE_245;
	else
		d.fft_sample_rate = SAMPLE_RATE_491;
}

void vL1CDPDRate(config_sample_rate_e rate)
{
	d.dpd_sample_rate = rate;
}

void vL1CDPDIfftRate(config_sample_rate_e rate)
{
	d.ifft_sample_rate = rate;
}

void vL1CDPDFftRate(config_sample_rate_e rate)
{
	d.fft_sample_rate = rate;
}

void vL1CDPDDACRate(config_sample_rate_e rate)
{
	d.tx_dcs_sample_rate = rate;
}

void vL1CDPDADCRate(config_sample_rate_e rate)
{
	d.rx_dcs_sample_rate = rate;
}

void l1c_sig_gen_msg(uint8_t core, uint32_t f_nco, config_sample_rate_e f_s)
{
	sig_gen_config_msg_t msg;

	msg.f_nco = swap_uint32(f_nco);
	msg.f_s = swap_uint32(f_s);

	l1c_vspa_agent_enqueue_msg(core, L1C_MSG_A2V_SIG_GEN_CONFIG, &msg, sizeof(msg));
}

void vL1CDPDGen(int64_t freq_mHz)
{
	int64_t Fs_Hz = sample_rate_enum2Hz(d.dpd_sample_rate);

	d.f_nco = nco_freq_calc(freq_mHz, Fs_Hz);

	PRINTF("Sending to VSPA f %d (%#x), FS = %u kHz, sample rate e = %u, to generate the requested frq = ",
		 d.f_nco, d.f_nco, (uint32_t)(Fs_Hz / 1000), d.dpd_sample_rate);
	print_freq_in_mHz(freq_mHz);

	l1c_sig_gen_msg(DPDH_DPD_CORE, d.f_nco, d.dpd_sample_rate);
}

void vL1CDPDCfgDump()
{
	uint64_t host_coeff_dpd_addr;
	uint64_t host_coeff_tx_qec_addr;
	uint64_t host_coeff_rx_qec_addr;
	uint64_t host_tx_signal_addr;
	uint64_t host_srx_signal_addr;
	uint64_t host_dpd_ref_addr;

	e200_trace(E200_TRACE_MSG_DPD_CONFIG_DUMP, E200_TRACE_PARAM_TRACK);

	l1c_get_tbgen_dcs_values_for_GeulB0( &d);

	host_coeff_dpd_addr     = DPD_COEFF_BUFFER    - GEUL_FECA_BASE_ADDR + HOST_PCIE_BAR2_ADDR;
	host_coeff_tx_qec_addr  = TX_QEC_COEFF_BUFFER - GEUL_FECA_BASE_ADDR + HOST_PCIE_BAR2_ADDR;
	host_coeff_rx_qec_addr  = RX_QEC_COEFF_BUFFER - GEUL_FECA_BASE_ADDR + HOST_PCIE_BAR2_ADDR;
#ifdef GEUL_LA1224
	mod_mem_region_t *scratch_buf = bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
	/* override HRAM locations with DDR scratch space */
	d.tx_address = scratch_buf->addr_v;
	d.rx_address = d.tx_address + DPDH_BUFFER_SIZE;
	/* start at the beggining of region*/
	host_tx_signal_addr =  HOST_VIRT_ADDR_START;
	host_srx_signal_addr = host_tx_signal_addr + DPDH_BUFFER_SIZE;
#else
	host_tx_signal_addr     = TX_SIGNAL_BUFFER    - GEUL_FECA_BASE_ADDR + HOST_PCIE_BAR2_ADDR;
	host_srx_signal_addr    = SRX_SIGNAL_BUFFER   - GEUL_FECA_BASE_ADDR + HOST_PCIE_BAR2_ADDR;
#endif
	host_dpd_ref_addr       = DPD_SIGNAL_BUFFER   - GEUL_FECA_BASE_ADDR + HOST_PCIE_BAR2_ADDR;

	PRINTF("\r\n");
	PRINTF(" _____________________________________________________________________________________________ \r\n" \
		   "|              |             |                            Buffers                             |\r\n" \
		   "|              |             |----------------------------------------------------------------|\r\n" \
		   "|              |    Name     |   Desc   |    Vspa    |     Host     |     Size (bytes)        |\r\n" \
		   "|--------------|-------------|----------+------------+--------------+-------------------------|\r\n" \
		   "| Tx Interface | LS-DCS%d:%d   | DPD coef | 0x%08x | %#x%08x | 0x%08x (%10d) |\r\n" \
		   "| SRx Interface| %cS-DCS%c:%d   | TX QEC   | 0x%08x | %#x%08x | 0x%08x (%10d) |\r\n" \
		   "|              |             | RX QEC   | 0x%08x | %#x%08x | 0x%08x (%10d) |\r\n" \
		   "|              |             | TX input | 0x%08x | %#x%08x | 0x%08x (%10d) |\r\n" \
		   "|              |             | DPD ref  | 0x%08x | %#x%08x | 0x%08x (%10d) |\r\n" \
		   "|              |             | SRX sig  | 0x%08x | %#x%08x | 0x%08x (%10d) |\r\n" \
		   "|______________|_____________|__________|____________|______________|_________________________|\r\n",
			d.interface_id >> 1,
			d.interface_id & 1,
			DPD_COEFF_BUFFER,
			(uint32_t)(host_coeff_dpd_addr >> 32), (uint32_t)host_coeff_dpd_addr,
			COEFF_BUFFER_SIZE,
			COEFF_BUFFER_SIZE,

			(d.srx_interface_id >> 2) ? 'H' : 'L',
			(d.srx_interface_id >> 2) ? ' ' : ((d.srx_interface_id >> 1) ? '1' : '0'),
			d.srx_interface_id & 1,
			TX_QEC_COEFF_BUFFER,
			(uint32_t)(host_coeff_tx_qec_addr >> 32), (uint32_t)host_coeff_tx_qec_addr,
			COEFF_BUFFER_SIZE,
			COEFF_BUFFER_SIZE,

			RX_QEC_COEFF_BUFFER,
			(uint32_t)(host_coeff_rx_qec_addr >> 32), (uint32_t)host_coeff_rx_qec_addr,
			COEFF_BUFFER_SIZE,
			COEFF_BUFFER_SIZE,

			d.tx_address,
			(uint32_t)(host_tx_signal_addr >> 32), (uint32_t)host_tx_signal_addr,
			d.signal_segment_count * d.segment_size * 4,
			d.signal_segment_count * d.segment_size * 4,

			DPD_SIGNAL_BUFFER,
			(uint32_t)(host_dpd_ref_addr >> 32), (uint32_t)host_dpd_ref_addr,
			d.training_segment_count * d.segment_size * 4,
			d.training_segment_count * d.segment_size * 4,

			d.rx_address,
			(uint32_t)(host_srx_signal_addr >> 32), (uint32_t)host_srx_signal_addr,
			(d.training_segment_count * TRAINING_SEGMENTS_MULTIPLIER) * d.segment_size * 4,
			(d.training_segment_count * TRAINING_SEGMENTS_MULTIPLIER) * d.segment_size * 4);

	PRINTF("\r\n" \
		   "\r\nsegment size = %d" \
		   "\r\nTx signal segment count = %d" \
		   "\r\nTx training segment count = %d" \
		   "\r\nSRx signal segment count = %d" \
		   "\r\nsupplemental Tx repeats = %d" \
		   "\r\nIFFT sample rate = %s" \
		   "\r\nFFT rate = %s" \
		   "\r\nDPD rate = %s" \
		   "\r\ntx_dcs_sample_rate (DAC) = %s" \
		   "\r\nrx_dcs_sample_rate (ADC) = %s\r\n", \
		   d.segment_size,
		   d.signal_segment_count,
		   d.training_segment_count,
		   d.training_segment_count*TRAINING_SEGMENTS_MULTIPLIER,
		   d.repeat,
		   sample_rate_enum2str(d.ifft_sample_rate),
		   sample_rate_enum2str(d.fft_sample_rate),
		   sample_rate_enum2str(d.dpd_sample_rate),
		   sample_rate_enum2str(d.tx_dcs_sample_rate),
		   sample_rate_enum2str(d.rx_dcs_sample_rate)
		   );
}

void vL1CDPDDump()
{
	/* debug stats */
	dump_core_msg_count();
	l1c_tdd_trx_dump(trx_allow, 3);
	if (d.srx_interface_id != d.interface_id)
		l1c_tdd_trx_dump(d.srx_trx_allow, 2);

	dump_vspa_debug_stats();
}

void vL1CUpdate_DPD_QEC(l1c_update_t *cfg)
{
	switch (cfg->type)
	{
		case L1C_UPDATE_DPD:
			/* send DPD coeff update message */
			e200_trace(E200_TRACE_MSG_UPDATE_DPD, E200_TRACE_PARAM_TRACK);
			vspa_coeff_update_msg.memory_address = swap_uint32(DPD_COEFF_BUFFER);
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_DPD);
			l1c_vspa_coeff_update_msg_send(cfg->core_id, &vspa_coeff_update_msg);
			break;
		case L1C_UPDATE_TX_QEC:
			/* send TX QEC coeff update message */
			e200_trace(E200_TRACE_MSG_UPDATE_TXQEC, E200_TRACE_PARAM_TRACK);
			vspa_coeff_update_msg.memory_address = swap_uint32(TX_QEC_COEFF_BUFFER);
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_TX_QEC);
			l1c_vspa_coeff_update_msg_send(cfg->core_id, &vspa_coeff_update_msg);
			break;
		case L1C_UPDATE_RX_QEC:
			/* send RX QEC coeff update message */
			e200_trace(E200_TRACE_MSG_UPDATE_RXQEC, E200_TRACE_PARAM_TRACK);
			vspa_coeff_update_msg.memory_address = swap_uint32(RX_QEC_COEFF_BUFFER);
			vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_RX_QEC);
			l1c_vspa_coeff_update_msg_send(cfg->core_id, &vspa_coeff_update_msg);
			break;
		case L1C_UPDATE_CFR:
			/* send CFR coeff update message */
			e200_trace(E200_TRACE_MSG_UPDATE_CFR, E200_TRACE_PARAM_TRACK);
			PRINTF("Not (yet) implemented for DPDH!\r\n");
			break;
		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
			break;
	}
	vPortFree(cfg);
}

void l1c_fem_ctrl_defaults()
{
#ifndef NO_RF
	uint64_t start_time;

	if (!config_common.rf_fem_ctrl)
		return;

	/* setup RF FEM control signals to Tx mode */
	start_time = ullTbgenGetMasterCounter(TBGEN_1);
	start_time += 4 * TBGEN1_25_US;
	l1c_rf_ctrl_sig_setup(rf_fem_ctrl_ptr, start_time);

	/* wait 100us */
	tbgen_wait_master_counter(TBGEN_1, start_time);

	/* transition RF FEM control signals from Tx to Rx mode */
	start_time += 4 * TBGEN1_25_US;
	l1c_rf_ctrl_sig_transition(rf_fem_ctrl_ptr, start_time, 1 /*tx->rx*/);

	l1c_gpio_pmux_setup();
#endif
}

void l1c_fem_ctrl_start(uint64_t start_time, uint8_t periodical)
{
#ifndef NO_RF
	if (!config_common.rf_fem_ctrl)
		return;

	/* program RF card TX enable */
	l1c_rf_ctrl_sig_transition(rf_fem_ctrl_ptr, start_time, 0 /*rx->tx*/);

	/* setup TDD timers for RF control */
	for (uint8_t i = 0; i < TDD_MAX_INSTANCE; i++)
	{
		if (!trx_enable_steps[i])
			continue;
		l1c_tdd_timer_program(l1c_get_tdd_ctrl(TBGEN_2, i),
					start_time - tbgen_offset() + trx_advance[i],
					trx_enable[i],
					trx_enable_steps[i],
					periodical,
					TDD_MODE_11);
	}
#else
	(void) start_time;
	(void) periodical;
#endif
}

void l1c_fem_ctrl_disable()
{
#ifndef NO_RF
	uint64_t time_offset;

	if (!config_common.rf_fem_ctrl)
		return;

	/* disable TDD timers controlling RF card */
	for (uint8_t i = 0; i < TDD_MAX_INSTANCE; i++)
	{
		if (!trx_enable_steps[i])
			continue;
			l1c_tdd_timer_program(l1c_get_tdd_ctrl(TBGEN_2, i),
						  0 /* time offset - don't care when steps=0 */,
						  trx_enable[i], /* don't care when steps=0 */
						  0 /* zero steps - disable */,
						  0 /* repetitive - don't care when steps=0 */,
						  TDD_MODE_11 /* idle mode - don't care when steps=0*/);
	}

	/* program RF card TX off */
	time_offset = ullTbgenGetMasterCounter(TBGEN_1);
	time_offset += 4 * TBGEN1_25_US;
	l1c_rf_ctrl_sig_transition(rf_fem_ctrl_ptr, time_offset, 1 /*tx->rx*/);
#endif
}

void l1c_vspa_signal_msg_tx_initialize(signal_config_t *vspa_signal_msg)
{
	vspa_signal_msg->axiq_path = swap_uint32(d.interface_id);
	vspa_signal_msg->segment_size = swap_uint32(DPDH_SEGMENT_SIZE);
	vspa_signal_msg->repeat = swap_uint32(d.repeat);

	vspa_signal_msg->signal_segment_count = swap_uint32(d.signal_segment_count);
	vspa_signal_msg->ifft_sample_rate = swap_uint32(d.ifft_sample_rate);
	vspa_signal_msg->dpd_sample_rate = swap_uint32(d.dpd_sample_rate);
	vspa_signal_msg->tx_dcs_sample_rate = swap_uint32(d.tx_dcs_sample_rate);

	// likely unused by this VSPA core
	vspa_signal_msg->training_segment_count = swap_uint32(d.training_segment_count);
}

void l1c_vspa_signal_msg_dpd_initialize(signal_config_t *vspa_signal_msg)
{
	vspa_signal_msg->axiq_path = swap_uint32(d.interface_id);
	vspa_signal_msg->segment_size = swap_uint32(DPDH_SEGMENT_SIZE);
	vspa_signal_msg->repeat = swap_uint32(d.repeat);

	// limit DPD out samples size to 16 x DPDH_SEGMENT_SIZE x 4B
	vspa_signal_msg->training_segment_count = swap_uint32((d.training_segment_count > 16) ?
							16 : d.training_segment_count);
	vspa_signal_msg->signal_segment_count = swap_uint32(d.signal_segment_count);
	vspa_signal_msg->dpd_sample_rate = swap_uint32(d.dpd_sample_rate);

	// likely unused by this VSPA core
	vspa_signal_msg->ifft_sample_rate = swap_uint32(d.ifft_sample_rate);
	vspa_signal_msg->tx_dcs_sample_rate = swap_uint32(d.tx_dcs_sample_rate);
}

void l1c_vspa_signal_msg_srx_initialize(signal_config_t *vspa_signal_msg)
{
	vspa_signal_msg->axiq_path = swap_uint32(d.srx_interface_id);
	vspa_signal_msg->segment_size = swap_uint32(DPDH_SEGMENT_SIZE);
	vspa_signal_msg->repeat = swap_uint32(d.repeat);

	vspa_signal_msg->training_segment_count = swap_uint32(d.training_segment_count * TRAINING_SEGMENTS_MULTIPLIER);
	vspa_signal_msg->rx_dcs_sample_rate = swap_uint32(d.rx_dcs_sample_rate);
	vspa_signal_msg->fft_sample_rate = swap_uint32(d.fft_sample_rate);

	// likely unused by this VSPA core
	vspa_signal_msg->fetch_address = 0;
	vspa_signal_msg->ifft_sample_rate = swap_uint32(d.ifft_sample_rate);
}

void l1c_dpd_reinit(void)
{
	uint64_t time_offset = 0;
	uint64_t tmp64 = 0;
	uint32_t tx_duration = 0;
	uint32_t rx_duration = 0;
	uint8_t  periodical = 0;
	uint32_t advance = 0;
	uint32_t advance_rx = 0;
	uint64_t time_offset_rx = 0;
	int32_t rate_matching_rx = 0;
	int32_t rate_matching_tx = 0;
	volatile struct gul_hif * pxHif = bsp_get_hif();
	int32_t tbgen1_tbgen2_delta = 0;
	static uint8_t first = 0;
	static uint8_t one_time_offset_compute = 1;

	tbgen1_tbgen2_delta = (sample_rate_kHz2enum(pxHif->tbgen_clk_info.tbgen1_freq_khz)- sample_rate_kHz2enum(pxHif->tbgen_clk_info.tbgen2_freq_khz));
	/* Only the case when we have configured TBGEN1 >= TBGEN2 is supported!! */
	tbgen1_tbgen2_delta = (tbgen1_tbgen2_delta < 0) ? 0 : tbgen1_tbgen2_delta;

#ifdef GEUL_LA1224
	mod_mem_region_t *scratch_buf = bsp_get_mem_region(MOD_MEM_SCRATCH_BUF);
	d.tx_address = scratch_buf->addr_v;
	d.rx_address = d.tx_address + DPDH_BUFFER_SIZE;
#endif
	if (d.dpd_seq_remaining > 0)
		d.dpd_running_mode = DPD_SEQUENCE;
	else
		d.dpd_running_mode = DPD_CONTINOUS;

	/* get axiq tdd timer control */
	axiq_timer_ctrl = l1c_get_axiq_ctrl(d.interface_id);
	srx_axiq_timer_ctrl = l1c_get_axiq_ctrl(d.srx_interface_id);
	srx_hs0_lp_timer_ctrl = l1c_get_timer_lp_ctrl(HS_DCS_IF0);
	srx_hs1_lp_timer_ctrl = l1c_get_timer_lp_ctrl(HS_DCS_IF1);
	rf_fem_ctrl_ptr = l1c_get_rf_fem_controls(d.interface_id);
	rf_fem_tdd_ctrl_ptr = l1c_get_rf_tdd_fem_controls(d.interface_id);

	/* send VSPA overlay messages */
	l1c_vspa_set_runtime_overlay(DPDH_TX_CORE, RT_OVLY_DPDH);
	l1c_vspa_set_runtime_overlay(DPDH_DPD_CORE, RT_OVLY_DPDH);
	if (d.srx_interface_id > LS_DCS1_IF1)
	{
		l1c_vspa_set_runtime_overlay(DPDH_SRX_HS_CORE, RT_OVLY_DPDH);
		l1c_vspa_set_runtime_overlay(DPDH_SRX_HS_EXTRA_CORE, RT_OVLY_DPDH);
	} else {
		l1c_vspa_set_runtime_overlay(DPDH_SRX_LS_CORE, RT_OVLY_DPDH);
	}
	// wait for overlays to finish
	cal_spin_loop(CAL_SPIN_10_US + first*4*CAL_SPIN_10_US);
	first = 1;

	/* send TX QEC coeff update message */
	vspa_coeff_update_msg.memory_address = swap_uint32(TX_QEC_COEFF_BUFFER);
	vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_TX_QEC);
	l1c_vspa_coeff_update_msg_send(DPDH_TX_CORE, &vspa_coeff_update_msg);

        /* send TX Sig Gen config message */
        l1c_sig_gen_msg(DPDH_DPD_CORE, d.f_nco, d.dpd_sample_rate);

	/* send SRX QEC coeff update message */
	vspa_coeff_update_msg.memory_address = swap_uint32(RX_QEC_COEFF_BUFFER);
	vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_RX_QEC);

	/* send DPD coeff update message */
	vspa_coeff_update_msg.memory_address = swap_uint32(DPD_COEFF_BUFFER);
	vspa_coeff_update_msg.coeff_type = swap_uint32(COEFF_TYPE_DPD);
	l1c_vspa_coeff_update_msg_send(DPDH_DPD_CORE, &vspa_coeff_update_msg);

	tmp64 = d.signal_segment_count;
	tmp64 *= d.segment_size * (d.repeat + 1);
	if (tmp64 >= 0x80000000ULL) {
		PRINTF("Invalid config, timing overflow, please reduce transmission length!\r\n");
		d.dpd_running_mode = DPD_IDLE;
		return;
	}
	tx_duration = (uint32_t)(tmp64 & 0xFFFFFFFF);

	tmp64 = TRAINING_SEGMENTS_MULTIPLIER * d.training_segment_count; // we need to receive 2x the number of training segments for DPD training
	tmp64 *= d.segment_size;  
	if (tmp64 >= 0x80000000ULL) {
		PRINTF("Invalid configuration, timing overflow, please reduce training length!\r\n");
		d.dpd_running_mode = DPD_IDLE;
		return;
	}
	rx_duration = (uint32_t)(tmp64 & 0xFFFFFFFF);

	/* Get RX/TX DCS values based on configured interface and DCS_PLL_CLK in PORSR1 register */
	l1c_get_tbgen_dcs_values_for_GeulB0( &d );

	/* Get VSPA compensated values for TX */
	int32_t comp_tx = l1c_get_vspa_comp_for_GeulB0( &d, 0);
	
	if(comp_tx < 0)
	{
		PRINTF("Invalid or not supported DCS/IFFT/FFT sample rate configuration, ignoring TX VSPA compensation! \r\n");
		comp_tx = 0;
	}

	if (d.interface_id > LS_DCS1_IF1) /* HS DCS is configured !*/
	{
		rate_matching_tx = (d.tx_dcs_sample_rate - comp_tx - sample_rate_kHz2enum(pxHif->tbgen_clk_info.tbgen2_freq_khz)) + tbgen2_div_B0;
	}
	else /* LS DCS is configured !*/
	{
		rate_matching_tx = (d.tx_dcs_sample_rate - comp_tx - sample_rate_kHz2enum(pxHif->tbgen_clk_info.tbgen1_freq_khz)) + tbgen1_div_B0;
	}

	/* Get VSPA compensated values for RX */
	int32_t comp_rx = l1c_get_vspa_comp_for_GeulB0( &d, 1);
	
	if(comp_rx < 0)
	{
		PRINTF("Invalid or not supported DCS/IFFT/FFT sample rate configuration, ignoring RX VSPA compensation! \r\n");
		comp_rx = 0;
	}

	if (d.srx_interface_id > LS_DCS1_IF1) /* HS DCS is configured !*/
	{
		l1c_vspa_coeff_update_msg_send(DPDH_SRX_HS_CORE, &vspa_coeff_update_msg);
		l1c_vspa_coeff_update_msg_send(DPDH_SRX_HS_EXTRA_CORE, &vspa_coeff_update_msg);
		rate_matching_rx = (d.rx_dcs_sample_rate - comp_rx - sample_rate_kHz2enum(pxHif->tbgen_clk_info.tbgen2_freq_khz)) + tbgen2_div_B0;
	} else { /* LS DCS is configured !*/
		l1c_vspa_coeff_update_msg_send(DPDH_SRX_LS_CORE, &vspa_coeff_update_msg);
		rate_matching_rx = (d.rx_dcs_sample_rate - comp_rx - sample_rate_kHz2enum(pxHif->tbgen_clk_info.tbgen1_freq_khz)) + tbgen1_div_B0;
	}

	tx_duration = (tx_duration >> (rate_matching_tx));
	rx_duration = (rx_duration >> (rate_matching_rx));

	if (e200_print_mask_get(DEBUG_DUMP_VSPA_MSGS)) 
	{
		PRINTF("\n\rConfigured rations and durations:"\
				"\r\n          rate_matching_tx     = %d;"\
				"\r\n          rate_matching_rx     = %d;"\
				"\r\n          tx_duration     = %d;"\
				"\r\n          rx_duration     = %d \n",\
				rate_matching_tx, 
				rate_matching_rx,
				tx_duration, 
				rx_duration);
	}

	memset(trx_allow, 0, sizeof(trx_allow));
	trx_allow_steps = 0;
	d.srx_trx_allow_steps = 0;

	l1c_rf_tdd_signals_compute(rf_fem_tdd_ctrl_ptr);

#ifndef NO_RF
	/* induces 100 us delay! */
	l1c_fem_ctrl_defaults(rf_fem_ctrl_ptr);
	/* induces large delay! */
	tbgen_offset_calc();
#endif

	/* set the time advance for the DPD start */
	if(one_time_offset_compute)
	{
		TBGEN_WRITE_REGISTER(srx_hs0_lp_timer_ctrl, 0);
		TBGEN_WRITE_REGISTER(srx_hs1_lp_timer_ctrl, 0);
		one_time_offset_compute = 0;
	}
	time_offset    = (d.tx_dcs_sample_rate == SAMPLE_RATE_983) ? ullTbgenGetMasterCounter(TBGEN_2) : ullTbgenGetMasterCounter(TBGEN_1);
	time_offset_rx = (d.rx_dcs_sample_rate == SAMPLE_RATE_983) ? ullTbgenGetMasterCounter(TBGEN_2) : time_offset;
	
	advance        = d.advance * ((d.tx_dcs_sample_rate == SAMPLE_RATE_983) ? TBGEN2_25_US : TBGEN1_25_US);

	if(d.rx_dcs_sample_rate == SAMPLE_RATE_983)
		advance_rx     = d.advance * TBGEN2_25_US - TBGEN1_TBGEN2_DELTA;
	else
		advance_rx     = d.advance * TBGEN1_25_US;


	/* Compute the TX offset based on TX timing as a reference, tx_duration is computed like this also */
	time_offset    += tx_duration + advance;
	time_offset_rx += (tx_duration >> tbgen1_tbgen2_delta) + advance_rx;

	periodical = ((d.dpd_running_mode == DPD_SEQUENCE) && (d.dpd_seq_remaining == 1)) ? 0 : 1;

	if (d.srx_interface_id == d.interface_id)
	{
		trx_allow[0].tx_rx_allowed = TDD_MODE_11;
		trx_allow[0].duration = rx_duration;
		trx_allow_steps = 2;
		trx_allow[1].tx_rx_allowed = TDD_MODE_10;
		trx_allow[1].duration = tx_duration - rx_duration;
		// this extra step is used by dpd stop to get back to default state
		trx_allow[2].tx_rx_allowed = TDD_MODE_00;
		trx_allow[2].duration = TBGEN_1S;

		l1c_tdd_timer_program(axiq_timer_ctrl,
					  time_offset,
					  trx_allow,
					  trx_allow_steps,
					  periodical,
					  TDD_MODE_00);
	} else {
		// Tx
		trx_allow[0].tx_rx_allowed = TDD_MODE_10;
		trx_allow[0].duration = tx_duration;
		trx_allow_steps = 1;
		// this extra step is used by dpd stop to get back to default state
		trx_allow[1].tx_rx_allowed = TDD_MODE_00;
		trx_allow[1].duration = (TBGEN_1S >> tbgen1_tbgen2_delta);

		// SRx
		d.srx_trx_allow[0].tx_rx_allowed = TDD_MODE_01;
		d.srx_trx_allow[0].duration = rx_duration;
		d.srx_trx_allow_steps = 2;
		d.srx_trx_allow[1].tx_rx_allowed = TDD_MODE_00;
		d.srx_trx_allow[1].duration = (tx_duration >> tbgen1_tbgen2_delta) - rx_duration;
		// this extra step is used by dpd stop to get back to default state
		d.srx_trx_allow[2].tx_rx_allowed = TDD_MODE_00;
		d.srx_trx_allow[2].duration = (TBGEN_1S >> tbgen1_tbgen2_delta) + (tx_duration >> tbgen1_tbgen2_delta) - rx_duration;

		l1c_tdd_timer_program(  axiq_timer_ctrl,
								time_offset,
								trx_allow,
								trx_allow_steps,
								periodical,
								TDD_MODE_00);

		l1c_tdd_timer_program(  srx_axiq_timer_ctrl,
								time_offset_rx,
								d.srx_trx_allow,
								d.srx_trx_allow_steps,
								periodical,
								TDD_MODE_00);
	}
#ifndef NO_RF
	l1c_fem_ctrl_start(time_offset, periodical);
#endif

	l1c_vspa_signal_msg_tx_initialize(&vspa_signal_msg_tx);
	l1c_vspa_signal_msg_dpd_initialize(&vspa_signal_msg_dpd);
	l1c_vspa_signal_msg_srx_initialize(&vspa_signal_msg_srx);

	/* configure TBGEN to issue ticks (tx_duration), 1 tick ahead */
	l1c_setup_periodical_tick(TBGEN_1, time_offset - advance, tx_duration, crt_core_id);
	e200_trace(E200_TRACE_MSG_DPD_RUN, (uint32_t)(time_offset - advance));
}

void l1c_send_vspa_msgs()
{
	l1c_vspa_signal_msg_send(DPDH_TX_CORE, &vspa_signal_msg_tx);

	/* address to read Tx samples from */
	vspa_signal_msg_dpd.fetch_address = swap_uint32(d.tx_address);
	/* address to dump DPD out samples to */
	vspa_signal_msg_dpd.dump_address = swap_uint32(d.dpd_dump_address);
	l1c_vspa_signal_msg_send(DPDH_DPD_CORE, &vspa_signal_msg_dpd);

	/* address to write SRx samples to */
	vspa_signal_msg_srx.dump_address = swap_uint32(d.rx_address);

	if (d.srx_interface_id > LS_DCS1_IF1)
	{
		l1c_vspa_signal_msg_send(DPDH_SRX_HS_CORE, &vspa_signal_msg_srx);
		l1c_vspa_signal_msg_send(DPDH_SRX_HS_EXTRA_CORE, &vspa_signal_msg_srx);
	} else {
		l1c_vspa_signal_msg_send(DPDH_SRX_LS_CORE, &vspa_signal_msg_srx);
	}

	e200_trace(E200_TRACE_MSG_DPD_RUN, 0xcafea000);
}

void l1c_dpd_finish()
{
	// program a transition to default state
	l1c_tdd_timer_update_steps(axiq_timer_ctrl,
					  trx_allow,
					  trx_allow_steps,
					  trx_allow_steps+1);
	if (d.srx_interface_id != d.interface_id)
		l1c_tdd_timer_update_steps(srx_axiq_timer_ctrl,
						  d.srx_trx_allow,
						  d.srx_trx_allow_steps,
						  d.srx_trx_allow_steps+1);
	// end DPD on next tick
	d.dpd_running_mode = DPD_END;

	e200_trace(E200_TRACE_MSG_DPD_RUN, 0xcafea001);
}

void l1c_dpd_end()
{
	l1c_stop_periodical_tick(TBGEN_1, crt_core_id);

	// delay for the advance btw our wake up and next TBGEN start (that goes into 00 mode)
	cal_spin_loop(CAL_SPIN_10_US * 1000);

	/* disable TDD timer controlling AXIQ */
	l1c_tdd_timer_program(axiq_timer_ctrl, 0, trx_allow, 0, 0, TDD_MODE_00);
	if (d.srx_interface_id != d.interface_id)
		l1c_tdd_timer_program(srx_axiq_timer_ctrl, 0, d.srx_trx_allow, 0, 0, TDD_MODE_00);

#ifndef NO_RF
	l1c_fem_ctrl_disable();
#endif

	// go to idle
	d.dpd_running_mode = DPD_IDLE;

	e200_trace(E200_TRACE_MSG_DPD_RUN, 0xcafea002);
	l1c_task_suspend(L1C_DPD_TASK);
}

void l1c_dpd_state_machine()
{

	e200_trace(E200_TRACE_MSG_DPD_RUN, (d.dpd_running_mode << 16) | (0xFF & d.dpd_seq_remaining));

	switch (d.dpd_running_mode) {
		// mode changes to DPD_CONTINOUS or DPD_SEQUENCE
		case DPD_INIT:
			l1c_dpd_reinit();
			break;
		// until dpd stop is called, run in a periodical loop
		case DPD_CONTINOUS:
			l1c_send_vspa_msgs();
			break;
		// repeat loop for a finite number of iterations
		case DPD_SEQUENCE:
			l1c_send_vspa_msgs();
			if (--d.dpd_seq_remaining <= 0)
				l1c_dpd_finish();
			break;
		// stop requested during run
		case DPD_STOP:
			l1c_dpd_finish();
			break;
		// sequence terminated, going to default state, suspend thread
		case DPD_END:
			l1c_dpd_end();
			break;
		case DPD_IDLE:
		default:
			vTaskDelay(1);
		break;
	}
}

void l1c_dpd_start_main(void *pvParameters)
{
	task_id_t task_id = *(task_id_t *)pvParameters;

	e200_trace(E200_TRACE_MSG_DPD_START, E200_TRACE_PARAM_TRACK);

	/* wait for tick, in a loop */
	for( ;; )
	{
		wait_for_tick(task_id);

		l1c_dpd_state_machine();
	}

	// never reached
}

void vL1CDPDInit()
{
	// default to idle state
	d.dpd_running_mode = DPD_IDLE;

	l1c_create_task(DPD_CORE, L1C_DPD_TASK, "DPD Task", TICK_ENABLE, L1C_MED_PRIO, DEFAULT_STACK_SIZE, l1c_dpd_start_main);

	l1c_task_suspend(L1C_DPD_TASK);

	e200_trace_enable(E200_TRACE_MSG_DPD_RUN);
}

void vL1CDPDStop()
{
	if (d.dpd_running_mode == DPD_STOP ||
		d.dpd_running_mode == DPD_END ||
		d.dpd_running_mode == DPD_IDLE)
		return;

	e200_trace(E200_TRACE_MSG_DPD_STOP, E200_TRACE_PARAM_TRACK);

	// request DPD to stop
	d.dpd_running_mode = DPD_STOP;

	// wait for DPD to finish
	while(d.dpd_running_mode != DPD_IDLE)
		vTaskDelay(2);
}

void vL1CDPDStart(uint32_t dpd_iter)
{
	uint64_t start_time;

	// if it's already running, stop it gracefully
	vL1CDPDStop();

	// wait for DPD to finish if it's not aleady in idle mode
	while(d.dpd_running_mode != DPD_IDLE)
		vTaskDelay(2);

	d.dpd_seq_remaining = dpd_iter;

	// request an init
	d.dpd_running_mode = DPD_INIT;

	// configure TBGEN to issue ticks, to kickstart the initialization, actual period will be overridden
	start_time = ullTbgenGetMasterCounter(TBGEN_1) + TBGEN_500NS;
	l1c_setup_periodical_tick(TBGEN_1, start_time, TBGEN_1S, crt_core_id);

	e200_trace(E200_TRACE_MSG_DPD_RUN, (uint32_t)start_time);
	l1c_task_resume(L1C_DPD_TASK);
}

void vL1CDPDRun()
{
	vL1CDPDStart(1);
}

void vspa_irq_task_main(void *pvParameters)
{
	uint32_t ulNotifiedValue;
	uint32_t irq_no;
	uint8_t core;

	(void) pvParameters;

	e200_trace(E200_TRACE_MSG_IRQ_ENABLE, E200_TRACE_PARAM_TRACK);

	// will wait for interrupts from several VSPA cores
	irq_no = IRQ_INTERNAL_VSPA(DPDH_SRX_HS_CORE, IRQ_VSPA_GROUP_C);
	core = DPDH_SRX_HS_CORE;
	l1c_register_vspa_irq( (void *) tasks_map.tasks[L1C_DPD_IRQ_TASK].task_handle, core, irq_no);

	irq_no = IRQ_INTERNAL_VSPA(DPDH_SRX_HS_EXTRA_CORE, IRQ_VSPA_GROUP_C);
	core = DPDH_SRX_HS_EXTRA_CORE;
	l1c_register_vspa_irq( (void *) tasks_map.tasks[L1C_DPD_IRQ_TASK].task_handle, core, irq_no);

	irq_no = IRQ_INTERNAL_VSPA(DPDH_SRX_LS_CORE, IRQ_VSPA_GROUP_C);
	core = DPDH_SRX_LS_CORE;
	l1c_register_vspa_irq( (void *) tasks_map.tasks[L1C_DPD_IRQ_TASK].task_handle, core, irq_no);

	while (1)
	{
		/* Wait to be notified of an interrupt. */
		xTaskNotifyWait(0, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);

		/* relay to host */
		send_host_notification(irq_c, HOST_NOT_L1C_CAPTURE_READY, 0, 0);

		e200_trace(E200_TRACE_MSG_DPD_RUN, 0xcafeabea);
	}
}

void vL1CIRQ(l1c_irq_type_e irqCmd)
{
	switch(irqCmd)
	{
		case L1C_IRQ_ENABLE:
			l1c_create_task(L1_CORE_0, L1C_DPD_IRQ_TASK, "VSPA IRQ", TICK_DISABLE, L1C_LOW_PRIO, DEFAULT_STACK_SIZE, vspa_irq_task_main);
			break;
		case L1C_IRQ_DUMP:
			e200_trace(E200_TRACE_MSG_IRQ_DUMP, E200_TRACE_PARAM_TRACK);
			PRINTF("Number of IRQs counts: %d\r\n", irq_c);
			break;
		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
	}
}


/* get_tbgen_dcs_values_for_GeulB0() function supports only the TX on LS-DCS 
   and SRX on LS-DCS/HS-DCS. The implementation should be updated to suport HS-DCS on TX*/

void l1c_get_tbgen_dcs_values_for_GeulB0( dpd_vars_t* d)
{
	vuint32 * addr_PORSR1 = ( vuint32 * ) ( DCFG_BASE_ADDR + DCFG_PORSR1_OFFSET );
	uint32_t dcs_pll_reg_val = in_le32( addr_PORSR1 );
	dcs_pll_reg_val = ( dcs_pll_reg_val >> DCFG_PORSR1_DCS_PLL_START_BIT ) & DCFG_PORSR1_DCS_PLL_MASK;
	uint32_t interface = d->srx_interface_id;

#if ( DCS_PLATFORM == GEUL )
switch( dcs_pll_reg_val )
	{
		case DCS_PLL_CLK_0:  /* HS: 3932.16 MHz	LS: 983.04 MHz*/
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_983;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_3932 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_1:  /* HS: 3932.16 MHz	LS: 491.52 MHz*/
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_3932 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_2:  /* HS: 1966.08 MHz	LS: 983.04 MHz*/
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_983;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_1966 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_3:  /* HS: 1966.08 MHz	LS: 491.52 MHz*/
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_1966 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_6:  /* Geul_B0: HS:983.04 MHz  LS: 491.52 MHz  */
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_983 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_7:  /* Geul_B0: HS:983.04 MHz  LS: OFF         */
		{
			//d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_983 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_10: /* Geul_B0: HS:1966.08 MHz LS: 491.52 MHz  */
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_1966 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_11: /* Geul_B0: HS:1966.08 MHz LS: OFF         */
		{
			d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_1966 : SAMPLE_RATE_245;
			break;
		}
		case DCS_PLL_CLK_14: /* Geul_B0: HS:OFF         LS: 491.52 MHz  */
		case DCS_PLL_CLK_15: /* Geul_B0: HS:OFF         LS: OFF         */
		case DCS_PLL_CLK_4:  /* Not Valid */
		case DCS_PLL_CLK_5:  /* Not Valid */
		case DCS_PLL_CLK_8:  /* HS: 3520 MHz LS: Off */
		case DCS_PLL_CLK_9:  /* Not Valid */
		case DCS_PLL_CLK_12: /* Not Valid */
		case DCS_PLL_CLK_13: /* Geul_B0: Not Valid */
		default:
		{
			PRINTF("ERROR: %s DCS_PLL_CLK not supported for GeulB0!! \r\n",__func__);
			d->tx_dcs_sample_rate = SAMPLE_RATE_491;
			d->rx_dcs_sample_rate = (interface > LS_DCS1_IF1)? SAMPLE_RATE_1966 : SAMPLE_RATE_245;
		}
	}
#else
	PRINTF("ERROR: %s is not supported/implemented for this platform!! \r\n",__func__);
#endif
}

/* function used to return the DCS vs ifft/DPD sample rate compensation embedded in VSPA */

int32_t l1c_get_vspa_comp_for_GeulB0( dpd_vars_t* d, uint32_t rx_tx)
{
#if ( DCS_PLATFORM == GEUL )
	switch( rx_tx )
	{
		case 0:  /* TX */
		{
			return (d->tx_dcs_sample_rate - d->ifft_sample_rate);
		}
		case 1:  /* RX */
		{
			return (d->rx_dcs_sample_rate - d->fft_sample_rate);
		}
		default:
		{
			PRINTF("ERROR: %s VSPA compensation not supported for this DCS in GeulB0!! \r\n",__func__);
			return 0;
		}
	}
#else
	return 0;
#endif
}
