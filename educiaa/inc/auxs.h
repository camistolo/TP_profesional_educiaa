/*=============================================================================
 * Copyright (c) 2020, Martin N. Menendez <menendezmartin81@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 2020/09/03
 * Version: v1.1
 *===========================================================================*/
#ifndef _AUXS_H_
#define _AUXS_H_

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"
#include "tasks_force.h"

#include <string.h>
#include "FreeRTOSConfig.h"

/*==================[definiciones y macros]==================================*/
#define LED_ERROR 		LEDR
#define MAX_SEM 5

#define BASE_PRIORITY 1
#define SIZE     2
#define STR_AUX  20
#define SIZE_UINT sizeof(unsigned int)

/*==================[prototipos]=========================*/
//void tecla_led_init(void);
//void tareas_crear(TaskFunction_t tarea,const char * const nombre);
void create_task(TaskFunction_t task, const char * const name, uint8_t stack,void * const params, uint8_t priority, TaskHandle_t * const ptr);
void create_queue(QueueHandle_t *q_name,UBaseType_t q_length,UBaseType_t q_scale);
void create_semaphore(SemaphoreHandle_t *sem_name);
void create_mutex(SemaphoreHandle_t *mutex_name);

#endif /* _AUXS_H_ */
