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

extern QueueHandle_t xMeasureTypeQueue;
extern QueueHandle_t xMeasurementQueue;


/* PRINCIPAL FUNCTION, INTIAL ENTRY PONT AFTER RESET. */
int main(void){

   /* Board initialization */
   boardConfig();

   // UART for debug messages
   uartConfig( UART_USB, 115200 );
   uartConfig( UART_USED, 115200 );
   uartWriteString( UART_USB, "wifi_simple_RTOS\n");

   /* Turn LED On to know the firmware is up and running */
   gpioWrite( LED3, ON );

   xMeasureTypeQueue = xQueueCreate( 1, sizeof(int) );
   xMeasurementQueue = xQueueCreate( 1, sizeof(char [20]) );

	xTaskCreate(
		task_wait_wifi,                  	// Function of the task to execute
		( const char * )"task_wait_wifi", 	// Name of the task, as a user friendly string
		configMINIMAL_STACK_SIZE*2,   		// Stack quantity of the task
		0,                            		// Task parameters
		tskIDLE_PRIORITY+1,           		// Task priority
		0                             		// Pointer to the task created in the system
	);

   vTaskStartScheduler();

   return 0;
}

/*==================[end of file]============================================*/
