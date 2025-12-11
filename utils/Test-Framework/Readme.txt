SPDX-License-Identifier: BSD-3-Clause
Copyright 2023 NXP

==========================
Test-Framework Test Cases
==========================

This document describes the functionalities of different test cases available
in test framework.


Valid Test Cases Available under Test-Framework
===============================================
To get updated list all available test cases use the following command
	$ test list 0x1 0

--TEST CASE NAME--	 --TEST CASE DESCRIPTION--
------------------	 -------------------------
list                     - Displays list of available Test Cases
status                   - Print Status of various Test Functions
sys_stats                - Print Status of System Debugging Artifacts
msi                      - Test MSI Functionality
timer                    - Test Timer Functionality
avi                      - Launches AVI test task
vspa_logs                - Launches VSPA logging support
overlay                  - Test Overlay Functionality
data_excp                - Test Data Exception
prg_excp                 - Test Program Exception
align_excp               - Test Align Exception
inst_strg_excp           - Test Instruction Storage Exception
efpu_data_excp           - Test EPFU Data Exception
efpu_round_excp          - Test EPFU Round Exception
ipi                      - Test Case for IPI feature
ipifromisr               - Test Case for IPI form ISR feature
ipi_stats                - Test Case for IPI Stats
spinlock                 - Test Case for Spinlock feature
qdma                     - Test Case for QDMA
qdma_mul                 - Test Case for QDMA Multipe Buffer (USF)
qdma_sg                  - Test Case for QDMA Scatter Gather Buffer (long)
qdma_sg_no_stride        - Test Case for QDMA Scatter Gather Buffer without striding (long)
qdma_single_buffer       - Test Case for QDMA Single Buffer (long)
qdma_legacy              - Test Case for QDMA Legacy Mode
watchdog                 - Test Case for Watchdog
memcheck                 - Test Case for MemCheck
gpio                     - Test Case for GPIO
pcimsi                   - Test Case for PCIMSI
i2c_tmu_read             - Test I2C TMU Read Functionality
tmu                      - TMU test case
HeapMemoryUsageInfo      - Check Heap memory usage
HeapMemoryUsageTest      - Check and Test Heap memory usage
latency                  - Test Latency of gpio and spinlock
ipilatency               - Test Latency of ipi
smem_text                - Test SMEM Text Functionality
core_smem_text           - Test CORE SMEM Text Functionality
ov_reuse                 - Test Overlay area reuse Functionality
cpu_loader_test          - Generate 100% CPU load for 0.5sec duration
tbgen1_non_tdd           - Test Tbgen1 Non Tdd Timers basic functionality
tbgen2_non_tdd           - Test Tbgen2 Non Tdd Timers basic functionality
tbgen1_host_tti          - Test Tbgen1 Host TTI functionality
tbgen2_host_tti          - Test Tbgen2 Host TTI functionality
tbgen1_enable_rfg        - Enable Tbgen1 RFG
tbgen2_enable_rfg        - Enable Tbgen2 RFG
tbgen1_disable_rfg       - Disable Tbgen1 RFG
tbgen2_disable_rfg       - Disable Tbgen2 RFG
tbgen1_ext_irq           - Tbgen1 Rx Align Int Test
tbgen2_ext_irq           - Tbgen2 Rx Align Int Test
tbgen2_tdd               - Test Tbgen2 Tdd Timers basic functionality
tbgen2_tdd_manual        - Test Tbgen2 Tdd Timers Maual mode basic functionality
xspi_rd_write            - FlexSpi Memory Read Write Test
feca_bbdev_se            - Test case for FECA shared encode
feca_bbdev_sd            - Test case for FECA shared decode
feca_bbdev_ce            - Test case for FECA control encode
feca_bbdev_cd            - Test case for FECA control decode
feca_bbdev_dcm_ARM       - PoC DCM using ARM Polar Decoder
pebm_port3               - Test PEBM port3 Functionality
bbdev_ipc1               - Test IPC(L2->L1 on Core 1 & L1->L2 on Core 2)
bbdev_ipc_raw            - Test BBDEV IPC Raw mode


HOW TO RUN TEST-CASE?
=====================

	test <Test-Case-Name>  <CORE-ID>  <Iterations>

	@Test-Case-Name:
		Name of the test-case as mentioned in the above list of available test
		cases which can be executed.

	@CORE-ID:
		Core-Id / Core Number for which that test case needs to be executed.
		CORE ID		        | 5 | 4 | 3 | 2 | 1 | 0 |
				    _128__64__32__16__8___4___2___1__
		Byte Rep:	| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
			        ---------------------------------
			          0   0   0   0   0   0   0   1  -> 0x1  Only Core 0
			 	      0   0   0   0   0   0   1   0  -> 0x2	Only Core 1
			 	      0   0   0   0   0   0   1   1  -> 0x3  Both Core 0 &1
			 	      ....
				      0   0   1   1   1   1   1   1  -> 0x3f All 6 Cores.


	@Iterations:
		Iterations by which that particular test case needs to be executed.

	e.g:
		$ test HeapMemoryUsageInfo 0x1 1
		With this command it will execute the HeapMemoryUsageInfo test
		case on core 0 with an iterations of 1.

		$ test HeapMemoryUsageInfo 0x3f 2
		With this command it will execute the HeapMemoryUsageInfo test
		case on all cores with an interation of 2.


HeapMemoryUsageInfo
-------------------
This test case will show the info about the Heap Memory Usage.
e.g:
	$ test HeapMemoryUsageInfo 0x1 1
	Current Core: 0
	Total Heap Memory: 30720 Bytes
	Available Heap Memory: 15488 Bytes
	Minimum Ever Heap Memory Size: 15488 Bytes

HeapMemoryUsageTest
-------------------
This test case will show the info about the Heap Memory Uage. In this test case
it will allocate and deallocate the memory and will show the status of
available memory.
e.g:
	$ test HeapMemoryUsageTest 0x1 1
	Current Core: 0
 	Total Heap Memory: 30720
 	Available Heap Memory : 15488Bytes
 	Minimum Ever Heap Memory Size Before Test: 15488Bytes
 	Available Heap Memory after array allocation : 15392Bytes
 	Allocating : 32 Bytes, Available Mem Before: 15392Bytes,  After : 15328Bytes, Difference: 64Bytes , metadataSize: 32Bytes
 	Minimum Ever Heap Memory Size: 13312Bytes
 	Available Heap Memory : 15488Bytes
