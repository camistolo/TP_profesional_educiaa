/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#ifndef _TASKS_FORCE_H_
#define _TASKS_FORCE_H_

/*==================[inclusiones]============================================*/

#include "auxs.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "sapi.h"
#include "semphr.h"
#include "task.h"
#include "tasks_jump.h"
#include "tasks_pressure.h"
#include "tasks_wifi.h"

/*====================[definicion de variables y macros]=====================*/

// Modo de medicion de fuerza
typedef enum {
	MEDIAN_MODE,
	SIMPLE_MODE
} force_measurement_mode_t;

// Estructura de parametros del salto
struct jump_parameters {
	double vel;
	double t;
	double height;
	double power;
};

// Pines del HX711
#define DataPin 		ENET_TXEN
#define ClockPin 		ENET_RXD1

// Constantes de los sensores de fuerza
#define SCALE					-23450
#define GAIN_128				24
#define GAIN_64					25
#define GAIN_32					26
#define XOR_VALUE 				0x800000
#define TOTAL_MEDIAN_VALUES		20

// Cantidad de mediciones tomadas durante el salto
#define JUMP_N			100

// Constantes auxiliares
#define GRAVITY						9.8
#define FORCE_MEASUREMENT_PERIOD	0.02

#define DOWNWARD_ACCELERATION_THRESHOLD -20
#define DEACCELERATION_THRESHOLD		5
#define ON_AIR_THRESHOLD				0

/*=========================[definicion de funciones]=========================*/

void task_measure_force( void* taskParmPtr );
void task_check_hx711_is_ready( void* taskParmPtr );
void task_calculate_median( void* taskParmPtr );
void task_tare( void* taskParmPtr );
void task_measure_weight( void* taskParmPtr );

#endif /* _TASKS_FORCE_H_ */
