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

/*=========================[definicion de funciones]=========================*/

void task_measure_jump( void* taskParmPtr );
void task_print_matrix( void* taskParmPtr );

#endif /* _TASKS_JUMP_H_ */
