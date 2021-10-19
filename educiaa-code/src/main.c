#include "main.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
extern QueueHandle_t queue_measurement;
extern QueueHandle_t queue_force;
extern QueueHandle_t queue_rx_wifi;
extern QueueHandle_t queue_jump;
//extern QueueHandle_t queue_pressure;
extern SemaphoreHandle_t sem_measurement;
extern SemaphoreHandle_t mutex;

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

    // Led para dar señal de vida
    gpioWrite( LED3, ON );

    // Creación de las colas
    create_queue(&queue_measurement,CANT_MEDICIONES,sizeof(unsigned long));
    create_queue(&queue_force,1,sizeof(unsigned long));
    create_queue(&queue_rx_wifi,1,sizeof(int));
    create_queue(&queue_jump,1,sizeof(double));
    //create_queue(&queue_pressure,1,sizeof(int[(MAX_ROW*2)-1][(MAX_COL*2)-1]));

    // Creación del semáforo
    create_semaphore(&sem_measurement);

    // Creación del mutex
    create_mutex(&mutex);

    // Creación y validacion de las tareas
	create_task(task_tare,"task_tare",SIZE,0,1,NULL);
	//create_task(task_Rx_WIFI,"task_Rx",SIZE,0,0,NULL);
	//create_task(task_Tx_WIFI,"task_Tx",SIZE,0,0,NULL);

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
