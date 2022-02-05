/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#ifndef TASKS_PRESSURE_H
#define TASKS_PRESSURE_H

/*==================[inclusiones]============================================*/

#include "auxs.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "sapi.h"
#include "semphr.h"
#include "task.h"

/*====================[definicion de variables y macros]=====================*/

// Dimensiones de la matriz
#define MAX_COL 14
#define MAX_ROW 14

// Pines del sensor de presion
#define demuxS0 GPIO1
#define demuxS1 GPIO3
#define demuxS2 GPIO5
#define demuxS3 GPIO7
#define demuxSIG GPIO8

#define muxS0 T_FIL0 // GPIO25
#define muxS1 T_FIL3 // GPIO28
#define muxS2 T_FIL2 // GPIO27
#define muxS3 T_COL0 //GPIO29
#define muxSIG CH3

/*=========================[definicion de funciones]=========================*/

void task_set_matrix_index( void* pvParameters );
void task_get_pressure_value( void* pvParameters );
void SetMuxChannel(uint8_t channel);
void SetDeMuxChannel(uint8_t channel);

#endif //TASKS_PRESSURE_H
