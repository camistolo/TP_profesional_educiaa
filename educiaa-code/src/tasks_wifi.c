/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"
#include "utilities.h"
#include "tasks_wifi.h"

/*==================[macros and definitions]=================================*/

//DEBUG_PRINT_ENABLE;

char message_buffer;
int index = 0;
char meas_char;

// These are test values. Thay arre declared global, in order to know
// the last "measurment". In the final code these variables will be
// declared in each task.
float weight_meas = 50;
float force_meas = 11;
float pressure_meas = 56.3;
float height_meas = 1;


QueueHandle_t xMeasureTypeQueue = NULL;
QueueHandle_t xMeasurementQueue = NULL;

void task_wait_wifi( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Periodic task every 1000 ms

	TickType_t xLastWakeTime = xTaskGetTickCount();

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{

		meas_char = uartRxRead( UART_USED );
		vTaskDelay(1); // Overrun Error. Receives information faster than the code can process it.

		if( FRAME_MAX_SIZE-1==index )
		{
			// reset frame
			index = 0;
		}

		if( meas_char == '>' )
		{
			if( index==0 )
			{
				// 1st byte of the frame
			}
			else
			{
				// For the start of the frame fuerzo el arranque del frame
				index = 0;
			}

			index++;
		}
		else if( meas_char == '<' )
		{
			// Close the frame only if I received the start char
			if( index>=1 )
			{
				// End of frame

				index++;

				xTaskCreate(
						task_choose_meas,                  	// Function of the task to execute
						( const char * )"task_choose_meas", // Name of the task, as a user friendly string
						configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
						0,                            		// Task parameters
						tskIDLE_PRIORITY+1,           		// Task priority
						0                             		// Pointer to the task created in the system
					);

				xQueueSend( xMeasureTypeQueue, ( void * ) &message_buffer, ( TickType_t ) 0 );

				vTaskDelete(NULL);
			}
			else
			{
				// Do nothing, discard the byte
			}
		}
		else
		{
			// Close the end of the frame only if I receive a start char
			if( index >= 1 )
			{
				// Save data
				message_buffer = meas_char;

				index++;
			}
			else
			{
				// Do nothing, discard the byte
			}
		}
	}
}

void task_choose_meas( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char meas_char_new;
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xMeasureTypeQueue, &( meas_char_new ), ( TickType_t ) 10 ) == pdTRUE)
		{
			switch (meas_char_new)
			{
				case WEIGHT_MEAS:
					// Activate weight measurement task

					xTaskCreate(
							task_weight,                  	// Function of the task to execute
							( const char * )"task_weight", 	// Name of the task, as a user friendly string
							configMINIMAL_STACK_SIZE*2,   	// Stack quantity of the task
							0,                            	// Task parameters
							tskIDLE_PRIORITY+1,           	// Task priority
							0                             	// Pointer to the task created in the system
					);

					vTaskDelete(NULL);

					break;
				case FORCE_MEAS:
					// Activate force measurement task

					xTaskCreate(
							task_force,                  	// Function of the task to execute
							( const char * )"task_force", 	// Name of the task, as a user friendly string
							configMINIMAL_STACK_SIZE*2,   	// Stack quantity of the task
							0,                            	// Task parameters
							tskIDLE_PRIORITY+1,           	// Task priority
							0                             	// Pointer to the task created in the system
					);

					vTaskDelete(NULL);

					break;

				case PRESSURE_MEAS:
					// Activate pressure measurement task

					xTaskCreate(
							task_pressure,                  	// Function of the task to execute
							( const char * )"task_pressure", 	// Name of the task, as a user friendly string
							configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
							0,                            		// Task parameters
							tskIDLE_PRIORITY+1,           		// Task priority
							0                             		// Pointer to the task created in the system
					);

					vTaskDelete(NULL);

					break;

				case HEIGHT_MEAS:
					// Activate height measurement task

					xTaskCreate(
							task_height,                  	// Function of the task to execute
							( const char * )"task_height", 	// Name of the task, as a user friendly string
							configMINIMAL_STACK_SIZE*2,   	// Stack quantity of the task
							0,                            	// Task parameters
							tskIDLE_PRIORITY+1,           	// Task priority
							0                          		// Pointer to the task created in the system
					);

					vTaskDelete(NULL);

					break;

				default:
					uartWriteString( UART_USB, "Error: Invalid measurement option.\n");

					xTaskCreate(
							task_wait_wifi,                  	// Function of the task to execute
							( const char * )"task_wait_wifi", 	// Name of the task, as a user friendly string
							configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
							0,                            		// Task parameters
							tskIDLE_PRIORITY+1,           		// Task priority
							0                          			// Pointer to the task created in the system
					);

					vTaskDelete(NULL);
			}

		}
		vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
	}
}


void task_weight( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char weight_meas_str[20];
	char data_aux [20] = ">1";
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		// Measure weight
		if(weight_meas > 100)
		{
			weight_meas = 50;
		}else
		{
			weight_meas++;
		}

		gcvt (weight_meas, 10, weight_meas_str);

		strcat(data_aux, weight_meas_str);
		strcat(data_aux, "<");

		xTaskCreate(
				task_send_wifi,                  	// Function of the task to execute
				( const char * )"task_send_wifi", 	// Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
				0,                            		// Task parameters
				tskIDLE_PRIORITY+1,           		// Task priority
				0                          			// Pointer to the task created in the system
		);

		xQueueSend( xMeasurementQueue, ( void * ) &data_aux, ( TickType_t ) 0 );

		vTaskDelete(NULL);
	}
}

void task_force( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char force_meas_str[20];
	char data_aux [20] = ">2";
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		// Measure weight
		if(force_meas > 20)
		{
			force_meas = 10;
		}else
		{
			force_meas++;
		}

		gcvt (force_meas, 10, force_meas_str);

		strcat(data_aux, force_meas_str);
		strcat(data_aux, "<");

		xTaskCreate(
				task_send_wifi,                  	// Function of the task to execute
				( const char * )"task_send_wifi", 	// Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
				0,                            		// Task parameters
				tskIDLE_PRIORITY+1,           		// Task priority
				0                          			// Pointer to the task created in the system
		);

		xQueueSend( xMeasurementQueue, ( void * ) &data_aux, ( TickType_t ) 0 );

		vTaskDelete(NULL);
	}
}


void task_pressure( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char pressure_meas_str[20];
	char data_aux [20] = ">3";
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		// Measure weight
		if(pressure_meas > 100)
		{
			pressure_meas = 50;
		}else
		{
			pressure_meas++;
		}

		gcvt (pressure_meas, 10, pressure_meas_str);

		strcat(data_aux, pressure_meas_str);
		strcat(data_aux, "<");

		xTaskCreate(
				task_send_wifi,                  	// Function of the task to execute
				( const char * )"task_send_wifi", 	// Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
				0,                            		// Task parameters
				tskIDLE_PRIORITY+1,           		// Task priority
				0                          			// Pointer to the task created in the system
		);

		xQueueSend( xMeasurementQueue, ( void * ) &data_aux, ( TickType_t ) 0 );

		vTaskDelete(NULL);
	}
}


void task_height( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char height_meas_str[20];
	char data_aux [20] = ">4";
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		// Measure weight
		if(height_meas > 2)
		{
			height_meas = 1;
		}else
		{
			height_meas = height_meas+0.1;
		}

		gcvt (height_meas, 10, height_meas_str);

		strcat(data_aux, height_meas_str);
		strcat(data_aux, "<");

		xTaskCreate(
				task_send_wifi,                  	// Function of the task to execute
				( const char * )"task_send_wifi", 	// Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
				0,                            		// Task parameters
				tskIDLE_PRIORITY+1,           		// Task priority
				0                          			// Pointer to the task created in the system
		);

		xQueueSend( xMeasurementQueue, ( void * ) &data_aux, ( TickType_t ) 0 );

		vTaskDelete(NULL);
	}
}

void task_send_wifi( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char meas_data[20];

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xMeasurementQueue, &( meas_data ), ( TickType_t ) 10 ) == pdTRUE)
		{
			uartWriteString( UART_USED, meas_data);

			// Go back to idle
			xTaskCreate(
					task_wait_wifi,                  	// Function of the task to execute
					( const char * )"task_wait_wifi", 	// Name of the task, as a user friendly string
					configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
					0,                            		// Task parameters
					tskIDLE_PRIORITY+1,           		// Task priority
					0                          			// Pointer to the task created in the system
			);

			vTaskDelete(NULL);
		}
		vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
	}
}

/*==================[end of file]============================================*/
