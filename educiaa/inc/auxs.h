/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#ifndef _AUXS_H_
#define _AUXS_H_

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "sapi.h"
#include "semphr.h"
#include "task.h"
#include "tasks_force.h"
#include "tasks_pressure.h"

/*====================[definicion de variables y macros]=====================*/

// Valores base para la creacion de tareas
#define BASE_PRIORITY 		1
#define BASE_SIZE     		2

// Constantes de la periodicidad de las tareas
#define RATE_1000_MS	1000
#define RATE_500_MS		500
#define RATE_100_MS		100
#define RATE_50_MS		50
#define RATE_20_MS		20
#define RATE_5_MS		5
#define RATE_1_MS		1

#define RATE_1000 		pdMS_TO_TICKS(RATE_1000_MS)
#define RATE_500 		pdMS_TO_TICKS(RATE_500_MS)
#define RATE_100 		pdMS_TO_TICKS(RATE_100_MS)
#define RATE_50 		pdMS_TO_TICKS(RATE_50_MS)
#define RATE_20 		pdMS_TO_TICKS(RATE_20_MS)
#define RATE_5	 		pdMS_TO_TICKS(RATE_5_MS)
#define RATE_1 			pdMS_TO_TICKS(RATE_1_MS)

/*=========================[declaracion de funciones]=========================*/

void create_task(TaskFunction_t task, const char * const name, uint8_t stack,void * const params, uint8_t priority, TaskHandle_t * const ptr);
void create_queue(QueueHandle_t *q_name,UBaseType_t q_length,UBaseType_t q_scale);
void create_semaphore(SemaphoreHandle_t *sem_name);
void gpio_config(void);

#endif /* _AUXS_H_ */
