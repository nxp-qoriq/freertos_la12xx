/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2021-2023 NXP */

#ifndef _L1C_TASKS_H
#define _L1C_TASKS_H

#define MAX_TASKS_ALLOWED       16
#define MAX_TICK_TASKS_PER_CORE 8

#define DEFAULT_STACK_SIZE (configMINIMAL_STACK_SIZE * 2)

typedef enum task_id_e
{
	L1C_UNKNOWN_TASK,
	/* tasks that must be alive all times*/
	L1C_HOST_IRQ_TASK,
	L1C_VSPA_AGENT_TASK,
	L1C_TIME_AGENT_TASK,
	L1C_TIME_AGENT2_TASK,
	/* tasks that must be detroyed when stopping app */
	L1C_TIME_PROC_TASK,
	L1C_TIME_ACTION_TASK,
	L1C_VSPA_SLOTS_TASK,
	L1C_DPD_TASK,
	L1C_DPD_IRQ_TASK,
	L1C_TASK_MAX_ID
} task_id_t;

typedef enum core_id_e
{
	L1_CORE_0,
	L1_CORE_1,
	L1_CORE_2,
	L1_CORE_3,
	L1_CORE_MAX
} core_id_t;

typedef enum tick_type_e
{
	TICK_DISABLE,
	TICK_ENABLE,
} tick_type_t;

typedef struct task_desc_s
{
	uint32_t          core_id;
	task_id_t         task_id;
	TaskHandle_t      task_handle;
	tick_type_t       tick;
	SemaphoreHandle_t tick_sem;
} task_desc_t;

typedef struct task_map_s
{
	task_desc_t tasks[L1C_TASK_MAX_ID];
} task_map_t;

/* external variables */
extern task_map_t tasks_map;
extern uint8_t crt_core_id;

void l1c_create_task(uint32_t, uint32_t, const char *, tick_type_t, UBaseType_t, const configSTACK_DEPTH_TYPE stack_size, TaskFunction_t);
void wait_for_tick(task_id_t);
void l1c_task_suspend(uint32_t task_id);
void l1c_task_resume(uint32_t task_id);

#endif
