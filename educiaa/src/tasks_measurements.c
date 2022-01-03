#include "tasks_measurements.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
QueueHandle_t queue_force;
QueueHandle_t queue_force_average;
QueueHandle_t queue_jump;
SemaphoreHandle_t sem_measure_force;

extern SemaphoreHandle_t sem_pressure_index;
extern SemaphoreHandle_t sem_pressure_finished;
extern TaskHandle_t TaskHandle_set_matrix_index;
extern TaskHandle_t TaskHandle_get_pressure_value;
extern TaskHandle_t TaskHandle_measure_force;

extern void format( float valor, char *dst, uint8_t pos );
/*==================[definiciones de datos internos]=========================*/

extern volatile unsigned long OFFSET;
extern volatile unsigned long PESO;

/*==================[definiciones de funciones internas]=====================*/

// Tarea que calcula el valor principal del salto
void task_measurements( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_5;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uartWriteString(UART_USB, "TAREA_JUMP \r\n");

	//unsigned long offset;
	unsigned long f = 0;
	unsigned long jump_values[JUMP_N];
	size_t i = 0;
	char str_aux[50] = {};
	char fl_str_aux[64] = {};
	bool on_air = false;

//	cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);
//	cyclesCounterReset();

	measurement_mode_t mode = SIMPLE_MODE;
	create_task(task_measure_force,"task_measure_force",SIZE,&mode,1,&TaskHandle_measure_force);

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		// Si todavia no recibi todos los valores del salto
		if (i <= sizeof(jump_values)/sizeof(unsigned long)) {
			// Mandar senal a la tarea que mide la fuerza
			xSemaphoreGive( sem_measure_force );

			// Si la presion termino y recibi el valor de la fuerza
			if(xSemaphoreTake(sem_pressure_finished, portMAX_DELAY) && xQueueReceive(queue_force , &f,  portMAX_DELAY)){

				if(i > 0 && f < OFFSET ) {
					f = OFFSET;
				}

				// Si se recibio el ultimo valor de fuerza, elimino la tarea de fuerza
				if (i == sizeof(jump_values)/sizeof(unsigned long)) {
					//create_task(task_jump_parameters,"task_jump_parameters",SIZE,0,1,NULL);
					//xQueueSend(queue_jump , jump_values,  portMAX_DELAY);
					vTaskDelete(&TaskHandle_measure_force);
				}
				// Guardar los valores de la fuerza en el arreglo
				jump_values[i++] = f;
				sprintf(fl_str_aux, "%lu \r\n", f);
				uartWriteString(UART_USB,fl_str_aux);

				// Si el usuario se encuentra en el aire, seteo la flag de on_air a true
				if( fabs((double)f-(double)OFFSET) <= 100000 ) {
					on_air = true;
				}

				if (on_air && f > PESO) {
					// Eliminar las tareas de presion y crear la que procesa los valores del salto
					vTaskDelete(&TaskHandle_set_matrix_index);
					vTaskDelete(&TaskHandle_get_pressure_value);
					create_task(task_jump_parameters,"task_jump_parameters",SIZE,0,1,NULL);
					xQueueSend(queue_jump , jump_values,  portMAX_DELAY);
				}

				// Luego de obtener la fuerza, senalizar a la tarea de presion para medir
				xSemaphoreGive( sem_pressure_index );
			}
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

/*==================[fin del archivo]========================================*/
