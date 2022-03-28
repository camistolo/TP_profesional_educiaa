/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#include "tasks_wifi.h"

/*=========================[declaracion y definicion de variables]========================*/

QueueHandle_t queue_command_wifi;
extern QueueHandle_t queue_jump_parameters;
extern QueueHandle_t queue_matrix;
extern QueueHandle_t queue_jump_force;
extern QueueHandle_t queue_weight;

extern SemaphoreHandle_t sem_pressure_finished;

TaskHandle_t TaskHandle_print_weight;
extern TaskHandle_t TaskHandle_measure_weight;
extern TaskHandle_t TaskHandle_measure_jump;

// Tipo de medicion
typedef enum{
	WEIGHT_MEAS = 1,
	JUMP_MEAS = 3
} measurement_type_t;

/*=========================[definicion de funciones]=========================*/

// Tarea que recibe una cadena por wifi del tipo >dato<, donde dato es el comando
// que se le va a enviar, a traves de la cola queue_command_wifi, a la tarea
// task_choose_measurement
void task_receive_wifi( void* pvParameters )
{
	TickType_t xPeriodicity =  RATE_1000;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char meas_char;
	uint8_t message_buffer;
	uint8_t index;

	while( TRUE )
	{
		meas_char = uartRxRead( UART_USED );
		// Como se recibe la informacion mas rapido que lo que el codigo lo puede procesar, es necesario colocar este
		// delay (esto se denomina Overrun Error)
		vTaskDelay(1);

		// Si se recibieron mas caracteres que los esperados, se reinicia el paquete
		if( FRAME_MAX_SIZE-1==index )
		{
			index = 0;
		}

		// Recepcion del caracter de apertura
		if( meas_char == '>' )
		{
			if( index==0 )
			{
				// 1er byte del frame
			}
			else
			{
				// Fuerzo el arranque del frame (descarto lo anterior)
				index = 0;
			}

			// Incremento del indice
			index++;
		}
		// Recepcion del caracter de cierre
		else if( meas_char == '<' )
		{
			// Solo se cierra el fin de frame si al menos se recibio un start
			if( index>=1 )
			{
				// Creacion de la tarea que recibe el comandon y rcea la tarea correspondiente a el
				create_task(task_choose_measurement,"task_choose_measurement",BASE_SIZE,0,1,NULL);
				xQueueSend(queue_command_wifi , &message_buffer,  portMAX_DELAY);

				vTaskDelete(NULL);
			}
			else
			{
				// Se descarta el byte
			}
		}
		else
		{
			// Solo se cierra el fin de frame si al menos se recibio un start
			if( index >= 1 )
			{
				// Se guarda el dato recibido
				message_buffer = meas_char - '0';

				index++;
			}
			else
			{
				// Se descarta el byte
			}
		}
		// Delay periodico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que recibe un comando y crea a la tarea correspondiente al valor recibido
void task_choose_measurement( void* pvParameters )
{

	TickType_t xPeriodicity =  RATE_1000;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	measurement_type_t meas_type;

	while( TRUE )
	{
		if(xQueueReceive(queue_command_wifi , &meas_type,  portMAX_DELAY))
		{
			switch (meas_type) {
				case WEIGHT_MEAS:
					// Se crea la tarea de medicion de peso
					create_task(task_measure_weight,"task_measure_weight",BASE_SIZE,0,1,&TaskHandle_measure_weight);
					break;
				case JUMP_MEAS:
					// Se crea la tarea que mide el salto
					stdioPrintf(UART_USB, "SALTE!\n");
					create_task(task_measure_jump,"task_measure_jump",BASE_SIZE,0,1,&TaskHandle_measure_jump);
					break;
			}
		}
		vTaskDelete(NULL);
	}
}

// Tarea que envia al modulo wifi el peso del usuario
void task_print_weight( void* pvParameters )
{
	double weight;
	//char weight_str[20] = {};
	char weight_str[20] = {};

	while( TRUE )
	{
		// Se recibe la estructura con los parametros y se envian al modulo wifi
		if(xQueueReceive( queue_weight, &( weight ), RATE_20))
		{
			gcvt(weight, 10, weight_str);
			stdioPrintf(UART_USED, ">1%s<\n",weight_str);

			// Se crea la tarea que recibe el comando por wifi
			create_task(task_receive_wifi,"task_receive_wifi",BASE_SIZE,0,1,NULL);
			vTaskDelete(NULL);
		}
	}
}

// Tarea que envia al modulo wifi la matriz de valores de presion
void task_print_matrix( void* pvParameters )
{
	uint16_t matrix_val;
	uint8_t row=0, col=0;

	while( TRUE )
	{
		// Impresion de la matriz, a medida que se reciben los valores
		if(xQueueReceive( queue_matrix, &( matrix_val ), RATE_20))
		{
			if (col < (MAX_COL-1))
			{
				if (col == 0)
				{
					stdioPrintf(UART_USED, ">%c[", 65+row);
				}
				stdioPrintf(UART_USED, "%d,", matrix_val);
				col++;
			}else{
				stdioPrintf(UART_USED, "%d]<\n", matrix_val);
				col = 0;
				row++;
			}
		}else{	// Fin de impresion de la matriz, se crea la tarea de impresion del vector

			create_task(task_print_parameters,"task_print_parameters",BASE_SIZE,0,1,NULL);

		    vTaskDelete(NULL);
		}
	}
}


// Tarea que envia al modulo wifi los parametros del salto
void task_print_parameters( void* pvParameters )
{
	struct jump_parameters jp;

	char vel_str[20] = {};
	char t_str[20] = {};
	char height_str[20] = {};
	char power_str[20] = {};

	while( TRUE )
	{
		// Se recibe la estructura con los parametros y se envian al modulo wifi
		if(xQueueReceive( queue_jump_parameters, &( jp ), RATE_20))
		{
			gcvt(jp.vel, 10, vel_str);
			gcvt(jp.t, 10, t_str);
			gcvt(jp.height, 10, height_str);
			gcvt(jp.power, 10, power_str);

			stdioPrintf(UART_USED, ">3{\"speed\":%s,\"power\":%s,\"time\":%s,\"height\":%s}<\n", vel_str, power_str, t_str, height_str);
			// Se crea la tarea que recibe el comando por wifi
			create_task(task_receive_wifi,"task_receive_wifi",BASE_SIZE,0,1,NULL);
			vTaskDelete(NULL);
		}else{
			// Si entra aca quiere decir que no se salto, ya que la cola
			// queue_jump_parameters se encontraba vacia
			stdioPrintf(UART_USED, ">3{\"speed\":0,\"power\":0,\"time\":0,\"height\":0}<\n");
			create_task(task_receive_wifi,"task_receive_wifi",BASE_SIZE,0,1,NULL);
			vTaskDelete(NULL);
		}
	}
}

/*=========================[fin del archivo]=================================*/
