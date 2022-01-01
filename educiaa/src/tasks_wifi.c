#include "tasks_wifi.h"

DEBUG_PRINT_ENABLE;

QueueHandle_t queue_command_wifi;
extern TaskHandle_t TaskHandle_weight;

#define FRAME_MAX_SIZE  200

typedef enum{
	WEIGHT_MEAS = 1,
	FORCE_MEAS,
	PRESSURE_MEAS,
	FORCE_PRESSURE_MEAS
} measurement_type_t;


char buffer[FRAME_MAX_SIZE];
uint16_t index;

char* buffer_tx ;
uint16_t size_tx;
uint16_t counter_tx;

/*==================[definiciones de funciones internas]=====================*/

void format( float valor, char *dst, uint8_t pos ){
	uint16_t val;
	val = 10 * valor;
	val = val % 1000;
	dst[pos] = (val / 100) + '0';
	pos++;
	dst[pos] = (val % 100) / 10 + '0';
	pos++;
	dst[pos] = '.';
	pos++;
	dst[pos] = (val % 10)  + '0';
	pos++;
	dst[pos] = '\0';
}

char* itoa(int value, char* result, int base) {
   // check that the base if valid
   if (base < 2 || base > 36) { *result = '\0'; return result; }

   char* ptr = result, *ptr1 = result, tmp_char;
   int tmp_value;

   do {
      tmp_value = value;
      value /= base;
      *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
   } while ( value );

   // Apply negative sign
   if (tmp_value < 0) *ptr++ = '-';
   *ptr-- = '\0';
   while(ptr1 < ptr) {
      tmp_char = *ptr;
      *ptr--= *ptr1;
      *ptr1++ = tmp_char;
   }
   return result;
}

/*==================[definiciones de funciones externas]=====================*/

void task_receive_wifi( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms

	TickType_t xLastWakeTime = xTaskGetTickCount();

	char meas_char;
	int message_buffer;

	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{
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

				create_task(task_choose_measurement,"task_choose_measurement",SIZE,0,1,NULL);
				xQueueSend(queue_command_wifi , &message_buffer,  portMAX_DELAY);

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
				message_buffer = meas_char - '0';
				//uartWriteByte(UART_USB, message_buffer);

				/* incremento el indice */
				index++;
			}
			else
			{
				/* no hago nada, descarto el byte */
			}
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

void task_choose_measurement( void* pvParameters )
{

	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	measurement_type_t meas_type;

	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE )
	{
		if(xQueueReceive(queue_command_wifi , &meas_type,  portMAX_DELAY))
		{
			switch (meas_type) {
				case WEIGHT_MEAS:
					// Activate weight measurement task
					create_task(task_weight,"task_weight",SIZE,0,1,NULL);
					break;
				case FORCE_MEAS:
					// Activate force measurement task
					create_task(task_jump,"task_jump",SIZE,0,1,NULL);
					break;
				case PRESSURE_MEAS:
					// Activate pressure measurement task
					uartWriteString( UART_USB, "Pressure\n");
					break;

				case FORCE_PRESSURE_MEAS:
					// Activate height measurement task
					uartWriteString( UART_USB, "Height\n");
					break;
				default:
					uartWriteString( UART_USB, "Error: Invalid measurement option.\n");
			}
		}
		vTaskDelete(NULL);
	}
}

/*==================[fin del archivo]========================================*/
