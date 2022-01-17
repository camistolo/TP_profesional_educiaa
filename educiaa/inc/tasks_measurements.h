/*===========================================================================*/
#ifndef _TASKS_MEASUREMENTS_H_
#define _TASKS_MEASUREMENTS_H_

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include "tasks_force.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "auxs.h"
#include "tasks_wifi.h"
#include "tasks_pressure.h"

/*==================[definiciones y macros]==================================*/

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// Prototipo de funcion de la tarea
void task_measurements( void* taskParmPtr );
void print_measurements( void* taskParmPtr );
void print_matrix( void* taskParmPtr );
void print_vector( void* taskParmPtr );
void print_parameters( void* taskParmPtr );

/*==================[funcion principal]======================================*/


#endif /* _TASKS_MEASUREMENTS_H_ */
