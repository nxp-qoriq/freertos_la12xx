// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2025 NXP
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "debug_console.h"
#include "Time.h"
#include "ipiQueue.h"
#include "gpio.h"
#include "gpio_regs.h"
#include "soc.h"

/* Test Framework Include */
#include "test_framework.h"

/* Sub-system Test Cases Include */
#include "tc_avi.h"
#include "tc_axiq.h"
#include "tc_exception.h"
#include "tc_ipi.h"
#include "tc_msi.h"
#include "tc_qdma.h"
#include "tc_qdma_legacy.h"
#include "tc_la12xx_float.h"
#ifdef BBDEV_IPC_MODE
#include "tc_bbdev_ipc.h"
#elif defined(CPE_IPC)
#include "tc_cpe_ipc.h"
#else
#include "tc_ipc.h"
#endif
#include "tc_spinlock.h"
#include "tc_system.h"
#include "tc_la12xx_tbgen.h"
#include "tc_timer.h"
#include "tc_vspa.h"
#include "tc_watchdog.h"
#include "tc_memcheck.h"
#include "tc_gpio.h"
#include "tc_pcimsi.h"
#include "tc_logger.h"
#include "tc_dcs.h"
#include "tc_dspi.h"
#ifdef GEUL_LA1246
#include "tc_i2c_eeprom.h"
#endif /* GEUL_LA1246 */
#include "tc_i2c_tmu.h"
#include "tc_i2c_lmp92xx.h"
#include "tc_heap_usage.h"
#include "tc_ipisr.h"
#include "tc_tmu.h"
#include "tc_latency.h"
#include "tc_smem_text.h"
#include "tc_core_smem_text.h"
#include "tc_cpu_loader.h"
#ifdef BBDEV_IPC_MODE
#include "tc_feca_bbdev.h"
#include "tc_bbdev_ipc_raw.h"
#endif
#ifndef VSPA_OV_PEBM_ENABLED
#include "tc_overlay_data.h"
#endif
#include "tc_ecpri_start_tdd.h"
#include "tc_peb_port3.h"
#ifdef CONFIG_FLEXSPI
#include "tc_xspi.h"
#endif

TestStatus_t xTestStatus  __attribute__ ((section (".smem")));
TfIpiData_t xTfXchangeMem __attribute__ ((section (".smem")));
#ifndef RELEASE_MODE

static inline char *stringFromeGeulDemoTestStatus(enum eGeulDemoTestStatus t)
{
	static char *strings[] = {
#if GEUL_DEMO_MSI_TEST
		"MSI_TEST_STATUS                ",
#endif
#if GEUL_DEMO_TIMER_TEST
		"TIMER_TEST_STATUS              ",
#endif
#if GEUL_DEMO_QDMA_TEST
		"QDMA_TEST_STATUS               ",
#endif
#if GEUL_DEMO_QDMA_LEGACY_TEST
		"QDMA_LEGACY_TEST_STATUS        ",
#endif

#if GEUL_DEMO_IPI_QUEUE_TEST
		"IPI_QUEUE_TEST_STATUS          ",
#endif
#if GEUL_DEMO_SPINLOCK_TEST
		"SPINLOCK_TEST_STATUS           ",
#endif
#if GEUL_DEMO_AVI_TEST
		"VSPA_AVI_TEST_STATUS           ",
#endif
		"TBGEN2_TDD_TEST_STATUS         ",
		"TBGEN2_TDD_MANUAL_TEST_STATUS  ",
#if GEUL_DEMO_OVERLAY_TEST
		"VSPA_OVERLAY_TEST_STATUS       ",
#endif
#if GEUL_DEMO_WDOG_TEST
		"WDOG_TEST_STATUS               ",
#endif
#if GEUL_DEMO_MEMCHECK_TEST
		"MEMCHECK_TEST_STATUS           ",
#endif
#if GEUL_DEMO_GPIO_TEST
		"GPIO_TEST_STATUS               ",
#endif
#if GEUL_DEMO_PCIMSI_TEST
		"PCIMSI_TEST_STATUS             ",
#endif
		"QDMA_MUL_TEST_STATUS           ",
		"QDMA_SG_TEST_STATUS            ",
#if GEUL_DEMO_IPC_TEST
		"IPC_TEST1_STATUS               ",
		"IPC_TEST2_STATUS               ",
#endif
#if GEUL_DEMO_DCS_TEST
		"DCS_TEST_STATUS                ",
#endif
#ifdef GEUL_BOOT_MODE_PCI
		"PEBM_LOGGER_STATUS             ",
#endif
#if GEUL_DEMO_I2C_TEST
		"I2C_TEST_STATUS               ",
#endif
#if GEUL_HEAP_USAGE_TEST
		"GEUL_HEAP_USAGE_TEST_STATUS   ",
#endif
		"TBGEN1_RX_ALIGN_INT_TEST_STATUS",
		"TBGEN2_RX_ALIGN_INT_TEST_STATUS",
		"TBGEN1_HOST_TTI_TEST_STATUS    ",
		"TBGEN2_HOST_TTI_TEST_STATUS    ",
#if GEUL_DEMO_DSPI_TEST
		"DSPI_TEST_STATUS               ",
#endif
#if GEUL_DEMO_TMU_TEST
		"TMU_TEST_STATUS                ",
#endif
#if GEUL_DEMO_IPI_ISR_TEST
		"IPI_ISR_TEST_STATUS            ",
#endif
#if GEUL_DEMO_LATENCY_TEST
		"LATENCY_TEST_STATUS            ",
#endif
#if GEUL_DEMO_SMEM_TEXT_TEST
		"SMEM_TEXT_TEST_STATUS          ",
#endif
		"FECA_BBDEV_TEST_STATUS         ",
#if (GEUL_DEMO_OVERLAY_REUSE_TEST) &&  !defined(VSPA_OV_PEBM_ENABLED)
		"OVERLAY_REUSE_TEST_STATUS      ",
#endif
		"TBGEN1_NON_TDD_TEST_STATUS     ",
		"TBGEN2_NON_TDD_TEST_STATUS     ",
		"TBGEN1_RFG_TEST_STATUS         ",
		"TBGEN2_RFG_TEST_STATUS         ",
#if GEUL_DEMO_CORE_SMEM_TEXT_TEST
		"CORE_SMEM_TEXT_TEST_STATUS     ",
#endif
#if GEUL_DEMO_PEBM_PORT3_TEST
		"PEB_PORT3_TEST_STATUS          ",
#endif
#if GEUL_DEMO_FLOAT_TEST
		"FLOAT_TEST_STATUS          ",
#endif
	};

	return strings[t];
}

static void vGeulTestDispStatus( void )
{
	uint32_t uiTestIndex = 0, uiCpuIndex = 0;

	log_info( "\n\r" );
	log_info( "LA12XX DEMO_TEST SUMMARY \n\r" );
	log_info( "============================================================================== \n\r" );
	log_info( "TESTCASE                      \t" );
	for( uiCpuIndex = 0; uiCpuIndex < get_soc_numcores(); uiCpuIndex++ )
		log_info("Core%d\t", uiCpuIndex);
	log_info("\r\n");
	log_info( "============================================================================== \n\r" );

	for ( uiTestIndex = 0; uiTestIndex < GEUL_DEMO_MAX_TEST_STATUS; uiTestIndex++ )
	{
		log_info( "%s\t", stringFromeGeulDemoTestStatus(uiTestIndex));
		for( uiCpuIndex = 0; uiCpuIndex < get_soc_numcores(); uiCpuIndex++ )
		{
			log_info("%s\t", GET_TEST_STATUS( uiCpuIndex, uiTestIndex ));
		}
		log_info("\r\n");
	}
}
#endif /* RELEASE_MODE */


/* Function Name <-> Function Pointer */
FunctionHashTable_t xFunctionMap[]  __attribute__ ((section (".shared.data"))) = 
{
	{ "list", vPrintValidTestCases, "Displays list of available Test Cases" },
#ifndef RELEASE_MODE
	/* { "Test CMD", TestFunctionName, "Test Description"} */

	/* Test Case Help Menu */
	{ "status", vGeulTestDispStatus, "Print Status of various Test Functions" },
#if GEUL_SYSTEM_DEBUG_CAPABILITIES
	/* System Debugging Stats */
	{ "sys_stats", vStatsUsageCommand, "Print Status of System Debugging Artifacts" },
#endif

#if GEUL_DEMO_MSI_TEST
	/* MSI Test Cases */
	{ "msi", vGeulDemoMSITest, "Test MSI Functionality" },
#endif

#if GEUL_DEMO_TIMER_TEST
	/* Timer Test Cases */
	{ "timer", vGeulDemoTimerTest, "Test Timer Functionality" },
#endif

#if GEUL_DEMO_AVI_TEST
	/* AVI Test Cases */
	{ "avi", vLaunchAviTestTask, "Launches AVI test task" },
#endif

#if GEUL_VSPA_LOG
	/* VSPA Logging support */
	{ "vspa_logs", vLaunchVspaLogs, "Launches VSPA logging support" },
#endif

#ifdef GEUL_BOOT_MODE_PEBM
	/* PEBM Logger */
	{ "logger", vLaunchPEBMLogger, "Launches PEBM Logging" },
#endif

#if GEUL_DEMO_OVERLAY_TEST
	{ "overlay", vGeulOverlayTest, "Test Overlay Functionality" },
#endif

	/* Exception Test Cases */
#if GEUL_DEMO_DATA_EXCEPTION
	{ "data_excp", vGeulDataExceptionTest, "Test Data Exception" },
#endif

#if GEUL_DEMO_PROGRAM_EXCEPTION
	{ "prg_excp", vGeulProgramExceptionTest, "Test Program Exception" },
#endif

#if GEUL_DEMO_ALIGNMENT_EXCEPTION
	{ "align_excp", vGeulAlignExceptionTest, "Test Align Exception" },
#endif

#if GEUL_DEMO_INSTR_STORAGE_EXCEPTION
	{ "inst_strg_excp", vGeulInstrStorageExceptionTest, "Test Instruction Storage Exception" },
#endif

#if GEUL_DEMO_EFPU_DATA_EXCEPTION
	{ "efpu_data_excp", vGeulEFPUDataExceptionTest, "Test EPFU Data Exception" },
#endif

#if GEUL_DEMO_EFPU_ROUND_EXCEPTION
	{ "efpu_round_excp", vGeulEFPURoundExceptionTest, "Test EPFU Round Exception" },
#endif

#if GEUL_DEMO_AXIQ_TEST
	/* AXIQ Test Cases */
	{ "axiq_demo_init", vAxiqInitDemo, "AXIQ Demo Init" },
	{ "axiq_demo_exit", vAxiqExitDemo, "AXIQ Demo Exit" },
	{ "rf_lb_task_demo", vRfLbTaskDemo, "Rf Loopback Demo Thread creation" },
#endif

#if GEUL_DEMO_IPI_QUEUE_TEST		
	/* IPI Test Cases */
	{ "ipi", vGeulIPIDemoEntry, "Test Case for IPI feature" },
	{ "ipifromisr", vGeulIPIFromISRDemoEntry, "Test Case for IPI form ISR feature" },
#endif

#if GEUL_IPI_STATS_TEST		
	/* IPI Test Cases */
	{ "ipi_stats", vGeulIPIStats, "Test Case for IPI Stats" },
#endif
#if GEUL_DEMO_SPINLOCK_TEST
	/* Spinlock Test Cases */
	{ "spinlock", vGeulDemoSpinlockTest, "Test Case for Spinlock feature" },
#endif
	
#if GEUL_DEMO_QDMA_TEST
	/* QDMA Test Cases */
	{ "qdma", vQdmaTest, "Test Case for QDMA" },
	{ "qdma_mul", vQdmaMulTest, "Test Case for QDMA Multipe Buffer (USF)" },
	{ "qdma_sg", vQdmaSGTest, "Test Case for QDMA Scatter Gather Buffer (long)" },
#ifdef QDMA_PEB_TO_FRAM
	{ "qdma_sg_no_stride", vQdmaSGNoStrideTest, "Test Case for QDMA Scatter Gather Buffer without striding (long)" },
	{ "qdma_single_buffer", vQdmaSingleBufferTest, "Test Case for QDMA Single Buffer (long)" },
#endif
#endif

#if GEUL_DEMO_QDMA_LEGACY_TEST
	/* QDMA Legacy Test Cases */
	{ "qdma_legacy", vQdmaLegacyTest, "Test Case for QDMA Legacy Mode" },
#endif
#ifndef BBDEV_IPC_MODE
#if GEUL_DEMO_IPC_TEST
	/* IPC Test Cases */
	{ "ipc_score", vIpcSingleCoreTest, "Test IPC on single core(0)." },
	{ "ipc1", vIpcTest, "Test IPC(L2->L1 on Core 1 & L1->L2 on Core 2)" },
	{ "ipc2", vIpcAllCoreTest, "Test IPC Tx/Rx on all cores (Channels distributed)." },
	{ "ipc3", vIpcSixCoreTest, "Test IPC Tx on three cores and Rx on three other cores." },
	{ "ipc_perf", vIpcPerfTest, "IPC Performance stats(L2->L1 on Core 1 & L1->L2 on Core 2)." },
	{ "ipc_perf_L1", vIpcL1PerfTest, "IPC L1 CPU utilization for 1CC case." },
	{ "ipc_perf_L1_8cc", vIpcL1PerfTest_8CC, "IPC L1 CPU utilization for 8CC case." },
	{ "ipc_perf_8cc", vIpcPerfTest_8CC, "IPC Performance with Carrier Aggregation usecase" },
	{ "ipc_latency", vIpcLatencyTest, "IPC Latency stats(L2->L1->L2 on Core 1." },
	{ "ipc_evnt_poll", vIpcIntrTest, "L2->L1 with event/polling support on Core 0." },
#endif
#endif

#if GEUL_DEMO_WDOG_TEST
	/* Watchdog Test Cases */
	{ "watchdog", vWatchdogTest, "Test Case for Watchdog" },
#endif

#if GEUL_DEMO_MEMCHECK_TEST
	/* MEMCHECK Test Cases */
	{ "memcheck", vMemCheck, "Test Case for MemCheck " },
#endif

#if GEUL_DEMO_GPIO_TEST
	/* GPIO Test Cases */
	{ "gpio", vGpioTest, "Test Case for GPIO " },
#endif

#if GEUL_DEMO_PCIMSI_TEST
	/* PCIMSI Test Cases */
	{ "pcimsi", vGeulDemoPCIMSITest, "Test Case for PCIMSI " },
#endif

	/* DCS Test Cases */
#if GEUL_DEMO_DCS_TEST
	{ "dcs_ls", vDcsLsDemo, "DCS LS test case" },
	{ "dcs_hs", vDcsHsDemo, "DCS HS test case" },
#endif

/*I2C Test Cases */
#if GEUL_DEMO_I2C_TEST
#ifdef GEUL_LA1246
	{ "i2c_eeprom_read", vI2CeepromTestRead, "Test I2C EEPROM Read Write Functionality" },
#if TEST_COMPLETE_EEPROM_RW
	{ "i2c_eeprom", vI2CeepromTest, "Test I2C Complete EEPROM Functionality" },
#endif /* TEST_COMPLETE_EEPROM_RW */
#endif /* GEUL_LA1246 */
	{ "i2c_tmu_read", vI2CtmuReadTest, "Test I2C TMU Read Functionality" },
	{ "i2c_lmp92xx", vI2Ctestlmp92xx, "Test I2C LMP92xx Read Write Functionality" },
#endif /* GEUL_DEMO_I2C_TEST */

	/* GPIO Toggle Test Cases */
#if GEUL_DEMO_GPIO_TOGGLE_TEST
	{ "gpiotoggle", NULL, "Test case for GPIO toggling" },
#endif
	/* TMU Test Cases */
#if GEUL_DEMO_TMU_TEST
	{ "tmu", vTmuTest, "TMU test case" },
#endif

#if GEUL_HEAP_USAGE_TEST
	{ "HeapMemoryUsageInfo", vHeapMemoryUsageInfo, "Check Heap memory usage"},
	{ "HeapMemoryUsageTest", vHeapMemoryUsage, "Check and Test Heap memory usage"},
#endif

#if GEUL_DEMO_DSPI_TEST
         /*DSPI Test cases*/
         { "dspi", vGeulDspiTest, "Test for DSPI read and write" },
#endif


#if GEUL_DEMO_LATENCY_TEST
        /*DSPI Test cases*/
        { "latency", vGeulLatencyTest, "Test Latency of gpio and spinlock" },
        { "ipilatency", vIPILatencyTest, "Test Latency of ipi" },
#endif
#if GEUL_DEMO_SMEM_TEXT_TEST
	/* SMEM Text Test Cases */
	{ "smem_text", vGeulDemoSmemTextTest, "Test SMEM Text Functionality" },
#endif
#if GEUL_DEMO_CORE_SMEM_TEXT_TEST
	/* CORE SMEM text Test Cases */
	{ "core_smem_text", vGeulDemoCoreSmemTextTest, "Test CORE SMEM Text Functionality" },
#endif
#if (GEUL_DEMO_OVERLAY_REUSE_TEST) &&  !defined(VSPA_OV_PEBM_ENABLED)
	/* OVERLAY Reuse Test Cases */
	{ "ov_reuse", vGeulDemoOvReuseTest, "Test Overlay area reuse Functionality" },
#endif
#if GEUL_DEMO_CPU_LOADER_TEST
	/* CPU Loader Test case */
	{ "cpu_loader_test", vCpuLoaderTestCommand, "Generate 100% CPU load for 0.5sec duration" },
#endif
	{ "tbgen1_non_tdd", vTbgen1NonTddTimerTest, "Test Tbgen1 Non Tdd Timers basic functionality" },
	{ "tbgen2_non_tdd", vTbgen2NonTddTimerTest, "Test Tbgen2 Non Tdd Timers basic functionality" },
	{ "tbgen1_host_tti", vTbgen1HostTTIEventTest, "Test Tbgen1 Host TTI functionality" },
	{ "tbgen2_host_tti", vTbgen2HostTTIEventTest, "Test Tbgen2 Host TTI functionality" },
	{ "tbgen1_enable_rfg", vTbgen1RFGTest, "Enable Tbgen1 RFG" },
	{ "tbgen2_enable_rfg", vTbgen2RFGTest, "Enable Tbgen2 RFG" },
	{ "tbgen1_disable_rfg", vTbgen1DisableRFGTest, "Disable Tbgen1 RFG" },
	{ "tbgen2_disable_rfg", vTbgen2DisableRFGTest, "Disable Tbgen2 RFG" },
	{ "tbgen1_ext_irq", vTbgen1RxAlignTimerTest, "Tbgen1 Rx Align Int Test" },
	{ "tbgen2_ext_irq", vTbgen2RxAlignTimerTest, "Tbgen2 Rx Align Int Test" },
	{ "tbgen2_tdd", vTestTbgen2TddTimer, "Test Tbgen2 Tdd Timers basic functionality " },
	{ "tbgen2_tdd_manual", vTestTbgen2TddTimerManual, "Test Tbgen2 Tdd Timers Maual mode basic functionality " },
#ifdef GEUL_DEMO_FLOAT_TEST
	{"float_test", vFloatTest, "App to test floating point value for different rounding function"},
#endif
#ifdef CONFIG_FLEXSPI
	{ "xspi_rd_write", vXspiReadWrite, "FlexSpi Memory Read Write Test" },
#endif
#ifdef BBDEV_IPC_MODE
#if GEUL_DEMO_FECA_BBDEV_TEST
	{ "feca_bbdev_se", vFecaBBDevTestSE, "Test case for FECA shared encode" },
	{ "feca_bbdev_sd", vFecaBBDevTestSD, "Test case for FECA shared decode" },
	{ "feca_bbdev_ce", vFecaBBDevTestCE, "Test case for FECA control encode" },
	{ "feca_bbdev_cd", vFecaBBDevTestCD, "Test case for FECA control decode" },
#if DCM_ARM_POLAR_DECODER_POC_EN
        { "feca_bbdev_dcm_ARM", vFecaBBDevTestDCM_ARM, "PoC DCM using ARM Polar Decoder"},
#endif
#endif
#endif
#if GEUL_DEMO_PEBM_PORT3_TEST
	/* PEBM port3 Test Cases */
	{ "pebm_port3", vGeulDemoPEBPort3Test, "Test PEBM port3 Functionality" },
#endif
#endif /* RELEASE_MODE */
#ifdef BBDEV_IPC_MODE
#ifdef BBDEV_IPC_MODE_DESIGN2
	{ "bbdev_ipc_du", vBbdevIpcTest_du, "Test IPC(L2->L1 , data via vspa/e200)" },
#else
        { "bbdev_ipc1", vBbdevIpcTest, "Test IPC(L2->L1 on Core 1 & L1->L2 on Core 2)" },
#endif
	{ "bbdev_ipc_raw", vBbdevIpcRawTest, "Test BBDEV IPC Raw mode" },
	/* FECA BBDEV test cases */
#endif

#ifdef GEUL_DEMO_ECPRI_START_TDD
	{ "start_ecpri_tdd_poll", vPollEcpriTddReady, " App to start  eCPRI ready TDD poll" },
#endif
	/* More Test Cases ...Add here. */
};


/* Returns TRUE for the commands with some restrictions */
bool bIsRestrictedCommand( const char * pcFunctionName )
{
	if( strcmp( pcFunctionName, "list" ) == 0 ||
		strcmp( pcFunctionName, "status" ) == 0 ||
		strcmp( pcFunctionName, "sys_stats" ) == 0 ||
		strcmp( pcFunctionName, "ipi_stats" ) == 0 )
	{
		return TRUE;
	}

	return FALSE;
}


/* ---------- Test Cases Declaration ---------- */
void vPrintValidTestCases( void )
{
	uint32_t uiIndex = 0;
	uint32_t uiNumTest = (sizeof( xFunctionMap ) / sizeof( xFunctionMap[ 0 ]));

	if ( uiNumTest <= 0 )
		return;

	PRINTF( "\n\rValid Test Cases" );
	PRINTF( "\n\r================\n\r" );
	for ( uiIndex = 0; uiIndex < uiNumTest; uiIndex++ )
	{
		PRINTF( "%-20s\t - %s\n\r", xFunctionMap[ uiIndex ].pcName, xFunctionMap[ uiIndex ].pcTestCaseDesc );
	}
}

/* Converts function name to function pointer via Hashtable */
int iCallFunction( const char *pcName, uint32 uiIterations )
{
	uint32_t uiIndex = 0;
	uint32_t uiSizeOfHashMap = ( sizeof( xFunctionMap ) / sizeof( xFunctionMap[ 0 ] ) );
	uint32_t uiIterIndex = 0;

	for ( uiIndex = 0; uiIndex < uiSizeOfHashMap; uiIndex++ )
	{
#ifndef RELEASE_MODE
		if (!strcmp( pcName, "gpiotoggle"))
		{
			if ( !bIsRestrictedCommand( pcName ) )
			{
				GpioModule_t gpioModule = (GpioModule_t) (uiIterations / 100);
				uint8_t ucPin = (uint8_t) (uiIterations % (gpioModule * 100));
				if ((uiIterations < 100) || (gpioModule > GPIO_MAX ) || (ucPin > GPIO_PIN_MAX))
				{
					log_info("\n\n Wrong format!! Please enter command in this format.\n\r\
						test gpiotoggle 0xF 123  (where 1: GPIO controller 23: Pin no)\n\r");
				}
				else
				{
					(void) gpioLedTestCase(gpioModule, ucPin);
				}
				return 0;
			}
		}
		else
#endif /* RELEASE_MODE */
		{
			if( !strcmp( xFunctionMap[ uiIndex ].pcName, pcName ) && xFunctionMap[ uiIndex ].pvFunc )
			{
				for( uiIterIndex = 0; uiIterIndex < uiIterations; uiIterIndex++)
				{
					if( !bIsRestrictedCommand( pcName ) )
					{
#ifndef BBDEV_DEFAULT_FUNC_CALL
						log_info( "\n\n\r[Test Framework] Iteration => %d Test Name => %s\n\r",
							( uiIterIndex + 1 ), xFunctionMap[ uiIndex ].pcName );
#endif
					}

					/* Call the test function pointer */
					xFunctionMap[ uiIndex ].pvFunc();
				}
				return 0;
			}
		}
	}

	return -1;
}
static TaskHandle_t xTFHandlerTask;
static char *pcTextForTfTask = "Test Framework task is running\r\n";

static char * pcFunctionName = NULL;
static uint32 uiIterations = 0;
static EventGroupHandle_t xTFEventGrpHndl;

void vTFEventCallback(enum IPIEventID eventID, void *userData, void *cookie)
{
#if GEUL_IPI_LATENCY_TEST
	ipi_latency = ulGetElapsedTimeMulticore(0);
	log_dbg("\n\rIPI Latency=%d",ipi_latency);
#endif
        //To avoid compilation errors
        (void) cookie;

        if(eventID == IPI_EVT_TESTFRAMEWORK) {
                if( userData != NULL )
                {
                        /* Extract function name to be called, and number of iterations */
                        pcFunctionName = ( ( TfIpiData_t * )userData )->pcFunctionName;
                        uiIterations = ( ( TfIpiData_t * )userData )->uiTestIteration;

        #if ENABLE_DEBUG_INFO
                        log_dbg("\n\r[Test Framework] Data received = %s\n\r", ( ( TfIpiData_t * )userData )->pcFunctionName );
        #endif   
                        xEventGroupSetBits( xTFEventGrpHndl, 0x01 );
                }
                else
                        log_err("\n\r[Test Framework] Failed to receive data\n\r", __func__);
        }
}


void vTestFrameworkTsk( void *pvParameters )
{
 /* To avoid compilation warning */
        ( void ) pvParameters;
#ifndef BBDEV_DEFAULT_FUNC_CALL
	void *userData;
#endif

        /* Register IPI_EVT_TESTFRAMEWORK */
	vIPIEventRegister(IPIGlobalEventID[IPI_EVT_TESTFRAMEWORK],&pxRxQueue[ IPI_EVT_TESTFRAMEWORK ], NULL, NULL);
        log_info("\n\r[Test Framework] Registering Test Framework IPI Event = %d", IPI_EVT_TESTFRAMEWORK);

        while( 1 )
        {
#ifdef BBDEV_DEFAULT_FUNC_CALL
#ifdef BBDEV_IPC_MODE_DESIGN2
		pcFunctionName = "bbdev_ipc_du";
#else
		pcFunctionName = "bbdev_ipc1";
#endif
		uiIterations = 1;
#else
                log_info("\n\r[Test_FrameWork] Ready to receive next command\n\r");
		xQueueReceive( pxRxQueue[ IPI_EVT_TESTFRAMEWORK ], &userData, portMAX_DELAY );

		pcFunctionName = ( ( TfIpiData_t * )userData )->pcFunctionName;
                uiIterations = ( ( TfIpiData_t * )userData )->uiTestIteration;
#endif
                /* Call corresponding function from hashtable */
                if( iCallFunction( pcFunctionName, uiIterations ) == -1 )
                {
                        log_err("\n\r[Test Framework] [Err] Not a Valid Test case. \n\r");
                        vPrintValidTestCases();
                }
#if defined(HOST_CLI_ENABLE) || (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
            send_host_cli_notification();
#endif

        };

}
void vInitTestFramework( void )
{
	/* Launch IPI handler thread (per core) */
	int	iRc = 0;
	u32 uiCurrentCore = ulMpicCurrentCore();
	char cTaskName[20] = {0,};
	uint32_t uiCoreIndex;

	/* Initialize test status bits */
	for( uiCoreIndex = 0; uiCoreIndex < get_soc_numcores(); uiCoreIndex++ ) {
		INIT_PREV_STATUS( uiCoreIndex, (u64)(-1) );
	}

	sprintf( cTaskName, "TFHndlr%d", uiCurrentCore );

	iRc = xTaskCreate(vTestFrameworkTsk, cTaskName, TEST_FRAMEWORK_TASK_STACKSIZE,
		pcTextForTfTask, TEST_FRAMEWORK_TASK_PRIORITY, &xTFHandlerTask);

	if( iRc != pdPASS )
	{
		log_info("[Test Framework] Failed to create Test Framework task...\n\r");
	}

  	xTFEventGrpHndl = xEventGroupCreate();
        if (!xTFEventGrpHndl)
        {
                log_err("[Test Framework] Failed to create Test Framework task event group\n\r");
        }

}

void vSetTestStatus( u32 iCoreId, u32 iBit )
{
	/* Check if PREV_STATUS is zero */
	if( !GET_PREV_STATUS( iCoreId, iBit ) )
	{
		/* If previous test had failed, then reset status bit */
		xTestStatus.uiGeulTestStatus[ iCoreId ] &= ~( ( u64 )1 << iBit );
	}
	else
	{
		/* If previous test was successful, then set status bit */
		xTestStatus.uiGeulTestStatus[ iCoreId ] |= ( ( u64 )1 << iBit );
	}
}

void vResetTestStatus( u32 iCoreId, u32 iBit )
{
	xTestStatus.uiGeulTestStatus[ iCoreId ] &= ~( ( u64 )1 << iBit );

	/* If fail is obtained even once, set the PREV_STATUS as zero */
	RESET_PREV_STATUS( iCoreId, iBit );
}

