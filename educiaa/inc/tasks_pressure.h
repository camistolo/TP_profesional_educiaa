/*===========================================================================*/
#ifndef _TASKS_PRESSURE_H_
#define _TASKS_PRESSURE_H_

/*==================[inclusiones]============================================*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"

/*==================[definiciones y macros]==================================*/

#define SAPI_USE_INTERRUPTS

#define MAX_COL 14
#define MAX_ROW 14
#define DELAY_TIME 10 // Tiempo de espera en ms.
DEBUG_PRINT_ENABLE;



/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

void task_pressure( void* pvParameters );
void task_print_matrix( void* pvParameters );

void SetMuxChannel(int channel);
void SetDeMuxChannel(int channel);

/*==================[declaraciones de funciones externas]====================*/

/*==================[funcion principal]======================================*/


#endif /* _TASKS_PRESSURE_H_ */
