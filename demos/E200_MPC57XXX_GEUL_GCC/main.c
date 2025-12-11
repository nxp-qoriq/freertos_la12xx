/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2019-2024 NXP
 *
 */

#include "FreeRTOS.h"
#include "ibr.h"
#include "soc.h"
#include "ipiQueue.h"
#include "Time.h"
#include "tmu.h"
#include "i2cAPI.h"
#include "la12xx_tbgen.h"
#include "ecc.h"
#include "pmc.h"
#include "mpu.h"
#include "qdma.h"
#ifdef ENABLE_SI551x
#include "sync_timing_device.h"
#include "sync_timing_device_cli.h"
#endif

#ifdef ENABLE_RF
#include "rf_dev.h"
#endif

#ifdef TESTFRAMEWORK_ENABLE
#include "test_framework.h"
#endif

#ifdef BBDEV_IPC_MODE
#include "bbdev_ipc.h"
#elif CPE_IPC
#include "ipc.h"
#endif

#ifdef FECA_ENABLED
#include "feca_main_lib.h"
#endif

#ifdef HAWK_ENABLED
#include "hawk.h"
#endif

#ifdef AGAVE
#include "agave_init.h"
#endif

#ifdef DCS_LS_ENABLED
#include "dcs.h"
#endif

#ifdef ADI_RF
#include "aib.h"
#endif

#include "vcxo.h"

#if (defined HOST_CLI_ENABLE) || (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
#include "FreeRTOS_CLI.h"
/* Variables used for host modem communication*/
static uint32_t rx_vars[4]  __attribute__ ((section (".shared.bss")));
static TaskHandle_t xCliCoreTask;
static uint32_t irq_counts = 0;
#endif
#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
#include "semphr.h"
#include "l1c_debug.h"
#include "l1c_fwk_tasks.h"
#endif

#ifdef APIPLAYER_ENABLED
#include "apiplayer.h"
#endif

#include "gul_host_if.h"

#ifdef WARMUP_ENABLE
#include "watchdog_api.h"
#endif


#if (defined LA12XX_DRIVER_PCI) || (defined LA12XX_DRIVER_PCI_LAT_FP)
#include "nxp-pcie.h"
static TaskHandle_t xPCIeTaskHandle;
volatile uint32_t ulPciDevInit  __attribute__ ((section (".smem"))) = 0;
#endif /* LA12XX_DRIVER_PCI || LA12XX_DRIVER_PCI_LAT_FP */

#ifdef CONFIG_L1C_ENABLE
void print_l1_ver(void);
void l1_tasks_init(void);
#else
#include "UARTCommandConsole.h"
/*
 * Register commands that can be used with FreeRTOS+CLI.  The commands are
 * defined in CLI-Commands.c.
 */
extern void vRegisterSampleCLICommands( void );
#ifdef DIORA_RF
extern void vRegisterDioraCLICommands( void );
#endif
#endif /* CONFIG_L1C_ENABLE */

#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
extern void init_l1c_refapp();
#endif

struct ibr_header ibr_h __attribute__ ((section (".hif.ibr")));
extern void vCoreMaskBasedWdogEnable(int mask);
/*Global variable*/
gul_mod_priv_t *pGulModPriv;

uint8_t crt_core_id;
uint32_t heap_size;
static volatile uint64_t cnt;
volatile struct debug_log_regs *pDbgLogRegs;
extern void init_dcs_host_if( void );
extern void vRedirectModemLogToHost(uint8_t core_id);
static TaskHandle_t xTMUTaskHandle;
volatile uint32_t core_started[GEUL_E200_CORE_GLOBAL_NUM] __attribute__ ((section(".smem")));
volatile uint32_t brd_ver __attribute__ ((section(".smem"))) = 0x0;

/* cOutputBuffer is used by FreeRTOS+CLI.  It is declared here so the
persistent qualifier can be used.  For the buffer to be declared here, rather
than in FreeRTOS_CLI.c, configAPPLICATION_PROVIDES_cOutputBuffer must be set to
1 in FreeRTOSConfig.h. */
char cOutputBuffer[ configCOMMAND_INT_MAX_OUTPUT_SIZE ] = { 0 };

volatile struct gul_hif * bsp_get_hif()
{
   return pGulModPriv->pHif;
}

uint32_t check_modem_log_to_host_enable(void)
{
	volatile struct gul_hif *hifx;

	hifx = bsp_get_hif();

	return SWAP_32(hifx->modem_host_uart);
}

ipc_metadata_t * bsp_get_ipc_md()
{
   return pGulModPriv->ipc_md;
}

gul_mod_priv_t * bsp_get_mod_priv()
{
	return pGulModPriv;
}

void vApplicationIdleHook()
{
	++cnt;
	out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].s_cnt_l, PRINT_64_LO(cnt));
	out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].s_cnt_h, PRINT_64_HI(cnt));
}
#if defined GEUL_LA1224
void vPrintSwitches(void)
{
	struct ccsr_dcsr *dcsr = NULL;
	uint32_t porsr1 = 0x0, porsr2 = 0x0, pci_ctrl;
	dcsr = (struct ccsr_dcsr *)CCSR_DCFG_BASE_ADDR;
	porsr1 = in_le32(&dcsr->ulPorsr1);
	porsr2 = in_le32(&dcsr->ulPorsr2);
	pci_ctrl = ((porsr2 >> 19) & SD_PRTCL_MASK);
	log_info("*******Power-on reset config: PORSR1: 0x%x PORSR20x%x\n\r", porsr1, porsr2);

	/* Parsing the values of PORSR1 and PORSR2
	 * for particular bits as decribed below
	 *  1. Take a mask with same number of bits as the number of
	 *	bits the functionality has
	 *  2. Left shift the mask till the position the functionality
	 *	is placed in register
	 *  3. AND the mask with the register to get the value of the
	 *	functionality
	 *  4. Compare with the expected values and print as per the result
	*/

	log_info("\tBOOTSRC:");
	switch (porsr1 & BOOT_SRC_MASK) {
		case BOOT_SRC_PEB:
			log_info("Preloaded PEB (pulldown resistors)\n\r");
			break;
		case BOOT_SRC_FSPI:
			log_info("FlexSPI Flash (GPIO_1[5] pulldown)\n\r");
			break;
		case BOOT_SRC_PCIe:
			log_info("PCIe1 Host Memory (no pulldowns)\n\r");
			break;
		case BOOT_SRC_RESERVED:
			log_info("Reserved\n\r");
			break;
		default:
			log_info("Invalid\n\r");
	}

	log_info("\tSYSCLK_SEL: %s\n\r", (porsr1 & SYS_PLL_ENABLE) ? "122.88MHz" : "Reserved");
	log_info("\tSystem PLL Clock: %sMhz\n\r", (porsr1 & SYS_PLL_ENABLE) ? "614.4" : "491.52");
	log_info("\tBOOT_HO_B: %s\n\r", (porsr1 & BOOT_HO_ENABLE) ? "inactive" : "active");

	switch(porsr1 & TBGEN_PLL_OP_MASK){
		case TBGEN_PLL_OP_ON:
			log_info("\tTBgen2_PLL: 1966.08 MHz, Tbgen1_PLL:491.52 MHz\n\r");
			break;
		case TBGEN_PLL_OP_2ON:
			log_info("\tTBgen2_PLL: 1966.08 MHz, Tbgen1_PLL:OFF\n\r");
			break;
		case TBGEN_PLL_OP_1ON:
			log_info("\tTBgen2_PLL: OFF, Tbgen1_PLL: 491.52 MHz\n\r");
			break;
		case TBGEN_PLL_OP_OFF:
			log_info("\tTBgen2_PLL: OFF, Tbgen1_PLL: OFF\n\r");
			break;
		default:
			log_info("\tTBgen_PLL: Reserved \n\r");
	}

	log_info("\tCFG_DELAY:");
	switch( porsr2 & CFG_SD_RX_DELAY_MASK) {
		case CFG_SD_RX_DELAY_100:
			log_info("100us\n\r");
			break;
		case CFG_SD_RX_DELAY_200:
			log_info("200us\n\r");
			break;
		case CFG_SD_RX_DELAY_300:
			log_info("300us\n\r");
			break;
		case CFG_SD_RX_DELAY_400:
			log_info("400us\n\r");
			break;
		case CFG_SD_RX_DELAY_600:
			log_info("600us\n\r");
			break;
		case CFG_SD_RX_DELAY_800:
			log_info("800us\n\r");
			break;
		case CFG_SD_RX_DELAY_1200:
			log_info("1200us\n\r");
			break;
		case 0x0:
			log_info("Disabled\n\r");
			break;
		default:
			log_info("Incorrect\n\r");
	}

	if (!(porsr2 & SD_PLLF_ENABLE))
		log_info("\tSerDes PLLF powered down\n\r");

	if (!(porsr2 & SD_PLLS_ENABLE))
		log_info("\tSerDes PLLS powered down\n\r");

	log_info("\tSerDes Ref clock: %sMhz\n\r", (porsr2 & SD_PLL_REF_CLK_ENABLE) ? "100" : "125");

	switch (pci_ctrl) {
		case 0:
			log_info("\t PCIe controller not enabled \n\r");
			break;
		case 1:
			log_info("\tPCIe Lane reversal: (%s)\n\r", (porsr2 & PCIE_LREV_NOT_DONE) ? "Not Done" : "Done");
			log_info("\tPCIe1 (%s)\n\r", (porsr2 & PCIE1_GEN_8GBPS_ENABLE) ? "Gen3 8Gbps" : "Gen2 5Gbps");
			log_info("\tPCIe2 (%s)\n\r", "Not Enabled");
			break;
		default:
			if ((in_le32((volatile uint32_t *) PEX_PF0_DBG(0)) & 0xff) == 0x11) {

				log_info("\tPCIe Lane reversal: (%s)\n\r", (porsr2 & PCIE_LREV_NOT_DONE) ? "Not Done" : "Done");
				log_info("\tPCIe1 (%s)\n\r", (porsr2 & PCIE1_GEN_8GBPS_ENABLE) ? "Gen3 8Gbps" : "Gen2 5Gbps");
			}
			if ((in_le32((volatile uint32_t *) PEX_PF0_DBG(1)) & 0xff) == 0x11) {
				log_info("\tPCIe2 (%s)\n\r", (porsr2 & PCIE2_GEN_8GBPS_ENABLE) ? "Gen3 8Gbps" : "Gen2 5Gbps");
				log_info("\tPCIe2 Function as: (%s)\n\r", (porsr2 & PCIE2_RC_ENABLE) ? "RC" : "EP");
			} else 
				log_info("\tPCIe2 (%s)\n\r", "Not Active");
			break;
	}
}
#endif

#if (CONFIG_IDLE_CALIBRATE)
static void prvGetStartIdleStamp()
{
	static uint8_t preset = 0;
	if (preset == 0) {
		//do stamp1, copy cnt to HIF.
		out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt1_l, PRINT_64_LO(cnt));
		out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt1_h, PRINT_64_HI(cnt));
		log_dbg("IdleHOOOK 64bit PRESET : 0x%x 0x%x \n\r", in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt1_h),
			in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt1_l) );
		preset = 1;
		out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].status, 1);
	} else {
		//do stamp2, copy cnt to HIF
		out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt2_l, PRINT_64_LO(cnt));
		out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt2_h, PRINT_64_HI(cnt));
		log_dbg("IdleHOOOK 64bit Set : 0x%x 0x%x \n\r", in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt2_h),
			in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].strt2_l) );
		preset= 0;
		out_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].status, 2);
	}
}
#endif /* CONFIG_IDLE_CALIBRATE */

static void prvBSPInit(gul_mod_priv_t *pGulModPriv)
{
	uint32_t i = 0;
	uint32_t uMSIAddrVal = 0;
	volatile uint32_t *pMsiAddrReg;
	volatile uint32_t *pMsiDataAddr;
	struct gul_msi_info *pMsiInfo;
	uint32_t SCRATCH_10 = (DCFG_BASE_ADDR + DCFG_SCRATCH10_OFFSET);
	uint32_t SCRATCH_11 = (DCFG_BASE_ADDR + DCFG_SCRATCH11_OFFSET);
	extern uint32_t __SMEM_DATA_START;
	extern uint32_t __SMEM_DATA_SIZE;
	mod_mem_region_t *mem_regions = pGulModPriv->mem_region;

	mem_regions->addr_p	= (volatile uint32_t)CORE0_DMEM_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)CORE0_DMEM_ADDR;
	mem_regions->size	= CORE_DMEM_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)CORE1_DMEM_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)CORE1_DMEM_ADDR;
	mem_regions->size	= CORE_DMEM_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)CORE2_DMEM_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)CORE2_DMEM_ADDR;
	mem_regions->size	= CORE_DMEM_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)CORE3_DMEM_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)CORE3_DMEM_ADDR;
	mem_regions->size	= CORE_DMEM_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)CORE4_DMEM_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)CORE4_DMEM_ADDR;
	mem_regions->size	= CORE_DMEM_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)CORE5_DMEM_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)CORE5_DMEM_ADDR;
	mem_regions->size	= CORE_DMEM_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)&__SMEM_DATA_START;
	mem_regions->addr_v	= (volatile uint32_t)&__SMEM_DATA_START;
	mem_regions->size	= (volatile uint32_t)&__SMEM_DATA_SIZE;
	(++mem_regions)->addr_p	= (volatile uint32_t)PEBM_BASE_ADDR;
	mem_regions->addr_v	= (volatile uint32_t)PEBM_BASE_ADDR;
	mem_regions->size	= PEBM_SIZE;

	pGulModPriv->mem_region[ MOD_MEM_FECA_AXI_SLAVE ].addr_p = ( volatile uint32_t ) FECA_RAM_BASE_ADDR;
	pGulModPriv->mem_region[ MOD_MEM_FECA_AXI_SLAVE ].addr_v = ( volatile uint32_t ) FECA_RAM_BASE_ADDR;
	pGulModPriv->mem_region[ MOD_MEM_FECA_AXI_SLAVE ].size = FECA_RAM_SIZE;
	pGulModPriv->mem_region[ MOD_MEM_FECA_APB_SLAVE ].addr_p = ( volatile uint32_t ) FECA_IP_REGS;
	pGulModPriv->mem_region[ MOD_MEM_FECA_APB_SLAVE ].addr_v = ( volatile uint32_t ) FECA_IP_REGS;
	pGulModPriv->mem_region[ MOD_MEM_FECA_APB_SLAVE ].size = FECA_CCSR_SIZE;

	pGulModPriv->pHif = &ipc_hif_area;

	if (crt_core_id == GEUL_E200_MASTER_CORE)
		memset((void *)pGulModPriv->pHif, 0, sizeof(struct gul_hif));

	out_le32(&pGulModPriv->pHif->hif_ver,
		GUL_VER_MAKE(GUL_HIF_MAJOR_VERSION, GUL_HIF_MINOR_VERSION));

	/* Initialize MSI */
	pMsiAddrReg = (uint32_t *) ((uint32_t) PCIE1_CONTROL_BASE_ADDR + PCIE_MSI_ADDR_REG);
	pMsiDataAddr = (uint32_t *) ((uint32_t) PCIE1_CONTROL_BASE_ADDR + PCIE_MSI_DATA_REG_1);
	pMsiInfo = &pGulModPriv->msi_info[MSI_IRQ_MUX];

	/* According to PCI bus standerd multiple MSIs are allocated consecutively*/
	uMSIAddrVal = (in_be32(pMsiAddrReg) & 0x3FF);
	PRINTF("%s: Handling HIF 0x%p MSI addr 0x%x\r\n", __func__,
		pGulModPriv->pHif, GUL_EP_TOHOST_MSI_PHY_ADDR | uMSIAddrVal);
#ifdef LS1046_HOST_MSI_RAISE
	/*This method to initialize MSI structure is non-standard and dedicated for
	  LS1046 host */
	int val=0;
	for (i = 0; i < GUL_MSI_MAX_CNT; i++) {
		pMsiInfo[i].addr = (GUL_EP_TOHOST_MSI_PHY_ADDR | uMSIAddrVal);
		val = in_be32(pMsiDataAddr);
		pMsiInfo[i].data = ((val + i) << 2);
		DPRINTF("%s: MSI init[%d], addr 0x%x, data 0x%x\r\n", __func__,
				i, pMsiInfo[i].addr, pMsiInfo[i].data);
	}
#else
	/*This method to initialize MSI structure is non-standard and dedicated for
	  LS1046 host */
	for (i = 0; i < GUL_MSI_MAX_CNT; i++) {
		pMsiInfo[i].addr = (GUL_EP_TOHOST_MSI_PHY_ADDR | uMSIAddrVal);
		pMsiInfo[i].data = (IN_32(pMsiDataAddr) + i);
		DPRINTF("%s: MSI init[%d], addr 0x%x, data 0x%x\r\n", __func__,
				i, pMsiInfo[i].addr, pMsiInfo[i].data);
	}
#endif

	/* update soc_rev and soc_numcores in HIF area */
	pGulModPriv->pHif->soc_rev = get_soc_version();
	pGulModPriv->pHif->soc_numcores = get_soc_numcores();

	uint32_t hif_offset = (uint32_t)&ipc_hif_area - PEBM_BASE_ADDR;

	/* Update HIF offset and size in scratch registers */
	out_le32(SCRATCH_10, hif_offset);
	out_le32(SCRATCH_11, sizeof(struct gul_hif));

	PRINTF("%s: HIF offset 0x%x, size 0x%x\r\n", __func__, hif_offset, sizeof(struct gul_hif));
}

#if (defined HOST_CLI_ENABLE) || (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
void send_host_cli_notification(void)
{
	struct gul_msi_info * pMsiInfo;
	uint32_t msiLine = 0xFFFFFFFF;

	pMsiInfo = &pGulModPriv->msi_info[ MSI_IRQ_MUX ];
	msiLine = in_le32(&pGulModPriv->pHif->msi_cli);
	if (msiLine < GUL_MSI_MAX_CNT)
	{
		out_le32(pMsiInfo[ msiLine ].addr, pMsiInfo[ msiLine ].data);
	}
}

void send_host_notification(uint32_t msg_id, uint32_t data, uint32_t data2, uint32_t data3)
{
	struct gul_msi_info * pMsiInfo;
	uint32_t msiLine = 0xFFFFFFFF;

	pMsiInfo = &pGulModPriv->msi_info[ MSI_IRQ_MUX ];
	msiLine = in_le32(&pGulModPriv->pHif->msi_cli);

	out_le32(&pGulModPriv->pHif->cli_geul_mbox.msg_id, msg_id);
	out_le32(&pGulModPriv->pHif->cli_geul_mbox.data, data);
	out_le32(&pGulModPriv->pHif->cli_geul_mbox.data2, data2);
	out_le32(&pGulModPriv->pHif->cli_geul_mbox.data3, data3);

	if (msiLine < GUL_MSI_MAX_CNT)
	{
		out_le32(pMsiInfo[ msiLine ].addr, pMsiInfo[ msiLine ].data);
	}
}

bool_t cli_msi_irq_handler(uint32_t ulIrqNo, void *pdata)
{
	uint32_t msiOffset = 0x10;
	uint32_t msiNumber = ulIrqNo - MSI_INTR_START;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	TaskHandle_t xTaskHandle = pdata;

	irq_counts++;
	rx_vars[0] = in_le32(&pGulModPriv->pHif->cli_host_mbox.msg_id);
	rx_vars[1] = in_le32(&pGulModPriv->pHif->cli_host_mbox.data);
	rx_vars[2] = in_le32(&pGulModPriv->pHif->cli_host_mbox.data2);
	rx_vars[3] = in_le32(&pGulModPriv->pHif->cli_host_mbox.data3);

	xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR(xTaskHandle, 1, eSetBits, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	mpic_in32(MPIC_REGS_MSIR0 + msiNumber * msiOffset);
	return 0;
}

void cli_register_host_interrupt(void * pdata)
{
	int ret;
	ret = lRegisterIrq((uint32_t)(MSI_INTR_START + HOST_CLI_IRQ), cli_msi_irq_handler, pdata);
	if (ret < 0)
	{
		PRINTF("IRQ register error!\r\n");
		return;
	}

	bMpicEnable(DEVICE_SHARE_MESSAGE, HOST_CLI_IRQ);
}

void host_cli_irq_task_main(void *pvParameters)
{
	uint32_t ulNotifiedValue;
	BaseType_t xReturned;
	char *pcOutputString;
	pcOutputString = FreeRTOS_CLIGetOutputBuffer();
	(void) pvParameters;

	cli_register_host_interrupt( (void *) xCliCoreTask);

	while (1)
	{
		/* Wait to be notified of an interrupt. */
		xTaskNotifyWait(0, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);
#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
		if (rx_vars[0] == GUL_CLI_MSG_ID_L1REFAPP && rx_vars[2] != 0) {
			if (vIPISendData(rx_vars[2], IPI_EVT_L1C_REFAPP_CLI, (void *)rx_vars[1]) == pdFALSE) {
				log_err("ERROR: API Player IPI send data failed\r\n");
			}
		} else 
#endif
		{
			PRINTF("\r\n(host)>%s\r\n", (char *)rx_vars[1]);
			do{
				pcOutputString[0] = '\0';
				/* Get the next output string from the command interpreter. */
				xReturned = FreeRTOS_CLIProcessCommand( (char *)rx_vars[1], pcOutputString, configCOMMAND_INT_MAX_OUTPUT_SIZE );
				/* Write the generated string to the UART. */
				PRINTF("%s",pcOutputString);
			} while( xReturned != pdFALSE );
			if (rx_vars[0] == GUL_CLI_MSG_ID_L1REFAPP)
				send_host_cli_notification();
		}
	}
}

void init_cli_host_if()
{
#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
	l1c_create_task(L1_CORE_0, L1C_HOST_IRQ_TASK, "HOST_IF", TICK_DISABLE,
				L1C_CLI_TASK_PRIORITY, CLI_TASK_STACK_SIZE,  host_cli_irq_task_main);
	xCliCoreTask = tasks_map.tasks[L1C_HOST_IRQ_TASK].task_handle;

#else
	xTaskCreate(host_cli_irq_task_main, ( const char * const ) "CLI",
					CLI_TASK_STACK_SIZE, NULL, HOST_CLI_TASK_PRIORITY,
					( TaskHandle_t * ) &xCliCoreTask);
#endif // HOST_CLI_ENABLE
}
#endif //(defined HOST_CLI_ENABLE) || (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)

#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
/*only creating it for L1C_REFAPP and RUDEMO to handle cli command on other cores */
void host_cli_ipi_task_main(void *pvParameters)
{
	void *cli_command;
	BaseType_t xReturned;

	UNUSED(pvParameters);

	vIPIEventRegister(IPIGlobalEventID[IPI_EVT_L1C_REFAPP_CLI], &pxRxQueue[IPI_EVT_L1C_REFAPP_CLI], NULL, NULL);
	PRINTF("Registering L1C RefApp IPI Event = %d \r\n", IPI_EVT_L1C_REFAPP_CLI);

	while(1)
	{
		xQueueReceive(pxRxQueue[IPI_EVT_L1C_REFAPP_CLI], &cli_command, portMAX_DELAY);

		e200_trace(E200_TRACE_MSG_IPI_HOST_CLI, E200_TRACE_PARAM_END);
		PRINTF("\r\n(host)>%s\r\n", (char *)cli_command);

		do {
			cOutputBuffer[0] = '\0';
			xReturned = FreeRTOS_CLIProcessCommand((char *)cli_command, cOutputBuffer, configCOMMAND_INT_MAX_OUTPUT_SIZE);
			PRINTF("%s", cOutputBuffer);
		} while( xReturned != pdFALSE );
		send_host_cli_notification();
	}
}

void create_core_cli_task ()
{
	TaskHandle_t cli_task_handle;
	int	ret = 0;

	ret = xTaskCreate(host_cli_ipi_task_main, "HostCLI_IPI", DEFAULT_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, &cli_task_handle);
	if (ret != pdPASS)
		PRINTF("Failed to create HostCLI_IPI task on core %d\n\r", crt_core_id);
}
#endif //(defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)

static void vInitDmem()
{
	/* DMEM segment is at IBR Header entry #0 and is the same for all cores */

	uint8_t *src = (uint8_t *)ibr_h.sgtable[0].src;
#if GUL_QDMA_FOR_BOOT_ENABLE == 1
	uint8_t *dst = (uint8_t *)(CORE_DMEM_ADDR);
#else
	uint8_t *dst = (uint8_t *)ibr_h.sgtable[0].dest;
#endif
	uint32_t len = ibr_h.sgtable[0].len;

#ifdef GEUL_BOOT_MODE_XSPI
//TODO - Once Flexspi driver will available, copy the dmem content from flexspi to dmem of other cores
	extern char dmem_start __asm__("__dmem_start");
	extern char load_dmem_end   __asm__("__bss_start");
	extern uint32_t __PEB_END;
	extern uint32_t __DMEM_START;

	src = (uint8_t *)((uint32_t)&__PEB_END);
	dst = (uint8_t *)((uint32_t)&__DMEM_START);
	len = (uint32_t)&load_dmem_end - (uint32_t)&dmem_start;
#endif
	for (uint32_t i = 0; i < len; ++i)
		dst[i] = src[i];
}

static void vInitPebBss()
{
	extern char shared_bss_start __asm__ ("__shared_bss_start");
	extern long __shared_bss_size;
/*
 * This guard is necessary because __cshared_bss_start is defined only in
 * geul_e200_la1238cpe.ld
 * TODO: Remove this guard here and in mpu.c when space for this section is
 * allocated for other platforms as well.
 */
#ifdef GEUL_LA1238CPE
	extern char cshared_bss_start __asm__ ("__cshared_bss_start");
	extern long __cshared_bss_size;
	memset(&cshared_bss_start, 0, (uint32_t)&__cshared_bss_size);
#endif

	memset(&shared_bss_start, 0, (uint32_t)&__shared_bss_size);
}

int iCheckCoreSmemTextAvail()
{
	extern char core_smem_text_start __asm__("__core_smem_text_start");
	extern char core_smem_text_end   __asm__("__core_smem_text_end");

	uint32_t core_smem_text_size = (uint32_t)&core_smem_text_end - (uint32_t)&core_smem_text_start;
	if( core_smem_text_size == 0 )
	{
		return 0;
	}

	return 1;
}

int iCheckCoreSmemAvail()
{
	extern char core_smem_start __asm__("__core_smem_start");
	extern char core_smem_end   __asm__("__core_smem_end");

	uint32_t core_smem_size = (uint32_t)&core_smem_end - (uint32_t)&core_smem_start;
	if( core_smem_size == 0 )
	{
		return 0;
	}

	return 1;
}

void vPopulateRegion(enum mem_type_id mem_id, uint32_t start_addr, uint32_t end_addr)
{
	uint32_t index = 0;
	uint8_t * ptr;
	uint8_t *src;
	uint8_t *dst;
	uint32_t len;

	if (!(mem_id < MOD_MEM_TYPE_END)) {
		PRINTF("Invalid region (%d) func:%s\r\n", mem_id, __func__);
		return;
	}

	ptr = (uint8_t *)((uint32_t)start_addr);
	for(index = 0; index < ibr_h.sgentries_num; index++) {
		if((uint32_t)ptr == ibr_h.sgtable[index].dest) {
			break;
		}
	}

	if(index == ibr_h.sgentries_num) {
		PRINTF("Info: Memory entry not found for region (%d) func:%s\r\n", mem_id, __func__);
		return;
	}

	src = (uint8_t *)ibr_h.sgtable[index].src;
	dst = (uint8_t *)ibr_h.sgtable[index].dest;
	len = ibr_h.sgtable[index].len;

	if( ( uint32_t )dst < start_addr || ( uint32_t )dst > end_addr )
	{
		PRINTF( "Invalid dst address %p for region (%d) in func:%s\r\n", dst, mem_id, __func__ );
		return;
	}

	PRINTF("Copying data @ dst: %p, len: %u for region(%d) \r\n", dst, len, mem_id);
	memcpy( dst, src, len );
}

void vApplicationMallocFailedHook( void )
{
	PRINTF("\r\n***** : MALLOC FAILED *****\r\n");
	PRINTF("\r\n Total Heap Memory: %d Available Heap Memory: %d\n\r", configTOTAL_HEAP_SIZE, xPortGetFreeHeapSize());
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	PRINTF("\r\n***** %s: STACK OVERFLOW *****\r\n", pcTaskName);
	/* Run trap instruction to generate program exception. */
	__asm__ __volatile__ ("twi 15,%0,%1" : : "r" (0x10), "i" (0x10): );
}

static int host_mem_atu_create(gul_mod_priv_t *pGulModPriv)
{
		int i;
	volatile struct gul_hif *pHif;

	pHif = pGulModPriv->pHif;

	for(i = HOST_MEM_HUGE_PAGE_BUF; i < HOST_MEM_FECA_AXI_SLAVE; i++)
		{
				/* Update address and size in priv structure */
				pGulModPriv->mem_region[i + MOD_MEM_HUGE_PAGE_BUF].addr_p = (uint32_t) (in_le32(&pHif->host_regions[i].mod_phys_l));
				pGulModPriv->mem_region[i + MOD_MEM_HUGE_PAGE_BUF].addr_v = (uint32_t) (in_le32(&pHif->host_regions[i].mod_phys_l));
				pGulModPriv->mem_region[i + MOD_MEM_HUGE_PAGE_BUF].size = (uint32_t) (in_le32(&pHif->host_regions[i].size_l));
		}

		return 0;
}
#ifdef WARMUP_ENABLE
void vGeulWARMUPTask(void *pvParameters)
{
       (void) pvParameters;
       while(1)
               lava_feca_app();
}
#endif

void vGeulTMUTask( void *pvParameters )
{

#ifndef WARMUP_ENABLE
	BaseType_t xResult;
	uint32_t ulNotifiedValue;
#endif

	(void) pvParameters;

#ifdef WARMUP_ENABLE
	volatile struct gul_hif *pHif = pGulModPriv->pHif;
	int32_t warmup_temp = in_le32(&pHif->warmup_info.warmup_temp);
	uint32_t timeout = in_le32(&pHif->warmup_info.warmup_timeout);
	uint32_t warmup_poll_time = in_le32(&pHif->warmup_info.warmup_poll_intvl);
	uint32_t count = warmup_poll_time;

	log_info("threshold temp: %d curr temp: %d timeout_val: %d warm_poll_intvl: %d\n\r",
					warmup_temp, mtdGetTemp(), timeout, warmup_poll_time);
	while(timeout--)
	{
		if((mtdGetTemp() >= warmup_temp) || (!timeout))
			break;

		vTaskDelay(1000);
		if(!(count--)){
			log_info("current temp: %d remaining time :%d\n\r", mtdGetTemp(), timeout);
			count = warmup_poll_time;
		}
	}
	if((mtdGetTemp() >= warmup_temp) || (!timeout)){
		out_le32(&pHif->warmup_info.warmup_current_temp, mtdGetTemp());
		out_le32(&pHif->warmup_info.warmup_flags, GUL_WDOG_WARMUP_MODEM_RESET);
		while(!CHK_HIF_HOST_RDY(pHif, HIF_HOST_PROBE_COMPLETE));
		vWatchdogStart(10,NULL);
	}
#else
	tmuRegisterHostinterrupt( (void *) xTMUTaskHandle );

	while ( 1 )
	{
		/* Wait to be notified of an interrupt. */
		xResult = xTaskNotifyWait( pdFALSE, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);

		if( xResult == pdPASS )
		{
			 if( ulNotifiedValue )
			 {
				 tmuEnableInterrupt();
			 }
		}
	}
#endif
}

#if (CONFIG_IDLE_CALIBRATE)
void vIdleCalib( void *pvParameters )
{
	(void) pvParameters;

	vTaskDelay(15000);
	/* All pending task like VSPA load to finish by this time*/

	/* Begin idle stamp */
	PRINTF("Calibrated value at 0sec: s_cnt_h:0x%x s_cnt_l:0x%x\n\r",
			in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].s_cnt_h),
			in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].s_cnt_l));
	PRINTF("Profile 1sec\n\r");
	prvGetStartIdleStamp();
	vTaskDelay(1000);
	prvGetStartIdleStamp();

	PRINTF("Profile 10sec\n\r");
	prvGetStartIdleStamp();
	vTaskDelay(10000);
	prvGetStartIdleStamp();

	PRINTF("Profile 271sec\n\r");
	prvGetStartIdleStamp();
	vTaskDelay(271000);
	prvGetStartIdleStamp();

	PRINTF("Calibrated value after 282 sec: s_cnt_h:0x%x s_cnt_l:0x%x\n\r",
			in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].s_cnt_h),
			in_le32(&pGulModPriv->pHif->stats.cpuidle_stats[crt_core_id].s_cnt_l));

	PRINTF("Delete Calib\n\r");
	vTaskDelete(NULL);
}
#endif /* CONFIG_IDLE_CALIBRATE */


#if (defined LA12XX_DRIVER_PCI) || (defined LA12XX_DRIVER_PCI_LAT_FP)
void vGeulPCIeTask( void *pvParameters )
{
	int32_t status;
	uint32_t ulNotifiedValue;

	(void) pvParameters;
#ifdef LA12XX_DRIVER_PCI_LAT_FP
		status = iPcieCheckAndInitCtrl (PCIE_2, (void *) xPCIeTaskHandle);
		if (status != pdTRUE)
			log_err ("%s: PCIE%d Initialization failed: status(0x%x) \r\n",
					__func__,GEUL_PCIE_ID(PCIE_2), status);
		else
			out_le32(&pGulModPriv->pHif->pcie2_link, 1);
#else
	uint32_t i;
	for (i = 0; i < PCIE_MAX; i++) {
		status = iPcieCheckAndInitCtrl (i, (void *) xPCIeTaskHandle);
		if (status != pdTRUE)
			log_err ("%s: PCIE%d Initialization failed: status(0x%x) \r\n",
					__func__,GEUL_PCIE_ID(i), status);
		else if (i == PCIE_2)
			out_le32(&pGulModPriv->pHif->pcie2_link, 1);
	}
#endif
	ulPciDevInit = 1;

	while (1) {
		status = xTaskNotifyWait( pdFALSE, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);
		if( status == pdPASS )
		{
				PcieHandleInterrupt ((uint8_t) ulNotifiedValue);
		}
	}

	vTaskDelete(NULL);
}
#endif //LA12XX_DRIVER_PCI||LA12XX_DRIVER_PCI_LAT_FP

int iCheckSmemTextAvail()
{
	extern char smem_text_start __asm__("__smem_text_start");
	extern char smem_text_end   __asm__("__smem_text_end");

	uint32_t smem_text_size = (uint32_t)&smem_text_end - (uint32_t)&smem_text_start;
	if( smem_text_size == 0 )
	{
		return 0;
	}

	return 1;
}

int iCheckSmemAvail()
{
	extern char smem_start __asm__("__smem_start");
	extern char smem_end   __asm__("__smem_end");

	uint32_t smem_size = (uint32_t)&smem_end - (uint32_t)&smem_start;
	if( smem_size == 0 )
	{
		return 0;
	}

	return 1;
}

#ifdef GEUL_LA1224
int iGetCurrentVdd()
{
	int32_t iRet;
	int32_t iVcode = 0, iVdd = 0;
	uint8_t ucData = VID_CHANNEL_EN;
	uint8_t ucVidAddr = VID_BASE_ADDRESS;
	uint8_t brd_version;
	volatile struct gul_hif *pHif;

	pHif = pGulModPriv->pHif;
	/*
	 * If code is not running LA1224, skip accessing VID.
	 */
	brd_version=get_host_board_rev();
	brd_ver = brd_version;

	switch(brd_version)
	{
	case GEUL_HOST_REVA_VAL:
		break;
	case GEUL_HOST_REVB_VAL:
		break;
	case GEUL_HOST_REVC_VAL:
		ucVidAddr = 0x09;
		PRINTF("%s : Not supported on REVC board\r\n", __func__);
		return 0;
	/* Supported only on NXP LA1224 boards*/
	default:
		PRINTF("%s : Not supported on this board\r\n", __func__);
		return 0;
	}

	/* Set channel number on I2C Mux to select VID */
	iRet = iI2C_Write( I2C1_BASE_ADDR, PCA9547PWMUX_BASE_ADDRESS, 0x0, 1, &ucData, 1);
	if( iRet < 0 )
		goto done;

	ucData = VID_PAGE_0;
	iRet = iI2C_Write( I2C1_BASE_ADDR, ucVidAddr, VID_PAGE_SEL, 1, &ucData, 1);

	if( iRet < 0 )
		goto done;

	iRet = iI2C_Read( I2C1_BASE_ADDR, ucVidAddr, VID_READ_VOUT, 1, (void *)&iVcode, 2);
	iVdd = SWAP_32(iVcode);

	if( iRet < 0 )
		goto done;

	/* reset I2C mux channel */
	ucData = 0;
	iRet = iI2C_Write( I2C1_BASE_ADDR, PCA9547PWMUX_BASE_ADDRESS, 0x0, 1, &ucData, 1);
	if( iRet < 0 )
		goto done;

	iRet = ((iVdd * 1000) + (4096 - 1))/4096;
done:
	return iRet;
}
#endif

int main( void )
{
	int iRc = 0;
	int ipc_retval;
	int iHsDcsClk = 0;
	volatile struct gul_hif *pHif;

#ifndef WARMUP_ENABLE
	uint32_t dbgcount = 0;
#endif

	extern uint32_t heap_start      __asm__ ("end");
	extern uint32_t heap_start_libc __asm__ ("_heap_end_ptr");

#ifdef GEUL_BOOT_MODE_XSPI
	extern char dmem_start __asm__("__dmem_start");
	extern char load_dmem_end   __asm__("__bss_start");
#endif
	extern uint32_t __HEAP_SIZE;

	uint32_t *heap_end_ptr = (uint32_t *)&heap_start_libc;

	/* Chop the global heap area into GEUL_E200_CORE_GLOBAL_NUM smaller pieces, one for each e200 core.
	 * Also tell libc about that. This must be done before the first use of malloc.
	 */
	heap_size = (uint32_t)&__HEAP_SIZE / GEUL_E200_CORE_GLOBAL_NUM;
	crt_core_id = (uint8_t)ulMpicCurrentCore();
	*heap_end_ptr = (uint32_t)&heap_start + crt_core_id * heap_size;
//	PRINTF("Build on: \nDate:\"%s\"at TIME:\"%s\" \n\r", __DATE__, __TIME__);

	core_started[crt_core_id] = 0;
	vMpuEnable();

	if (crt_core_id == GEUL_E200_MASTER_CORE)
	{
		vCreateMpuEntry(MPU_REGION_SMEM_TEXT_DATA, SMEM_BASE_ADDR, SMEM_TEXT_END_ADDR);
		vInitPebBss();
		vEccDisable();
		vInitSmem();
		vInitSmemText();
		vDeleteMpuEntry( MAS0_SEL_1 | MAS0_ESEL_DATA_11 );

		if ( __get_soc_revision() == GEUL_SVR_REVB_VAL ) {
			vCreateMpuEntry(MPU_REGION_CORE_SMEM_TEXT_DATA, CORE_SMEM_BASE_ADDR, CORE_SMEM_TEXT_END_ADDR);
			vInitCoreSmem();
			vInitCoreSmemText();
			vDeleteMpuEntry( MAS0_SEL_1 | MAS0_ESEL_DATA_11 );
		}
		vEccEnable();
	}
	else // the master core has the DMEM already initialized by the bootrom
	{
#ifdef GEUL_BOOT_MODE_XSPI
		vCreateMpuEntry( MPU_REGION_FSPI_DMEM, PEBM_BASE_ADDR + PEBM_SIZE, PEBM_BASE_ADDR + PEBM_SIZE + ( (uint32_t)&load_dmem_end - (uint32_t)&dmem_start) );
#endif
		vInitDmem();
#ifdef GEUL_BOOT_MODE_XSPI
		vDeleteMpuEntry( MAS0_SEL_1 | MAS0_ESEL_DATA_11 );
#endif
	}

	vBoardEarlyInit(crt_core_id);

	vSocInit(crt_core_id);
	PRINTF("\r\nInside main E200 Core: %u\r\n", crt_core_id);
	if (crt_core_id == GEUL_E200_MASTER_CORE)
	{
		if( iCheckSmemTextAvail() )
		{
		    vCreateMpuEntry(MPU_REGION_SMEM_TEXT_DATA, SMEM_TEXT_BASE_ADDR, SMEM_TEXT_END_ADDR);
		    vPopulateRegion(MOD_MEM_TYPE_SMEMTEXT, SMEM_TEXT_BASE_ADDR, SMEM_TEXT_END_ADDR);
		    vDeleteMpuEntry(MAS0_SEL_1 | MAS0_ESEL_DATA_11);
		}

		if( iCheckSmemAvail() )
		{
		    vCreateMpuEntry(MPU_REGION_SMEM_DATA, SMEM_BASE_ADDR, SMEM_END_ADDR);
		    vPopulateRegion(MOD_MEM_TYPE_SMEM, SMEM_BASE_ADDR, SMEM_END_ADDR);
		    vDeleteMpuEntry(MAS0_SEL_1 | MAS0_ESEL_DATA_11);
		}

		if ( get_soc_revision() == GEUL_SVR_REVB_VAL ) {
			if( iCheckCoreSmemTextAvail() )
			{
				vCreateMpuEntry(MPU_REGION_CORE_SMEM_TEXT_DATA, CORE_SMEM_TEXT_BASE_ADDR, CORE_SMEM_TEXT_END_ADDR);
				vPopulateRegion(MOD_MEM_TYPE_CORESMEMTEXT, CORE_SMEM_TEXT_BASE_ADDR, CORE_SMEM_TEXT_END_ADDR);
				vDeleteMpuEntry(MAS0_SEL_1 | MAS0_ESEL_DATA_11);
			}

			if( iCheckCoreSmemAvail() )
			{
				vCreateMpuEntry(MPU_REGION_CORE_SMEM_DATA, CORE_SMEM_BASE_ADDR, CORE_SMEM_END_ADDR);
				/* This section is not populated currently due to capping of 8 sections */
				vPopulateRegion(MOD_MEM_TYPE_CORESMEM, CORE_SMEM_BASE_ADDR, CORE_SMEM_END_ADDR);
				vDeleteMpuEntry(MAS0_SEL_1 | MAS0_ESEL_DATA_11);
			}
		}
	}

	pGulModPriv = pvGeulMalloc(sizeof(gul_mod_priv_t));
	if (unlikely(!pGulModPriv)) {
		PRINTF("pGulModPriv alloc failed. going for while(true)\r\n");
		iRc = -12;		// No ERRNO defined.
		goto out;
	}

	memset(pGulModPriv, 0, sizeof(gul_mod_priv_t));

	/*IMPORTANT XXX: DO NOT CALL log_*() before prvBSPInit(), use PRINTF instead.*/
	prvBSPInit( pGulModPriv );

	/* in case PCIe host not running: Independent mode */
	pHif = pGulModPriv->pHif;
	pDbgLogRegs = &pHif->dbg_log_regs[crt_core_id];

	PMC_START();
	PMC_SET_EVENT(PMR_PMLCa0, PMLCa_EVENT_PROC_CYCLES);
	PMC_SET_EVENT(PMR_PMLCa1, PMLCa_EVENT_PIPELINE_STALLS);
	PMC_SET_EVENT(PMR_PMLCa2, PMLCa_EVENT_PIPELINE_STALLS_2);
	PMC_SET_EVENT(PMR_PMLCa3, PMLCa_EVENT_ST_BUF_FULL_STALLS);

	if (crt_core_id == GEUL_E200_MASTER_CORE){
		RESET_CORE_STATUS();
	}

	SET_CORE_STATUS_RUNNING(crt_core_id);
#ifdef GEUL_BOOT_MODE_PCI
	//for geul core 0 prompt
	l1_dcache_invalidate();
	PRINTF("%s: WAITING HIF STATUS %x HOST READY %x\r\n", __func__, in_le32(&pHif->status), in_le32(&pHif->host_ready) );
	while (!(CHK_HIF_HOST_RDY(pHif, HIF_HOST_READY_HOST_REGIONS))) {};
	PRINTF("%s: HIF STATUS %x HOST READY %x\r\n", __func__, in_le32(&pHif->status), in_le32(&pHif->host_ready) );
#endif

	out_le32(&pDbgLogRegs->log_level, GUL_LOG_LEVEL);

	log_info("SoC SVR: 0x%x PVR: 0x%x Fuse:0x%x\r\n",
		get_soc_version(), get_processor_version(), get_fusesr()& 0xf);
	log_info("Core Freq: %d MHz with def.h:SYS_CLK_MULTIPLIER-%d\r\n", PLAT_FREQ/1000000, SYS_CLK_MULTIPLIER);
	log_info("SHA ID : %s\r\n", REL_VERSION);
	log_info("FreeRTOS KERNEL_VERSION_NUMBER %s \n\r",tskKERNEL_VERSION_NUMBER);
	
	if ((crt_core_id == GEUL_E200_MASTER_CORE)
			|| (crt_core_id == TMU_TASK_CORE))
		tmuInit();

#ifdef BBDEV_IPC_MODE
	ipc_retval = bbdev_ipc_init(BBDEV_IPC_DEV_ID_0, crt_core_id);
#else
	ipc_retval = ipc_init(pGulModPriv, crt_core_id);
#endif
	if (unlikely(ipc_retval != IPC_SUCCESS)) {
		log_err("%s: IPC Initialization failed:%d\r\n",__func__,ipc_retval);
		return ipc_retval;
	}

#ifdef FECA_ENABLED
        iRc = geul_feca_init( 0 );
        configASSERT( iRc == 0 );
        log_dbg( "FECA initialization done.\r\n");
#endif
	iRc = vIPICoreInit(crt_core_id);
	configASSERT(iRc == pdPASS);

	if (crt_core_id == GEUL_E200_MASTER_CORE)
	{
		vFillQdmaValidBlk();
#ifdef ENABLE_RF
		xRFDevicePreInit();
#endif /* ENABLE_RF */
#ifndef WARMUP_ENABLE
		vBootRelease(GEUL_E200_MASTER_CORE);
#endif
#ifndef CONFIG_L1C_ENABLE
#if !(CONFIG_IDLE_CALIBRATE)
		vRegisterSampleCLICommands();
		vUARTCommandConsoleStart( mainUART_COMMAND_CONSOLE_STACK_SIZE,
								  mainUART_COMMAND_CONSOLE_TASK_PRIORITY );
#endif /* CONFIG_IDLE_CALIBRATE */
#ifdef DIORA_RF
		vRegisterDioraCLICommands();
#endif
#endif /* CONFIG_L1C_ENABLE */
	}

#ifdef HAWK_ENABLED
	xHawkInit();
#endif
	host_mem_atu_create(pGulModPriv);

#ifndef CONFIG_L1C_ENABLE
#ifdef TESTFRAMEWORK_ENABLE
#ifndef WARMUP_ENABLE
	/* Init Test Framework */
	vInitTestFramework();
#endif /* WARMUP_ENABLE*/
#endif /* TESTFRAMEWORK_ENABLE */
#endif /* L1C_ENABLE */

    if( crt_core_id == GEUL_E200_MASTER_CORE ) {
        /* VCXO Initialization */
    iRc = -1;
#ifdef DCS_LS_ENABLED
    /* Wait for DCS_LS PCLK & AXIQ_CLK */
    if (SUCCESS != uiCheckDcsLsConfig( pHif )) {
        log_err("DCS_LS PCLK and AXIQ_CLK is not enabled from Host\r\n");
    }
    else{
        iRc = iDcsInit(pHif);
        if( 0 != iRc ) {
            log_err( "%s: VDcsInit Init failed, err=%d\r\n", __func__, iRc );
        }
    }
#endif
#ifdef TBGEN_ENABLED
        /*  TBgen clk source depends on return value of iDcsInit function */
	vSCFGInitTbgenClk(pHif, iRc, &iHsDcsClk );	/* Init both Tbgen1 and Tbgen2 Clk */
#endif

#if defined( GEUL_LA1224 ) && defined( ENABLE_RF )
if ((brd_ver == GEUL_HOST_REVA_VAL) || (brd_ver == GEUL_HOST_REVB_VAL))
{
	iRc = iVcxoInit();
        if( 0 != iRc ) {
            log_err( "%s: VCXO Init failed, err=%d\r\n", __func__, iRc );
            goto out;
        }
}
#endif

#ifdef GEUL_LA1224
	log_info("\r\nCurrent VDD value is : %d mV\r\n", iGetCurrentVdd());
	/* Printing the values of porsr1tich settings on board */
	vPrintSwitches();
#endif
    }

#ifdef TBGEN_ENABLED
#ifdef DCS_LS_ENABLED
	iTbgenDevOpen(TBGEN_1);
#endif
	if( iHsDcsClk )
		iTbgenDevOpen(TBGEN_2);
	vInitTbgenTTIConnectivity();
#endif
	if (crt_core_id == GEUL_E200_MASTER_CORE) {
		vPrintTbgenClkinfo();
	}

#ifdef APIPLAYER_ENABLED
	xApiPlayerInit();
#endif

#if defined(GEUL_LA1224) || defined (GEUL_LA1224CPE)
	switch(brd_ver) {
		case GEUL_HOST_REVC_VAL:
#ifdef ADI_RF
			/* ADI RF intialization
			 * return fail - encounter error during init
			 * */
			if (crt_core_id == GEUL_E200_MASTER_CORE) {
				iRc = iAdi_AibInit();
				if( 0 != iRc ) {
					log_err( "%s: ADI_RF Init failed, err=%d\r\n",
							__func__, iRc );
					goto out;
				}
				log_info("\r\niAdi_AibInit : Success\r\n");
			}
#endif
			break;
		case GEUL_HOST_REVA_VAL:
		case GEUL_HOST_REVB_VAL:
		default:
#ifdef AGAVE
			/* Agave RF intialization
			* return success - In case of RF card is not detected
			* return fail - Card detected and encounter error during init
			* */
				if( crt_core_id == GEUL_E200_MASTER_CORE ) {
					iRc = iAgaveInit();
					if( 0 != iRc ) {
						log_err( "%s: Agave Init failed, err=%d\r\n",
							__func__, iRc );
						goto out;
					}
				}
#endif
			break;
	}
#endif

#if defined( ENABLE_RF )
	xRFDeviceInit();
#endif /*ENABLE_RF*/

#ifdef TBGEN_ENABLED
#ifndef MPIC_TIME
	// Fixme: Which TBGEN is enabled on the board
	timeLibInit(TBGEN_1);
#endif
#endif

#if (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
	init_l1c_refapp();
	/* register CLI commands for non-master cores as well */
	if (crt_core_id != GEUL_E200_MASTER_CORE) {
		vRegisterSampleCLICommands();
		create_core_cli_task();
	}
#endif
#if defined(HOST_CLI_ENABLE) || (defined L1C_REFAPP_ENABLE) || (defined L1C_RUDEMO_ENABLE)
    if (crt_core_id == GEUL_E200_MASTER_CORE )
    {
        init_cli_host_if();
    }
#endif

#ifdef CONFIG_L1C_ENABLE
	print_l1_ver();
	l1_tasks_init();
#endif /* CONFIG_L1C_ENABLE */

#if !(CONFIG_IDLE_CALIBRATE)
	if (crt_core_id == TMU_TASK_CORE)
	{
		iRc = xTaskCreate(vGeulTMUTask, "Geul TMU Task", TMU_TASK_STACKSIZE,
				NULL, TMU_TASK_PRIORITY, &xTMUTaskHandle);

		if (unlikely( iRc != pdPASS ))
		{
			log_err("Failed to create tmu task\n\r");
		}
	}
#endif
#ifdef DCS_LS_ENABLED
	/* HSDCS TMon task*/
	if (crt_core_id == DCSTMON_TASK_CORE)
	{
        init_dcs_host_if();
	}
#endif

#ifdef TESTFRAMEWORK_ENABLE
#ifndef RELEASE_MODE
	vCoreMaskBasedWdogEnable(0x1);
#endif
#endif

#ifdef ENABLE_SI551x
	if (crt_core_id == GEUL_E200_MASTER_CORE && (brd_ver == MW_REVB_VERSION || brd_ver == GEUL_HOST_REVC_VAL))
	{
		if( pxSyncTimingDeviceInit() != NULL )
		{
#ifdef ENABLE_SI551x_CLI
			vRegisterTimesyncCLICommands();
#endif
		}
		else
		{
			log_err(" \n\r Failed to init time Sync Device \n\r");
		}
	}
#endif

#if (defined LA12XX_DRIVER_PCI) || (defined LA12XX_DRIVER_PCI_LAT_FP)
	if (crt_core_id == GEUL_E200_MASTER_CORE)
	{
		iRc = xTaskCreate(vGeulPCIeTask, "LA12xx PCIe Task", PCIE_TASK_STACKSIZE,
				NULL, PCIE_TASK_PRIORITY, &xPCIeTaskHandle);

		if (unlikely( iRc != pdPASS ))
		{
			log_err("Failed to create PCIe task\n\r");
		}
	}
#endif /* LA12XX_DRIVER_PCI || LA12XX_DRIVER_PCI_LAT_FP */

#if (CONFIG_IDLE_CALIBRATE)
	iRc = xTaskCreate(vIdleCalib, "Geul Idle CPU Calibrate", IDLE_CALIB_INIT_TASK_STACKSIZE,
			NULL, IDLE_CALIB_INIT_PRIORITY, NULL);

	if( iRc != pdPASS )
	{
		log_err("Failed to create Idle calibration init task\n\r");
	}
#endif

	/* This synchronization prevents cores from starting the task scheduler
	 * before the initialization procedures listed before this point terminate
	 * (e.g. l1_tasks_init, etc.). It does not help, aim or make sense as a
	 * synchronization mechanism of the tasks started by the scheduler */
	core_started[crt_core_id] = 1;
#ifndef WARMUP_ENABLE
	while (1)
	{
		uint32_t i, sum;
		for (i = 0, sum = 0; i < get_soc_numcores(); i++)
			sum += core_started[i];
		if (dbgcount == 100000000)
		{
			for (i = 0; i < get_soc_numcores(); i++)
			{
				if (core_started[i] != 1)
					log_err("\r\n Core %d not started",i);
			}
			dbgcount = 0;
		}
		dbgcount++;
		if (sum == get_soc_numcores())
			break;
	}
#else
	iRc = xTaskCreate(vGeulWARMUPTask, "feca warm-up app", WARMUP_TASK_STACKSIZE,
			NULL, WARMUP_TASK_PRIORITY, NULL);

	if (unlikely( iRc != pdPASS ))
	{
		log_err("Failed to create tmu task\n\r");
	}
#endif

	log_info(" Total Heap Memory: %d Available Heap Memory: %d\n\r", configTOTAL_HEAP_SIZE, xPortGetFreeHeapSize());
	if(check_modem_log_to_host_enable())
		vRedirectModemLogToHost(crt_core_id);
	/* Start FreeRTOS scheduler */
	vTaskStartScheduler();

out:
	log_err("%s: Something terrible has happend, rc %d\r\n", __func__, iRc);
	log_err("%s: Going for infinite loop of death\r\n", __func__);
	/* Should never reach this point */
	while( true )
		;
}

void *pvGeulMalloc( size_t xWantedSize )
{
	void *pvReturn=NULL;
	static unsigned int allocated;

	if( (xWantedSize+allocated) >= heap_size - 50)
	{
		PRINTF( "%s: Heap size %d exceeded, Already allocated=%d, Requested=%d \
				returning NULL\r\n",__func__ , heap_size, allocated, xWantedSize );
		return pvReturn;
	}
	vTaskSuspendAll();
		{
				pvReturn = malloc( xWantedSize );
				traceMALLOC( pvReturn, xWantedSize );
		}
		( void ) xTaskResumeAll();

		#if( configUSE_MALLOC_FAILED_HOOK == 1 )
		{
				if( pvReturn == NULL )
				{
						extern void vApplicationMallocFailedHook( void );
						vApplicationMallocFailedHook();
				}
		}
		#endif
		allocated=allocated+xWantedSize;
		return pvReturn;
}

void vGeulFree( void *pv )
{
		if( pv )
		{
				vTaskSuspendAll();
				{
						free( pv );
						traceFREE( pv, 0 );
				}
				( void ) xTaskResumeAll();
		}
}

mod_mem_region_t * bsp_get_mem_region(enum mem_region_id reg_id)
{
		if (reg_id < MOD_MEM_END)
				return (&pGulModPriv->mem_region[reg_id]);

		return NULL;
}

uint32_t get_modem_share_area_size(void)
{
	volatile struct gul_hif *hifx;

	hifx = bsp_get_hif();

	return SWAP_32(hifx->modem_share_area_size);
}

uint32_t get_modem_host_data_offset(void)
{
	return get_modem_share_area_size();
}

uint32_t get_modem_host_data_size(void)
{
	volatile struct gul_hif *hifx;

	hifx = bsp_get_hif();

	return SWAP_32(hifx->modem_host_data_size);
}

uint32_t get_modem_rf_data_offset(void)
{
	volatile struct gul_hif *hifx;

	hifx = bsp_get_hif();

	return SWAP_32(hifx->modem_share_area_size) + SWAP_32(hifx->modem_host_data_size);
}

uint32_t get_modem_rf_data_size(void)
{
	volatile struct gul_hif *hifx;

	hifx = bsp_get_hif();

	return SWAP_32(hifx->modem_rf_data_size);
}
