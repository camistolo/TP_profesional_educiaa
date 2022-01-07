#include "tasks_force.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
QueueHandle_t queue_force;
QueueHandle_t queue_force_average;
QueueHandle_t queue_jump;
QueueHandle_t queue_jump_parameters;
SemaphoreHandle_t sem_measure_force;

extern TaskHandle_t TaskHandle_print_measurements;

extern void format( float valor, char *dst, uint8_t pos );
/*==================[definiciones de datos internos]=========================*/

volatile unsigned long OFFSET = 0;
volatile unsigned long PESO = 0;

// Handles de las tareas
TaskHandle_t TaskHandle_hx711_ready;
TaskHandle_t TaskHandle_measure_force;
TaskHandle_t TaskHandle_average_force;
TaskHandle_t TaskHandle_median_force;
TaskHandle_t TaskHandle_weight;

/*==================[definiciones de funciones internas]=====================*/

// Tarea que espera a que el HX711 este listo para medir
void task_hx711_ready( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_50;		// Tarea periodica cada 50 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LEDR , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LEDR , OFF );

		uartWriteString(UART_USB,"waiting");

		if( !gpioRead(DataPin) ){
			// Crear la tarea de medicion
			measurement_mode_t mode = AVERAGE_MODE;
			create_task(task_measure_force,"task_measure_force",SIZE,&mode,1,&TaskHandle_measure_force);
			// Indicar que el modulo ya esta listo para empezar a medir
			xSemaphoreGive( sem_measure_force );
			vTaskDelete(NULL);
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que mide la fuerza
void task_measure_force( void* taskParmPtr )
{
	// ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_20;		// Tarea periodica cada 20 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	// ---------- CONFIGURACIONES ------------------------------
	measurement_mode_t* mode = (measurement_mode_t*) taskParmPtr;

	// Inicialización de parámetros
	//size_t i;
	unsigned long Count = 0;
	char str_aux[50] = {};
//	float time_diff;
//	uint32_t Counter = 0;
	uint32_t cyclesElapsed = 0;
	uint32_t msElapsed = 0, usElapsed = 0;

	//gpioWrite(DataPin,1);
	gpioWrite(ClockPin , 0);

//	SystemCoreClockUpdate();
//	cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);

	if (*mode == AVERAGE_MODE) {
//		create_task(task_average_force,"task_average_force",SIZE,0,1,&TaskHandle_average_force);
		create_task(task_median_force,"task_median_force",SIZE,0,1,&TaskHandle_median_force);
	}

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

//		if(*mode == SIMPLE_MODE) {
//			cyclesCounterReset();
//			stdioPrintf(UART_USB, "Testi segundos: %d\n\r", usElapsed);
//			cyclesElapsed = cyclesCounterRead();
//		   // Convierte el valor de ciclos en micro segundos
//		   usElapsed = cyclesCounterToUs(cyclesElapsed);
//		   // Imprime por pantalla el valor de los ciclos y los micro segundos transcurridos.
//		   stdioPrintf(UART_USB, "Milli segundos: %d\n\r", usElapsed);
//
//		}

//		uartWriteString(UART_USB, "1");

		// Esperar a que el módulo HX711 esté listo
		if (xSemaphoreTake( sem_measure_force  ,  portMAX_DELAY )){

			//time_start = xTaskGetTickCount();
//			cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);

			Count = 0;

			// Medir 24 pulsos
			for (size_t i = 0;i < GAIN_128;i++)
			{
				gpioWrite(ClockPin , ON);
				Count=Count<<1;
				gpioWrite(ClockPin , OFF);
				if(gpioRead(DataPin)){
						Count++;
				}
			}

			// Hacer medicion final
			gpioWrite(ClockPin , ON);
			Count=Count^XOR_VALUE;
			gpioWrite(ClockPin , OFF);
			//vTaskDelay( TASK_RATE_50 );

			//time_end = xTaskGetTickCount();
			//time_diff = (time_end - time_start) / 1000;
//			Counter = cyclesCounterRead();
//			time_diff = (float)Counter/((float)EDU_CIAA_NXP_CLOCK_SPEED/1000000.0);
//
//			gcvt(time_diff,10,str_aux);
			//sprintf(str_aux, "TIME: %f \r\n",time_diff);
//			uartWriteString(UART_USB,str_aux);
			if (Count < OFFSET) {
				Count = OFFSET;
			}

			// Enviar medicion a la tarea correspondiente
			xQueueSend(queue_force , &Count,  portMAX_DELAY);
			// A esta tarea la elimino en la tarea que recibe la medicion porque si se trata de una medicion
			// con promedio, entonces se va a tener que hacer un back and forth de esta tarea con la de
			// average y no tiene sentido eliminarla y crearla constantemente.
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

//// Tarea que promedia los valores medidos
//void task_average_force( void* taskParmPtr )
//{
//    // ---------- CONFIGURACIONES ------------------------------
//	TickType_t xPeriodicity =  TASK_RATE_1;		// Tarea periodica cada 1 ms
//	TickType_t xLastWakeTime = xTaskGetTickCount();
//
//	unsigned long f;
//	unsigned long sum = 0;
//	int counter = 0;
//	unsigned long avg;
//	char str_aux[50] = {};
//
//	//SystemCoreClockUpdate();
//	//cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);
//
////	float time_diff;
////	uint32_t Counter = 0;
//
//    // ---------- REPETIR POR SIEMPRE --------------------------
//	while ( TRUE ){
//
//		//gpioWrite( LEDB , ON );
//		//vTaskDelay( 40 / portTICK_RATE_MS );
//		//gpioWrite( LEDB , OFF );
//
//		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){
//
////			cyclesCounterReset();
//
//			if (counter == AVERAGE_N){
//				// Calcular el promedio de las AVERAGE_N mediciones
//				avg = sum / AVERAGE_N;
//				// Enviar el promedio por cola
//				xQueueSend(queue_force_average , &avg,  portMAX_DELAY);
//				vTaskDelete(TaskHandle_measure_force);
//				vTaskDelete(NULL);
//			}
//			else{
//				sum += f;
//				//sprintf(str_aux, "f: %lu \r\n", f);
//				//uartWriteString(UART_USB,str_aux);
//				counter++;
//				// Volver a hacer otra medicion
//				xSemaphoreGive( sem_measure_force );
//			}
//		}
//
////		Counter = cyclesCounterRead();
////		time_diff = (float)Counter/((float)EDU_CIAA_NXP_CLOCK_SPEED/1000000.0);
//		//gcvt(time_diff,10,str_aux);
//		//uartWriteString(UART_USB,str_aux);
//
//		// Delay periódico
//		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
//	}
//}

// Tarea que promedia los valores medidos
void task_median_force( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_1;		// Tarea periodica cada 1 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	unsigned long f;
	unsigned long f_values[AVERAGE_N];
	unsigned long sum = 0;
	int counter = 0;
	unsigned long median;
	char str_aux[50] = {};
	char p_str_aux[50] = {};
	double p = 0;

	//SystemCoreClockUpdate();
	//cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);

//	float time_diff;
//	uint32_t Counter = 0;

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LEDB , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LEDB , OFF );

		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){

//			cyclesCounterReset();
//			p = (f - OFFSET)*10 / SCALE;
//			gcvt(p,10,p_str_aux);
//			sprintf(str_aux,"weight: %s \r\n", p_str_aux);
//			uartWriteString(UART_USB,str_aux);

			if (counter == AVERAGE_N){
				// Calcular el promedio de las AVERAGE_N mediciones
				median = f_values[counter/2];
				// Enviar el promedio por cola
				xQueueSend(queue_force_average , &median,  portMAX_DELAY);
				vTaskDelete(TaskHandle_measure_force);
				vTaskDelete(NULL);
			}
			else{
				f_values[counter] = f;
				//sprintf(str_aux, "f: %lu \r\n", f);
				//uartWriteString(UART_USB,str_aux);
				counter++;
				// Volver a hacer otra medicion
				xSemaphoreGive( sem_measure_force );
			}
		}

//		Counter = cyclesCounterRead();
//		time_diff = (float)Counter/((float)EDU_CIAA_NXP_CLOCK_SPEED/1000000.0);
		//gcvt(time_diff,10,str_aux);
		//uartWriteString(UART_USB,str_aux);

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que setea el OFFSET
void task_tare( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	//Creo la tarea de medicion
	create_task(task_hx711_ready,"task_hx711_ready",SIZE,0,1,&TaskHandle_hx711_ready);

	unsigned long offset;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		if(xQueueReceive(queue_force_average , &offset,  portMAX_DELAY)){			// Esperamos tecla
			OFFSET = offset;
			sprintf(str_aux, "Offset: %lu \r\n", offset);
			uartWriteString(UART_USB,str_aux);
			//gpioWrite( LED2, ON );
			//vTaskDelay( 40 / portTICK_RATE_MS );
			//gpioWrite( LED2 , OFF );

//			procotol_x_init( UART_232, 115200 );
//
//			create_task(wait_frame,"wait_frame",SIZE,0,1,NULL);

			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);
//			create_task(task_send_to_measurement,"task_send_to_measurement",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que setea el OFFSET
void task_weight( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	//Creo la tarea de medicion
	create_task(task_hx711_ready,"task_hx711_ready",SIZE,0,1,&TaskHandle_hx711_ready);

	unsigned long fu = 0;
	char str_aux[80] = {};
	char fl_str_aux[64] = {};
	double p;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LED2 , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LED2 , OFF );

		if(xQueueReceive(queue_force_average , &fu,  portMAX_DELAY)){			// Esperamos tecla
			gpioWrite( LED1, ON );
			vTaskDelay(40 / portTICK_RATE_MS);
			gpioWrite( LED1, OFF );

			PESO = fu;

			p = (fu - OFFSET) * 10 / SCALE;

//			format(p,fl_str_aux,0);
			gcvt(p,10,fl_str_aux);
			sprintf(str_aux, "Offset: %lu, Fuerza: %lu\r\n", OFFSET, fu);
			uartWriteString(UART_USB,str_aux);
			uartWriteString(UART_USB, fl_str_aux);

			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

//// Tarea que calcula el valor principal del salto
//void task_jump( void* taskParmPtr )
//{
//    // ---------- CONFIGURACIONES ------------------------------
//	TickType_t xPeriodicity =  TASK_RATE_5;		// Tarea periodica cada 500 ms
//	TickType_t xLastWakeTime = xTaskGetTickCount();
//
//	uartWriteString(UART_USB, "TAREA_JUMP \r\n");
//
//	//unsigned long offset;
//	unsigned long f = 0;
//	unsigned long jump_values[JUMP_N];
//	size_t i = 0;
//	char str_aux[50] = {};
//	char fl_str_aux[64] = {};
//
////	cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);
////	cyclesCounterReset();
//
//	measurement_mode_t mode = SIMPLE_MODE;
//	create_task(task_measure_force,"task_measure_force",SIZE,&mode,1,&TaskHandle_measure_force);
//
//    // ---------- REPETIR POR SIEMPRE --------------------------
//	while ( TRUE ){
////		gpioWrite( LED2 , ON );
////		vTaskDelay( 40 / portTICK_RATE_MS );
////		gpioWrite( LED2 , OFF );
//
//		xSemaphoreGive( sem_measure_force );
//
//		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){
//
//			// El HX711 genera picos esporadicos que son debidos a una falla de diseno del chip.
//			// Por esta razon, si se observa un pico, se lo filtra, poniendole a la fuerza el mismo
//			// valor que el que se obtuvo anteriormente.
////			if (i > 0 && ( f < OFFSET || abs(f - jump_values[i - 1]) >= ((15 * SCALE) + OFFSET) ) ) {
////				f = jump_values[i - 1];
////			}
//
//			if(i > 0 && f < OFFSET ) {
//				f = OFFSET;
//			}
//
//			if (i == sizeof(jump_values)/sizeof(unsigned long)) {
//				create_task(task_jump_parameters,"task_jump_parameters",SIZE,0,1,NULL);
//				xQueueSend(queue_jump , jump_values,  portMAX_DELAY);
//				vTaskDelete(NULL);
//			}
//
//			xSemaphoreGive( sem_pressure_index );
//
//			jump_values[i++] = f;
//			sprintf(fl_str_aux, "%lu \r\n", f);
//			uartWriteString(UART_USB,fl_str_aux);
//		}
//		// Delay periódico
//		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
//	}
//}

// Tarea que calcula los parametros del salto
void task_calculate_jump_parameters( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_5;		// Tarea periodica cada 5 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	//unsigned long jump_values[JUMP_N];
	double force_sum_1 = 0;
	double force_sum_2 = 0;
	double force_sum_3 = 0;
	char str_aux[50] = {};
	bool accelerating_downards = false;
	bool deaccelerating = false;
	bool accelerating_upwards = false;
	char vel_str_aux[8] = {};
//	char t_str_aux[64] = {};
//	char height_str_aux[64] = {};
//	char power_str_aux[64] = {};
	struct jump_parameters jp;
	const double gravity = 9.8;
	const double time_diff = 0.02;
	const double jump_constant = 1.5;
	const double weight = ((double)(PESO) - (double)(OFFSET)) / (double)SCALE;

	double zeroed_newton;

	uartWriteString( UART_USB, "task_calculate_jump_parameters\n");

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		if(xQueuePeek(queue_jump , &zeroed_newton,  portMAX_DELAY)){
//			for (size_t i = 0; i < sizeof(jump_values)/sizeof(unsigned long); i++) {
			if (accelerating_downards == false) {
				if(zeroed_newton <= -20.0) {
					accelerating_downards = true;
				}
			}
			if (accelerating_downards == true && deaccelerating == false) {
				if (zeroed_newton > 5.0) {
					deaccelerating = true;
				} else {
					force_sum_1 += fabs(zeroed_newton);
				}
			}
			if (deaccelerating == true && accelerating_upwards == false) {
				if ((force_sum_2 + zeroed_newton) < force_sum_1) {
					force_sum_2 += zeroed_newton;
				} else {
					accelerating_upwards = true;
				}
			}
			if (accelerating_downards == true && deaccelerating == true && accelerating_upwards == true){
				if (zeroed_newton > 0.0){
					force_sum_3 += zeroed_newton;
				} else {
					jp.vel = force_sum_3 * time_diff * jump_constant / weight;
					jp.t =jp.vel / gravity;
					jp.height = (jp.vel * jp.vel)/ (2 * gravity);
					jp.power = jp.vel * weight * gravity;

					create_task(print_measurements,"task_print_measurements",SIZE,0,1,&TaskHandle_print_measurements);
					xQueueSend(&queue_jump_parameters , (void *) &jp,  portMAX_DELAY);

//					format(jp.vel,vel_str_aux,0);
//						format(jp.t,t_str_aux,0);
//						format(jp.height,height_str_aux,0);
//						format(jp.power,power_str_aux,0);
//						sprintf(str_aux, "vel: %s, t: %s, height: %s, power: %s \r\n", vel_str_aux, t_str_aux, height_str_aux, power_str_aux);
//						uartWriteString(UART_USB,str_aux);
//					sprintf(str_aux, "vel: %s \r\n", vel_str_aux);
//					uartWriteString(UART_USB,str_aux);

					vTaskDelete(NULL);
				}
			}
		}
//		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

//// Tarea que calcula los parametros del salto
//void task_jump_parameters( void* taskParmPtr )
//{
//    // ---------- CONFIGURACIONES ------------------------------
//	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
//	TickType_t xLastWakeTime = xTaskGetTickCount();
//
//	uartWriteString(UART_USB, "TAREA_JUMP_PARAMETERS");
//
//	double sup = 0;
//	double vel = 0;
//	double t = 0;
//	double height = 0;
//	double power = 0;
//	double gravity = 9.81;
//	char str_aux[50] = {};
//    // ---------- REPETIR POR SIEMPRE --------------------------
//	while ( TRUE ){
//		gpioWrite( LED2 , ON );
//		vTaskDelay( 40 / portTICK_RATE_MS );
//		gpioWrite( LED2 , OFF );
//
//		if(xQueueReceive(queue_jump , &sup,  portMAX_DELAY)){			// Esperamos tecla
//			vel = sup / PESO;
//			t = 2 * (vel / gravity);
//			height = (vel * vel)/ (2 * gravity);
//			power = vel * PESO;
//
//			sprintf(str_aux, "vel: %d, t: %d, h: %d, pot: %d \r\n", vel, t, height, power);
//			uartWriteString(UART_USB,str_aux);
//
//			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);
//
//			vTaskDelete(NULL);
//		}
//		// Delay periódico
//		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
//	}
//}

/*==================[fin del archivo]========================================*/
