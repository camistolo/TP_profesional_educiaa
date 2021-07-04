#include "main.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
extern QueueHandle_t queue_measurement;
extern QueueHandle_t queue_tare;
extern QueueHandle_t queue_force;
extern QueueHandle_t queue_jump;
extern SemaphoreHandle_t sem_measurement;

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

    // UART for debug messages
    debugPrintConfigUart( UART, BAUD_RATE );
    debugPrintlnString( "TP Profesional." );

    // Led para dar señal de vida
    gpioWrite( LED3, ON );

    // Creación de la cola
    /*queue_measurement = xQueueCreate(CANT_MEDICIONES,sizeof(unsigned long));
    queue_tare = xQueueCreate(1,sizeof(unsigned long));
    queue_force = xQueueCreate(1,sizeof(unsigned long));
    queue_jump = xQueueCreate(1,sizeof(float));

    if (queue_measurement == NULL || queue_tare == NULL || queue_force == NULL){
    	gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_QUEUE );
		while(TRUE);
    }*/

    create_queue(&queue_measurement,CANT_MEDICIONES,sizeof(unsigned long));
    create_queue(&queue_tare,1,sizeof(unsigned long));
    create_queue(&queue_force,1,sizeof(unsigned long));
    //create_queue(&queue_jump,1,sizeof(float));

    // Creación del semáforo
    /*sem_measurement = xSemaphoreCreateBinary();

    if(sem_measurement == NULL){
    	gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_SEM );
		while(TRUE);
    }*/

    create_semaphore(&sem_measurement);

    // Creación y validacion de las tareas
	create_task(task_tare,"task_tare",SIZE,0,PRIORITY+1,NULL);
	create_task(task_Rx_WIFI,"task_Rx",SIZE,0,PRIORITY,NULL);
	create_task(task_Tx_WIFI,"task_Tx",SIZE,0,PRIORITY,NULL);
	//procotol_x_init( UART_232, 115200 );



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
