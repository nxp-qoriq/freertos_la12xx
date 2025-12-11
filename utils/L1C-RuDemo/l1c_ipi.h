/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2022 NXP */

#ifndef _L1C_IPI_H
#define _L1C_IPI_H

#include "ipiQueue.h"

typedef enum {
	L1C_IPI_TDD_START = 0,
	L1C_IPI_TDD_STOP,
	L1C_IPI_CMD_0,
	L1C_IPI_CMD_1,
	L1C_IPI_CMD_2,
	L1C_IPI_CMD_3,
	L1C_IPI_CMD_MAX
} ipi_cmds;
//#define MAX_IPI_AGENT_CMDS	8

typedef void (*ipi_funcptr)(void);

typedef struct {
	uint32_t core_mask;
	ipi_funcptr callback;
} ipi_evt_entry;

extern void init_l1c_ipi( void );

extern bool_t l1c_bind_ipi_cmd(uint32_t ipi_cmd_id, uint32_t core_mask, ipi_funcptr callback);
extern bool_t l1c_send_ipi_cmd(uint32_t ipi_cmd_id);

extern uint32_t cores_to_mask(uint8_t num_cores, ...);
#endif
