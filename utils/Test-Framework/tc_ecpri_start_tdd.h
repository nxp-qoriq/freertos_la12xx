/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 NXP
 */


#define ECPRI_TDD_PARAMS_OFFSET 0x6600000

typedef struct{
          uint32_t DlSlotNum;
          uint32_t DlSymsNum;
          uint32_t UlSlotNum;
          uint32_t UlSymsNum;
          uint32_t Dcs;
          uint32_t AntennaId;
          uint32_t ecpri_tdd_data_ready;
          uint32_t flag;
}tdd_ready_params_t;

void vPollEcpriTddReady( void );
