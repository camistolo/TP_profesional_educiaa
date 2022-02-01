#include "tasks_measurements.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
extern QueueHandle_t queue_force;
extern QueueHandle_t queue_jump;
extern QueueHandle_t queue_jump_parameters;
extern SemaphoreHandle_t sem_measure_force;
extern QueueHandle_t xPrintQueue;

extern SemaphoreHandle_t sem_pressure_index;
extern SemaphoreHandle_t sem_pressure_finished;
extern TaskHandle_t TaskHandle_set_matrix_index;
extern TaskHandle_t TaskHandle_get_pressure_value;
extern TaskHandle_t TaskHandle_measure_force;
TaskHandle_t TaskHandle_measurements;
TaskHandle_t TaskHandle_print_measurements;

SemaphoreHandle_t sem_matrix_print_finished;
SemaphoreHandle_t sem_vector_print_finished;
SemaphoreHandle_t sem_parameters_print_finished;

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

	//unsigned long offset;
	unsigned long f = 0;
	unsigned long last_f = OFFSET;
	int force_amount = 0;
	char str_aux[50] = {};
	char fl_str_aux[64] = {};
//	bool back_down = false;
//	bool on_air = false;
	int zeroed_newton;
	//double zeroed_newton;
	const double gravity = 9.8;

	bool aux = false;

	// Crea la tarea de medicion de fuerza para empezar a medir
	measurement_mode_t mode = SIMPLE_MODE;
	create_task(task_measure_force,"task_measure_force",SIZE,&mode,1,&TaskHandle_measure_force);

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		// Si todavia no recibi todos los valores del salto
		if (force_amount < JUMP_N) {
			// Mandar senal a la tarea que mide la fuerza
			xSemaphoreGive( sem_measure_force );

			// Si la presion termino y recibi el valor de la fuerza
			if(xQueueReceive(queue_force , &f,  (TickType_t) 10)){

				// Indicar que hay un valor mas de fuerza
				force_amount++;

				// Si se recibio el ultimo valor de fuerza, elimino la tarea de fuerza
				if (force_amount == JUMP_N) {
					vTaskDelete(TaskHandle_measure_force);

					// Hoy en dia, se mide la presion cuando termina de medirse los 100 valores de fuerza.
					// Esto podria cambiarse
					xSemaphoreGive( sem_pressure_index );
					create_task(set_matrix_index,"set_matrix_index",SIZE,0,2,&TaskHandle_set_matrix_index);
					create_task(get_pressure_value,"get_pressure_value",SIZE,0,2,&TaskHandle_get_pressure_value);
					// Esto se puso para debuggear nada mas, pero en realidad aca se llamaria a la tarea
					// task_calculate_jump_parameters para calcular la velocidad, tiempo, potencia y altura y
					// despues se llamaria a las tareas de impresion.

					create_task(print_matrix,"print_matrix",SIZE,0,1,NULL);

					vTaskDelete(NULL);
					//create_task(task_calculate_jump_parameters,"task_calculate_jump_parameters",SIZE,0,1,NULL);
					//xQueueSend(queue_jump , jump_values,  portMAX_DELAY);
				}

				// Si el valor que se midio es mayor al offset (es como si se midieran kilogramos negativos), se
				// setea ese valor al valor anterior de fuerza medido.
				if (f > OFFSET) {
					f = last_f;
				}

				// Se actualiza el valor anterior de fuerza al actual
				last_f = f;

				// Se calcula el valor de fuerza en Newtons pero se pone el cero en el valor del peso del usuario
				// en Newton
				//zeroed_newton = ((double)(f) - (double)(PESO)) * gravity / SCALE;
				zeroed_newton = (int)(f);

				// Enviar los valores de la fuerza a la cola queue_jump para dsps calcular los parametros
				xQueueSend( queue_jump, &zeroed_newton, portMAX_DELAY);
				// Los valores de la fuerza solo se imprimen para debuggear
				sprintf(fl_str_aux, "%lu \r\n", f);
				uartWriteString(UART_USB,fl_str_aux);

				////////////////////////////////////////////////////////////////////////////////////////////////
				// Todo lo siguiente se hacia para medir la presion apenas la persona cayera, pero ahora esto se
				// hace cuando se terminan de medir los 100 valores (se puede cambiar)

				// Si el usuario se encuentra en el aire, seteo la flag de on_air a true
//				if( (((double)f-(double)OFFSET) <= 100000) && !back_down) {
//					on_air = true;
//				}

//				xSemaphoreGive( sem_pressure_index );
//				xSemaphoreTake(sem_pressure_finished, portMAX_DELAY);
//				create_task(print_matrix,"print_matrix",SIZE,0,1,NULL);
//				vTaskDelete(NULL);

//				if (on_air && !back_down) {
//					if (zeroed_newton > 20) {
//						uartWriteString( UART_USB, "pressure\n");
//						xSemaphoreGive( sem_pressure_index );
//						create_task(set_matrix_index,"set_matrix_index",SIZE,0,2,&TaskHandle_set_matrix_index);
//						create_task(get_pressure_value,"get_pressure_value",SIZE,0,2,&TaskHandle_get_pressure_value);
//						back_down = true;
//					}
//				}
			}
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

//void print_measurements( void* pvParameters )
//{
//	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
//	TickType_t xLastWakeTime = xTaskGetTickCount();
//
//	uartWriteString( UART_USB, "print_values\n");
//
////	char buffer[700];
//
//	//stdioPrintf(UART_USED, ">3["); // 3 to indicate pressure measurement.
//	//sprintf(buffer, ">[");
//	// ---------- REPEAT FOR EVER --------------------------
//	while( TRUE )
//	{
//		create_task(print_matrix,"task_print_matrix",SIZE,0,1,NULL);
//
//		if(xSemaphoreTake(sem_matrix_print_finished, portMAX_DELAY)) {
//			create_task(print_vector,"task_print_vector",SIZE,0,1,NULL);
//		}
//
////		if(xSemaphoreTake(sem_vector_print_finished, portMAX_DELAY)) {
////			create_task(print_parameters,"task_print_parameters",SIZE,0,1,NULL);
////		}
//
//		if(xSemaphoreTake(sem_parameters_print_finished, portMAX_DELAY)) {
//			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);
//			vTaskDelete(NULL);
//		}
//	}
//}

void print_matrix( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	int matrix_val;
	int row=0, col=0;

	stdioPrintf(UART_USED, ">3{\"matrix\":["); // 3 to indicate pressure measurement.

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( xPrintQueue, &( matrix_val ), (TickType_t) 10)) // Dequeue matrix data
		{
			if (col < (MAX_COL-1))
			{
				stdioPrintf(UART_USED, "%d,", matrix_val);
				col++;
			}else{
				col = 0;
				row++;
				if (row < MAX_ROW)
				{
					stdioPrintf(UART_USED, "%d;", matrix_val);
				}else{
					stdioPrintf(UART_USED, "%d", matrix_val);
				}
			}
		}else{
			stdioPrintf(UART_USED, "],");

			create_task(print_vector,"print_vector",SIZE,0,1,NULL);

		    vTaskDelete(NULL);
		}
	}
}

void print_vector( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	int zeroed_newton_val;
	int index = 0;

	stdioPrintf(UART_USED, "\"vector\":[");

	//stdioPrintf(UART_USB, "print_vector \n");
	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		if(xQueueReceive( queue_jump, &( zeroed_newton_val ), (TickType_t) 10)) // Dequeue matrix data
		{
			stdioPrintf(UART_USED, "%d,", zeroed_newton_val);
			index ++;
		}else{
			stdioPrintf(UART_USED, "],");
			create_task(print_parameters,"print_parameters",SIZE,0,1,NULL);
			vTaskDelete(NULL);
		}
	}
}

void print_parameters( void* pvParameters )
{
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	struct jump_parameters jp;

	char vel_aux[20] = {};
	char t_aux[20] = {};
	char height_aux[20] = {};
	char power_aux[20] = {};

	// test values for struct:
	jp.vel = 2.6;
	jp.t = 1.3;
	jp.height = 0.5;
	jp.power = 5.6;

	// ---------- REPEAT FOR EVER --------------------------
	while( TRUE )
	{
		//if(xQueueReceive( queue_jump_parameters, &( jp ), (TickType_t) 10)) // Dequeue matrix data
		//{

			gcvt(jp.vel, 10, vel_aux);
			gcvt(jp.t, 10, t_aux);
			gcvt(jp.height, 10, height_aux);
			gcvt(jp.power, 10, power_aux);

			stdioPrintf(UART_USED, "\"speed\":%s,\"power\":%s,\"time\":%s,\"height\":%s}<\n", vel_aux, power_aux, t_aux, height_aux);

			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		//}
	}
}

/*==================[fin del archivo]========================================*/
