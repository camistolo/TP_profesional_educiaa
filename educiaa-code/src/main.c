/*==================[inclusions]=============================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"

#include "FreeRTOSConfig.h"
#include "tasks_pressure.h"
#include "utilities.h"
#include "semphr.h"

/*==================[macros and definitions]=================================*/

#define SAPI_USE_INTERRUPTS

extern QueueHandle_t xMeasurePressureQueue;
extern QueueHandle_t xSetIndexQueue;
extern QueueHandle_t xPrintQueue;
extern TaskHandle_t TaskHandle_set_matrix_index;

/*==================[internal functions definition]==========================*/

int main(void){

   /* ------------- INICIALIZACIONES ------------- */

   /* Board initialization */
   boardConfig();

   // UART for debug messages
   uartConfig( UART_USB, 115200 );
   uartWriteString( UART_USB, "TP Profesional - sensor de presion.\n");


   adcConfig( ADC_ENABLE );

   /* Turn LED On to know the firmware is up and running */
   gpioWrite( LED3, ON );

   gpio_config();

   xMeasurePressureQueue = xQueueCreate( 1, sizeof(int[2]) );
   xSetIndexQueue = xQueueCreate( 1, sizeof(int) );
   xPrintQueue = xQueueCreate( 729, sizeof(int) ); // ((MAX_ROW*2)-1) * ((MAX_COL*2)-1) = 729

   int aux = 0;
   xQueueSend( xSetIndexQueue, ( void * ) &aux, ( TickType_t ) 0 );

   xTaskCreate(
		   set_matrix_index,                  	// Function of the task to execute
     		( const char * )"set_matrix_index",	// Name of the task, as a user friendly string
     		configMINIMAL_STACK_SIZE*2, // Stack quantity of the task
     		0,                          // Task parameters
     		tskIDLE_PRIORITY+1,         // Task priority
     		&TaskHandle_set_matrix_index     						// Pointer to the task created in the system
     		);

   xTaskCreate(
		get_pressure_value,                  	// Function of the task to execute
		( const char * )"get_pressure_value",		// Name of the task, as a user friendly string
		configMINIMAL_STACK_SIZE*2,   	// Stack quantity of the task
		0,                            	// Task parameters
		tskIDLE_PRIORITY+1,           	// Task priority
		0                             	// Pointer to the task created in the system
   );


   vTaskStartScheduler();

   return 0;
}

/*==================[end of file]============================================*/
