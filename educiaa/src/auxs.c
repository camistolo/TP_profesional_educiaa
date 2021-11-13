/*=============================================================================
 * Copyright (c) 2020, Martin N. Menendez <menendezmartin81@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 2020/09/03
 * Version: v1.1
 *===========================================================================*/

/*==================[inclusiones]============================================*/
#include "auxs.h"
/*==================[definiciones y macros]==================================*/

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

/*==================[tareas]====================*/

// Funcion que inicializa la estructura principal


// Funcion que crea y valida las tareas de FreeRTOS
void create_task(TaskFunction_t task, const char * const name, uint8_t stack, void * const params, uint8_t priority, TaskHandle_t * const ptr)
{
	// Crear tarea en freeRTOS
	BaseType_t res = xTaskCreate(task, name, configMINIMAL_STACK_SIZE*stack, params, tskIDLE_PRIORITY+BASE_PRIORITY+priority, ptr);
 	// Gestion de errores
	configASSERT(res != pdFAIL);
}


// Funcion que crea y valida las colas de FreeRTOS
void create_queue(QueueHandle_t *q_name, UBaseType_t q_length, UBaseType_t q_scale){
	*q_name = xQueueCreate(q_length, q_scale);
	configASSERT(q_name != NULL);
}

void create_semaphore(SemaphoreHandle_t *sem_name){
	*sem_name =  xSemaphoreCreateBinary();
	configASSERT(sem_name != NULL);
}

void create_mutex(SemaphoreHandle_t *mutex_name){
	*mutex_name =  xSemaphoreCreateMutex();
	configASSERT(mutex_name != NULL);
}
