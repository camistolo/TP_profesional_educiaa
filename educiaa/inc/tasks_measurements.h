/*===========================================================================*/
#ifndef _TASKS_MEASUREMENTS_H_
#define _TASKS_MEASUREMENTS_H_

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "auxs.h"
#include "tasks_wifi.h"
#include "tasks_pressure.h"
#include "tasks_force.h"

/*==================[definiciones y macros]==================================*/
// Constantes de la comunicacion
#define BAUD_RATE 115200
#define UART UART_232

// Constantes de los pines
#define DataPin 	ENET_TXEN
#define ClockPin 	ENET_RXD1

// Constantes de los sensores de fuerza
#define SCALE			17000
#define GAIN_128		24
#define GAIN_64			25
#define GAIN_32			26
#define XOR_VALUE 		0x800000
#define AVERAGE_N		20

// Cantidad de mediciones tomadas durante el salto
#define JUMP_N		200

// Constantes de la periodicidad de las tareas
#define TASK_RATE_1000_MS	1000
#define TASK_RATE_500_MS	500
#define TASK_RATE_100_MS	100
#define TASK_RATE_75_MS		75
#define TASK_RATE_50_MS		50
#define TASK_RATE_25_MS		25
#define TASK_RATE_20_MS		20
#define TASK_RATE_5_MS		5
#define TASK_RATE_1_MS		1


#define TASK_RATE_1000 pdMS_TO_TICKS(TASK_RATE_1000_MS)
#define TASK_RATE_500 pdMS_TO_TICKS(TASK_RATE_500_MS)
#define TASK_RATE_100 pdMS_TO_TICKS(TASK_RATE_100_MS)
#define TASK_RATE_75 pdMS_TO_TICKS(TASK_RATE_75_MS)
#define TASK_RATE_50 pdMS_TO_TICKS(TASK_RATE_50_MS)
#define TASK_RATE_25 pdMS_TO_TICKS(TASK_RATE_25_MS)
#define TASK_RATE_20 pdMS_TO_TICKS(TASK_RATE_20_MS)
#define TASK_RATE_5 pdMS_TO_TICKS(TASK_RATE_5_MS)
#define TASK_RATE_1 pdMS_TO_TICKS(TASK_RATE_1_MS)


/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

//TickType_t get_diff();
//void clear_diff();

// Prototipo de funcion de la tarea
void task_measurements( void* taskParmPtr );

/*==================[funcion principal]======================================*/


#endif /* _TASKS_MEASUREMENTS_H_ */
