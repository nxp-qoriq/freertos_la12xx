// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2022 NXP
 * 
 */
#ifndef __GEUL_IPC_TEST_H__
#define __GEUL_IPC_TEST_H__

#include "FreeRTOS.h"
#include "test_framework.h"
#include "timers.h"

void vIpcIntrTest(void);
void vIpcLatencyTest(void);
void vIpcL1PerfTest(void);
void vIpcL1PerfTest_8CC(void);
int ipc_perf_tx_l1_8CC(void);
void ipc_perf_loop_l1_8CC(void);
void vIpcPerfTest(void);
void vIpcPerfTest_8CC(void);
void vIpcSingleCoreTest(void);
void vIpcTest(void);
void vIpcAllCoreTest(void);
void vIpcSixCoreTest(void);
int ipc_perf_l1_rx(void);
void ipc_l1_rxloop(ipc_instance_t * ipc_handle);
int ipc_perf_l1_tx(void);
void ipc_l1_txloop(void);
int ipc_core1_tx_rx_loop(void);
int ipc_tx_ptr_ch1_loop(void);
int ipc_tx_msg_ch4_loop(void);
int ipc_tx_msg_ch5_loop(void);
int ipc_rx_msg_ch3_loop(void);
int ipc_rx_msg_ch1_loop(void);
int ipc_rx_msg_ch2_loop(void);
int ipc_core1_init(ipc_instance_t * ipc_handle);
void ipc_core1_loop(ipc_instance_t * ipc_handle);
int ipc_core2_tx_rx_loop(void);
int ipc_core2_init(ipc_instance_t * ipc_handle);
void ipc_core2_loop(ipc_instance_t * ipc_handle);
int ipc_core3_tx_rx_loop(void);
int ipc_core3_init(ipc_instance_t * ipc_handle);
void ipc_core3_loop(ipc_instance_t * ipc_handle);
void wait_for_host_ready(void);
void wait_for_ipc_init(volatile ipc_instance_t * ipc_handle);
int ipc_single_core_test(void);
int ipc_latency_rx(void);
int ipc_perf_rx(void);
int ipc_perf_rx_init(ipc_instance_t * ipc_handle);
void ipc_latency_loopback(ipc_instance_t * ipc_handle);
void ipc_perf_rx_loop(ipc_instance_t * ipc_handle);
int ipc_perf_tx(void);
int ipc_perf_tx_8CC(void);
int ipc_perf_load_pci(void);
void ipc_perf_loop(void);
void ipc_perf_loop_8CC(void);
int ipc_test_rx(void);
int ipc_test_tx(void);
int ipc_register_rx_int(void);
void ipc_single_core_rxtx(void);
void ipc_test_app_recv_msg(void);
void ipc_test_app_send_data(void);
void ipc_test_sent_ptr(uint32_t channel_id);
int ipc_test_process_recvd_msg(void *msg, int32_t lenght);
void ipc_test_recev_msg(uint32_t channel_id);
int ipc_test_init(void);
void ipc_test_free(void);
void ipc_xdump(unsigned char *buf, int32_t n);
#endif /* __IPC_TEST_H__ */
