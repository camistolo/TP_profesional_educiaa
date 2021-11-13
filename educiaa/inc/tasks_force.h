/*===========================================================================*/
#ifndef _TASKS_FORCE_H_
#define _TASKS_FORCE_H_

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "auxs.h"
#include "tasks_wifi.h"

/*==================[definiciones y macros]==================================*/
// Constantes de la comunicacion
#define BAUD_RATE 115200
#define UART UART_232

// Constantes de los pines
#define DataPin 	ENET_TXEN
#define ClockPin 	ENET_RXD1

//Constantes de los sensores de fuerza
#define SCALE			17000//15500//17110
#define GAIN_128		24
#define GAIN_64			25
#define GAIN_32			26
#define XOR_VALUE 		0x800000
#define AVERAGE_N	15

// Constantes de la periodicidad de las tareas
#define TASK_RATE_1000_MS	1000
#define TASK_RATE_500_MS	500
#define TASK_RATE_50_MS		50

#define TASK_RATE_1000 pdMS_TO_TICKS(TASK_RATE_1000_MS)
#define TASK_RATE_500 pdMS_TO_TICKS(TASK_RATE_500_MS)
#define TASK_RATE_50 pdMS_TO_TICKS(TASK_RATE_50_MS)

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

TickType_t get_diff();
void clear_diff();

// Prototipo de funcion de la tarea
void task_measurement( void* taskParmPtr );
void task_hx711_ready( void* taskParmPtr );
void task_average( void* taskParmPtr );
void task_tare( void* taskParmPtr );
void task_weight( void* taskParmPtr );
void task_jump( void* taskParmPtr );
void task_jump_parameters( void* taskParmPtr );

/*==================[funcion principal]======================================*/


#endif /* _TASKS_FORCE_H_ */
