/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#ifndef _TASKS_WIFI_H_
#define _TASKS_WIFI_H_

/*==================[inclusiones]============================================*/

#include "auxs.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "sapi.h"
#include "semphr.h"
#include "task.h"
#include "tasks_force.h"
#include "tasks_jump.h"
#include "tasks_pressure.h"

/*====================[definicion de variables y macros]=====================*/

// Comunicacion Wifi
#define FRAME_MAX_SIZE  4
#define UART_USED 		UART_232
#define BAUD_RATE 		115200

/*=========================[definicion de funciones]=========================*/

void task_receive_wifi( void* pvParameters );
void task_choose_measurement( void* pvParameters );
void task_print_weight( void* taskParmPtr );
void task_print_matrix( void* taskParmPtr );
void task_print_vector( void* taskParmPtr );
void task_print_parameters( void* taskParmPtr );
void task_calculate_jump_parameters( void* taskParmPtr );

#endif /* _TASKS_WIFI_H_ */
