#include "tasks_pressure.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"

// ****************************
// Sin PCB:
/*
const int demuxY0 = GPIO8;
const int demuxY1 = GPIO7;
const int demuxY2 = GPIO5;
const int demuxY3 = GPIO3;
const int demuxY4 = GPIO1;
const int demuxY5 = LCD1;
const int demuxY6 = LCD2;
const int demuxY7 = LCD3;
const int demuxY8 = LCDRS;
const int demuxY9 = LCD4;
const int demuxY10 = SPI_MISO;
const int demuxY11 = ENET_TXD1;
const int demuxY12 = ENET_TXD0;
const int demuxY13 = ENET_MDIO;
*/
// ****************************


const int demuxS0 = GPIO1;
const int demuxS1 = GPIO3;
const int demuxS2 = GPIO5;
const int demuxS3 = GPIO7;
const int demuxSIG = GPIO8;


const int muxS0 = T_FIL0; // GPIO25
const int muxS1 = T_FIL3; // GPIO28
const int muxS2 = T_FIL2; // GPIO27
const int muxS3 = T_COL0; //GPIO29
const int muxSIG = CH3;


QueueHandle_t xMeasurePressureQueue = NULL;
QueueHandle_t xSetIndexQueue = NULL;
QueueHandle_t xPrintQueue = NULL;
TaskHandle_t TaskHandle_set_matrix_index;

int sensor_value_test = 1;

float time_diff;
uint32_t Counter = 0;
char * str_aux;
uint32_t cyclesElapsed = 0;
uint32_t msElapsed = 0, usElapsed = 0;

void set_matrix_index( void* pvParameters )
{

    TickType_t xPeriodicity =  100 / portTICK_RATE_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    int row = 0;
    int col = -1;
    int aux_queue;
    int index[2] = {row, col};


    cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);
    cyclesCounterReset();
    // ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xSetIndexQueue, &( aux_queue ), ( TickType_t ) 10 ) == pdTRUE)
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

    int sensor_value = 0;
	// ****************************
	// Sin PCB:
	// int sensor_in[MAX_ROW] = {demuxY0, demuxY1, demuxY2, demuxY3, demuxY4, demuxY5, demuxY6, demuxY7, demuxY8, demuxY9, demuxY10, demuxY11, demuxY12, demuxY13};
	// ****************************

	//int sensor_matrix[(MAX_ROW*2)-1][(MAX_COL*2)-1]={}; // Armo la matriz con los espacios para interpolar

	int row = 0;
	int col = 1;
	int index[2];
	int i = 0;
	int int_zero = 0;

	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xMeasurePressureQueue, &( index ), ( TickType_t ) 10 ) == pdTRUE)
		{
			row = index[0];
			col= index[1];

			// Finished to Read all matrix values:
			//if (row >= ((MAX_ROW*2)-1))
			if (row >= MAX_ROW)
			{
				// Time execution measurement:
				cyclesElapsed = cyclesCounterRead();
			   // Convierte el valor de ciclos en micro segundos
			   usElapsed = cyclesCounterToUs(cyclesElapsed);
			   // Imprime por pantalla el valor de los ciclos y los micro segundos transcurridos.
			   stdioPrintf(UART_USB, "Los ciclos en esperar 3 ms son: %d. En micro segundos (aprox) %d\n\r", cyclesElapsed, usElapsed);

				xTaskCreate(
					print_matrix,                  		// Function of the task to execute
					( const char * )"print_matrix", 	// Name of the task, as a user friendly string
					configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
					0,                            		// Task parameters
					tskIDLE_PRIORITY+1,           		// Task priority
					0                             		// Pointer to the task created in the system
				);


				vTaskDelete(TaskHandle_set_matrix_index);
				vTaskDelete(NULL);
			}


			SetDeMuxChannel(row);
			gpioWrite(demuxSIG, HIGH);
			// ****************************
			// Sin PCB:
			//gpioWrite( sensor_in[row], HIGH );
			// ****************************

			SetMuxChannel(col);
			//SetMuxChannel(index[1]);
			// ****************************
			// Sin PCB:
			/*
			gpioWrite( muxS0, col & 1 );
			gpioWrite( muxS1, (col >> 1) & 1 );
			gpioWrite( muxS2, (col >> 2) & 1 );
			gpioWrite( muxS3, (col >> 3) & 1 );
			*/
			// ****************************
			sensor_value = adcRead(muxSIG);
			//sensor_matrix[row*2][col*2] = sensor_value;

			gpioWrite(demuxSIG, LOW);
			// ****************************
			// Sin PCB:
			//gpioWrite( sensor_in[row], LOW );



			// To test project, without connecting the sensor matrix.
/*
			sensor_matrix[row][col] = sensor_value_test;
			sensor_value_test ++;
			if (sensor_value_test > 14)
			{
				sensor_value_test = 1;
			}
*/

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
				}
			}

			xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );
		}
	}
}


void print_matrix( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	int row_value;
	int sensor_test;
	int row=0, col=0;

	stdioPrintf(UART_USB, "[");
	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{

		if(xQueueReceive( xPrintQueue, &( sensor_test ), ( TickType_t ) 10 ) == pdTRUE) // Dequeue matrix data
		{
			if (col < ((MAX_COL*2)-2))
			{
				stdioPrintf(UART_USB, "%d,", sensor_test);
				col++;
			}else{
				col = 0;
				row++;
				if (row < ((MAX_ROW*2)-1))
				{
					stdioPrintf(UART_USB, "%d;\n", sensor_test);
				}else{
					stdioPrintf(UART_USB, "%d", sensor_test);
				}
			}
		}else{
			stdioPrintf(UART_USB, "]\n\n");

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

		   xQueueSend( xSetIndexQueue, ( void * ) &row, ( TickType_t ) 0 );

			vTaskDelete(NULL);
		}
	}
}


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

/*==================[end of file]============================================*/

