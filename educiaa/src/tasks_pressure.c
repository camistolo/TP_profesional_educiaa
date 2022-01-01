#include "tasks_pressure.h"

DEBUG_PRINT_ENABLE;


/*==================[definiciones de datos internos]=========================*/

QueueHandle_t queue_pressure;

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

int sensor_matrix[(MAX_ROW*2)-1][(MAX_COL*2)-1]={}; // Armo la matriz con los espacios para interpolar

SemaphoreHandle_t xSemaphore;

/*==================[definiciones de funciones internas]=====================*/

void task_pressure( void* pvParameters )
{

	//debugPrintlnString( "entra tarea measure\n" );

	xSemaphore = xSemaphoreCreateMutex();

    TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms

    TickType_t xLastWakeTime = xTaskGetTickCount();

	int row = 0; // Contador
	int col = 0; // Contador
	int aux = 0;

	int sensor_value = 0;
	//int sensor_in[MAX_ROW] = {demuxY0, demuxY1, demuxY2, demuxY3, demuxY4, demuxY5, demuxY6, demuxY7, demuxY8, demuxY9, demuxY10, demuxY11, demuxY12, demuxY13};

	//int sensor_matrix[(MAX_ROW*2)-1][(MAX_COL*2)-1]={}; // Armo la matriz con los espacios para interpolar

	//debugPrintlnString( "entra while measure \n" );


    // ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{
		//if (xSemaphore != NULL)
		//{


			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 10 ticks to see if it becomes free. */
			//if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
			//{


				/*
				gpioWrite( LEDB ,1 );
				debugPrintlnString( "LEDB ON \n" );
				vTaskDelay( 500 / portTICK_RATE_MS );
				gpioWrite( LEDB ,0 );
				debugPrintlnString( "LEDB OFF \n" );
				*/
				uartWriteString(UART_USB,  "task_pressure entro \n" );
				for (row = 0; row < MAX_ROW; row++)
				{
					SetDeMuxChannel(row);
					gpioWrite(demuxSIG, HIGH);
					//gpioWrite( sensor_in[row], HIGH );

					for (col = 0; col<MAX_COL; col++)
					{
						//SetMuxChannel(col);
						gpioWrite( muxS0, col & 1 );
						gpioWrite( muxS1, (col >> 1) & 1 );
						gpioWrite( muxS2, (col >> 2) & 1 );
						gpioWrite( muxS3, (col >> 3) & 1 );
						////////////////////////////////
						sensor_value = adcRead(muxSIG);
						sensor_matrix[row*2][col*2] = sensor_value;

						//delay(DELAY_TIME);
					}
					gpioWrite(demuxSIG, LOW);
					//gpioWrite( sensor_in[row], LOW );
					//delay(100);
					//vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
				}

				// Envia la tarea al estado bloqueado durante xPeriodicity (delay periodico)
				//vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
				//debugPrintlnString( "termina task \n" );


				//################################################
				// Calculo de los datos:

				// Primera interpolación:
				for (row = 1; row<(MAX_ROW*2)-1; row+=2)
				{
					for (col = 1; col<(MAX_COL*2)-1; col+=2)
					{
						sensor_matrix[row][col] = (sensor_matrix[row-1][col-1] + sensor_matrix[row-1][col+1] + sensor_matrix[row+1][col+1] + sensor_matrix[row+1][col-1])/4;
					}
				}

				// Segunda interpolación:
				for (row = 0; row<(MAX_ROW*2)-1; row++)
				{
					if (row%2 == 0) // Fila par
					{
						for (col = 1; col<(MAX_COL*2)-1; col+=2)
						{
							if (row == 0)
								sensor_matrix[row][col] = (sensor_matrix[row][col-1] + sensor_matrix[row][col+1] + sensor_matrix[row+1][col])/3;
							else if (row == (MAX_ROW*2)-2) // Ultima fila
								sensor_matrix[row][col] = (sensor_matrix[row][col-1] + sensor_matrix[row][col+1] + sensor_matrix[row-1][col])/3;
							else
								sensor_matrix[row][col] = (sensor_matrix[row-1][col] + sensor_matrix[row][col+1] + sensor_matrix[row+1][col] + sensor_matrix[row][col-1])/4;
						}
					}
					else // Fila impar:
					{
						for (col = 0; col<(MAX_COL*2)-1; col+=2)
						{
							if (col == 0)
								sensor_matrix[row][col] = (sensor_matrix[row-1][col] + sensor_matrix[row][col+1] + sensor_matrix[row+1][col])/3;
							else if (col == (MAX_COL*2)-2)
								sensor_matrix[row][col] = (sensor_matrix[row-1][col] + sensor_matrix[row][col-1] + sensor_matrix[row+1][col])/3;
							else
								sensor_matrix[row][col] = (sensor_matrix[row-1][col] + sensor_matrix[row][col+1] + sensor_matrix[row+1][col] + sensor_matrix[row][col-1])/4;
						}
					}
				}
				//xQueueSend(queue_pressure , &sensor_matrix,  portMAX_DELAY);

				uartWriteString(UART_USB,  "task_pressure salio \n" );
				vTaskDelete(NULL);

				vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}
}
	//################################################

void task_print_matrix( void* pvParameters )
{

	//debugPrintlnString( "entra tarea print \n" );

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms

	TickType_t xLastWakeTime = xTaskGetTickCount();


	int row = 0; // Contador
	int col = 0; // Contador

	int sensor_matrix_aux[(MAX_ROW*2)-1][(MAX_COL*2)-1]={};
	//debugPrintlnString( "entra while print \n" );


	// ---------- REPETIR POR SIEMPRE --------------------------

	/*while( TRUE )
	{

	    //if( xSemaphore != NULL )
		if(xQueueReceive(queue_pressure , &sensor_matrix_aux,  portMAX_DELAY))
		{


			// Imprimo los datos por puerto serie de manera ordenada:
			debugPrintString("[");

			for (row = 0; row < (MAX_ROW*2)-1; row++)
			{
				for (col = 0; col<(MAX_COL*2)-1; col++)
				{
					debugPrintInt(sensor_matrix[row][col]);
					if (col != (MAX_COL*2)-2)
						debugPrintString(",");
				}
				if (row != (MAX_ROW*2)-2)
					debugPrintString(";\n");
			}

			debugPrintString("]\n");
			vTaskDelete(NULL);
			//vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

	    }
	}*/

	while( TRUE )
	{

	    if( xSemaphore != NULL )
	    {
	    	/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 10 ticks to see if it becomes free. */
			if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
			{

				// Imprimo los datos por puerto serie de manera ordenada:
				//debugPrintString("[");
				uartWriteString(UART_USB, "[");
				for (row = 0; row < (MAX_ROW*2)-1; row++)
				{
					for (col = 0; col<(MAX_COL*2)-1; col++)
					{
						debugPrintInt(sensor_matrix[row][col]);
						if (col != (MAX_COL*2)-2)
							uartWriteString(UART_USB, ",");//debugPrintString(",");
					}
					if (row != (MAX_ROW*2)-2)
						uartWriteString(UART_USB, ";\n");//debugPrintString(";\n");
				}

				uartWriteString(UART_USB, "]\n");//debugPrintString("]\n");
				vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

				xSemaphoreGive( xSemaphore );
			}else{
				continue;
			}
	    }
	    vTaskDelete(NULL);
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


/*==================[fin del archivo]========================================*/
