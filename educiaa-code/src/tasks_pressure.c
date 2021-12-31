#include "tasks_pressure.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"

QueueHandle_t xMeasurePressureQueue = NULL;
QueueHandle_t xSetIndexQueue = NULL;
QueueHandle_t xPrintQueue = NULL;
TaskHandle_t TaskHandle_set_matrix_index;

int aux_array_a[(MAX_COL*2)-1] = {};
int aux_array_b[(MAX_COL*2)-1] = {};
int aux_array_c[(MAX_COL*2)-1] = {};

void set_matrix_index( void* pvParameters )
{

    TickType_t xPeriodicity =  100 / portTICK_RATE_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    int row = 0;
    int col = -1;
    int aux_queue;
    int index[2] = {row, col};
    // ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xSetIndexQueue, &( row ), ( TickType_t ) 10 ) == pdTRUE)
		{
			if(col < MAX_COL-1)
			{
				col++;
			}else{ // col == MAX_COL
				row ++;
				col = 0;
			}

			index[0] = row;
			index[1] = col;

			xQueueSend( xMeasurePressureQueue, ( void * ) &index, ( TickType_t ) 0 );
		}
	}
}

void get_pressure_value( void* pvParameters )
{

    TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    int sensor_value = 1;
	// ****************************
	// Sin PCB:
	#ifdef SENSOR
	int sensor_in[MAX_ROW] = {demuxY0, demuxY1, demuxY2, demuxY3, demuxY4, demuxY5, demuxY6, demuxY7, demuxY8, demuxY9, demuxY10, demuxY11, demuxY12, demuxY13};
	#endif

	int row = 0;
	int col = 1;
	int index[2];
	int i = 0;
	int int_zero = 0;

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xMeasurePressureQueue, &( index ), ( TickType_t ) 10 ) == pdTRUE)
		{
			row = index[0];
			col= index[1];

			// Finished to Read all matrix values:
			if (row >= MAX_ROW)
			{
				stdioPrintf(UART_USED, "calculate_arrays create task\n");

				xTaskCreate(
					calculate_arrays,                  		// Function of the task to execute
					( const char * )"calculate_arrays", 	// Name of the task, as a user friendly string
					configMINIMAL_STACK_SIZE*2,   			// Stack quantity of the task
					0,                            			// Task parameters
					tskIDLE_PRIORITY+1,           			// Task priority
					0                             			// Pointer to the task created in the system
				);

				row = 0;
				xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );

				vTaskDelete(TaskHandle_set_matrix_index);
				vTaskDelete(NULL);
			}

			#ifdef SENSOR
			gpioWrite( sensor_in[row], HIGH );

			gpioWrite( muxS0, col & 1 );
			gpioWrite( muxS1, (col >> 1) & 1 );
			gpioWrite( muxS2, (col >> 2) & 1 );
			gpioWrite( muxS3, (col >> 3) & 1 );

			sensor_value = adcRead(muxSIG);
			gpioWrite( sensor_in[row], LOW );
			#endif

			#ifdef SENSOR_PCB
			SetDeMuxChannel(row);
			gpioWrite( demuxSIG, HIGH );
			SetMuxChannel(col);
			sensor_value = adcRead(muxSIG);
			gpioWrite(demuxSIG, LOW);
			#endif

			if (col != MAX_COL-1)
			{
				xQueueSendToBack( xPrintQueue, ( void * ) &sensor_value, ( TickType_t ) 10 ); // Enqueue matrix data
				xQueueSendToBack( xPrintQueue, ( void * ) &int_zero, ( TickType_t ) 10 );
			}else{
				xQueueSendToBack( xPrintQueue, ( void * ) &sensor_value, ( TickType_t ) 10 );
				if(row != MAX_ROW-1)
				{
					for( i = 0; i < ((MAX_COL*2)-1); i++)
					{
						xQueueSendToBack( xPrintQueue, ( void * ) &int_zero, ( TickType_t ) 10 ); // Row for interpolation
					}
					#ifdef SENSOR_TEST
					sensor_value = 5;
					#endif
				}
			}

			#ifdef SENSOR_TEST
			sensor_value++;
			#endif

			xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );
		}
	}
}

void calculate_arrays( void* pvParameters)
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	int row;
	int col = 0;
	int index[2];

	bool_t interpol = false;

	int i = 0;
	int val;

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xSetIndexQueue, &( row ), ( TickType_t ) 10 ) == pdTRUE)
		{
			//Get row
		}

		// First row of matrix:
		if (row == 0)
		{
			xQueueReceive( xPrintQueue, &( val ), ( TickType_t ) 10 );
			aux_array_a[col] = val;
			col ++;
			if (col == ((MAX_COL*2)-1))
			{
				row = 1;
				col = 0;
			}
		} else{ // Other rows

			// Get row values:
			if (row % 2 == 0 ) // # Measurement row
			{
				xQueueReceive( xPrintQueue, &( val ), ( TickType_t ) 10 );
				aux_array_b[col] = val;
				col ++;
				if (col == ((MAX_COL*2)-1))
				{

					xTaskCreate(
						interpolate_matrix,                  	// Function of the task to execute
						( const char * )"interpolate_matrix", 	// Name of the task, as a user friendly string
						configMINIMAL_STACK_SIZE*2,   			// Stack quantity of the task
						0,                            			// Task parameters
						tskIDLE_PRIORITY+1,           			// Task priority
						0                             			// Pointer to the task created in the system
					);

					row ++;
					xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );
					vTaskDelete(NULL);
				}

			}else{ // Zeros row
				xQueueReceive( xPrintQueue, &( val ), ( TickType_t ) 10 );
				aux_array_c[col] = val;
				col ++;
				if (col == ((MAX_COL*2)-1))
				{
					row ++;
					col = 0;
				}
			}
		}
	}
}

void interpolate_matrix( void* pvParameters)
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	int row = 0;
	int col = 0;

	bool_t interpol = false;

	int i = 0;
	int j = 0;
	int val;

	stdioPrintf(UART_USED, "interpolate_matrix\n");

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{

		if(xQueueReceive( xSetIndexQueue, &( row ), ( TickType_t ) 10 ) == pdTRUE)
		{
			// Get row value
		}

		// Interpolation 1:
		aux_array_c[(i*2)+1] = (aux_array_a[i*2] + aux_array_a[(i*2)+2] + aux_array_b[i*2] + aux_array_b[(i*2)+2])/4;

		// Interpolation 2:
		if (i != 0){
			aux_array_c[(i*2)] = (aux_array_a[i*2] + aux_array_b[i*2] + aux_array_c[(i*2)+1] + aux_array_c[(i*2)])/4;
		}else{
			// First col:
			aux_array_c[(i*2)] = (aux_array_a[i*2] + aux_array_b[i*2] + aux_array_c[(i*2)+1] )/3;}

		if (row != 3){
			aux_array_a[(i*2)+1] = (aux_array_a[i*2] + aux_array_a[(i*2)+2] + aux_array_c[(i*2)+1] + aux_array_a[(i*2)+1])/4;
		}else{
			// First row:
			aux_array_a[(i*2)+1] = (aux_array_a[i*2] + aux_array_a[(i*2)+2] + aux_array_c[(i*2)+1])/3;}

		if (row != (MAX_ROW*2)-1 ){
			aux_array_b[(i*2)+1] = aux_array_c[(i*2)+1];
		}else{
			// Last row:
			aux_array_b[(i*2)+1] = (aux_array_b[(i*2)] + aux_array_b[(i*2)+2] + aux_array_c[(i*2)+1])/3;}

		if (i != (MAX_COL -2)){
			// Set for next iteration:
			aux_array_c[(i*2)+2] = aux_array_c[(i*2)+1];
		}else{
			// Last col:
			aux_array_c[(i*2)+2] = (aux_array_a[(i*2)+2] + aux_array_b[(i*2)+2] + aux_array_c[(i*2)+1])/3;}

		i++;

		if (i == MAX_COL-1)
		{
			// Enqueue array a:
			for (j = 0; j < ((MAX_COL*2)-1); j++)
			{
				xQueueSendToBack( xPrintQueue, ( void * ) &aux_array_a[j], ( TickType_t ) 10 ); // Enqueue matrix data
			}

			// Enqueue array a:
			for (j = 0; j < ((MAX_COL*2)-1); j++)
			{
				xQueueSendToBack( xPrintQueue, ( void * ) &aux_array_c[j], ( TickType_t ) 10 );
			}

			// Copy array b to a:
			for (j = 0; j < (sizeof(aux_array_a)/sizeof(aux_array_a[0])); j++)
			{
				aux_array_a[j] = aux_array_b[j];
			}

			interpol = false;
			if (row == ((MAX_COL*2)-1))
			{
				for (j = 0; j < ((MAX_COL*2)-1); j++)
				{
					xQueueSendToBack( xPrintQueue, ( void * ) &aux_array_b[j], ( TickType_t ) 10 );
				}

				xTaskCreate(
						print_matrix,                  	// Function of the task to execute
					( const char * )"print_matrix", 	// Name of the task, as a user friendly string
					configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
					0,                            		// Task parameters
					tskIDLE_PRIORITY+1,           		// Task priority
					0                          			// Pointer to the task created in the system
				);

				vTaskDelete(NULL);
			}

			xTaskCreate(
				calculate_arrays,                  	// Function of the task to execute
				( const char * )"calculate_arrays", // Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
				0,                            		// Task parameters
				tskIDLE_PRIORITY+1,           		// Task priority
				0                          			// Pointer to the task created in the system
			);

			xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );
			vTaskDelete(NULL);

		}
	}
}


void print_matrix( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	int row_value;
	int matrix_val;
	int row=0, col=0;

	stdioPrintf(UART_USED, "print_matrix\n");

	stdioPrintf(UART_USED, ">[");
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{

		if(xQueueReceive( xPrintQueue, &( matrix_val ), ( TickType_t ) 10 ) == pdTRUE) // Dequeue matrix data
		{
			if (col < ((MAX_COL*2)-2))
			{
				stdioPrintf(UART_USED, "%d,", matrix_val);
				col++;
			}else{
				col = 0;
				row++;
				if (row < ((MAX_ROW*2)-1))
				{
					#ifdef SENSOR_TEST
					stdioPrintf(UART_USED, "%d;\n", matrix_val);
					#else
					stdioPrintf(UART_USED, "%d;", matrix_val);
					#endif
				}else{
					stdioPrintf(UART_USED, "%d", matrix_val);
				}
			}
		}else{
			stdioPrintf(UART_USED, "]<\n");

			xTaskCreate(
				set_matrix_index,                  		// Function of the task to execute
				( const char * )"set_matrix_index",		// Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   			// Stack quantity of the task
				0,                            			// Task parameters
				tskIDLE_PRIORITY+2,           			// Task priority
				&TaskHandle_set_matrix_index     		// Pointer to the task created in the system
				);

		   xTaskCreate(
				   get_pressure_value,                  // Function of the task to execute
				( const char * )"get_pressure_value",	// Name of the task, as a user friendly string
				configMINIMAL_STACK_SIZE*2,   			// Stack quantity of the task
				0,                            			// Task parameters
				tskIDLE_PRIORITY+2,           			// Task priority
				0                             			// Pointer to the task created in the system
		   );

		   row = 0;
		   xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );

			vTaskDelete(NULL);
		}
	}
}

#ifndef SENSOR_TEST
void SetMuxChannel(int channel)
{
	gpioWrite( muxS0, channel & 1 );
	gpioWrite( muxS1, (channel >> 1) & 1 );
	gpioWrite( muxS2, (channel >> 2) & 1 );
	gpioWrite( muxS3, (channel >> 3) & 1 );
}

void SetDeMuxChannel(int channel)
{
	gpioWrite( demuxS0, channel & 1 );
	gpioWrite( demuxS1, (channel >> 1) & 1 );
	gpioWrite( demuxS2, (channel >> 2) & 1 );
	gpioWrite( demuxS3, (channel >> 3) & 1 );
}
#endif

/*==================[end of file]============================================*/

