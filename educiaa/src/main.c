/*=============================================================================
 * Copyright (c) 2022, Brian Landler <blandler@fi.uba.ar>,
 * Camila Stolowicz <cstolowicz@fi.uba.ar>, Martin Bergler <mbergler@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2022/02/02
 * Version: v1.0
 *===========================================================================*/
#include "main.h"

/*==================[definiciones de datos externos]=========================*/

// Handles de las colas
extern QueueHandle_t queue_force;
extern QueueHandle_t queue_force_average;
extern QueueHandle_t queue_jump;
extern QueueHandle_t queue_command_wifi;
extern QueueHandle_t queue_jump_parameters;
extern QueueHandle_t queue_measure_pressure;
extern QueueHandle_t queue_print;

// Handles de los semaforos
extern SemaphoreHandle_t sem_measure_force;
extern SemaphoreHandle_t sem_pressure_index;
extern SemaphoreHandle_t sem_pressure_finished;
extern SemaphoreHandle_t sem_matrix_print_finished;
extern SemaphoreHandle_t sem_vector_print_finished;
extern SemaphoreHandle_t sem_parameters_print_finished;

/*==================[funcion principal]======================================*/

// Funcion principal, punto de entrada luego de encendido o reset
int main( void )
{
    // ---------- CONFIGURACIONES ------------------------------

    // Inicializacion y configuracion de la plataforma
    boardConfig();

	// Inicializacion del ADC
    adcConfig( ADC_ENABLE );

    // Configuracion de la UART
    uartConfig(UART_USED, BAUD_RATE);

    gpio_config();

    // Creacion de las colas
    create_queue(&queue_force,1,sizeof(uint32_t));
    create_queue(&queue_force_average,1,sizeof(uint32_t));
    create_queue(&queue_jump,JUMP_N,sizeof(uint16_t));
    create_queue(&queue_jump_parameters,1,sizeof(struct jump_parameters));
    create_queue(&queue_command_wifi,1,sizeof(uint8_t));
    create_queue(&queue_measure_pressure,1,sizeof(uint8_t[2]));
    create_queue(&queue_print,196,sizeof(uint16_t)); // MAX_ROW * MAX_COL = 196

    // Creacion de los semaforos
    create_semaphore(&sem_measure_force);
    create_semaphore(&sem_pressure_index);
    create_semaphore(&sem_pressure_finished);
    create_semaphore(&sem_matrix_print_finished);
    create_semaphore(&sem_vector_print_finished);
    create_semaphore(&sem_parameters_print_finished);

    // Creacion y validacion de la tarea de tara
	create_task(task_tare,"task_tare",BASE_SIZE,0,1,NULL);

    // Iniciar scheduler
    vTaskStartScheduler();


    // ---------- REPETIR POR SIEMPRE --------------------------
    while( TRUE )
    {
        // Si cae en este while 1 significa que no pudo iniciar el scheduler
    }

    return 0;
}

/*==================[fin del archivo]========================================*/
