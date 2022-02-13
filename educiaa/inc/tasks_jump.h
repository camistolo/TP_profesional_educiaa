/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#ifndef _TASKS_JUMP_H_
#define _TASKS_JUMP_H_

/*==================[inclusiones]============================================*/

#include "auxs.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "sapi.h"
#include "semphr.h"
#include "task.h"
#include "tasks_force.h"
#include "tasks_pressure.h"
#include "tasks_wifi.h"

/*====================[definicion de variables y macros]=====================*/

// Estructura de parametros del salto
struct jump_parameters {
	double vel;
	double t;
	double height;
	double power;
};

// Cantidad de mediciones tomadas durante el salto
#define JUMP_N			100

// Constantes auxiliares
#define GRAVITY						9.8
#define FORCE_MEASUREMENT_PERIOD_MS	20

#define DOWNWARD_ACCELERATION_THRESHOLD -20
#define DEACCELERATION_THRESHOLD		5
#define ON_AIR_THRESHOLD				0

/*=========================[declaracion de funciones]=========================*/

void task_measure_jump( void* taskParmPtr );
void task_calculate_jump_parameters( void* taskParmPtr );

#endif /* _TASKS_JUMP_H_ */
