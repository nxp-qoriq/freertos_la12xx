// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 * 
 */
#include "FreeRTOS.h"
#include "tc_cpe_ipc.h"
#include "geul_cpe_ipc_api.h"
#include "ipc.h"
#include "Time.h"
#include "timers.h"

 /*MSG channel configuration*/
#define MSG_CH_DEPTH		4
#define MSG_SIZE_2K 		(1024 * 2)
#define MSG_SIZE_4K		192 /* QDMA compatible size */

/*Modem configuration*/
#define MODEM_SINGLE_CORE				0
#define MODEM_RX_CORE					1
#define MODEM_TX_CORE					2
#define RX_IPC_TEST_TASK				0
#define TX_IPC_TEST_TASK				1
#define MAX_IPC_TEST_TASKS				2
#define MAX_TEST_APP_GENERATE_RETRIES	5000
#define NUM_OF_TEST_MSGS	 			1
#define POISON							0x12345678
#define PERF_RX_CORE					1
#define PERF_TX_CORE					2
#define PERF_PCI_LOAD_CORE				3
#define MODEM_RX_INT_CORE				0
#define MODEM_TX_INT_CORE				1
#define MAX_CORE_COUNT				GEUL_E200_CORE_GLOBAL_NUM

/* Test status.*/
#define TC_IPC_SUCCESS					0
#define TC_IPC_FAIL					1

/*L1 packet macros. */
#define PERF_L1_RX_PACKETS				2000
#define PERF_L1_TX_PACKETS				2000

volatile uint8_t is_channed_initialized[MAX_CORE_COUNT] __attribute__ ((section (".hif"))) __attribute__ ((aligned (64)));

static void wait_for_cfg_done(void)
{
	volatile struct gul_hif *hif = bsp_get_hif();

	while (!(hif->mod_ready & HIF_MOD_READY_IPC_APP));
}

void vIpcIntrTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	/*Start Rx and Tx on respective cores.*/
	if (core_id == MODEM_RX_INT_CORE) {
		if (ipc_register_rx_int() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
        } else if (core_id == MODEM_TX_INT_CORE) {
		if (ipc_test_tx() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	}
}

void vIpcLatencyTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	if (core_id != 0) {
		if (ipc_latency_rx() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
        }
}

void vIpcPerfTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

        /*Start Rx and Tx on respective cores.*/
        if (core_id == PERF_RX_CORE)
        {
		if(ipc_perf_rx()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
        }
        else if(core_id == PERF_TX_CORE)
        {
		if(ipc_perf_tx()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
        }
        else if(core_id == PERF_PCI_LOAD_CORE)
        {
		//if(ipc_perf_load_pci()!= TC_IPC_SUCCESS) {
		//	RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		//}
		//else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
        }

	return;
}

void vIpcL1PerfTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

        /*Start Rx and Tx on respective cores.*/
        if (core_id == PERF_RX_CORE)
        {
		if(ipc_perf_l1_rx()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
        }
        else if(core_id == PERF_TX_CORE)
        {
		if(ipc_perf_l1_tx()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
        }
        else if(core_id == PERF_PCI_LOAD_CORE)
        {
		//if(ipc_perf_load_pci()!= TC_IPC_SUCCESS) {
		//	RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		//}
		//else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
        }

	return;
}


void vIpcL1PerfTest_8CC(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	/*Start Rx and Tx on respective cores.*/
	if (core_id == PERF_RX_CORE) {
		if (ipc_perf_l1_rx() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	} else if (core_id == PERF_TX_CORE) {
		if (ipc_perf_tx_l1_8CC() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	} else if (core_id == PERF_PCI_LOAD_CORE) {
		//if (ipc_perf_load_pci() != TC_IPC_SUCCESS)
		//	RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		//else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	}

	return;
}

void vIpcPerfTest_8CC(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	/*Start Rx and Tx on respective cores.*/
	if (core_id == PERF_RX_CORE) {
		if (ipc_perf_rx() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	} else if (core_id == PERF_TX_CORE) {
		if (ipc_perf_tx_8CC() != TC_IPC_SUCCESS)
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	} else if (core_id == PERF_PCI_LOAD_CORE) {
		//if (ipc_perf_load_pci() != TC_IPC_SUCCESS)
		//	RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		//else
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
	}

	return;
}
void vIpcSingleCoreTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	if (core_id == MODEM_SINGLE_CORE)
	{
		if(ipc_single_core_test()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
		}
        }

	return;
}

void vIpcTest(void)
{
	u8 core_id = (u8)ulMpicCurrentCore();

        /*Start Rx and Tx on respective cores.*/
        if (core_id == MODEM_RX_CORE)
        {
			if(ipc_test_rx()!= TC_IPC_SUCCESS) {
				RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
			}
			else {
				SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
			}
        }
        else if(core_id == MODEM_TX_CORE)
        {
			if(ipc_test_tx()!= TC_IPC_SUCCESS) {
				RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
			}
			else {
				SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST1_STATUS);
			}
        }

	return;
}

void vIpcAllCoreTest(void)
{

	u8 core_id = (u8)ulMpicCurrentCore();

	switch(core_id) {

	case 0:
		/* Core-0 not used for now for IPC.*/
		pr_debug("\n%s: IPC Test not running on core 0.\n",__func__);
		break;

	case 1:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 1 IPC channel configuration and transmission.*/
		if (ipc_core1_tx_rx_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 2:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 2 IPC channel configuration and transmission.*/
		if (ipc_core2_tx_rx_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 3:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 3 IPC channel configuration and transmission.*/
		if (ipc_core3_tx_rx_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	default:
		pr_err("\n%s: Invalid core id.\n",__func__);
		break;
	}


}

void vIpcSixCoreTest(void)
{

	u8 core_id = (u8)ulMpicCurrentCore();

	switch(core_id) {

	case 0:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 0 IPC channel configuration and transmission.*/
		if (ipc_tx_ptr_ch1_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 1:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 1 IPC channel configuration and transmission.*/
		if (ipc_tx_msg_ch4_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 2:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 2 IPC channel configuration and transmission.*/
		if (ipc_tx_msg_ch5_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 3:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 3 IPC channel configuration.*/
		if (ipc_rx_msg_ch3_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 4:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 4 IPC channel configuration.*/
		if (ipc_rx_msg_ch1_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	case 5:
		pr_debug("\n%s: IPC setup and launch on core:%d\n",__func__,core_id);
		/* Core 5 IPC channel configuration.*/
		if (ipc_rx_msg_ch2_loop()!= TC_IPC_SUCCESS) {
			RESET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		else {
			SET_TEST_STATUS(core_id, GEUL_DEMO_IPC_TEST2_STATUS);
		}
		break;

	default:
		pr_err("\n%s: Invalid core id.\n",__func__);
		break;
	}

}

int ipc_single_core_test(void)
{
	int				retval;

	pr_debug("\n%s: ==>\n", __func__);

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure the consumer channels
	   There are 3 MSG channels with L1 as the consumer.
	   L2_TO_L1_MSG_CH_1
	   L2_TO_L1_MSG_CH_2
	   L2_TO_L1_MSG_CH_3
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_1,MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_1 error %d \n",retval);
	}

	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_2,MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_2 error %d \n",retval);
	}

	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_3, MSG_CH_DEPTH, IPC_CH_PTR, MSG_SIZE_4K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_3 error %d \n",retval);
	}

	/* Wait for host ready.*/
	wait_for_host_ready();
	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

	ipc_single_core_rxtx();

	return TC_IPC_SUCCESS;
}

int ipc_test_rx(void)
{
	int				retval;

	pr_debug("\n%s: ==>\n", __func__);

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure the consumer channels
	   There are 3 MSG channels with L1 as the consumer.
	   L2_TO_L1_MSG_CH_1
	   L2_TO_L1_MSG_CH_2
	   L2_TO_L1_MSG_CH_3
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_1,MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_1 error %d \n",retval);
	}

	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_2,MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_2 error %d \n",retval);
	}

	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_3, MSG_CH_DEPTH,
				IPC_CH_PTR, MSG_SIZE_4K, 0, ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_3 error %d \n",retval);
	}

	/* Wait for host ready.*/
	wait_for_host_ready();
	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

	ipc_test_app_recv_msg();

	return TC_IPC_SUCCESS;
}

int ipc_test_tx(void)
{
	ipc_test_app_send_data();
	return TC_IPC_SUCCESS;
}

static void fill_test_buffer(void *buffer, size_t len)
{
	uint32_t i, count;
	uint32_t *val = NULL;

	memset(buffer, 0, len);
	val = (uint32_t *)buffer;
	count = len/sizeof(int);

	for (i = 0; i < count; i++) {
		ipc_m2h_32(POISON,val);
		val++;
	}
}

void ipc_xdump(unsigned char *buf, int32_t n)
{
	int32_t i;
	for(i=0;i<n;i++) {
		if (i%16 == 0)
			fsl_print("\n");

		fsl_print(" %x",buf[i]);
	}
	fsl_print("\n");
}


static int validate_buffer(void *buffer, size_t len)
{

	int ret = 0;
	uint32_t i, count;
	uint32_t *val = NULL;
	uint32_t data;

	val = (uint32_t *)buffer;
	count = len/sizeof(int);
	/* XXX Whatif len is not word aligned */
	for (i = 0; i < count; i++){
		data = ipc_h2m_32(val);
		if (data != POISON) {
			fsl_print("val =%x || POISON = %x / Count %d ", data, POISON, i);
			ret = 1; /* Failed */
			break;
		} else
			val++;
	}

	return ret;
}

void ipc_recv_chan_msg_ch1(ipc_t pvDevData)
{
	uint32_t len;
	int32_t error;
	void *l2_to_l1_buf;

	error = ipc_recv_msg_ptr(L2_TO_L1_MSG_CH_1, &l2_to_l1_buf, &len, pvDevData);

	if (IPC_SUCCESS == error)
		(void)ipc_set_consumed_status(L2_TO_L1_MSG_CH_1, pvDevData);
}

void ipc_recv_chan_msg_ch2(ipc_t pvDevData)
{
	uint32_t len;
	int32_t error;
	void *l2_to_l1_buf;

	error = ipc_recv_msg_ptr(L2_TO_L1_MSG_CH_2, &l2_to_l1_buf, &len, pvDevData);

	if (IPC_SUCCESS == error)
		(void) ipc_set_consumed_status(L2_TO_L1_MSG_CH_2, pvDevData);

}

void ipc_recv_chan_msg_ch3(ipc_t pvDevData)
{
	int32_t error;
	ipc_sh_buf_t rcv_sh_buf;

	error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, pvDevData);

	if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
		if (error != IPC_CH_EMPTY)
			pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
	} else {

#ifdef IPC_TEST_DEBUG
		void *buf_addr = (void *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
						+ (uint32_t)PEBM_BASE_ADDR);
		len = ipc_h2m_32(&(rcv_sh_buf.data_size));
		fsl_print("\n L2_TO_L1_MSG_CH_3 content\n ");
		ipc_xdump((char *)buf_addr, len);
		if (validate_buffer(buf_addr, len))
			pr_debug("\n====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_3 / data_size %d \n", len);
#endif
		ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, pvDevData);
	}

}

void ipc_rx_chan_loop(void (*chan1_loop)(ipc_t), void (*chan2_loop)(ipc_t),
		      void (*chan3_loop)(ipc_t), ipc_t ipc_handle)
{

	do {
		if (chan1_loop)
			chan1_loop(ipc_handle);
	
		if (chan2_loop)
			chan2_loop(ipc_handle);
	
		if (chan3_loop)
			chan3_loop(ipc_handle);

	} while (1);

}

int ipc_register_rx_int(void)
{
	int	 retval;
	void (*chan1_loop)(ipc_t) = 0;
	void (*chan2_loop)(ipc_t) = 0;
	void (*chan3_loop)(ipc_t) = 0;
	ipc_instance_t *ipc_instance = ipc_handle;

	pr_debug("\n%s: ==>\n", __func__);

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure the consumer channels
	 * There are 3 MSG channels with L1 as the consumer.
	 * L2_TO_L1_MSG_CH_1
	 * L2_TO_L1_MSG_CH_2
	 * L2_TO_L1_MSG_CH_3
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_1, MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS)
		pr_err("\n\n\n###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_1 error %d \n", retval);

	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_2, MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS)
		pr_err("\n\n\n###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_2 error %d \n", retval);

	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_3, MSG_CH_DEPTH, IPC_CH_PTR, MSG_SIZE_4K,
			0, ipc_handle);

	if (retval != IPC_SUCCESS)
		pr_err("\n\n\n###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_3 error %d \n", retval);

	/* Wait for host ready.*/
	wait_for_host_ready();

	/* There are multiple ways to configure interrupt based channel:
	 * 1) Either ipc_configure_channel() needs to be configured with en_event parameter.
	 * 2) Host can set msi_valid property of channel.
	 * Current testcase demonstrates 2).
	 */
	if (ipc_h2m_32(&(ipc_instance->ch_list[L2_TO_L1_MSG_CH_1].msi_valid))) {
		if (ipc_register_channel_callback(L2_TO_L1_MSG_CH_1, ipc_recv_chan_msg_ch1, ipc_handle,
						&(ipc_instance->ch_list[L2_TO_L1_MSG_CH_1]) ) != IPC_SUCCESS)
			return TC_IPC_FAIL;
	} else {
		chan1_loop = ipc_recv_chan_msg_ch1;
	}

	if (ipc_h2m_32(&(ipc_instance->ch_list[L2_TO_L1_MSG_CH_2].msi_valid))) {
		if (ipc_register_channel_callback(L2_TO_L1_MSG_CH_2, ipc_recv_chan_msg_ch2, ipc_handle,
						&(ipc_instance->ch_list[L2_TO_L1_MSG_CH_2])) != IPC_SUCCESS)
			return TC_IPC_FAIL;
	} else {
		chan2_loop = ipc_recv_chan_msg_ch2;
	}

	if (ipc_h2m_32(&(ipc_instance->ch_list[L2_TO_L1_MSG_CH_3].msi_valid))) {
		if (ipc_register_channel_callback(L2_TO_L1_MSG_CH_3, ipc_recv_chan_msg_ch3, ipc_handle,
						&(ipc_instance->ch_list[L2_TO_L1_MSG_CH_3])) != IPC_SUCCESS)
			return TC_IPC_FAIL;
	} else {
		chan3_loop = ipc_recv_chan_msg_ch3;
	}

	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

	ipc_rx_chan_loop(chan1_loop, chan2_loop, chan3_loop, ipc_handle);

	return TC_IPC_SUCCESS;

}

void ipc_single_core_rxtx(void)
{
	uint32_t len, count=0;
	int32_t error;
	int     send_error, index;
	uint32_t msg_len=1024 *2;
	ipc_sh_buf_t *sh_buf_1, *sh_buf_2, rcv_sh_buf;
	mod_mem_region_t *huge_page;

	/* Buffer to be used by consumer message channels */
	void *msg_2k_buf;

	pr_debug("\n %s running.... \n",__func__);

	do {
		count++;
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg_ptr..count =%d , error = %d\n",
				count, error);
#endif

		error = ipc_recv_msg_ptr(L2_TO_L1_MSG_CH_1, &msg_2k_buf, &len, ipc_handle);
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg_ptr.on L2_TO_L1_MSG_CH_1.count =%d , error = %d\n",
				count, error);
#endif

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg_ptr on L2_TO_L1_MSG_CH_1  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_1 content\n ");
			ipc_xdump(msg_2k_buf, len);
#endif
			if (validate_buffer(msg_2k_buf, len)) {
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1 \n");
			}
			ipc_set_consumed_status(L2_TO_L1_MSG_CH_1, ipc_handle);
		}

		/*Start receiving the message on channels*/
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg_ptr. on L2_TO_L1_MSG_CH_2 \n");
#endif
		error = ipc_recv_msg_ptr(L2_TO_L1_MSG_CH_2, &msg_2k_buf, &len, ipc_handle);

#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg_ptr. on L2_TO_L1_MSG_CH_2 .count =%d , error = %d\n",
				count, error);
#endif
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg_ptr on L2_TO_L1_MSG_CH_2  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_2 content\n");
			ipc_xdump(msg_2k_buf, len);
#endif
			if (validate_buffer(msg_2k_buf, len))
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_2  \n");
			ipc_set_consumed_status(L2_TO_L1_MSG_CH_2, ipc_handle);
		}

#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_ptr. on L2_TO_L1_MSG_CH_3 \n");
#endif
		error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_ptr. on L2_TO_L1_MSG_CH_3.count =%d , error = %d\n",
			count, error);
#endif
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			if (error != IPC_CH_EMPTY)
				pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
		} else {
			void *buf_addr = (void *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
							+ (uint32_t)PEBM_BASE_ADDR);
			len = ipc_h2m_32(&(rcv_sh_buf.data_size));
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_3 content\n ");
			ipc_xdump((char *)buf_addr, len);
#endif
			if (validate_buffer(buf_addr, len))
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_3 / data_size %d \n", len);
			ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);
		}

		error = ipc_get_msg_ptr(L1_TO_L2_MSG_CH_4, ipc_handle, &msg_2k_buf);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_get_msg_ptr on L1_TO_L2_MSG_CH_4,  (error=%d)!\n", error);
#endif
		} else {
			fill_test_buffer(msg_2k_buf, msg_len);
			error =  ipc_send_msg_ptr(L1_TO_L2_MSG_CH_4, msg_len, ipc_handle);
			if (IPC_SUCCESS != error)
				pr_err(" ERROR  Failed ipc_send_msg_ptr on L1_TO_L2_MSG_CH_4, (error=%d)!\n", error);
		}

		error = ipc_get_msg_ptr(L1_TO_L2_MSG_CH_5, ipc_handle, &msg_2k_buf);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_get_msg_ptr on L1_TO_L2_MSG_CH_5,  (error=%d)!\n", error);
#endif
		} else {
			fill_test_buffer(msg_2k_buf, msg_len);
			error =  ipc_send_msg_ptr(L1_TO_L2_MSG_CH_5, msg_len, ipc_handle);
			if (IPC_SUCCESS != error)
				pr_err(" ERROR  Failed ipc_send_msg_ptr on L1_TO_L2_MSG_CH_5, (error=%d)!\n", error);
		}

		/*Get the buffer and transmit it on PTR_CH_1 */
		sh_buf_1 = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &send_error);
		if (sh_buf_1 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_1,  (error=%d)!\n", send_error);
#endif
		}
		else
		{
			huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
			fsl_print("\n ===> sh_buf_1->mod_phys =%x \n",ipc_h2m_32(&sh_buf_1->mod_phys));
			fsl_print("\n ===>huge_page->addr_v =%x \n",huge_page->addr_v);
#endif

			for (index = 0; index < 64; index++) {
				memcpy((void *)((ipc_h2m_32(&sh_buf_1->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2)) ), msg_2k_buf, (1024 * 2));
			}
#ifdef IPC_TEST_DEBUG
			pr_debug("%s: sh_buf_1->mod_phys[0x%08x]\n",__func__, ipc_h2m_32(&sh_buf_1->mod_phys));
#endif
			ipc_m2h_32(1024 * 128, &(sh_buf_1->buf_size));
			ipc_m2h_32(1024 * 128, &(sh_buf_1->data_size));

			error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf_1, ipc_handle);
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			}
		}

		sh_buf_2 = ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &send_error);
		if (sh_buf_2 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_2,  (error=%d)!\n", send_error);
#endif
		}
		else
		{
			huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
			fsl_print("\n ===> sh_buf_2->mod_phys =%x \n", ipc_h2m_32(&sh_buf_2->mod_phys));
			fsl_print("\n ===>huge_page->addr_v =%x \n", huge_page->addr_v);
#endif
			//memcpy((void *)((ipc_h2m_32(&sh_buf_2->mod_phys)) + huge_page->addr_v), &buff2, (1024 * 16));

			for(index=0; index < 64; index++){
				memcpy((void *)((ipc_h2m_32(&sh_buf_2->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2))), msg_2k_buf, (1024 * 2));
			}
#ifdef IPC_TEST_DEBUG
			pr_debug("%s: sh_buf_2->mod_phys[0x%08x]\n", __func__, ipc_h2m_32(&sh_buf_2->mod_phys));
#endif
			ipc_m2h_32(1024 * 128, &(sh_buf_2->buf_size));
			ipc_m2h_32(1024 * 128, &(sh_buf_2->data_size));

			error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf_2, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			}
		}
	}while (1);

	vGeulFree(msg_2k_buf);
	return;
}

void ipc_test_app_recv_msg(void)
{
	uint32_t len, count=0;
	int32_t error;

	/*Buffer to be used by consumer message channels*/
	uint8_t *msg_2k_buf   = pvGeulMalloc(1024*2);
	ipc_sh_buf_t rcv_sh_buf;

	if (!msg_2k_buf) {
		pr_err("Cannot get memory for rx buffer\r\n");
		return;
	}

	pr_debug("\n ipc_test_app_recv_msg running.... \n");

	do {
		count++;
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg..count =%d , error = %d\n",
				count, error);
#endif

		error = ipc_recv_msg(L2_TO_L1_MSG_CH_1, msg_2k_buf, &len, ipc_handle);
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg.on L2_TO_L1_MSG_CH_1.count =%d , error = %d\n",
				count, error);
#endif

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg on L2_TO_L1_MSG_CH_1  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_1 content\n ");
			ipc_xdump(msg_2k_buf, len);
#endif
			if (validate_buffer(msg_2k_buf, len)) {
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1 \n");
			}
		}

		/*Start receiving the message on channels*/
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg. on L2_TO_L1_MSG_CH_2 \n");
#endif
		error = ipc_recv_msg(L2_TO_L1_MSG_CH_2, msg_2k_buf, &len, ipc_handle);

#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_msg. on L2_TO_L1_MSG_CH_2 .count =%d , error = %d\n",
				count, error);
#endif
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg on L2_TO_L1_MSG_CH_2  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_2 content\n");
			ipc_xdump(msg_2k_buf, len);
#endif
			if (validate_buffer(msg_2k_buf, len))
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_2  \n");
		}

#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_ptr. on L2_TO_L1_MSG_CH_3 \n");
#endif
		error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_ptr. on L2_TO_L1_MSG_CH_3.count =%d , error = %d\n",
			count, error);
#endif
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			if (error != IPC_CH_EMPTY)
				pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
		} else {
			void *buf_addr = (void *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
							+ (uint32_t)PEBM_BASE_ADDR);
			len = ipc_h2m_32(&(rcv_sh_buf.data_size));
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_3 content\n ");
			ipc_xdump((char *)buf_addr, len);
#endif
			if (validate_buffer(buf_addr, len))
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_3 / data_size %d \n", len);
			ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);
		}

	} while (1);

	vGeulFree(msg_2k_buf);
	return;
}

void ipc_test_app_send_data(void)
{

	uint32_t msg_len=1024 *2;
	int error, index;
	mod_mem_region_t * huge_page;
	ipc_sh_buf_t* sh_buf_1, *sh_buf_2;
	uint8_t * buff1		= pvGeulMalloc(1024*2);

	if (!buff1) {
		pr_err("Cannot get memory for tx buffer\r\n");
		return;
	}

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);
	/* Wait for host ready.*/
	wait_for_host_ready();
	/* Wait for Configuration to be completed */
	wait_for_cfg_done();

	pr_debug("ipc_test_app_send_data running ....\n");

	do {
repeat:
		/*Start sending the message on channels*/
		fill_test_buffer(buff1,(1024*2));

		error =  ipc_send_msg(L1_TO_L2_MSG_CH_4, buff1, msg_len, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_msg on L1_TO_L2_MSG_CH_4,  (error=%d)!\n", error);
#endif
		}

		error =  ipc_send_msg(L1_TO_L2_MSG_CH_5, buff1, msg_len, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_msg on L1_TO_L2_MSG_CH_5,  (error=%d)!\n", error);
#endif
		}

		/*Get the buffer and transmit it on PTR_CH_1 */
		sh_buf_1 = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
		if (sh_buf_1 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			/*Do not attempt to send this ptr channel instead try another */
			goto next_ptr;
		}

		huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
		fsl_print("\n ===> sh_buf_1->mod_phys =%x \n",ipc_h2m_32(&sh_buf_1->mod_phys));
		fsl_print("\n ===>huge_page->addr_v =%x \n",huge_page->addr_v);
#endif

		for (index = 0; index < 64; index++) {
			memcpy((void *)((ipc_h2m_32(&sh_buf_1->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2)) ), buff1, (1024 * 2));
		}
#ifdef IPC_TEST_DEBUG
		pr_debug("%s: sh_buf_1->mod_phys[0x%08x]\n",__func__, ipc_h2m_32(&sh_buf_1->mod_phys));
#endif
		ipc_m2h_32(1024 * 128, &(sh_buf_1->buf_size));
		ipc_m2h_32(1024 * 128, &(sh_buf_1->data_size));

		error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf_1, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
		}

next_ptr:
		/*Get the buffer and transmit it on PTR_CH_2 */
		sh_buf_2 = ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &error);
		if (sh_buf_2 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
			/*Do not attempt to send the ptr. Loop back to msg channel*/
			goto repeat;
		}

		huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
		fsl_print("\n ===> sh_buf_2->mod_phys =%x \n", ipc_h2m_32(&sh_buf_2->mod_phys));
		fsl_print("\n ===>huge_page->addr_v =%x \n", huge_page->addr_v);
#endif
		//memcpy((void *)((ipc_h2m_32(&sh_buf_2->mod_phys)) + huge_page->addr_v), &buff2, (1024 * 16));

		for(index=0; index < 64; index++){
			memcpy((void *)((ipc_h2m_32(&sh_buf_2->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2))), buff1, (1024 * 2));
		}
#ifdef IPC_TEST_DEBUG
		pr_debug("%s: sh_buf_2->mod_phys[0x%08x]\n", __func__, ipc_h2m_32(&sh_buf_2->mod_phys));
#endif
		ipc_m2h_32(1024 * 128, &(sh_buf_2->buf_size));
		ipc_m2h_32(1024 * 128, &(sh_buf_2->data_size));

		error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf_2, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
		}

	} while(1);

	vGeulFree(buff1);
	return;
}

int ipc_rx_msg_ch1_loop(void)
{
        int retval, error;
	uint32_t len;
	uint8_t *msg_2k_recv_buf = pvGeulMalloc(1024*2);

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure consumer channel for this core.
	 * L2_TO_L1_MSG_CH_1
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_1, MSG_CH_DEPTH,
			IPC_CH_MSG, MSG_SIZE_2K, 0, (ipc_t)ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel failed for L2_TO_L1_MSG_CH_1 error %d \n",
				retval);
		vGeulFree(msg_2k_recv_buf);
		return TC_IPC_FAIL;
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	/* Mark channel as initialized by core */
	is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

	if (!msg_2k_recv_buf) {
		pr_err("Cannot get memory for rx test\r\n");
		vGeulFree(msg_2k_recv_buf);
		return TC_IPC_FAIL;
	}

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Receive on L2_TO_L1_MSG_CH_1 and send on L1_TO_L2_MSG_CH_4.*/
		error = ipc_recv_msg(L2_TO_L1_MSG_CH_1, msg_2k_recv_buf, &len, ipc_handle);

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg on L2_TO_L1_MSG_CH_1  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_1 content\n ");
			ipc_xdump(msg_2k_recv_buf, len);
#endif
			if (validate_buffer(msg_2k_recv_buf, len)) {
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1 \n");
			}
		}
	} while(1);

	return TC_IPC_SUCCESS;
}

int ipc_tx_msg_ch4_loop(void)
{
	int error;
	uint32_t msg_len = 1024 *2;
	uint8_t *msg_2k_send_buf = pvGeulMalloc(1024*2);

	wait_for_ipc_init(ipc_handle);
	wait_for_host_ready();
	wait_for_cfg_done();

	if (!msg_2k_send_buf) {
		pr_err("Cannot get memory for tx test\r\n");
		vGeulFree(msg_2k_send_buf);
		return TC_IPC_FAIL;
	}

	fill_test_buffer(msg_2k_send_buf,(1024*2));

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Send on L1_TO_L2_MSG_CH_4 */
		error =  ipc_send_msg(L1_TO_L2_MSG_CH_4, msg_2k_send_buf, msg_len, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_msg on L1_TO_L2_MSG_CH_4,  (error=%d)!\n", error);
#endif
		}
	} while(1);

	return TC_IPC_SUCCESS;
}

int ipc_rx_msg_ch2_loop(void)
{
        int retval, error;
	uint32_t len;
	uint8_t *msg_2k_recv_buf = pvGeulMalloc(1024*2);

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure consumer channel for this core.
	 * L2_TO_L1_MSG_CH_2
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_2, MSG_CH_DEPTH,
			IPC_CH_MSG, MSG_SIZE_2K, 0, (ipc_t)ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel failed for L2_TO_L1_MSG_CH_2 error %d \n",
				retval);
		vGeulFree(msg_2k_recv_buf);
		return TC_IPC_FAIL;
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	/* Mark channel as initialized by core */
	is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

	if (!msg_2k_recv_buf) {
		pr_err("Cannot get memory for rx test\r\n");
		vGeulFree(msg_2k_recv_buf);
		return TC_IPC_FAIL;
	}

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Receive on L2_TO_L1_MSG_CH_2 */
		error = ipc_recv_msg(L2_TO_L1_MSG_CH_2, msg_2k_recv_buf, &len, ipc_handle);

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg on L2_TO_L1_MSG_CH_2  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_2 content\n ");
			ipc_xdump(msg_2k_recv_buf, len);
#endif
			if (validate_buffer(msg_2k_recv_buf, len)) {
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_2 \n");
			}
		}
	} while(1);

	return TC_IPC_SUCCESS;
}

int ipc_tx_msg_ch5_loop(void)
{
	int error;
	uint32_t msg_len = 1024 *2;
	uint8_t *msg_2k_send_buf = pvGeulMalloc(1024*2);

	wait_for_ipc_init(ipc_handle);
	wait_for_host_ready();
	wait_for_cfg_done();

	if (!msg_2k_send_buf) {
		pr_err("Cannot get memory for tx test\r\n");
		vGeulFree(msg_2k_send_buf);
		return TC_IPC_FAIL;
	}

	fill_test_buffer(msg_2k_send_buf,(1024*2));

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Send on L1_TO_L2_MSG_CH_5 */
		error =  ipc_send_msg(L1_TO_L2_MSG_CH_5, msg_2k_send_buf, msg_len, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_msg on L1_TO_L2_MSG_CH_5,  (error=%d)!\n", error);
#endif
		}
	} while(1);

	return TC_IPC_SUCCESS;
}

int ipc_rx_msg_ch3_loop(void)
{
	int retval, error;
	uint32_t len;
	ipc_sh_buf_t rcv_sh_buf;

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure consumer channel for this core.
	   L2_TO_L1_MSG_CH_3
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_3, MSG_CH_DEPTH,
			IPC_CH_PTR, MSG_SIZE_4K, 0, (ipc_t)ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel failed for L2_TO_L1_MSG_CH_3 error %d \n",retval);
		return TC_IPC_FAIL;
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	/* Mark channel as initialized by core */
	is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

	/* Wait for other channels to finish configuration.*/
	while(!(is_channed_initialized[4] && is_channed_initialized[5]));

	/* Set modem ready to start Rx/Tx*/
	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Receive on L2_TO_L1_MSG_CH_3 */
		error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
		} else {
			void *buf_addr = (void *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
							+ (uint32_t)PEBM_BASE_ADDR);
			len = ipc_h2m_32(&(rcv_sh_buf.data_size));
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_3 content\n ");
			ipc_xdump((char *)buf_addr, len);
#endif
			if (validate_buffer(buf_addr, len))
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_3 / data_size %d \n", len);
			ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);
		}

	} while(1);

	return TC_IPC_SUCCESS;
}

int ipc_tx_ptr_ch1_loop(void)
{
	int error, index;
	uint8_t *msg_2k_send_buf = pvGeulMalloc(1024*2);
	ipc_sh_buf_t *sh_buf;
	mod_mem_region_t *huge_page;

	wait_for_ipc_init(ipc_handle);
	wait_for_host_ready();
	wait_for_cfg_done();

	if ((!msg_2k_send_buf)) {
		pr_err("Cannot get memory for rx/tx test\r\n");
		return TC_IPC_FAIL;
	}

	fill_test_buffer(msg_2k_send_buf,(1024*2));

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/*Get the buffer and transmit it on PTR_CH_1 */
		sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
		if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
		} else {
			huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
			fsl_print("\n ===> sh_buf->mod_phys =%x \n",ipc_h2m_32(&sh_buf->mod_phys));
			fsl_print("\n ===>huge_page->addr_v =%x \n",huge_page->addr_v);
#endif

			for (index = 0; index < 64; index++)
				memcpy((void *)((ipc_h2m_32(&sh_buf->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2)) ), msg_2k_send_buf, (1024 * 2));

#ifdef IPC_TEST_DEBUG
			pr_debug("%s: sh_buf->mod_phys[0x%08x]\n",__func__, ipc_h2m_32(&sh_buf->mod_phys));
#endif
			ipc_m2h_32(1024 * 128, &(sh_buf->buf_size));
			ipc_m2h_32(1024 * 128, &(sh_buf->data_size));

			error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf, ipc_handle);
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			}
		}

	} while(1);
}

int ipc_core1_tx_rx_loop(void)
{
	int retval;

	/* Channel configuration.*/
	retval = ipc_core1_init(ipc_handle);
	if (retval!= TC_IPC_SUCCESS) {
		pr_err("%s: IPC Init failed\n", __func__);
		return TC_IPC_FAIL;
	}

	/* Loop for rx/tx.*/
	ipc_core1_loop(ipc_handle);
	return TC_IPC_SUCCESS;
}

int ipc_core1_init(ipc_instance_t * ipc_handle)
{
	int	retval;

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure consumer channel for this core.
	   L2_TO_L1_MSG_CH_1
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_1,MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, (ipc_t)ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel failed for L2_TO_L1_MSG_CH_1 error %d \n",retval);
		return TC_IPC_FAIL;
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	return TC_IPC_SUCCESS;

}
void ipc_core1_loop(ipc_instance_t * ipc_handle)
{
	int32_t  error;
	uint32_t len;
	uint32_t msg_len			= 1024 *2;
	uint8_t *msg_2k_send_buf	= pvGeulMalloc(1024*2);
	uint8_t *msg_2k_recv_buf	= pvGeulMalloc(1024*2);

	if ((!msg_2k_send_buf)||(!msg_2k_recv_buf)) {
		pr_err("Cannot get memory for rx/tx test\r\n");
		if (msg_2k_send_buf)
			vGeulFree(msg_2k_send_buf);
		if (msg_2k_recv_buf)
			vGeulFree(msg_2k_recv_buf);
		return;
	}

	fill_test_buffer(msg_2k_send_buf,(1024*2));

	/* Mark channel as initialized by core */
	is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

	/* Wait for other channels to finish configuration */
	while(!(is_channed_initialized[2] && is_channed_initialized[3]));

	/* Set modem ready to start Rx/Tx */
	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Receive on L2_TO_L1_MSG_CH_1 and send on L1_TO_L2_MSG_CH_4.*/
		error = ipc_recv_msg(L2_TO_L1_MSG_CH_1, msg_2k_recv_buf, &len, ipc_handle);

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg on L2_TO_L1_MSG_CH_1  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_1 content\n ");
			ipc_xdump(msg_2k_recv_buf, len);
#endif
			if (validate_buffer(msg_2k_recv_buf, len)) {
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1 \n");
			}
		}

		error =  ipc_send_msg(L1_TO_L2_MSG_CH_4, msg_2k_send_buf, msg_len, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_msg on L1_TO_L2_MSG_CH_4,  (error=%d)!\n", error);
#endif
		}

	}while(1);

}

int ipc_core2_tx_rx_loop(void)
{
	int retval;

	/* Channel configuration.*/
	retval = ipc_core2_init(ipc_handle);
	if (retval!= TC_IPC_SUCCESS) {
		pr_err("%s: IPC Init failed.\n", __func__);
		return TC_IPC_FAIL;
	}

	/* Loop for rx/tx.*/
	ipc_core2_loop(ipc_handle);
	return TC_IPC_SUCCESS;
}

int ipc_core2_init(ipc_instance_t * ipc_handle)
{
	int	retval;

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure consumer channel for this core.
	   L2_TO_L1_MSG_CH_2
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_2,MSG_CH_DEPTH, IPC_CH_MSG, MSG_SIZE_2K,
			0, (ipc_t)ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_2 error %d \n",retval);
		return TC_IPC_FAIL;
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	return TC_IPC_SUCCESS;

}
void ipc_core2_loop(ipc_instance_t * ipc_handle)
{
	int      error;
	uint32_t len;
	uint32_t msg_len			= 1024 *2;	
	uint8_t *msg_2k_send_buf	= pvGeulMalloc(1024*2);
	uint8_t *msg_2k_recv_buf	= pvGeulMalloc(1024*2);

	if ((!msg_2k_send_buf)||(!msg_2k_recv_buf)) {
		pr_err("Cannot get memory for rx/tx test\r\n");
		if (msg_2k_send_buf)
			vGeulFree(msg_2k_send_buf);
		if (msg_2k_recv_buf)
			vGeulFree(msg_2k_recv_buf);
		return;
	}

	fill_test_buffer(msg_2k_send_buf,(1024*2));

	/* Mark channel as initialized by core */
	is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

	wait_for_cfg_done();

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Receive on L2_TO_L1_MSG_CH_2 and send on L1_TO_L2_MSG_CH_5.*/
		error = ipc_recv_msg(L2_TO_L1_MSG_CH_2, msg_2k_recv_buf, &len, ipc_handle);

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg on L2_TO_L1_MSG_CH_2  (error=%d)!\n", error);
#endif
		} else {
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_1 content\n ");
			ipc_xdump(msg_2k_recv_buf, len);
#endif
			if (validate_buffer(msg_2k_recv_buf, len)) {
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1 \n");
			}
		}

		error =  ipc_send_msg(L1_TO_L2_MSG_CH_5, msg_2k_send_buf, msg_len, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_msg on L1_TO_L2_MSG_CH_5,  (error=%d)!\n", error);
#endif
		}

	}while(1);

}

int ipc_core3_tx_rx_loop(void)
{
	int retval;

	/* Channel configuration.*/
	retval = ipc_core3_init(ipc_handle);
	if (retval != TC_IPC_SUCCESS) {
		pr_err("%s: IPC Init failed.\n", __func__);
		return TC_IPC_FAIL;
	}

	/* Loop for rx/tx.*/
	ipc_core3_loop(ipc_handle);
	return TC_IPC_SUCCESS;
}
int ipc_core3_init(ipc_instance_t * ipc_handle)
{
	int	retval;

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	/* Configure consumer channel for this core.
	   L2_TO_L1_MSG_CH_3
	 */
	retval = ipc_configure_channel(L2_TO_L1_MSG_CH_3, MSG_CH_DEPTH,
			IPC_CH_PTR, MSG_SIZE_4K, 0, (ipc_t)ipc_handle);

	if (retval != IPC_SUCCESS) {
		pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_3 error %d \n",retval);
		return TC_IPC_FAIL;
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	return TC_IPC_SUCCESS;

}
void ipc_core3_loop(ipc_instance_t * ipc_handle)
{
	int error,index;
	uint32_t   len;
	ipc_sh_buf_t *		sh_buf_1,*sh_buf_2;
	mod_mem_region_t *	huge_page;
	uint8_t *msg_2k_send_buf	= pvGeulMalloc(1024*2);
	ipc_sh_buf_t rcv_sh_buf;

	if ((!msg_2k_send_buf)) {
		pr_err("Cannot get memory for rx/tx test\r\n");
		return;
	}

	fill_test_buffer(msg_2k_send_buf,(1024*2));

	/* Mark channel as initialized by core */
	is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

	wait_for_cfg_done();

	pr_debug("\n ipc_test_app running.... \n");

	do {
		/* Core loop: Receive on L2_TO_L1_MSG_CH_3 and send on L1_TO_L2_PRT_CH_1 and L1_TO_L2_PRT_CH_2.*/
		error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_msg_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
		} else {
			void *buf_addr = (void *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
							+ (uint32_t)PEBM_BASE_ADDR);
			len = ipc_h2m_32(&(rcv_sh_buf.data_size));
#ifdef IPC_TEST_DEBUG
			fsl_print("\n L2_TO_L1_MSG_CH_3 content\n ");
			ipc_xdump((char *)buf_addr, len);
#endif
			if (validate_buffer(buf_addr, len))
				pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_3 / data_size %d \n", len);
			ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);
		}

		/*Get the buffer and transmit it on PTR_CH_1 */
		sh_buf_1 = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
		if (sh_buf_1 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
		}
		else
		{
			huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
			fsl_print("\n ===> sh_buf_1->mod_phys =%x \n",ipc_h2m_32(&sh_buf_1->mod_phys));
			fsl_print("\n ===>huge_page->addr_v =%x \n",huge_page->addr_v);
#endif

			for (index = 0; index < 64; index++) {
				memcpy((void *)((ipc_h2m_32(&sh_buf_1->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2)) ), msg_2k_send_buf, (1024 * 2));
			}
#ifdef IPC_TEST_DEBUG
			pr_debug("%s: sh_buf_1->mod_phys[0x%08x]\n",__func__, ipc_h2m_32(&sh_buf_1->mod_phys));
#endif
			ipc_m2h_32(1024 * 128, &(sh_buf_1->buf_size));
			ipc_m2h_32(1024 * 128, &(sh_buf_1->data_size));

			error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf_1, ipc_handle);
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			}
		}

		/*Get the buffer and transmit it on PTR_CH_2 */
		sh_buf_2= ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &error);
		if (sh_buf_2 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
		}
		else
		{
			huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
			fsl_print("\n ===> sh_buf_2->mod_phys =%x \n",ipc_h2m_32(&sh_buf_2->mod_phys));
			fsl_print("\n ===>huge_page->addr_v =%x \n",huge_page->addr_v);
#endif

			for (index = 0; index < 64; index++) {
				memcpy((void *)((ipc_h2m_32(&sh_buf_2->mod_phys)) + huge_page->addr_v + (uint32_t)(index * (1024 * 2)) ), msg_2k_send_buf, (1024 * 2));
			}
#ifdef IPC_TEST_DEBUG
			pr_debug("%s: sh_buf_2->mod_phys[0x%08x]\n",__func__, ipc_h2m_32(&sh_buf_2->mod_phys));
#endif
			ipc_m2h_32(1024 * 128, &(sh_buf_2->buf_size));
			ipc_m2h_32(1024 * 128, &(sh_buf_2->data_size));

			error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf_2, ipc_handle);
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_ptr on L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
			}
		}

	}while(1);

}

int ipc_latency_rx(void)
{
	int retval;

	/* Channel configuration.*/
	retval = ipc_perf_rx_init(ipc_handle);
	if (retval!= TC_IPC_SUCCESS) {
		pr_err("%s: IPC Init failed\n", __func__);
		return TC_IPC_FAIL;
	}

	/* Loop for rx.*/
	ipc_latency_loopback(ipc_handle);
	return TC_IPC_SUCCESS;
}

int ipc_perf_rx(void)
{
	int retval;

	/* Channel configuration.*/
	retval = ipc_perf_rx_init(ipc_handle);
	if (retval!= TC_IPC_SUCCESS) {
		pr_err("%s: IPC Init failed\n", __func__);
		return TC_IPC_FAIL;
	}

	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

	/* Loop for rx.*/
	ipc_perf_rx_loop(ipc_handle);
	return TC_IPC_SUCCESS;
}

int ipc_perf_l1_rx(void)
{
	int retval;

	/* Channel configuration.*/
	retval = ipc_perf_rx_init(ipc_handle);
	if (retval!= TC_IPC_SUCCESS) {
		pr_err("%s: IPC Init failed\n", __func__);
		return TC_IPC_FAIL;
	}

	SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);
	/* Loop for rx.*/
	ipc_l1_rxloop(ipc_handle);
	return TC_IPC_SUCCESS;
}

int ipc_perf_rx_init(ipc_instance_t * ipc_handle)
{
	int	retval;
	u8 core_id = (u8)ulMpicCurrentCore();

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);

	if (core_id == 1) {
		/* Configure consumer channel for this core.
		 * L2_TO_L1_MSG_CH_3
		 */
		retval = ipc_configure_channel(L2_TO_L1_MSG_CH_3, MSG_CH_DEPTH,
				IPC_CH_PTR, MSG_SIZE_4K, 0, (ipc_t)ipc_handle);

		if (retval != IPC_SUCCESS) {
			pr_err("\n\n\n ###### ipc_configure_channel failed for L2_TO_L1_MSG_CH_3 error %d\n",
					retval);
			return TC_IPC_FAIL;
		}
	} else if (core_id == 2) {
		/* Configure consumer channel for this core.
		 * L2_TO_L1_MSG_CH_2
		 */
		retval = ipc_configure_channel(L2_TO_L1_MSG_CH_2, MSG_CH_DEPTH,
					IPC_CH_MSG, MSG_SIZE_2K, 0, ipc_handle);
		if (retval != IPC_SUCCESS) {
			pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_2 error %d\n",
					retval);
			return TC_IPC_FAIL;
		}
	} else if (core_id == 3) {
		/* Configure consumer channel for this core.
		 * L2_TO_L1_MSG_CH_1
		 */
		retval = ipc_configure_channel(L2_TO_L1_MSG_CH_1, MSG_CH_DEPTH,
					IPC_CH_MSG, MSG_SIZE_2K, 0, ipc_handle);
		if (retval != IPC_SUCCESS) {
			pr_err("\n\n\n ###### ipc_configure_channel filed for L2_TO_L1_MSG_CH_2 error %d\n",
					retval);
			return TC_IPC_FAIL;
		}
	}

	/* Wait for host ready.*/
	wait_for_host_ready();

	return TC_IPC_SUCCESS;

}

void ipc_l1_rxloop(ipc_instance_t * ipc_handle)
{
	int32_t  error;
	uint32_t messages_received = 0;
	ipc_sh_buf_t rcv_sh_buf;
	struct Time ipc_start;
	struct Time meas_start;
	uint32_t TimeDiffInUs = 0;
	uint32_t total_ipc_time = 0;
	uint32_t total_meas_time = 0;
	uint32_t first_packet = 0;

	pr_debug("\n ipc_l1_cpu_loop running.... \n");

	do {

		vGetCurrentTimeMpic(&ipc_start);
		error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);

		if (IPC_SUCCESS != error) {
			continue;
		}

		ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);
		TimeDiffInUs = ulGetElapsedTimeMpic(&ipc_start);

		if (!first_packet) {
			meas_start = ipc_start;
			first_packet = 1;
		}

		total_ipc_time = total_ipc_time + TimeDiffInUs;
		messages_received++;

	} while (messages_received < PERF_L1_RX_PACKETS);

	total_meas_time = ulGetElapsedTimeMpic(&meas_start);

	PRINTF("\n Total Rx throughput(percentage) = %d values= IPC time(%d):Total time(%d)", (total_ipc_time)/(total_meas_time/100), total_ipc_time, total_meas_time);

}

void ipc_perf_core1_loop(ipc_t ipc_handle)
{
	ipc_sh_buf_t rcv_sh_buf;
	uint32_t len = 2048;
	int error;
	uint32_t *msg_2k_buf;

	error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);
#ifdef IPC_TEST_DEBUG
	count++;
	pr_debug("\n invoking ipc_recv_ptr.on L2_TO_L1_MSG_CH_3 .count =%d , error = %d\n",
			count, error);
#endif
	if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
		pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
	} else {
#ifdef IPC_TEST_DEBUG
		len = ipc_h2m_32(&(rcv_sh_buf.data_size));
		fsl_print("\n L2_TO_L1_MSG_CH_3 content\n ");
		ipc_xdump((char *)buf_addr, len);
		if (validate_buffer((void *)buf_addr, len))
			pr_debug("\n====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_3 \n");
#endif
		error = ipc_get_msg_ptr(L1_TO_L2_MSG_CH_4, ipc_handle, (void **)&msg_2k_buf);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_get_msg_ptr on L1_TO_L2_MSG_CH_4,  (error=%d)!\n", error);
#endif
		}
		/* copy 64 bit Host Timestamp */
		uint32_t *buf_addr = (uint32_t *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
						+ (uint32_t)PEBM_BASE_ADDR);
		*msg_2k_buf++ =  *buf_addr++;
		*msg_2k_buf =  *buf_addr;

		error =  ipc_send_msg_ptr(L1_TO_L2_MSG_CH_4, len, ipc_handle);
		if (IPC_SUCCESS != error)
			pr_err(" ERROR  Failed ipc_send_msg_ptr on L1_TO_L2_MSG_CH_4, (error=%d)!\n", error);
		ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);
	}

}

void ipc_perf_core2_loop(ipc_t ipc_handle)
{
	uint32_t len = 2048;
	int error;
	uint32_t *msg_2k_buf, *buf_send;

	error = ipc_recv_msg_ptr(L2_TO_L1_MSG_CH_2, (void **)&msg_2k_buf, &len, ipc_handle);
#ifdef IPC_TEST_DEBUG
	count++;
	pr_debug("\n invoking ipc_recv_ptr.on L2_TO_L1_MSG_CH_2 .count =%d , error = %d\n",
			count, error);
#endif
	if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
		pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_2  (error=%d)!\n", error);
#endif
	} else {
#ifdef IPC_TEST_DEBUG
		fsl_print("\n L2_TO_L1_MSG_CH_2 content\n");
		ipc_xdump(msg_2k_buf, len);
		if (validate_buffer(msg_2k_buf, len))
			pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_2  \n");
#endif
		ipc_set_consumed_status(L2_TO_L1_MSG_CH_2, ipc_handle);

		error = ipc_get_msg_ptr(L1_TO_L2_MSG_CH_5, ipc_handle, (void **)&buf_send);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_get_msg_ptr on L1_TO_L2_MSG_CH_5,  (error=%d)!\n", error);
#endif
		}
		/* copy 64 bit Host Timestamp */
		*buf_send++ =  *msg_2k_buf++;
		*buf_send =  *msg_2k_buf;

		error =  ipc_send_msg_ptr(L1_TO_L2_MSG_CH_5, len, ipc_handle);
		if (IPC_SUCCESS != error)
			pr_err(" ERROR  Failed ipc_send_msg_ptr on L1_TO_L2_MSG_CH_5, (error=%d)!\n", error);
	}
}

void ipc_perf_core3_loop(ipc_t ipc_handle)
{
	uint32_t len = 2048;
	int error;
	uint32_t *msg_2k_buf, *buf_send;
	ipc_sh_buf_t *sh_buf_1;
	mod_mem_region_t *huge_page;

	error = ipc_recv_msg_ptr(L2_TO_L1_MSG_CH_1, (void **)&msg_2k_buf, &len, ipc_handle);
#ifdef IPC_TEST_DEBUG
	count++;
	pr_debug("\n invoking ipc_recv_ptr.on L2_TO_L1_MSG_CH_1 .count =%d , error = %d\n",
				count, error);
#endif
	if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
		pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_1  (error=%d)!\n", error);
#endif
	} else {
#ifdef IPC_TEST_DEBUG
		fsl_print("\n L2_TO_L1_MSG_CH_1 content\n");
		ipc_xdump(msg_2k_buf, len);
		if (validate_buffer(msg_2k_buf, len))
			pr_debug("\n====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1\n");
#endif
		ipc_set_consumed_status(L2_TO_L1_MSG_CH_1, ipc_handle);

		sh_buf_1 = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
		if (sh_buf_1 == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  ipc_get_buf failed for L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
		}

		huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
#ifdef IPC_TEST_DEBUG
		fsl_print("\n===> sh_buf_1->mod_phys =%x \n", ipc_h2m_32(&sh_buf_1->mod_phys));
		fsl_print("\n===>huge_page->addr_v =%x \n", huge_page->addr_v);
#endif
	/* copy 64 bit Host Timestamp */
		buf_send = (uint32_t *) ((ipc_h2m_32(&sh_buf_1->mod_phys)) + huge_page->addr_v);
		*buf_send++ =  *msg_2k_buf++;
		*buf_send =  *msg_2k_buf;
		ipc_m2h_32(1024 * 2, &(sh_buf_1->data_size));

		error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf_1, ipc_handle);
		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
		}
	}
}

void ipc_latency_loopback(ipc_instance_t *ipc_handle)
{
	u8 core_id = (u8)ulMpicCurrentCore();

	/* Receive on L2_TO_L1_MSG_CH_1 and loopback on L1_TO_L2_PRT_CH_1 */
	if (core_id == 3) {
		if (ipc_h2m_32(&(ipc_handle->ch_list[L2_TO_L1_MSG_CH_1].msi_valid))) {
			if (ipc_register_channel_callback(L2_TO_L1_MSG_CH_1, ipc_perf_core3_loop, ipc_handle,
							&(ipc_handle->ch_list[L2_TO_L1_MSG_CH_1]) ) != IPC_SUCCESS) {
				pr_err("Error in registering handler:%d", L2_TO_L1_MSG_CH_1);
				return;
			}

			/* Mark channel as initialized by core */
			is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

			/* Wait for other channels to finish configuration */
			while(!(is_channed_initialized[1] && is_channed_initialized[2]));

			/* Set modem ready to start Rx/Tx */
			SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);
		} else {
			/* Mark channel as initialized by core */
			is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

			/* Wait for other channels to finish configuration */
			while(!(is_channed_initialized[1] && is_channed_initialized[2]));

			/* Set modem ready to start Rx/Tx */
			SET_HIF_MOD_RDY(bsp_get_hif(), HIF_MOD_READY_IPC_APP);

			while(1)
				ipc_perf_core3_loop(ipc_handle);
		}
	}

	/* Receive on L2_TO_L1_MSG_CH_2 and loopback on L1_TO_L2_MSG_CH_5 */
	if (core_id == 2) {
		if (ipc_h2m_32(&(ipc_handle->ch_list[L2_TO_L1_MSG_CH_2].msi_valid))) {
			if (ipc_register_channel_callback(L2_TO_L1_MSG_CH_2, ipc_perf_core2_loop, ipc_handle,
							&(ipc_handle->ch_list[L2_TO_L1_MSG_CH_2]) ) != IPC_SUCCESS) {
				pr_err("Error in registering handler:%d", L2_TO_L1_MSG_CH_2);
				return;
			}

			/* Mark channel as initialized by core */
			is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;
		} else {
			/* Mark channel as initialized by core */
			is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

			while(1)
				ipc_perf_core2_loop(ipc_handle);
		}
	}

	/* Receive on L2_TO_L1_MSG_CH_3 and loopback on L1_TO_L2_MSG_CH_4 */
	if (core_id == 1) {
		if (ipc_h2m_32(&(ipc_handle->ch_list[L2_TO_L1_MSG_CH_3].msi_valid))) {
			if (ipc_register_channel_callback(L2_TO_L1_MSG_CH_3, ipc_perf_core1_loop, ipc_handle,
							&(ipc_handle->ch_list[L2_TO_L1_MSG_CH_3]) ) != IPC_SUCCESS) {
				pr_err("Error in registering handler:%d", L2_TO_L1_MSG_CH_3);
				return;
			}

			/* Mark channel as initialized by core */
			is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;
		} else {
			/* Mark channel as initialized by core */
			is_channed_initialized[(u8)ulMpicCurrentCore()] = 1;

			while(1)
				ipc_perf_core1_loop(ipc_handle);
		}
	}
}

void ipc_perf_rx_loop(ipc_instance_t * ipc_handle)
{
	int32_t  error;
	uint32_t count = 0;
	ipc_sh_buf_t rcv_sh_buf;

	pr_debug("\n ipc_perf_app running.... \n");

	do {
		count++;
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_ptr ..count =%d , error = %d\n", count, error);
#endif
		error = ipc_recv_ptr(L2_TO_L1_MSG_CH_3, (void *)&rcv_sh_buf, ipc_handle);
#ifdef IPC_TEST_DEBUG
		pr_debug("\n invoking ipc_recv_ptr.on L2_TO_L1_MSG_CH_3 .count =%d , error = %d\n",
				count, error);
#endif

		if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
			pr_err(" ERROR  Failed  ipc_recv_ptr on L2_TO_L1_MSG_CH_3  (error=%d)!\n", error);
#endif
			continue;
		}
#ifdef IPC_TEST_DEBUG
		uint32_t *buf_addr = (uint32_t *)(ipc_h2m_32(&(rcv_sh_buf.mod_phys))
						+ (uint32_t)PEBM_BASE_ADDR);
		len = ipc_h2m_32(&(rcv_sh_buf.data_size));
		fsl_print("\n L2_TO_L1_MSG_CH_1 content\n ");
		ipc_xdump((char *)buf_addr, len);
		if (validate_buffer((void *)buf_addr, len))
			pr_debug("\n ====> Received Buffer validation Failed for L2_TO_L1_MSG_CH_1 \n");
#endif
		ipc_put_buf(L2_TO_L1_MSG_CH_3, &rcv_sh_buf, ipc_handle);

	} while (1);
}

int ipc_perf_tx(void)
{
	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);
	/* Wait for host ready.*/
	wait_for_host_ready();
	/* Wait for Configuration to be completed */
	wait_for_cfg_done();

	ipc_perf_loop();
	return TC_IPC_SUCCESS;

}

int ipc_perf_l1_tx(void)
{
	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);
	/* Wait for host ready.*/
	wait_for_host_ready();
	/* Wait for Configuration to be completed */
	wait_for_cfg_done();

	ipc_l1_txloop();
	return TC_IPC_SUCCESS;

}

void ipc_l1_txloop(void)
{
	int error;
	ipc_sh_buf_t* sh_buf;
	struct Time meas_start;
	struct Time ipc_start1;
	struct Time ipc_start2;
	uint32_t total_meas_time = 0;
	uint32_t total_ipc_time1 = 0;
	uint32_t total_ipc_time2 = 0;
	uint32_t messages_sent=0;

	vGetCurrentTimeMpic(&meas_start);

	do {

		vBusyWaitMpic(100);

		vGetCurrentTimeMpic(&ipc_start1);
		/*Get the buffer and transmit it on ptr channel 1*/
		sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
		total_ipc_time1 += ulGetElapsedTimeMpic(&ipc_start1);

		if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_1,error=%d\n",
						error);
#endif
		}
		else {
			m2h_32(1000 * 64, &(sh_buf->buf_size));
			m2h_32(1000 * 64, &(sh_buf->data_size));

			vGetCurrentTimeMpic(&ipc_start2);
			error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf, ipc_handle);
			total_ipc_time2 += ulGetElapsedTimeMpic(&ipc_start2);


			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			} else {
				messages_sent++;
			}
		}

		/*Get the buffer and transmit it on ptr channel 2 */
		vGetCurrentTimeMpic(&ipc_start1);
		sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &error);
		total_ipc_time1 += ulGetElapsedTimeMpic(&ipc_start1);

		if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_2,error=%d\n",
						error);
#endif
		}
		else {
			m2h_32(1000 * 64, &(sh_buf->buf_size));
			m2h_32(1000 * 64, &(sh_buf->data_size));

			vGetCurrentTimeMpic(&ipc_start2);			
			error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf, ipc_handle);
			total_ipc_time2 += ulGetElapsedTimeMpic(&ipc_start2);
			
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
			} else {
				messages_sent++;
			}
		}

	} while(messages_sent < PERF_L1_TX_PACKETS);

	total_meas_time	= ulGetElapsedTimeMpic(&meas_start);

	PRINTF("\n Total tx throughput(percentage) = %d values=IPC time(%x:%x):Total Time(%x)", (total_ipc_time1+total_ipc_time2)/(total_meas_time/100), total_ipc_time1, total_ipc_time2, total_meas_time);

	return;

}

void ipc_perf_loop(void)
{
	int error;
	ipc_sh_buf_t* sh_buf;

	do {
		/* Delay of 100 usec adjusted to generate 8Kpps rate on both PTR  channels */

		/* I observed that interrupt gneration process pattern not appears
		 * to be stable/regular when we try to wait for, say 100 usec, and then send
		 * two packets with interrupts back to back.
		 * Splitting the overall wait into two helped in stable loading of
		 * interrupts @ HOST.
		 */
		vBusyWaitMpic(55);
		/*Get the buffer and transmit it on ptr channel 1*/
		sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
		if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_1,error=%d\n",
						error);
#endif
		}
		else {
			m2h_32(1000 * 64, &(sh_buf->buf_size));
			m2h_32(1000 * 64, &(sh_buf->data_size));
			error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf, ipc_handle);
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
			}
		}

		vBusyWaitMpic(55);
		/*Get the buffer and transmit it on ptr channel 2 */
		sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &error);
		if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
			pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_2,error=%d\n",
						error);
#endif
		}
		else {
			m2h_32(1000 * 64, &(sh_buf->buf_size));
			m2h_32(1000 * 64, &(sh_buf->data_size));
			error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf, ipc_handle);
			if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
				pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
			}
		}

	} while(1);

	return;

}

int ipc_perf_tx_8CC(void)
{
	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);
	/* Wait for host ready.*/
	wait_for_host_ready();
	/* Wait for Configuration to be completed */
	wait_for_cfg_done();

	ipc_perf_loop_8CC();
	return TC_IPC_SUCCESS;

}

void ipc_perf_loop_8CC(void)
{
	int error, i;
	ipc_sh_buf_t *sh_buf;

	do {
		/* I observed that interrupt gneration process pattern not appears
		 * to be stable/regular when we try to wait for, say 100 usec, and then send
		 * two packets with interrupts back to back.
		 * Splitting the overall wait into two helped in stable loading of
		 * interrupts @ HOST.
		 */
		vBusyWaitMpic(40);
		/* Send 4 packets on each channel with in 125 usec to genrate 64Kpps */
		for (i = 0; i < 4; i++) {
		/*Get the buffer and transmit it on ptr channel 1*/
retry_buf_1:
			sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
			if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
				pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_1,error=%d\n",
						error);
#endif
				goto retry_buf_1;
			} else {
				m2h_32(1000 * 64, &(sh_buf->buf_size));
				m2h_32(1000 * 64, &(sh_buf->data_size));
retry:
				error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf, ipc_handle);
				if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
					pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
					goto retry;
				}
			}
		}

		vBusyWaitMpic(40);
		for (i = 0; i < 4; i++) {
retry_buf_2:
			/*Get the buffer and transmit it on ptr channel 2 */
			sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &error);
			if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
				pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_2,error=%d\n", error);
#endif
				goto retry_buf_2;
			} else {
				m2h_32(1000 * 64, &(sh_buf->buf_size));
				m2h_32(1000 * 64, &(sh_buf->data_size));
retry_2:
				error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf, ipc_handle);
				if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
					pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
					goto retry_2;
				}
			}
		} /* For Loop*/
	} while (1);

	return;

}

int ipc_perf_tx_l1_8CC(void)
{
	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);
	/* Wait for host ready.*/
	wait_for_host_ready();
	/* Wait for Configuration to be completed */
	wait_for_cfg_done();

	ipc_perf_loop_l1_8CC();
	return TC_IPC_SUCCESS;

}

void ipc_perf_loop_l1_8CC(void)
{
	int error, i;
	ipc_sh_buf_t *sh_buf;
	struct Time meas_start;
	struct Time ipc_start1;
	struct Time ipc_start2;
	uint32_t total_meas_time = 0;
	uint32_t total_ipc_time1 = 0;
	uint32_t total_ipc_time2 = 0;
	uint32_t messages_sent=0, rem;
	struct Time wait;

	vGetCurrentTimeMpic(&meas_start);

	do {
		/* Send 4 packets on each channel with in 125 usec to genrate 64Kpps */
		vGetCurrentTimeMpic(&wait);

		for (i = 0; i < 4; i++) {

			/*Get the buffer and transmit it on ptr channel 1*/
retry_buf_1:
			vGetCurrentTimeMpic(&ipc_start1);
			sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_1, ipc_handle, &error);
			if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
				pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_1,error=%d\n",
						error);
#endif
				goto retry_buf_1;
			} else {
				total_ipc_time1 += ulGetElapsedTimeMpic(&ipc_start1);

				m2h_32(1000 * 64, &(sh_buf->buf_size));
				m2h_32(1000 * 64, &(sh_buf->data_size));
retry:
				vGetCurrentTimeMpic(&ipc_start2);
				error = ipc_send_ptr(L1_TO_L2_PRT_CH_1, sh_buf, ipc_handle);
				if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
					pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_1,  (error=%d)!\n", error);
#endif
					goto retry;

				} else {
					total_ipc_time2 += ulGetElapsedTimeMpic(&ipc_start2);
					messages_sent++;
				}
			}
		//}

			/* We observed that interrupt gneration process pattern not appears
			 * to be stable/regular when we try to wait and then send
			 * two packets with interrupts back to back.
			 * Splitting the overall wait into two helped in stable loading of
			 * interrupts @ HOST.
			 */
			vBusyWaitMpic(5);

		//for (i = 0; i < 4; i++) {
retry_buf_2:
			/*Get the buffer and transmit it on ptr channel 2 */
			vGetCurrentTimeMpic(&ipc_start1);
			sh_buf = ipc_get_buf(L1_TO_L2_PRT_CH_2, ipc_handle, &error);
			if (sh_buf == NULL) {
#ifdef IPC_TEST_DEBUG
				pr_debug(" ipc_get_buf failed for L1_TO_L2_PTR_CH_2,error=%d\n", error);
#endif
				goto retry_buf_2;
			} else {
				total_ipc_time1 += ulGetElapsedTimeMpic(&ipc_start1);

				m2h_32(1000 * 64, &(sh_buf->buf_size));
				m2h_32(1000 * 64, &(sh_buf->data_size));
retry_2:
				vGetCurrentTimeMpic(&ipc_start2);
				error = ipc_send_ptr(L1_TO_L2_PRT_CH_2, sh_buf, ipc_handle);
				if (IPC_SUCCESS != error) {
#ifdef IPC_TEST_DEBUG
					pr_err(" ERROR  Failed  ipc_send_otr on L1_TO_L2_PTR_CH_2,  (error=%d)!\n", error);
#endif
					goto retry_2;
				} else {
					total_ipc_time2 += ulGetElapsedTimeMpic(&ipc_start2);
					messages_sent++;
				}
			}
		} /* For Loop*/

		/* Wait for 125 usec to complete */
		rem = ulGetElapsedTimeMpic(&wait);
		if (rem < 125)
			vBusyWaitMpic((125 - rem));

	} while (messages_sent < PERF_L1_TX_PACKETS);

	total_meas_time	= ulGetElapsedTimeMpic(&meas_start);

	PRINTF("\n Total tx throughput(percentage) = %d values=IPC time(%d:%d):Total Time(%d) \n", (total_ipc_time1+total_ipc_time2)/(total_meas_time/100), total_ipc_time1, total_ipc_time2, total_meas_time);

	return;

}

int ipc_perf_load_pci(void)
{
	int index, ptr_index = 0;
	mod_mem_region_t * huge_page;
	ipc_sh_buf_t  * bd    = ((ipc_instance_t *)ipc_handle)->ch_list[L1_TO_L2_PRT_CH_1].br_bl_desc.bd;
	uint8_t *buff2 = pvGeulMalloc(1024 * 16);

	/* Wait for instance ready.*/
	wait_for_ipc_init(ipc_handle);
	/* Wait for host ready.*/
	wait_for_host_ready();
	/* Wait for Configuration to be completed */
	wait_for_cfg_done();

	pr_debug("\n ipc_perf_copy_ptr_ch running.... \n");

	fill_test_buffer(buff2,(1024*16));
	huge_page = bsp_get_mem_region(MOD_MEM_HUGE_PAGE_BUF);
	do {
		/* Empty Looping */
		for (index = 0; index < 1000; index++);

		/* Just keep on writing to Host memory shared buffer
		   pointer to simulate PCIe transaction load */
			memcpy((void *)((h2m_32(&bd[ptr_index].mod_phys)) + huge_page->addr_v),
								buff2, 2000/*2K*/);

			ptr_index = (ptr_index + 1)%IPC_MAX_DEPTH;

	} while (1);

	return TC_IPC_SUCCESS;
}

void wait_for_ipc_init(volatile ipc_instance_t * ipc_handle)
{
	/* Wait until instance is ready.*/
	if (!ipc_h2m_32(&(ipc_handle->initialized))) {
		pr_debug("%s: Warning IPC instance not ready.",__func__);
	}

	while (!(ipc_h2m_32(&(ipc_handle->initialized)))) { }

}

void wait_for_host_ready(void)
{
	/* Wait for Host LIB ready */
	while(!CHK_HIF_HOST_RDY(bsp_get_hif(), HIF_HOST_READY_IPC_LIB)){ }

	/* Wait for Host APP ready */
	while(!(CHK_HIF_HOST_RDY(bsp_get_hif(), HIF_HOST_READY_IPC_APP))){ }
}
