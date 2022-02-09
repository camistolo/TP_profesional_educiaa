/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#include "tasks_force.h"

/*=========================[declaracion de variables]========================*/

QueueHandle_t queue_force;
QueueHandle_t queue_median;
QueueHandle_t queue_jump_force;
QueueHandle_t queue_weight;

SemaphoreHandle_t sem_measure_force;

uint32_t OFFSET = 10000000;
uint32_t WEIGHT = 0;

TaskHandle_t TaskHandle_check_hx711_is_ready;
TaskHandle_t TaskHandle_measure_force;
TaskHandle_t TaskHandle_average_force;
TaskHandle_t TaskHandle_calculate_median;
TaskHandle_t TaskHandle_measure_weight;
extern TaskHandle_t TaskHandle_print_weight;

/*=========================[declaracion de funciones]=========================*/

// Tarea que libera un semaforo cuando el HX711 esta listo para medir
void task_check_hx711_is_ready( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_50;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	while ( TRUE )
	{
		// Solo cuando se mide el valor de DataPin en 0, se sabe que el HX711 esta listo
		// para medir
		if( !gpioRead(DataPin) ){
			// Se crea la tarea de medicion (para que calcule la mediana de varios valores)
			// y se libera un semafoto indicando que el modulo esta listo para medir
			force_measurement_mode_t mode = MEDIAN_MODE;
			create_task(task_measure_force,"task_measure_force",BASE_SIZE,&mode,1,&TaskHandle_measure_force);
			xSemaphoreGive( sem_measure_force );
			vTaskDelete(NULL);
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que mide la fuerza
void task_measure_force( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_20;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	force_measurement_mode_t* mode = (force_measurement_mode_t*) taskParmPtr;

	uint32_t count = 0;
	gpioWrite(ClockPin , 0);

	// Si el modo es median, entonces se crea la tarea que va a hacer la mediana de todos los valores
	if (*mode == MEDIAN_MODE) {
		create_task(task_calculate_median,"task__calculate_median",BASE_SIZE,0,1,&TaskHandle_calculate_median);
	}

	while ( TRUE )
	{
		// Se espera a que el modulo HX711 este listo
		if (xSemaphoreTake( sem_measure_force  ,  portMAX_DELAY )){

			count = 0;
			// Se envian tantos pulsos como la ganancia elegida lo indique, se hace
			// un shift de la variable count por cada uno de ellos y se incrementa
			// si el valor de DataPin es 1
			for (size_t i = 0;i < GAIN_128;i++)
			{
				gpioWrite(ClockPin , ON);
				count=count<<1;
				gpioWrite(ClockPin , OFF);
				if(gpioRead(DataPin)){
						count++;
				}
			}

			// Se realiza la medicion final
			gpioWrite(ClockPin , ON);
			count=count^XOR_VALUE;
			gpioWrite(ClockPin , OFF);

			// Se envia la medicion a la tarea correspondiente
			xQueueSend(queue_force , &count,  portMAX_DELAY);
			// Esta tarea se elimina en la tarea que recibe la medicion porque si se trata de una medicion
			// con calculo de mediana, entonces se va a llamar a esta tarea tantas veces como valores se
			// necesiten para el calculo.
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que calcula la mediana de los valores medidos
void task_calculate_median( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_1;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uint32_t f;
	uint32_t f_values[TOTAL_MEDIAN_VALUES];
	uint8_t counter = 0;
	uint32_t median;

	while ( TRUE ){

		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){
			// Si ya se tienen los 20 valores, se calcula la mediana como el valor medio
			// entre todos los valores medidos (esto permite evitar valores extremos que
			// en el promedio se incluirian para el calculo)
			if (counter == TOTAL_MEDIAN_VALUES){
				median = f_values[counter/2];
				// Se envia el valor a traves de queue_force_average y se elimnan las tareas
				// de medicion de fuerza
				xQueueSend(queue_median , &median,  portMAX_DELAY);
				vTaskDelete(TaskHandle_measure_force);
				vTaskDelete(NULL);
			}
			else{
				// Si todavia no se tienen los TOTAL_MEDIAN_VALUES valores, se agrega el valor
				// medido al arreglo (solo si da un valor positivo de peso ) y se aumenta el
				// contador
				if (f < OFFSET) {
					f_values[counter] = f;
					counter++;
				}
				// Se libera el semaforo para realizar una medicion nueva
				xSemaphoreGive( sem_measure_force );
			}
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que setea el OFFSET de la plataforma
void task_tare( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_500;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	// Se crea la tarea que indica que el modulo hx711 esta listo para medir
	create_task(task_check_hx711_is_ready,"task_check_hx711_is_ready",BASE_SIZE,0,1,&TaskHandle_check_hx711_is_ready);

	uint32_t offset;

	while ( TRUE )
	{
		// Si se recibe el valor de fuerza ya procesado por el calculo de la mediana,
		// se lo guarda en la variable global OFFSET
		if(xQueueReceive(queue_median , &offset,  portMAX_DELAY)){
			OFFSET = offset;

			// Se crea la tarea que recibe un comando por wifi
			create_task(task_receive_wifi,"task_receive_wifi",BASE_SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que mide el peso del usuario
void task_measure_weight( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_500;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	// Se crea la tarea que indica que el modulo hx711 esta listo para medir
	create_task(task_check_hx711_is_ready,"task_check_hx711_is_ready",BASE_SIZE,0,1,&TaskHandle_check_hx711_is_ready);

	uint32_t fu = 0;
	double weight_kg;

	while ( TRUE )
	{
		// Si se recibe el valor del peso, ya procesado con la mediana, se lo guarda
		// en la variable global peso y se toma ese valor para calcular el peso en kg
		if(xQueueReceive(queue_median , &fu,  portMAX_DELAY)){

			WEIGHT = fu;

			weight_kg = ((double)fu - (double)OFFSET) / SCALE;

			// Se crea la tarea que envia el peso por wifi
			create_task(task_print_weight,"task_print_weight",BASE_SIZE,0,1,&TaskHandle_print_weight);
			xQueueSend(queue_weight , &weight_kg,  portMAX_DELAY);

			vTaskDelete(NULL);
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

/*=========================[fin del archivo]=================================*/
