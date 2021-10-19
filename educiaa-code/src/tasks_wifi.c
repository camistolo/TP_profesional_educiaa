#include "tasks_wifi.h"

DEBUG_PRINT_ENABLE;

SemaphoreHandle_t mutex;
QueueHandle_t queue_rx_wifi;
extern TaskHandle_t TaskHandle_weight;

#define FRAME_MAX_SIZE  200

#define WEIGHT_MEAS "1"
#define FORCE_MEAS "2"
#define PRESSURE_MEAS "3"
#define HEIGHT_MEAS "4"
#define ERROR_MEAS "5"

#define WEIGHT_MEAS_STR ">1<"
#define FORCE_MEAS_STR ">2<"
#define PRESSURE_MEAS_STR ">3<"
#define HEIGHT_MEAS_STR ">4<"
#define ERROR_MEAS_STR ">5<"

uartMap_t         uart_used;
SemaphoreHandle_t new_frame_signal;
SemaphoreHandle_t sem_tx;
SemaphoreHandle_t mutex;


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


void wait_frame( void* pvParameters )
{
    char* data;
    uint16_t size;

    uint16_t frame_counter = 0;

    while( TRUE )
    {
        protocol_wait_frame();

        protocol_get_frame_ref( &data, &size );

        // int a = sprintf( &data[size], " %u\n", frame_counter );

        /* envio respuesta */
        protocol_transmit_frame( data, size); //+ a );

        protocol_discard_frame();

        /* hago un blink para que se vea */
        gpioToggle( LEDB );
        vTaskDelay( 100/portTICK_RATE_MS );
        gpioToggle( LEDB );

        frame_counter++;
    }
}

void protocol_tx_event( void *noUsado )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uartTxWrite( uart_used, buffer_tx[counter_tx] );

    counter_tx++;

    if( counter_tx==size_tx )
    {
        /* deshabilito la isr de transmision */
        uartCallbackClr( uart_used, UART_TRANSMITER_FREE );

        xSemaphoreGiveFromISR( sem_tx, &xHigherPriorityTaskWoken );
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void protocol_rx_event( void *noUsado )
{
    ( void* ) noUsado;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* leemos el caracter recibido */
    //char c = uartRxRead( UART_232 );
    char c = uartRxRead( UART_232 );

    BaseType_t signaled = xSemaphoreTakeFromISR( mutex, &xHigherPriorityTaskWoken );

    if( signaled )
    {
        if( FRAME_MAX_SIZE-1==index )
        {
            /* reinicio el paquete */
            index = 0;
        }

        if( c=='>' )
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

            buffer[index] = c;

            /* incremento el indice */
            index++;
        }
        else if( c=='<' )
        {
            /* solo cierro el fin de frame si al menos se recibio un start.*/
            if( index>=1 )
            {
                /* se termino el paquete - guardo el dato */
                buffer[index] = c;

                /* incremento el indice */
                index++;

                /* Deshabilito todas las interrupciones de UART_USB */
                uartCallbackClr( uart_used, UART_RECEIVE );

                /* señalizo a la aplicacion */
                xSemaphoreGiveFromISR( new_frame_signal, &xHigherPriorityTaskWoken );
            }
            else
            {
                /* no hago nada, descarto el byte */
            }
        }
        else
        {
            /* solo cierro el fin de frame si al menos se recibio un start.*/
            if( index>=1 )
            {
                /* guardo el dato */
                buffer[index] = c;

                /* incremento el indice */
                index++;
            }
            else
            {
                /* no hago nada, descarto el byte */
            }
        }

        xSemaphoreGiveFromISR( mutex, &xHigherPriorityTaskWoken );
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void procotol_x_init( uartMap_t uart, uint32_t baudRate )
{
    /* CONFIGURO EL DRIVER */

    uart_used = uart;

    /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
    uartConfig( uart, baudRate );

    /* Seteo un callback al evento de recepcion y habilito su interrupcion */
    uartCallbackSet( uart, UART_RECEIVE, protocol_rx_event, NULL );

    /* Habilito todas las interrupciones de UART_USB */
    uartInterrupt( uart, true );

    /* CONFIGURO LA PARTE LOGICA */
    index = 0;
    new_frame_signal = xSemaphoreCreateBinary();
    sem_tx = xSemaphoreCreateBinary();
    mutex = xSemaphoreCreateMutex();
}

void protocol_wait_frame()
{
    xSemaphoreTake( new_frame_signal, portMAX_DELAY );
    xSemaphoreTake( mutex, 0 );
}

void  protocol_get_frame_ref( char** data, uint16_t* size )
{
    *data = buffer;
    *size = index;
}

void protocol_discard_frame()
{
    /* indico que se puede inciar un paquete nuevo */
    index = 0;

    /* libero la seccion critica, para que el handler permita ejecutarse */
    xSemaphoreGive( mutex );

    /* limpio cualquier interrupcion que hay ocurrido durante el procesamiento */
    uartClearPendingInterrupt( uart_used );

    /* habilito isr rx  de UART_USB */
    uartCallbackSet( uart_used, UART_RECEIVE, protocol_rx_event, NULL );
}

void protocol_transmit_frame( char* data, uint16_t size )
{
	uartConfig( UART_USB, 115200 );

	if (strcmp(data, WEIGHT_MEAS_STR)==0)
	{
		uartConfig( UART_USB, 115200 );

		data = itoa(50+counter_tx, data, 10);

		// Hacer medidición de peso
		//data = "75"; // Supongo que el peso me dio 65. Luego se reemplazaría por el valor medido.
		print_data_uart (WEIGHT_MEAS, &data);
		//char saux[50] = "";
		//sprintf(saux, "%u", eTaskGetState(TaskHandle_weight));
		//uartWriteString(UART_USB, saux);
		create_task(task_weight,"task_weight",SIZE,0,1,TaskHandle_weight);

	}else if (strcmp(data, FORCE_MEAS_STR)==0)
	{
		// Hacer medidición de fuerza
		data = "245";
		print_data_uart (FORCE_MEAS, &data);
		create_task(task_jump,"task_jump",SIZE,0,1,NULL);
		//sprintf(data,"%s%s%s%s", ">", "2", "42", "<");
	}else if (strcmp(data, PRESSURE_MEAS_STR)==0)
	{
		// Hacer medidición de presion
		data = "102";
		print_data_uart (PRESSURE_MEAS, &data);
		create_task(task_pressure, "task_pressure",SIZE,0,1,NULL);
		create_task(task_print_matrix, "task_print_matrix",SIZE,0,1,NULL);
		//sprintf(data,"%s%s%s%s", ">", "3", "102", "<");
	}

	uartWriteString( UART_USB, data );

	uartWriteString( uart_used, data );

    //buffer_tx = data ;
	//buffer_tx = ">420<" ;
	//uartConfig( uart_used, 115200 );
	//uartWriteString( uart_used, buffer_tx );
	size_tx = size;

    counter_tx = 0;

    uartCallbackSet( uart_used, UART_TRANSMITER_FREE, protocol_tx_event, NULL );

    /* dispara la 1ra interrupcion */
    uartSetPendingInterrupt( uart_used );

    xSemaphoreTake( sem_tx, portMAX_DELAY );
}

void print_data_uart (char * meas_type, char ** data_str)
{

	char data_aux[FRAME_MAX_SIZE] = ">";
	char closing_str[FRAME_MAX_SIZE] = "<";
	size_t i = 0;

	strcat(data_aux, meas_type);
	strcat(data_aux, *data_str);
	strcat(data_aux, closing_str);

	uartWriteString( UART_USB, "\n Data aux: " );
	uartWriteString( UART_USB, data_aux );

/*
	for(i = 0; i<strlen(data_aux); i++)
	{
		uartWriteString( UART_USB, "for \n" );
		data_str[i] = data_aux[i];
		uartWriteString( UART_USB, "for after \n" );
	}
	*/
	*data_str = data_aux;
	uartWriteString( UART_USB, "\n Data aux: " );
	uartWriteString( UART_USB, *data_str );

}
/*==================[fin del archivo]========================================*/
