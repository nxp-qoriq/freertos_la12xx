/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2024 NXP */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "Time.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "debug_console.h"
#include "l1c_defs.h"
#include "l1c_cli.h"
#include "l1c_bench.h"
#include "l1c_ipi.h"
#include "ipiQueue.h"
#include "l1c_vspa_agent.h"
#include "l1c_vspa_proc.h"
#include "l1c_freq.h"

typedef enum {
	TYPE_INT32,
	TYPE_UINT32,
	TYPE_UINT8,
	TYPE_BOOL,
} type_t;

typedef enum {
	CLI_CFG = 0,
	CLI_DPD = 1,
	CLI_BENCH = 2,
	CLI_DEBUG = 3,
	CLI_MISC_PARAMS = 4,
	CLI_IRQ = 5,
	CLI_COMP_MAX
} cli_component_t;

void l1c_shell_string_to_value(const char *str, cli_component_t cli_comp, void *value, type_t type)
{
	uint32_t i = 0;
	char *s = NULL;
	char *str_tmp = NULL;
	struct {
		char *str;
		int cfg;
	} map[CLI_COMP_MAX][19] = {
		[CLI_CFG] = {
			/* commands */
			{ "trace",          L1C_DEBUG_TRACE          },
			{ "stats",          L1C_DEBUG_STATS          },
			{ "errs",           L1C_DEBUG_ERROR_REGISTER },
			{ "pattern2",       L1C_CONFIG_PATTERN2      },
			{ "pattern1",       L1C_CONFIG_PATTERN       },
			{ "pattern",        L1C_CONFIG_PATTERN       },
			{ "listen",         L1C_CONFIG_RX_ALWAYS_ON  },
			{ "rf_ctrl",        L1C_CONFIG_RF_CTRL       },
			{ "dump",           L1C_CONFIG_DUMP          },
			{ "interface3",     L1C_CONFIG_INTERFACE3    },
			{ "interface2",     L1C_CONFIG_INTERFACE2    },
			{ "interface1",     L1C_CONFIG_INTERFACE1    },
			{ "interface",      L1C_CONFIG_INTERFACE     },
			{ "obs_if1",   	    L1C_CONFIG_OBS_IF1	     },
			{ "obs_if",   	    L1C_CONFIG_OBS_IF	     },
			{ "ul_dl_gap",      L1C_CONFIG_TDD_UL_DL_GAP },
			{ "pps_offset",     L1C_CONFIG_TDD_PPS_OFFSET},
			{ "scs",            L1C_CONFIG_SCS           },
			{ "mixer",          L1C_CONFIG_MIXER         },
		},
		[CLI_DPD] = {
			/* coeffs */
			{ "dpd",            L1C_UPDATE_DPD           },
			{ "txqec",          L1C_UPDATE_TX_QEC        },
			{ "rxqec",          L1C_UPDATE_RX_QEC        },
			{ "cfr",            L1C_UPDATE_CFR           },
			/* dpd commands */
			{ "run",            L1C_DPD_RUN              },
			{ "config",         L1C_DPD_CONFIG           },
			{ "srx",            L1C_DPD_CONFIG_SRX       },
			{ "start",          L1C_DPD_START            },
			{ "stop",           L1C_DPD_STOP             },
			{ "dump",           L1C_DPD_DUMP             },
			{ "491",            SIGNAL_SAMPLE_RATE_491   },
			{ "245",            SIGNAL_SAMPLE_RATE_245   },
			{ "gen",            L1C_DPD_GEN              },
		},
		[CLI_DEBUG] = {
			/* debug params */
			{ "vspa",           L1C_DEBUG_VSPA           },
			{ "e200",           L1C_DEBUG_E200           },
			{ "print",          L1C_DEBUG_PRINT          },
		},
		[CLI_BENCH] = {
			/* debug params */
			{ "e200_dmem0",     MEM_DMEM0                },
			{ "e200_dmem1",     MEM_DMEM1                },
			{ "e200_dmem2",     MEM_DMEM2                },
			{ "e200_dmem3",     MEM_DMEM3                },
			/* Geul B0 (6 e200 cores)
			{ "e200_dmem4",     MEM_DMEM4                },
			{ "e200_dmem5",     MEM_DMEM5                },
			*/
			{ "vspa",           VSPA_BENCH               },
			{ "e200",           E200_BENCH               },
			{ "qdma",           QDMA_BENCH               },
			{ "peb",            MEM_PEB                  },
			{ "sram",           MEM_SRAM                 },
			{ "ddr",            MEM_DDR                  },
			{ "fram",           MEM_FRAM                 },
			{ "hram",           MEM_HRAM                 },
			{ "rd",             DIR_RD                   },
			{ "wr",             DIR_WR                   },
		},
		[CLI_MISC_PARAMS] = {
			/* scs */
			//{ "15",             SCS_kHz15                },
			{ "30",             SCS_kHz30                },
			{ "60",             SCS_kHz60                },
			{ "120",            SCS_kHz120               },
			{ "240",            SCS_kHz240               },
			/* string parameters */
			{ "hs",  DCS_HS  },
			{ "ls0", DCS_LS0 },
			{ "ls1", DCS_LS1 },
			{ "tx", 0 }, { "rx",  1 },
			{ "no", 0 }, { "yes", 1 },
			{ "0",  0 }, { "1",   1 },
		},
		[CLI_IRQ] = {
			{ "enable",         L1C_IRQ_ENABLE           },
			{ "dump",           L1C_IRQ_DUMP             },
		},
	};

	if (!str)
		return;

	str_tmp = strdup(str);
	if (!str_tmp)
		return;

	s = strtok(str_tmp, " ");
	if (!s) {
		free(str_tmp);
		return;
	}

	for (i = 0; i < ARRAY_SIZE( map[cli_comp] ); i++) {
		if (!map[cli_comp][i].str) {
			free(str_tmp);
			return;
		}
#if 0
		PRINTF("iter (%s):(%#x)\r\n", map[cli_comp][i].str, map[cli_comp][i].cfg);
#endif
		if ((!strncmp(s, map[cli_comp][i].str, strlen(map[cli_comp][i].str))) && (!strncmp(s, map[cli_comp][i].str, strlen(s)))) {
#if 0
			PRINTF("matched (%s):(%#x)\r\n", map[cli_comp][i].str, map[cli_comp][i].cfg);
#endif
			switch (type) {
				case TYPE_INT32:
					*(int *)value = (int) map[cli_comp][i].cfg;
					break;
				case TYPE_UINT8:
				case TYPE_BOOL:
					*(uint8_t *)value = (uint8_t) map[cli_comp][i].cfg;
					break;
				case TYPE_UINT32:
				default:
					*(uint32_t *)value = (uint32_t) map[cli_comp][i].cfg;
					break;
			}
			break;
		}
	}

	free(str_tmp);
}

/* default to FR1 TDD */
static ovly_options_t crt_e200_run_mode = RT_OVLY_FR1_TDD;

void set_e200_run_mode(ovly_options_t new_e200_run_mode)
{
	if (crt_e200_run_mode != new_e200_run_mode) {
		PRINTF("Run mode changed from %s to %s.\r\n", ovly_option_name(crt_e200_run_mode), ovly_option_name(new_e200_run_mode));
		crt_e200_run_mode = new_e200_run_mode;
	}
}

void set_e200_run_mode_tdd()
{
	set_e200_run_mode(l1c_vspa_configured_fr1() ? RT_OVLY_FR1_TDD : RT_OVLY_FR2_TDD);
}

ovly_options_t get_e200_run_mode()
{
	return crt_e200_run_mode;
}

/* utils for parsing "l1c_config" command line */
#define CHECK_COMMAND(str, const_str) \
			( strncmp(str, const_str, strlen(const_str)) == 0 )

extern uint64_t vL1CDemoStart(int limited_slot_no);
extern void vL1CDemoStop();
extern void vL1CDemoConfig(l1c_config_t *);
extern void vL1CDPDConfig(l1c_config_dpd_t *);
extern void vL1CDPDConfigSRx(uint8_t srx);
extern void vL1CDPDInput(signal_config_sample_rate_e rate);
extern void vL1CDPDRate(signal_config_sample_rate_e rate);
extern void vL1CDPDRun();
extern void vL1CDPDStart(uint32_t dpd_iter);
extern void vL1CDPDStop();
extern void vL1CDPDCfgDump();
extern void vL1CDPDDump();
extern void vL1CDPDGen(int64_t freq_mHz);
extern void vL1CUpdate_DPD_QEC(l1c_update_t *);
extern void vL1CUpdate_TDD_QEC(l1c_update_t *);
extern void vL1CDemoDebug(l1c_debug_t *);
extern void vL1CIRQ(l1c_irq_type_e );
extern void send_host_notification(uint32_t msg_id, uint32_t data, uint32_t data2, uint32_t data3);

char l1c_help_buf[] __attribute__ ((section (".shared.data"))) = "\
	\r\nl1c version \
	\r\n \
	\r\nl1c config \
	\r\n        pattern   <dl-slots> <dl-syms> <ul-slots> <ul-syms> \
	\r\n        pattern2  <dl-slots> <dl-syms> <ul-slots> <ul-syms> \
	\r\n        interface  <dcs> <interface-no> \
	\r\n        interface2 <dcs> <interface-no> \
	\r\n        interface3  <dcs> <interface-no> \
	\r\n        interface4 <dcs> <interface-no> \
	\r\n        obs_if <dcs> <interface-no> <pulse_samples> <period_samples> \
	\r\n        obs_if1 <dcs> <interface-no> <pulse_samples> <period_samples> \
	\r\n        listen <yes/no> \
	\r\n        ul_dl_gap <UL advance in ns> \
	\r\n        pps_offset <ns> \
	\r\n        scs <kHz> \
	\r\n        mixer <interface|interface2> <mili Hz> \
	\r\n        dump \
	\r\nl1c no_vspa_start [optional limited slot number]\
	\r\nl1c start_cpe [optional limited slot number]\
	\r\nl1c stop \
	\r\n \
	\r\nl1c bench <bench_type> <memory> [memory2] [rd/wr>] <size> <repeats> \
	\r\n        <bench_type> -> vspa | e200 | qdma \
	\r\n        <memory>     -> e200_dmemX | peb | sram | ddr | fram | hram \
	\r\n        [memory2]    -> e200_dmemX | peb | sram | ddr | fram | hram \
	\r\n                     - this argument is required only for QDMA bench type \
	\r\n                     - X -> core ID (0, 1, 2, 3, [4, 5 - for geul B0)]) \
	\r\n        Example: l1c bench vspa peb rd 32 10      --> VSPA will read 10 times 32 bytes from PEB \
	\r\n                 l1c bench e200 sram wr 64 10     --> E200 will write 10 times 64 bytes into SRAM \
	\r\n                 l1c bench qdma sram peb 32 10    --> QDMA will read 10 times 32 bytes from SRAM and write into PEB \
	\r\n \
	\r\nl1c dpd \
	\r\n        config \
	\r\n            <tx dcs> <tx intf-no> <signal-segment-count> <dpd-training-segment-count> <repeat> <input-sample-rate> \
        \r\n            input <input samples rate 122.88/245.76/491.52/983.04> \
        \r\n            rate <input samples rate 122.88/245.76/491.52/983.04> \
	\r\n            dump \
	\r\n        srx <srx dcs> <srx interface-no> \
        \r\n        gen <freq in mHz, Hz, KHz, MHz> \
        \r\n        run \
	\r\n        start \
	\r\n        stop \
	\r\n        dump \
	\r\n \
	\r\nl1c update-coef <dpd/cfr/txqec/rxqec> <core-id> \
	\r\n \
	\r\nl1c debug vspa trace \
	\r\n               errs \
	\r\n          e200 trace \
	\r\n               stats \
	\r\n          print <hex mask>";

static inline void prvL1CParseHelp( const char *pcCommandString, uint8_t uParamOffset )
{
	( void ) pcCommandString;
	( void ) uParamOffset;

	PRINTF("%s\r\n", l1c_help_buf);
}

static inline void prvL1CParseStart( const char *pcCommandString, uint8_t uParamOffset )
{
	( void ) pcCommandString;
	( void ) uParamOffset;
	BaseType_t xParameterStringLength;
	int limited_slot_no = 0;
	uint64_t rf_start_time, curr_time;

	const char *pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2U,
							&xParameterStringLength);
	if (pcParameter != NULL)
		limited_slot_no = strtoul(pcParameter, (char **)NULL, BASE_DEC);

	rf_start_time = vL1CDemoStart(limited_slot_no);
	if (!rf_start_time) {
		log_err("vL1CDemoStart failed\n\r");
		return;
	}

	curr_time = ullTbgenGetMasterCounter(TBGEN_1);

	PRINTF("%s: rf_start_time: 0x%lx%08lx, curr_time: 0x%lx%08lx\n\r",
	       modem_mode == CPE? "CPE":"RU",
		PRINT_64_HI(rf_start_time), PRINT_64_LO(rf_start_time),
		PRINT_64_HI(curr_time), PRINT_64_LO(curr_time));

}

static inline void prvL1CParseStop( const char *pcCommandString, uint8_t uParamOffset )
{
	( void ) pcCommandString;
	( void ) uParamOffset;

	vL1CDemoStop();
}

static inline void prvL1CParseVersion( const char *pcCommandString, uint8_t uParamOffset )
{
	( void ) pcCommandString;
	( void ) uParamOffset;

	vL1CVersion();
}

static void prvL1CParseDPD( const char *pcCommandString, uint8_t uParamOffset )
{
	const char *pcParameter = NULL;
	BaseType_t xParameterStringLength;

	l1c_config_dpd_t *pCfgMsg = NULL;
	l1c_dpd_type_e cmd_type = L1C_DPD_INVALID;

	uint32_t dpd_iter = 0;
	uint8_t intf, dcs, srx;

	( void ) pcCommandString;
	configASSERT( pCfgMsg );

	/* supported commands:
	 *     dpd config
	 *     dpd run
	 */

	pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 1L, &xParameterStringLength );
	if ( pcParameter == NULL ) {
		PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
		return;
	}

	l1c_shell_string_to_value(pcParameter, CLI_DPD, (void *)&cmd_type, TYPE_UINT32);

	switch (cmd_type)
	{
		case L1C_DPD_CONFIG:

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 2L, &xParameterStringLength);

			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				break;
			}
			else if (!strcmp(pcParameter, "dump"))
			{
				vL1CDPDCfgDump();
				break;
			}
			else if (!strncmp(pcParameter, "input", 5))
			{
				pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
									uParamOffset + 3L,
									&xParameterStringLength);
				if (pcParameter) {
					vL1CDPDInput(sample_rate_str2enum(pcParameter));
				}
				break;
			}
			else if (!strncmp(pcParameter, "rate", 4))
			{
				pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
									uParamOffset + 3L,
									&xParameterStringLength);
				if (pcParameter) {
					vL1CDPDRate(sample_rate_str2enum(pcParameter));
				}
				break;
			}

			/* dpd config <dcs> <intfs_id> <sig seg_cnt> <train seg_cnt> <repeat> <in samp_rate> */

			pCfgMsg = ( l1c_config_dpd_t * )pvPortMalloc( sizeof( l1c_config_dpd_t ) );
			memset( pCfgMsg, 0, sizeof( l1c_config_dpd_t ) );

			/* expecting 6 parameters - check if there are that many */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 7L,
												   &xParameterStringLength);

			/* if incorrect or no parameters are given send the message, telling app to disable dpd */
			if (pcParameter == NULL) {
				pCfgMsg->dcs = ~0;
				pCfgMsg->interface = ~0;
				pCfgMsg->signal_segment_cnt = 0;
				pCfgMsg->training_segment_cnt = 0;
				pCfgMsg->repeat = 0;
				pCfgMsg->dpd_sample_rate = 0;
				vL1CDPDConfig( pCfgMsg );
				break;
			}

			l1c_shell_string_to_value(pcParameter, CLI_DPD, (void *)&pCfgMsg->dpd_sample_rate, TYPE_UINT32);

			/* number of repeats */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 6L,
												   &xParameterStringLength);

			pCfgMsg->repeat = strtoul(pcParameter, (char **)NULL, BASE_DEC);

			/* dpd training segment count */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 5L,
												   &xParameterStringLength);

			pCfgMsg->training_segment_cnt = strtoul(pcParameter, (char **)NULL, BASE_DEC);

			/* input signal segment count */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 4L,
												   &xParameterStringLength);

			pCfgMsg->signal_segment_cnt = strtoul(pcParameter, (char **)NULL, BASE_DEC);

			/* interface */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 3L,
												   &xParameterStringLength);

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->interface, TYPE_UINT8);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->dcs, TYPE_UINT8);

			if (app_started)
			{
				PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
				vPortFree( pCfgMsg );
			}

			/* config message allocated memory will be freed later */
			vL1CDPDConfig( pCfgMsg );

			break;

		case L1C_DPD_CONFIG_SRX: /* <dcs: ls0/ls1/hs> <if: 0/1> */
			/* 2 parameters are required */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
								uParamOffset + 3L,
								&xParameterStringLength);

			if (pcParameter == NULL)
				break;

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&intf, TYPE_UINT8);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
								uParamOffset + 2L,
								&xParameterStringLength);

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&dcs, TYPE_UINT8);


			srx = 2 * dcs + intf;

			vL1CDPDConfigSRx(srx);
			break;

		case L1C_DPD_RUN:
			vL1CDPDRun();
			break;

		case L1C_DPD_START:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 2L, &xParameterStringLength);

			if (pcParameter != NULL)
			{
				dpd_iter = strtoul(pcParameter, (char **)NULL, BASE_DEC);
			}

			vL1CDPDStart(dpd_iter);
			break;

		case L1C_DPD_STOP:
			vL1CDPDStop();
			break;

		case L1C_DPD_DUMP:
			vL1CDPDDump();
			break;

		case L1C_DPD_GEN: /* <freq> */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 2L, &xParameterStringLength);
			if (pcParameter != NULL)
			{
				uint64_t freq_mHz;

				freq_mHz = parse_freq_in_mHz(pcParameter);
				vL1CDPDGen(freq_mHz);
			}
			break;

		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
	}
}

static void prvL1CParseUpdate( const char *pcCommandString, uint8_t uParamOffset )
{
	const char *pcParameter = NULL;
	BaseType_t xParameterStringLength;
	l1c_update_t *pCfgMsg = ( l1c_update_t * )pvPortMalloc( sizeof( l1c_update_t ) );

	( void ) pcCommandString;
	configASSERT( pCfgMsg );

	memset( pCfgMsg, 0, sizeof( l1c_update_t ) );

	pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 1L, &xParameterStringLength );
	if ( pcParameter == NULL )
	{
		PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
		goto UpdateExit;
	}

	l1c_shell_string_to_value(pcParameter, CLI_DPD, (void *)&pCfgMsg->type, TYPE_UINT32);

	switch (pCfgMsg->type)
	{
		case L1C_UPDATE_DPD:
		case L1C_UPDATE_TX_QEC:
		case L1C_UPDATE_RX_QEC:
		case L1C_UPDATE_CFR:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto UpdateExit;
			}

			pCfgMsg->core_id = (uint8_t) strtoul(pcParameter, (char **)NULL, BASE_DEC);
			break;
		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
			goto UpdateExit;
	}

	/* update coeffs for current operation mode */
	switch (get_e200_run_mode()) {
		case RT_OVLY_FR1_TDD:
		case RT_OVLY_FR2_TDD:
			vL1CUpdate_TDD_QEC(pCfgMsg);
		break;
		case RT_OVLY_DPDH:
			vL1CUpdate_DPD_QEC(pCfgMsg);
		break;
		default:
			PRINTF("Invalid command for current operation mode: %s\r\n", ovly_option_name(get_e200_run_mode()));
	}
	return;

UpdateExit:
	vPortFree( pCfgMsg );
}

static void prvL1CParseDebug( const char *pcCommandString, uint8_t uParamOffset )
{
	const char *pcParameter = NULL;
	BaseType_t xParameterStringLength;
	l1c_debug_t *pCfgMsg = ( l1c_debug_t * )pvPortMalloc( sizeof( l1c_debug_t ) );

	( void ) pcCommandString;
	configASSERT( pCfgMsg );

	/* supported commands:
	 *     debug vspa trace
	 *     debug e200 trace
	 */

	memset( pCfgMsg, 0, sizeof( l1c_debug_t ) );

	pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 1L, &xParameterStringLength );
	if ( pcParameter == NULL )
	{
		PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
		goto DebugExit;
	}

	l1c_shell_string_to_value(pcParameter, CLI_DEBUG, (void *)&pCfgMsg->type, TYPE_UINT32);

	switch (pCfgMsg->type)
	{
		case L1C_DEBUG_VSPA:
		case L1C_DEBUG_E200:
		case L1C_DEBUG_PRINT:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto DebugExit;

			}
			if (pCfgMsg->type == L1C_DEBUG_PRINT)
				pCfgMsg->data.mask = strtoul(pcParameter, (char **)NULL, BASE_HEXA);
			else
				l1c_shell_string_to_value(pcParameter, CLI_CFG, (void *)&pCfgMsg->data.component, TYPE_UINT32);
			break;
		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
			goto DebugExit;
	}

	/* config message allocated memory will be freed later */
	vL1CDemoDebug( pCfgMsg );
	return;

DebugExit:
	vPortFree( pCfgMsg );
}

static int prvL1CParseConfig( const char *pcCommandString, uint8_t uParamOffset )
{
	const char *pcParameter = NULL;
	BaseType_t xParameterStringLength;
	l1c_config_t *pCfgMsg = ( l1c_config_t * )pvPortMalloc( sizeof( l1c_config_t ) );
	int skip_run_mode = 0;
	uint8_t u;

	( void ) pcCommandString;
	configASSERT( pCfgMsg );

	/* supported commands:
	 *     config pattern  <dl_slots> <dl_syms> <ul_slots> <ul_syms>
	 *     config pattern2 <dl_slots> <dl_syms> <ul_slots> <ul_syms>
	 *     config listen <0/1>
	 *     config dump
	 */

	memset( pCfgMsg, 0, sizeof( l1c_config_t ) );

	pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 1L, &xParameterStringLength );
	if ( pcParameter == NULL )
	{
		PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
		goto ConfigExit;
	}

	l1c_shell_string_to_value(pcParameter, CLI_CFG, (void *)&pCfgMsg->type, TYPE_UINT32);

	switch (pCfgMsg->type)
	{
		case L1C_CONFIG_PATTERN:
		case L1C_CONFIG_PATTERN2:
			/* expecting 4 additional parameters - check if there are that many */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 5L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			pCfgMsg->data.cfg_pattern.p.ul_syms = strtoul(pcParameter, (char **)NULL, BASE_DEC);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			pCfgMsg->data.cfg_pattern.p.dl_slots = strtoul(pcParameter, (char **)NULL, BASE_DEC);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 3L,
												   &xParameterStringLength);

			pCfgMsg->data.cfg_pattern.p.dl_syms = strtoul(pcParameter, (char **)NULL, BASE_DEC);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 4L,
												   &xParameterStringLength);
			pCfgMsg->data.cfg_pattern.p.ul_slots = strtoul(pcParameter, (char **)NULL, BASE_DEC);
			break;

		case L1C_CONFIG_TDD_UL_DL_GAP:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			pCfgMsg->data.tdd_ul_dl_gap = strtoul(pcParameter, (char **)NULL, BASE_DEC);
			break;

		case L1C_CONFIG_RX_ALWAYS_ON:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&u, TYPE_UINT8);
			pCfgMsg->data.rx_always_listen = u;
			break;

		case L1C_CONFIG_RF_CTRL:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&u, TYPE_UINT8);
			pCfgMsg->data.rf_fem_ctrl = u;
			skip_run_mode = 1;
			break;

		case L1C_CONFIG_DUMP:
			break;

		case L1C_CONFIG_INTERFACE:
		case L1C_CONFIG_INTERFACE1:
		case L1C_CONFIG_INTERFACE2:
		case L1C_CONFIG_INTERFACE3:
			/* 2 parameters are required */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 3L,
												   &xParameterStringLength);

			/* if incorrect or no parameters are given send the message, telling app to disable interface */
			if (pcParameter == NULL) {
				pCfgMsg->data.cfg_interface.dcs = ~0;
				pCfgMsg->data.cfg_interface.interface = ~0;
				break;
			}

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->data.cfg_interface.interface, TYPE_UINT8);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->data.cfg_interface.dcs, TYPE_UINT8);
			break;

		case L1C_CONFIG_OBS_IF:
		case L1C_CONFIG_OBS_IF1:
			/* 2 mandatory parameters are required */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 3L,
												   &xParameterStringLength);

			/* if incorrect or no parameters are given send the message, telling app to disable interface */
			if (pcParameter == NULL) {
				pCfgMsg->data.cfg_obs_if.dcs = ~0;
				pCfgMsg->data.cfg_obs_if.interface = ~0;
				break;
			}
			
			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->data.cfg_obs_if.interface, TYPE_UINT8);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->data.cfg_obs_if.dcs, TYPE_UINT8);

			/* Number of samples */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 4L, &xParameterStringLength);
			if (pcParameter == NULL) {
				PRINTF("Samples to be collected in pulse not provided\r\n");
				goto ConfigExit;
			}

			pCfgMsg->data.cfg_obs_if.samples = strtoull(pcParameter, (char **)NULL, BASE_DEC);

			/* Period in samples */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 5L, &xParameterStringLength);
			if (pcParameter == NULL) {
				PRINTF("Periodicty not given\r\n");
				goto ConfigExit;
			}

			pCfgMsg->data.cfg_obs_if.period = strtoull(pcParameter, (char **)NULL, BASE_DEC);

			break;

		case L1C_CONFIG_TDD_PPS_OFFSET:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			pCfgMsg->data.pps_offset = strtoul(pcParameter, (char **)NULL, BASE_DEC);
			break;

		case L1C_CONFIG_SCS:
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
												   uParamOffset + 2L,
												   &xParameterStringLength);
			if (pcParameter == NULL)
			{
				PRINTF("No parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			l1c_shell_string_to_value(pcParameter, CLI_MISC_PARAMS, (void *)&pCfgMsg->data.scs, TYPE_UINT32);
			break;

		case L1C_CONFIG_MIXER:
			/* 2 parameters are required */
			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
							       uParamOffset + 3L,
							       &xParameterStringLength);

			/* if incorrect or no parameters are given */
			if (pcParameter == NULL) {
				PRINTF("Invalid parameters! Use 'l1c help' command to see all l1c commands\r\n");
				goto ConfigExit;
			}

			/* freq is given in mili Hertz => 5000 means 5 Hz */
			pCfgMsg->data.mixer.freq = strtol(pcParameter, (char **)NULL, BASE_DEC);

			pcParameter = FreeRTOS_CLIGetParameter(pcCommandString,
							       uParamOffset + 2L,
							       &xParameterStringLength);
			l1c_shell_string_to_value(pcParameter, CLI_CFG, (void *)&pCfgMsg->data.mixer.intf_idx, TYPE_UINT8);

			/* user provides interface, interface1 or interface2, these will map to indexes 0, 0 or 1 */
			pCfgMsg->data.mixer.intf_idx = (pCfgMsg->data.mixer.intf_idx == L1C_CONFIG_INTERFACE2) ? 1 : 0;
			break;

		default:
			PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
			goto ConfigExit;
	}

//ConfigTDD:
	/* config message allocated memory will be freed later */
	vL1CDemoConfig( pCfgMsg );

	return skip_run_mode;

ConfigExit:
	vPortFree( pCfgMsg );
	return skip_run_mode;
}

void prvL1CParseBench( const char *pcCommandString, uint8_t uParamOffset )
{
	BaseType_t xParamBenchType = 0;
	BaseType_t xParamMemType = 0;
	BaseType_t xParamSizeInBytes = 0;
	BaseType_t xParamTransferDirection = 0;
	BaseType_t xParamRepeats = 0;

	char * pcParamBenchType = NULL;
	char * pcParamMemType = NULL;
	char * pcParamSizeInBytes = NULL;
	char * pcParamTransferDirection = NULL;
	char * pcParamRepeats = NULL;

	mem_type_e xMemType1 = MEM_UNKW;
	mem_type_e xMemType2 = MEM_UNKW;
	bench_type_e xBenchType = UNKW_BENCH;
	uint32_t xSizeInBytes = 0;
	direction_e xTransferDirection = DIR_UNKW;
	uint32_t xRepeats = 0;

	// possible values for pcParamBenchType: vspa | e200 | qdma
	pcParamBenchType = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 1U, &xParamBenchType );
	l1c_shell_string_to_value(pcParamBenchType, CLI_BENCH, (void *)&xBenchType, TYPE_UINT32);
	if (xBenchType == UNKW_BENCH)
	{
		PRINTF("Wrong value for bench type !!!\r\n");
		return;
	}

	// possible values for pcParamMemType: e200_dmem | peb | sram | ddr | fram | hram
	pcParamMemType = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 2U, &xParamMemType );
	l1c_shell_string_to_value(pcParamMemType, CLI_BENCH, (void *)&xMemType1, TYPE_UINT32);
	if (xMemType1 == MEM_UNKW)
	{
		PRINTF("Wrong value for memory type !!!\r\n");
		return;
	}

	switch (xBenchType)
	{
		case QDMA_BENCH:
			pcParamMemType = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 3U, &xParamMemType );
			l1c_shell_string_to_value(pcParamMemType, CLI_BENCH, (void *)&xMemType2, TYPE_UINT32);
			if (xMemType2 == MEM_UNKW)
			{
				PRINTF("Wrong value for memory type!!\r\n");
				return;
			}
			break;
		case VSPA_BENCH:
			xMemType2 = MEM_UNKW;
			break;
		case E200_BENCH:
			xMemType2 = MEM_DMEM0;
			break;
		default:
			PRINTF("Wrong value for benchmark type!\r\n");
			return;
	}

	pcParamTransferDirection = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 3U, &xParamTransferDirection );
	l1c_shell_string_to_value(pcParamTransferDirection, CLI_BENCH, (void *)&xTransferDirection, TYPE_UINT32);
	if (xTransferDirection == DIR_UNKW)
	{
		PRINTF("Wrong value for direction!\r\n");
		return;
	}

	pcParamSizeInBytes = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 4U, &xParamSizeInBytes );
	pcParamRepeats = ( char * ) FreeRTOS_CLIGetParameter( pcCommandString, uParamOffset + 5U, &xParamRepeats );

	xSizeInBytes = strtoul( pcParamSizeInBytes , (char **)NULL, BASE_DEC );
	xRepeats = strtoul( pcParamRepeats , (char **)NULL, BASE_DEC );

	if ((xSizeInBytes == 0) || (xRepeats == 0))
	{
		PRINTF("The value for size and repeats must be greater than 0!\r\n");
		return;
	}

	vL1CBench(xBenchType, xMemType1, xMemType2, xSizeInBytes, xTransferDirection, xRepeats);
}

void prvL1CParseIRQ(const char *pcCommandString, uint8_t uParamOffset)
{
	BaseType_t xParameterStringLength = 0;
	l1c_irq_type_e irqCommand = L1C_IRQ_UNKW;
	char *param = NULL;

	param = ( char * ) FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 1L, &xParameterStringLength);

	l1c_shell_string_to_value(param, CLI_IRQ, (void *)&irqCommand, TYPE_UINT32);
	if (irqCommand == L1C_IRQ_UNKW)
	{
		PRINTF("Invalid parameter! Use 'l1c help' command to see all l1c commands\r\n");
		return;
	}

	vL1CIRQ(irqCommand);
}

void prvL1CParseMSI(const char *pcCommandString, uint8_t uParamOffset)
{
	BaseType_t xParameterStringLength = 0;
	char *dummy_vars[4] = { NULL };

	dummy_vars[0] = (char *)FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 1L, &xParameterStringLength);
	dummy_vars[1] = (char *)FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 2L, &xParameterStringLength);
	dummy_vars[2] = (char *)FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 3L, &xParameterStringLength);
	dummy_vars[3] = (char *)FreeRTOS_CLIGetParameter(pcCommandString, uParamOffset + 4L, &xParameterStringLength);

	send_host_notification(strtoul(dummy_vars[0], (char **)NULL, BASE_DEC),
		    strtoul(dummy_vars[1], (char **)NULL, BASE_DEC),
		    strtoul(dummy_vars[2], (char **)NULL, BASE_DEC),
		    strtoul(dummy_vars[3], (char **)NULL, BASE_DEC));
}

portBASE_TYPE prvL1CDemoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *pcParameter = NULL;
	BaseType_t xParameterStringLength;

	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* supported commands:
	 *     l1c config ...
	 *     l1c no_vspa_start
	 *     l1c start_cpe
	 *     l1c stop
	 *     l1c bench ...
	 *     l1c version
	 *     l1c help
	 *     l1c dpd ...
	 *     l1c debug ...
	 *     l1c update-coef ...
	 */

	pcParameter = FreeRTOS_CLIGetParameter( pcCommandString, 1U, &xParameterStringLength );
	if ( pcParameter == NULL )
	{
		PRINTF("No L1C command! Use 'l1c help' command to see all l1c commands\r\n");
		goto CommandExit;
	}

	if ( CHECK_COMMAND( pcParameter, "config" ) )
	{
		if (!prvL1CParseConfig( pcCommandString, 1U ))
			/* requires interface to be set to determine FR1 vs FR2 */
			set_e200_run_mode_tdd();
	}
	else if ( CHECK_COMMAND( pcParameter, "no_vspa_start" ) )
	{
		set_e200_run_mode_tdd();
		modem_mode = RU;
		prvL1CParseStart( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "start_cpe" ) )
	{
		set_e200_run_mode_tdd();
		modem_mode = CPE;
		prvL1CParseStart( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "stop" ) )
	{
		set_e200_run_mode_tdd();
		prvL1CParseStop( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "bench" ) )
	{
		set_e200_run_mode(RT_OVLY_BENCH);
		if (app_started)
			PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
		else
			prvL1CParseBench( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "version" ) )
	{
		if (app_started)
			PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
		else
			prvL1CParseVersion( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "help" ) )
	{
		prvL1CParseHelp( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "dpd" ) )
	{
		set_e200_run_mode(RT_OVLY_DPDH);
		if (app_started)
			PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
		else
			prvL1CParseDPD( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "debug" ) )
	{
		prvL1CParseDebug( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "update-coef" ) )
	{
		/*
		if (app_started)
			PRINTF("L1C RefApp is running. Use 'l1c stop' command.\r\n");
		else
		*/
		prvL1CParseUpdate( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "msi" ) )
	{
		prvL1CParseMSI( pcCommandString, 1U );
	}
	else if ( CHECK_COMMAND( pcParameter, "irq" ) )
	{
		prvL1CParseIRQ( pcCommandString, 1U );
	}
	else
	{
		PRINTF("Invalid L1C command! Use 'l1c help' command to see all l1c commands\r\n");
	}

CommandExit:
	pcWriteBuffer[ 0 ] = 0x00;
	return pdFALSE;
}
