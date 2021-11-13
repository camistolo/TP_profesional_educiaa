/*==================[inclusions]=============================================*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"
#include "utilities.h"

/*==================[macros and definitions]=================================*/

//#define SAPI_USE_INTERRUPTS
#define DELAY_TIME 10 // Tiempo de espera en ms.
#define FRAME_MAX_SIZE 4 // Se espera que el mensaje sea del estilo >n<, con n entre 0 y 9.

#define WEIGHT_MEAS '1'
#define FORCE_MEAS '2'
#define PRESSURE_MEAS '3'
#define HEIGHT_MEAS '4'
#define ERROR_MEAS '5'
#define UART_USED UART_232

DEBUG_PRINT_ENABLE;

char message_buffer;
int index = 0;
char * index_str;
bool_t wait_flag = true;
char meas_char;

SemaphoreHandle_t xSemaphoreMeasureType;
QueueHandle_t xMeasureQueue = NULL;

/*==================[internal functions definition]==========================*/

void task_wait_wifi( void* pvParameters );
void task_send_wifi( void* pvParameters );

/* FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE RESET. */
int main(void){

   /* ------------- INICIALIZACIONES ------------- */

   /* Inicializar la placa */
   boardConfig();

   // UART for debug messages
   uartConfig( UART_USB, 115200 );
   uartConfig( UART_USED, 115200 );
   uartWriteString( UART_USB, "wifi_simple_RTOS\n");

   /* Prendo un led para dar señal de vida*/
   gpioWrite( LED3, ON );

   //xSemaphoreMeasureType = xSemaphoreCreateBinary();
   xMeasureQueue = xQueueCreate( 1, sizeof(int) );

	xTaskCreate(
		task_wait_wifi,                  	// Funcion de la tarea a ejecutar
		( const char * )"task_wait_wifi", 	// Nombre de la tarea como String amigable para el usuario
		configMINIMAL_STACK_SIZE*2,   		// Cantidad de stack de la tarea
		0,                            		// Parametros de tarea
		tskIDLE_PRIORITY+1,           		// Prioridad de la tarea
		0                             		// Puntero a la tarea creada en el sistema
	);

//	xTaskCreate(
//		task_send_wifi,                  	// Funcion de la tarea a ejecutar
//		( const char * )"task_send_wifi", 	// Nombre de la tarea como String amigable para el usuario
//		configMINIMAL_STACK_SIZE*2,   		// Cantidad de stack de la tarea
//		0,                            		// Parametros de tarea
//		tskIDLE_PRIORITY+1,           		// Prioridad de la tarea
//		0                             		// Puntero a la tarea creada en el sistema
//	);




   vTaskStartScheduler();

   return 0;
}


void task_wait_wifi( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms

	TickType_t xLastWakeTime = xTaskGetTickCount();

	//xSemaphoreMeasureType = xSemaphoreCreateBinary();



	//char c;
	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{

//		if (wait_flag)
//		{
//			xSemaphoreTake( xSemaphoreMeasureType, ( TickType_t ) 0 );
//			wait_flag = false;
//		}

		//if(xSemaphoreTake( xSemaphoreMeasureType, ( TickType_t ) 0 ) == pdTRUE)


		meas_char = uartRxRead( UART_USED );
		vTaskDelay(1); // Overrun Error. Esta recibiendo información más rápido que lo que el código lo puede procesar.

		if( FRAME_MAX_SIZE-1==index )
		{
			/* reinicio el paquete */
			index = 0;
		}

		if( meas_char == '>' )
		{
			if( index==0 )
			{
				/* 1er byte del frame*/
			}
			else
			{
				/* fuerzo el arranque del frame (descarto lo anterior)*/
				index = 0;
			}

			/* incremento el indice */
			index++;
		}
		else if( meas_char == '<' )
		{
			/* solo cierro el fin de frame si al menos se recibio un start.*/
			if( index>=1 )
			{
				/* se termino el paquete - guardo el dato */

				/* incremento el indice */
				index++;

				/* señalizo a la aplicacion */
				// HAY QUE ENVIAR EL DATO A LA TAREA DE MEDICIÓN
				//meas_char = message_buffer[1];
				//uartWriteByte(UART_USB, c);

				xTaskCreate(
						task_send_wifi,                  	// Funcion de la tarea a ejecutar
						( const char * )"task_send_wifi", 	// Nombre de la tarea como String amigable para el usuario
						configMINIMAL_STACK_SIZE*2,   		// Cantidad de stack de la tarea
						0,                            		// Parametros de tarea
						tskIDLE_PRIORITY+1,           		// Prioridad de la tarea
						0                             		// Puntero a la tarea creada en el sistema
					);

				xQueueSend( xMeasureQueue, ( void * ) &message_buffer, ( TickType_t ) 0 );

				vTaskDelete(NULL);
			}
			else
			{
				/* no hago nada, descarto el byte */
			}
		}
		else
		{
			/* solo cierro el fin de frame si al menos se recibio un start.*/
			if( index >= 1 )
			{
				/* guardo el dato */
				message_buffer = meas_char;

				/* incremento el indice */
				index++;
			}
			else
			{
				/* no hago nada, descarto el byte */
			}
		}


		// Envia la tarea al estado bloqueado durante xPeriodicity (delay periodico)
		//vTaskDelayUntil( &xLastWakeTime , xPeriodicity );


	}
}

void task_send_wifi( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	char meas_char_new;
	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{
		//if( xSemaphoreTake( xSemaphoreMeasureType, ( TickType_t ) 10 ) == pdTRUE )
		//{
			if(xQueueReceive( xMeasureQueue, &( meas_char_new ), ( TickType_t ) 10 ) == pdTRUE)
			{
			switch (meas_char_new)
			{
				case WEIGHT_MEAS:
					// Activate weight measurement task
					//xSemaphoreTake( xSemaphoreMeasureWeight, ( TickType_t ) 0 );
					//xSemaphoreGive( xSemaphoreMeasureWeight);
					uartWriteString( UART_USB, "Weight\n");
					uartWriteString( UART_USED, ">155<");
					break;
				case FORCE_MEAS:
					// Activate force measurement task
					uartWriteString( UART_USB, "Force\n");
					break;

				case PRESSURE_MEAS:
					// Activate pressure measurement task
					uartWriteString( UART_USB, "Pressure\n");
					break;

				case HEIGHT_MEAS:
					// Activate height measurement task
					uartWriteString( UART_USB, "Height\n");
					break;

				default:
					uartWriteString( UART_USB, "Error: Invalid measurement option.\n");
			}

			//xQueueReset(xMeasureQueue);

			//wait_flag = true;
			//xSemaphoreGive( xSemaphoreMeasureType );

			xTaskCreate(
					task_wait_wifi,                  	// Funcion de la tarea a ejecutar
					( const char * )"task_wait_wifi", 	// Nombre de la tarea como String amigable para el usuario
					configMINIMAL_STACK_SIZE*2,   		// Cantidad de stack de la tarea
					0,                            		// Parametros de tarea
					tskIDLE_PRIORITY+1,           		// Prioridad de la tarea
					0                             		// Puntero a la tarea creada en el sistema
				);
			vTaskDelete(NULL);
		}
		vTaskDelayUntil(&xLastWakeTime, xPeriodicity); // Si no pongo un delay el semaforo lo vuelve a tomar esta tarea y se queda aca en un loop infinito.
	}
}

/*==================[end of file]============================================*/
