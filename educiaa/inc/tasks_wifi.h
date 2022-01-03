/*===========================================================================*/
#ifndef _TASKS_WIFI_H_
#define _TASKS_WIFI_H_

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "auxs.h"
#include "tasks_force.h"
#include <string.h>
#include "tasks_pressure.h"

/*==================[definiciones y macros]==================================*/

/*==================[definiciones de datos internos]=========================*/

#define FRAME_MAX_SIZE  4
#define UART_USED 		UART_232

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// Prototipo de funcion de la tarea
void task_receive_wifi( void* pvParameters );
void task_choose_measurement( void* pvParameters );


void format( float valor, char *dst, uint8_t pos );

#endif /* _TASKS_WIFI_H_ */
