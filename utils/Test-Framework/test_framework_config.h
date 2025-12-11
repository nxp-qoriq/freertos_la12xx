// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2019-2024 NXP
 */

#ifndef __TEST_FRAMEWORK_CONFIG_H__
#define __TEST_FRAMEWORK_CONFIG_H__

/* VSPA Test defines */
#define GEUL_DEMO_AVI_TEST		    1
#define GEUL_VSPAMBOX_TEST          1
#define GEUL_VSPAMBOX_ERROR_TEST    1
#define GEUL_VSPA_LOG		    1
#define GEUL_DEMO_OVERLAY_TEST	    1

/* IPC test defines */
#define GEUL_DEMO_IPC_TEST      1

/* AXIQ test defines */
#define GEUL_DEMO_AXIQ_TEST 0

/* Exception test defines */
#define GEUL_DEMO_DATA_EXCEPTION			1
#define GEUL_DEMO_PROGRAM_EXCEPTION     	1
#define GEUL_DEMO_ALIGNMENT_EXCEPTION   	1
#define GEUL_DEMO_INSTR_STORAGE_EXCEPTION	1
#define GEUL_DEMO_EFPU_DATA_EXCEPTION		1
#define GEUL_DEMO_EFPU_ROUND_EXCEPTION		1

/* GPIO test defines */
#define GEUL_DEMO_GPIO_TEST 1
#define GEUL_GPIO_LATENCY_TEST    1

/* IPI test defines */
#define GEUL_DEMO_IPI_QUEUE_TEST 1
#define GEUL_IPI_STATS_TEST 1
#define GEUL_DEMO_IPI_ISR_TEST 1
#define GEUL_IPI_LATENCY_TEST 1

/* MEMCHECK test defines */
#define GEUL_DEMO_MEMCHECK_TEST 1

/* MSI test defines */
#define GEUL_DEMO_MSI_TEST		1

/* PCIMSI test defines */
#define GEUL_DEMO_PCIMSI_TEST		1

/* QDMA test defines */
#define GEUL_DEMO_QDMA_TEST 	1

/* QDMA Legacy mode test defines */
#define GEUL_DEMO_QDMA_LEGACY_TEST     1

/* SPINLOCK test defines */
#define GEUL_DEMO_SPINLOCK_TEST		1

#define GEUL_SPINLOCK_LATENCY_TEST 	0

/* DEBUG capabilities test defines */
#define GEUL_SYSTEM_DEBUG_CAPABILITIES	1

/*  TBGEN test defines */
#define GEUL_DEMO_TBGEN_CSG_TEST			1	/* Simulator */
#define GEUL_DEMO_TBGEN_TIMED_INT_TEST		1
#define GEUL_DEMO_TBGEN_EXT_VSPA_GO_TEST	1
#define GEUL_DEMO_TBGEN_GP_EVENT_TEST		1
#define GEUL_DEMO_TBGEN_GP_TIMER_TEST		1
#define GEUL_DEMO_TBGEN_AGCEN_TEST		1
#define GEUL_DEMO_TBGEN_AGC_TIMER_TEST		1
#ifdef GEUL_LA1238RDB
#define GEUL_DEMO_TBGEN_HOST_TTI_TEST		1
#else
#define GEUL_DEMO_TBGEN_TTI_GP_EVENT_TEST	1
#endif
#define GEUL_DEMO_TBGEN_RFG_TEST		1
#define GEUL_DEMO_TBGEN_SRX_EVENT_TEST		1
#define GEUL_DEMO_TBGEN_SPI_EVENT_TEST		1
#ifdef GEUL_LA1224
#define GEUL_DEMO_TBGEN_TDD_SEQ1		1
#endif

/*  TIMER test defines */
#define GEUL_DEMO_TIMER_TEST	1

/*  WDOG test defines */
#define GEUL_DEMO_WDOG_TEST         1

/* DCS HS test defines */
#define GEUL_DEMO_DCS_TEST	0

/* I2C test defines */
#define GEUL_DEMO_I2C_TEST   1

/* GPIO Toggle test defines */
#define GEUL_DEMO_GPIO_TOGGLE_TEST 0

#if defined(ENABLE_DSPI_STREAM_MODE)
/* DSPI test defines */
#define GEUL_DEMO_DSPI_TEST	1
#endif

/* TMU test defines */
#define GEUL_DEMO_TMU_TEST 1

/* GEUL Check Heap Memory Usage  */
#define GEUL_HEAP_USAGE_TEST 1

/* GEUL Demo Latency Test  */
#define GEUL_DEMO_LATENCY_TEST 1

/* GEUL Demo SMEM Text Test  */
#define GEUL_DEMO_SMEM_TEXT_TEST 1

/* GEUL Demo CORE SMEM Text Test  */
#define GEUL_DEMO_CORE_SMEM_TEXT_TEST 1

/* GEUL Demo Loader Test */
#define GEUL_DEMO_CPU_LOADER_TEST	1

#if !defined(GEUL_LA1238CPE) && !defined(GEUL_LA1224CPE)
/* FECA BBDV test defination */
#define GEUL_DEMO_FECA_BBDEV_TEST 1
#endif

/* GEUL Demo Overlay Text Test  */
#define GEUL_DEMO_OVERLAY_REUSE_TEST 1

#if defined(TDD_DEMOAPP_ENABLE)
#ifdef GEUL_LA1224
#define GEUL_DEMO_ECPRI_START_TDD 1
#endif
#endif

/* GEUL Demo PEBM Port3 Test  */
#define GEUL_DEMO_PEBM_PORT3_TEST 1

/*  GEUL Demo Flaoting Point Test */
#define GEUL_DEMO_FLOAT_TEST 1
#endif
