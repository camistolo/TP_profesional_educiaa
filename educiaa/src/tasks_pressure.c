/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#include "tasks_pressure.h"

/*=========================[declaracion de variables]========================*/

QueueHandle_t queue_measure_pressure;
QueueHandle_t queue_matrix;
SemaphoreHandle_t sem_pressure_index;
SemaphoreHandle_t sem_pressure_finished;
TaskHandle_t TaskHandle_set_matrix_index;
TaskHandle_t TaskHandle_get_pressure_value;

/*=========================[declaracion de funciones]=========================*/

// Tarea que setea los indices de la celda de la matriz que tiene que ser medida
// y se los envia (mediante la cola queue_measure_pressure) a la tarea
// task_get_pressure_value
void task_set_matrix_index( void* pvParameters )
{
    TickType_t xPeriodicity =  RATE_1;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint8_t row = 0;
    uint8_t col = MAX_COL*2;
    uint8_t index[2] = {row, col};

	while( TRUE )
	{
		if (xSemaphoreTake(sem_pressure_index, portMAX_DELAY))
		{
			// Si todavia no se llego a la ultima columna, se aumenta la columna
			if(col < MAX_COL-1)
			{
				col++;
			} else // Si se llego a la ultima columna, se reinicia la columna y se aumenta la fila
			{
				if (col != MAX_COL*2)
				{
					row ++;
					col = 0;
				}else{
					// Para inicializar el valor de col para la primera fila, ya que
					// el tipo de dato es uint8_t y si se pone como -1 en la inicializacion
					// empieza en la fila 1 en vez de la fila 0.
					row = 0;
					col = 0;
				}
			}

			// Se setea el indice y se envia a traves de la cola queue_measure_pressure a la tarea
			// task_get_pressure_value
			index[0] = row;
			index[1] = col;

			xQueueSend( queue_measure_pressure, &index, portMAX_DELAY );
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que recibe un indice y lee el valor de presion de la celda correspondiente
// a ese indice. El valor de presion se envia a la cola queue_print
void task_get_pressure_value( void* pvParameters )
{
    TickType_t xPeriodicity =  RATE_1;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint16_t sensor_value = 1;
	uint8_t row,col;
	uint8_t index[2];

	while( TRUE )
	{
		if(xQueueReceive( queue_measure_pressure, &( index ), portMAX_DELAY))
		{
			row = index[0];
			col = index[1];

			// Si se llega al final de la matriz, se eliminan las tareas de presion
			// y se envia un semaforo indicando que la medicion se completo
			if (row == MAX_ROW)
			{
				xSemaphoreGive( sem_pressure_finished );
				vTaskDelete(TaskHandle_set_matrix_index);
				//vTaskDelete(NULL);
				vTaskDelete(TaskHandle_get_pressure_value);
			}

			// Se setean la fila y columna correspondientes y se lee el valor
			SetDeMuxChannel(row);
			gpioWrite( demuxSIG, HIGH );
			SetMuxChannel(col);
			sensor_value = adcRead(muxSIG);
			gpioWrite(demuxSIG, LOW);

			// Se encola el valor de presion de la celda en queue_print
			xQueueSend( queue_matrix, &sensor_value, portMAX_DELAY );

			// Se pide un indice diferente a traves de la liberacion del semaforo
			// sem_pressure_index
			xSemaphoreGive( sem_pressure_index );
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Funcion que setea la columna a leer
void SetMuxChannel(uint8_t channel)
{
	gpioWrite( muxS0, channel & 1 );
	gpioWrite( muxS1, (channel >> 1) & 1 );
	gpioWrite( muxS2, (channel >> 2) & 1 );
	gpioWrite( muxS3, (channel >> 3) & 1 );
}

// Funcion que setea la fila a leer
void SetDeMuxChannel(uint8_t channel)
{
	gpioWrite( demuxS0, channel & 1 );
	gpioWrite( demuxS1, (channel >> 1) & 1 );
	gpioWrite( demuxS2, (channel >> 2) & 1 );
	gpioWrite( demuxS3, (channel >> 3) & 1 );
}

/*=========================[fin del archivo]=================================*/
