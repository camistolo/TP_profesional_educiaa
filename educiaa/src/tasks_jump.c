/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#include "tasks_jump.h"

/*=========================[declaracion de variables]========================*/

extern uint32_t OFFSET;
extern uint32_t PESO;

QueueHandle_t queue_jump_parameters;
QueueHandle_t queue_print;
QueueHandle_t queue_test;
extern QueueHandle_t queue_force;
extern QueueHandle_t queue_jump;
extern QueueHandle_t queue_print;

SemaphoreHandle_t sem_matrix_print_finished;
SemaphoreHandle_t sem_vector_print_finished;
SemaphoreHandle_t sem_parameters_print_finished;
extern SemaphoreHandle_t sem_measure_force;
extern SemaphoreHandle_t sem_pressure_index;
extern SemaphoreHandle_t sem_pressure_finished;

TaskHandle_t TaskHandle_measure_jump;
TaskHandle_t TaskHandle_print_measurements;
extern TaskHandle_t TaskHandle_set_matrix_index;
extern TaskHandle_t TaskHandle_get_pressure_value;
extern TaskHandle_t TaskHandle_measure_force;

/*=========================[declaracion de funciones]=========================*/

// Tarea que calcula que organiza el calculo de la fuerza y de la presion durante
// el salto
void task_measure_jump( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_5;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uint32_t f = 0;
	uint32_t last_f = OFFSET;
	uint8_t force_counter = 0;
	uint16_t zeroed_newton;

	// Se crea la tarea de medicion de fuerza para empezar a medir en modo simple
	force_measurement_mode_t mode = SIMPLE_MODE;
	create_task(task_measure_force,"task_measure_force",BASE_SIZE,&mode,1,&TaskHandle_measure_force);

	while ( TRUE )
	{
		// Si todavia no se recibieron todos los valores del salto, se libera el semaforo
		// sem_measure_force para que se siga midiendo
		if (force_counter < JUMP_N) {
			xSemaphoreGive( sem_measure_force );

			// Se recibe un valor de fuerza
			if(xQueueReceive(queue_force , &f,  RATE_100)){

				force_counter++;

				// Si se recibio el ultimo valor de fuerza, se elimina la tarea de fuerza
				if (force_counter == JUMP_N) {
					vTaskDelete(TaskHandle_measure_force);

					// Se mide la presion cuando se terminan de medir los 100 valores de fuerza
					xSemaphoreGive( sem_pressure_index );
					create_task(task_set_matrix_index,"task_set_matrix_index",BASE_SIZE,0,2,&TaskHandle_set_matrix_index);
					create_task(task_get_pressure_value,"task_get_pressure_value",BASE_SIZE,0,2,&TaskHandle_get_pressure_value);
					xSemaphoreTake(sem_pressure_finished, portMAX_DELAY);

					// Se llama a la tarea task_calculate_jump_parameters para calcular la velocidad,
					// tiempo, potencia y altura.
					create_task(task_calculate_jump_parameters,"task_calculate_jump_parameters",BASE_SIZE,0,1,NULL);
					vTaskDelete(NULL);
				}

				// Si el valor que se midio es mayor al offset (es como si se midieran kilogramos negativos), se
				// setea la fuerza al valor anterior de fuerza medido.
				if (f > OFFSET) {
					f = last_f;
				}

				// Se actualiza el valor anterior de fuerza al actual
				last_f = f;

				// Se calcula el valor de fuerza en Newtons pero se pone el cero en el valor del peso del usuario
				// en Newton
				zeroed_newton = ((int16_t)f - (int16_t)PESO) * GRAVITY / SCALE;

				// Se envia el valor de la fuerza a la cola queue_jump para dsps calcular los parametros
				xQueueSend( queue_jump, &zeroed_newton, portMAX_DELAY);
			}
		}

		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que calcula los parametros del salto
void task_calculate_jump_parameters( void* taskParmPtr )
{
	TickType_t xPeriodicity =  RATE_5;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	bool accelerating_downards = false;
	bool deaccelerating = false;
	bool accelerating_upwards = false;
	bool on_air = false;
	uint8_t time_on_air = 0;
	struct jump_parameters jp;
	const double weight = ((double)(PESO) - (double)(OFFSET)) / (double)SCALE;
	uint16_t zeroed_newton;

	while ( TRUE )
	{
		// Se hace un peek para que los valores se reciban despues para imprimirse por wifi
		if(xQueuePeek(queue_jump , &zeroed_newton,  portMAX_DELAY)){
			// Etapa 0: todavia no empezo el salto
			if (accelerating_downards == false) {
				// Si se nota un valor en Newton menor a -20 (es como decir menor a 0, pero
				// se usa un umbral para estar seguros)
				if(zeroed_newton <= DOWNWARD_ACCELERATION_THRESHOLD) {
					accelerating_downards = true;
				}
			}
			// Etapa 1: acelerando hacia abajo
			if (accelerating_downards == true && deaccelerating == false) {
				// Si el valor en Newton es positivo, ya se comenzo la desaceleracion
				if (zeroed_newton > DEACCELERATION_THRESHOLD) {
					deaccelerating = true;
				}
			}
			// Etapa 2: desaceleracion y aceleracion hacia arriba
			if (deaccelerating == true && on_air == false) {
				// Si el valor en Newton es negativo, la persona ya esta en el aire
				if (zeroed_newton < ON_AIR_THRESHOLD) {
					on_air = true;
				}
			}
			// Etapa 3: En el aire
			if (on_air == true){
				// Si el valor es negativo (sigue en el aire), se aumenta el contador,
				// porque el tiempo entre cada medicion es siempre de 20 ms, entonces
				// el tiempo en el aire va a ser la cantidad de mediciones por 0,02 s
				if (zeroed_newton < ON_AIR_THRESHOLD){
					time_on_air++;
				// Si se mide un valor positivo de fuerza, el usuario ya cayo y se
				// pueden calcular los parametros
				} else {
					jp.t = time_on_air * FORCE_MEASUREMENT_PERIOD / 2;	// t es el tiempo hasta la altura maxima, por eso se lo divide por 2
					jp.vel = jp.t * GRAVITY;
					jp.height = (jp.vel * jp.vel)/ (2 * GRAVITY);
					jp.power = jp.vel * weight * GRAVITY;

					create_task(task_print_matrix,"task_print_matrix",BASE_SIZE,0,1,NULL);
					vTaskDelete(NULL);
				}
			}
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que envia al modulo wifi la matriz de valores de presion
void task_print_matrix( void* pvParameters )
{
	TickType_t xPeriodicity =  RATE_1000;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uint16_t matrix_val;
	uint8_t row=0, col=0;

	// El numero 2 le indica al modulo wifi que es una medicion de salto
	stdioPrintf(UART_USED, ">2{\"matrix\":[");

	while( TRUE )
	{
		// Impresion de la matriz, a medida que se reciben los valores
		if(xQueueReceive( queue_print, &( matrix_val ), RATE_20))
		{
			if (col < (MAX_COL-1))
			{
				stdioPrintf(UART_USED, "%d,", matrix_val);
				col++;
			}else{
				col = 0;
				row++;
				if (row < MAX_ROW)
				{
					stdioPrintf(UART_USED, "%d;", matrix_val);
				}else{
					stdioPrintf(UART_USED, "%d", matrix_val);
				}
			}
		}else{	// Fin de impresion de la matriz, se crea la tarea de impresion del vector
			stdioPrintf(UART_USED, "],");

			create_task(task_print_vector,"task_print_vector",BASE_SIZE,0,1,NULL);

		    vTaskDelete(NULL);
		}
	}
}

// Tarea que envia al modulo wifi el vector de valores de fuerza durante el salto
void task_print_vector( void* pvParameters )
{
	TickType_t xPeriodicity =  RATE_1000;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uint16_t zeroed_newton_val;
	uint8_t index = 0;

	stdioPrintf(UART_USED, "\"vector\":[");

	while( TRUE )
	{
		// Impresion del vector, a medida que se reciben los valores
		if(xQueueReceive( queue_jump, &( zeroed_newton_val ), (TickType_t) 10)) // Dequeue matrix data
		{
			stdioPrintf(UART_USED, "%d,", zeroed_newton_val);
			index ++;
		}else{	// Fin de impresion del vector, se crea la tarea de impresion del vector
			stdioPrintf(UART_USED, "],");
			create_task(task_print_parameters,"task_print_parameters",BASE_SIZE,0,1,NULL);
			vTaskDelete(NULL);
		}
	}
}

// Tarea que envia al modulo wifi los parametros del salto
void task_print_parameters( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	struct jump_parameters jp;

	while( TRUE )
	{
		// Se recibe la estructura con los parametros y se envian al modulo wifi
		if(xQueueReceive( queue_jump_parameters, &( jp ), RATE_20))
		{
			stdioPrintf(UART_USED, "\"speed\":%d,\"power\":%d,\"time\":%d,\"height\":%d}<\n", jp.vel, jp.power, jp.t, jp.height);

			// Se crea la tarea que recibe el comando por wifi
			create_task(task_receive_wifi,"task_receive_wifi",BASE_SIZE,0,1,NULL);
			vTaskDelete(NULL);
		}
	}
}

/*=========================[fin del archivo]=================================*/
