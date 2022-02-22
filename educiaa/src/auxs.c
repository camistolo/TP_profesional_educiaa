/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#include "auxs.h"

/*=========================[definicion de funciones]=========================*/

// Funcion que crea y valida las tareas de FreeRTOS
void create_task(TaskFunction_t task, const char * const name, uint8_t stack, void * const params, uint8_t priority, TaskHandle_t * const ptr)
{
	// Creacion de la tarea
	BaseType_t res = xTaskCreate(task, name, configMINIMAL_STACK_SIZE*stack, params, tskIDLE_PRIORITY+BASE_PRIORITY+priority, ptr);
 	// Gestion de errores
	configASSERT(res != pdFAIL);
}


// Funcion que crea y valida las colas de FreeRTOS
void create_queue(QueueHandle_t *q_name, UBaseType_t q_length, UBaseType_t q_scale)
{
	// Creacion de la cola
	*q_name = xQueueCreate(q_length, q_scale);
	// Gestion de errores
	configASSERT(q_name != NULL);
}

//Funcion que crea y valida los semaforos de FreeRTOS
void create_semaphore(SemaphoreHandle_t *sem_name)
{
	// Creacion del semaforo
	*sem_name =  xSemaphoreCreateBinary();
	//Gestion de errores
	configASSERT(sem_name != NULL);
}

// Funcion configura los pines GPIO
void gpio_config(void)
{
	// Configuracion de los pines del modulo HX711
	gpioInit(ClockPin, GPIO_OUTPUT);
	gpioInit(DataPin, GPIO_INPUT);

	// Configuracion del mux/demux del sensor de presion
	gpioInit(demuxS0, GPIO_OUTPUT);
	gpioInit(demuxS1, GPIO_OUTPUT);
	gpioInit(demuxS2, GPIO_OUTPUT);
	gpioInit(demuxS3, GPIO_OUTPUT);
	gpioInit(demuxSIG, GPIO_OUTPUT);

   gpioInit(muxS0, GPIO_OUTPUT);
   gpioInit(muxS1, GPIO_OUTPUT);
   gpioInit(muxS2, GPIO_OUTPUT);
   gpioInit(muxS3, GPIO_OUTPUT);
}

/*=========================[fin del archivo]=================================*/
