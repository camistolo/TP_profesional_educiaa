#include "tasks_wifi.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
extern QueueHandle_t queue_force;
extern TaskHandle_t TaskHandle_measurement;

extern volatile unsigned long OFFSET;
extern float PESO;
extern int new_measurement;
extern int stage;

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

/*==================[definiciones de funciones externas]=====================*/


void task_Rx_WIFI( void* taskParmPtr )
{
	fsmButtonInit();

	while( TRUE )
	{
		fsmButtonUpdate( TEC1 );
	 	vTaskDelay( 1 / portTICK_RATE_MS );
	}
}

// Implementacion de funcion de la tarea
void task_Tx_WIFI( void* taskParmPtr )
{
	unsigned long fu = 0;
	char str_aux[50] = {};
	char fl_str_aux[64] = {};
	float p;
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_1000;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();
	// ---------- REPETIR POR SIEMPRE --------------------------
    while( TRUE )
    {
    	if(xQueueReceive(queue_force , &fu,  portMAX_DELAY)){			// Esperamos tecla

			gpioWrite( LED1, ON );
			vTaskDelay(40 / portTICK_RATE_MS);
			gpioWrite( LED1, OFF );

			p = (fu - OFFSET) / SCALE;

			if (p > 300){
				p = 0;
			}

			PESO = p;

			format(p,fl_str_aux,0);
			sprintf(str_aux, "Offset: %lu, Fuerza: %lu, Peso: %s \r\n", OFFSET, fu, fl_str_aux);
			uartWriteString(UART,str_aux);


			//stage = 1;

			// Delay periódico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
    	}

    }
}

void wait_frame( void* pvParameters )
{
    char* data;
    uint16_t size;

    //uint16_t frame_counter = 0;

    while( TRUE )
    {
        protocol_wait_frame();

        protocol_get_frame_ref( &data, &size );

        if (strcmp(data,">1<") == 0) {
        	uartWriteString(UART_USB, "data1");
        	create_task(task_weight,"task_weight",SIZE,0,PRIORITY+1,NULL);
        }
        else if (strcmp(data,">2<") == 0) {
        	uartWriteString(UART_USB, "data2");
        	create_task(task_jump,"task_jump",SIZE,0,PRIORITY+1,NULL);
        }
        else {
        	uartWriteString(UART_USB, data);
        }

        //int a = sprintf( &data[size], " %u\n", frame_counter );

        /* envio respuesta */
        protocol_transmit_frame( data, size);// + a );

        protocol_discard_frame();

        /* hago un blink para que se vea */
        gpioToggle( LEDB );
        vTaskDelay( 100/portTICK_RATE_MS );
        gpioToggle( LEDB );

        //frame_counter++;
    }
}

/*==================[fin del archivo]========================================*/
