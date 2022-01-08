#include "tasks_pressure.h"

QueueHandle_t xMeasurePressureQueue;
QueueHandle_t xPrintQueue;
SemaphoreHandle_t sem_pressure_index;
SemaphoreHandle_t sem_pressure_finished;
TaskHandle_t TaskHandle_set_matrix_index;
TaskHandle_t TaskHandle_get_pressure_value;

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
		if (xSemaphoreTake(sem_pressure_index, portMAX_DELAY))
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

			stdioPrintf(UART_USB, "%d, %d\n", index[0], index[1]);

			xQueueSend( xMeasurePressureQueue, ( void * ) &index, portMAX_DELAY );
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

	int row,col;
	int index[2];

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xMeasurePressureQueue, &( index ), portMAX_DELAY))
		{
			row = index[0];
			col = index[1];

			// Finished to Read all matrix values:
			if (row == MAX_ROW)
			{
				uartWriteString(UART_USB, "pressure_done\n");
//				xTaskCreate(
//					print_matrix,					// Function of the task to execute
//					( const char * )"print_matrix", // Name of the task, as a user friendly string
//					configMINIMAL_STACK_SIZE*2,   	// Stack quantity of the task
//					0,                            	// Task parameters
//					tskIDLE_PRIORITY+1,           	// Task priority
//					0                             	// Pointer to the task created in the system
//				);
//
				xSemaphoreGive( sem_pressure_finished );
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

			stdioPrintf(UART_USB, "%d\t", sensor_value);

			xQueueSendToBack( xPrintQueue, ( void * ) &sensor_value, portMAX_DELAY ); // Enqueue matrix data

			#ifdef SENSOR_TEST
			sensor_value++;
			if (col == MAX_COL-1)
			{
				sensor_value = 1;
			}

			#endif

			xSemaphoreGive( sem_pressure_index );
		}
	}
}

//void print_matrix( void* pvParameters )
//{
//	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
//	TickType_t xLastWakeTime = xTaskGetTickCount();
//
//	int row_value;
//	int matrix_val;
//	int row=0, col=0;
//
//	char buffer[700];
//
//	stdioPrintf(UART_USED, ">3["); // 3 to indicate pressure measurement.
//	//sprintf(buffer, ">[");
//	// ---------- REPEAT FOR EVER --------------------------
//	while( TRUE )
//	{
//
//		if(xQueueReceive( xPrintQueue, &( matrix_val ), portMAX_DELAY)) // Dequeue matrix data
//		{
//			if (col < (MAX_COL-1))
//			{
//				stdioPrintf(UART_USED, "%d,", matrix_val);
//				//sprintf(buffer, "%s%d,", buffer, matrix_val);
//				col++;
//			}else{
//				col = 0;
//				row++;
//				if (row < MAX_ROW)
//				{
//					stdioPrintf(UART_USED, "%d;", matrix_val);//stdioPrintf(UART_USED, "%d;\n", matrix_val);
//					//sprintf(buffer, "%s%d;", buffer, matrix_val);
//				}else{
//					stdioPrintf(UART_USED, "%d", matrix_val);
//					//sprintf(buffer, "%s%d", buffer, matrix_val);
//				}
//			}
//		}else{
//			stdioPrintf(UART_USED, "]<\n");
//			//sprintf(buffer, "%s]<\n", buffer);
//
//			//stdioPrintf(UART_USED, "%s", buffer);
//
//		   vTaskDelete(NULL);
//		}
//	}
//}

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
