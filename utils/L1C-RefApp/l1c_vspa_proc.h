/* SPDX-License-Identifier: BSD-3-Clause /
/ Copyright 2021-2024 NXP */

#ifndef _L1C_VSPA_PROC_H
#define _L1C_VSPA_PROC_H

#include "l1c_vspa_agent.h"

#define VSPA_TX_CORE    0
#define VSPA_RX_CORE    1
#define VSPA_TX_RX_CORE_NUM 2

void l1c_vspa_proc_main(void *pvParameters);
void l1c_vspa_proc_slots(void *pvParameters);
void l1c_slot_config_dump(tdd_slot_config_t *s);
void l1c_vspa_slot_config_dump_all();
void l1c_vspa_proc_stop(bool_t param);
uint8_t l1c_vspa_core_mapping(uint8_t intf, uint8_t tx_rx, uint8_t idx);
bool l1c_vspa_configured_fr1();
void dump_vspa_debug_stats();
void reset_vspa_slots();
#endif
