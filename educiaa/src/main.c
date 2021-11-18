#include "main.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
extern QueueHandle_t queue_force;
extern QueueHandle_t queue_force_average;
extern QueueHandle_t queue_jump;
extern QueueHandle_t queue_command_wifi;
extern SemaphoreHandle_t sem_measure_force;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

/*==================[definiciones de datos internos]=========================*/

extern volatile unsigned long OFFSET;

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main( void )
{
    // ---------- CONFIGURACIONES ------------------------------
    // Inicializar y configurar la plataforma
    boardConfig();

    gpioInit(ClockPin, GPIO_OUTPUT);
	gpioInit(DataPin, GPIO_INPUT);

    adcConfig( ADC_ENABLE );

    // UART for debug messages
    debugPrintConfigUart( UART_USB, BAUD_RATE );
    debugPrintlnString( "TP Profesional." );
    uartConfig(UART_232, BAUD_RATE);

    // Led para dar se�al de vida
    gpioWrite( LED3, ON );

    // Creaci�n de las colas
    create_queue(&queue_force,1,sizeof(unsigned long));
    create_queue(&queue_force_average,1,sizeof(unsigned long));
    create_queue(&queue_jump,1,sizeof(double));
    create_queue(&queue_command_wifi,1,sizeof(int));

    // Creaci�n del sem�foro
    create_semaphore(&sem_measure_force);

    // Creaci�n y validacion de las tareas
	create_task(task_tare,"task_tare",SIZE,0,1,NULL);

    // Iniciar scheduler
    vTaskStartScheduler();


    // ---------- REPETIR POR SIEMPRE --------------------------
    while( TRUE )
    {
        // Si cae en este while 1 significa que no pudo iniciar el scheduler
    }

    // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
    // directamente, no sobre un microcontrolador y no es llamado por ningun
    // Sistema Operativo, como en el caso de un programa para PC.
    return 0;
}

/*==================[fin del archivo]========================================*/