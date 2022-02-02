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

unsigned long OFFSET = 10000000;
unsigned long PESO = 0;

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

		// Solo cuando se mide el valor de DataPin en 0, se sabe que el hx711 esta listo
		// para medir
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

	gpioWrite(ClockPin , 0);

	// Si el modo es average (cambiar el nombre a median), entonces se crea la tarea que va
	// a hacer la mediana de todos los valores
	if (*mode == AVERAGE_MODE) {
		create_task(task_median_force,"task_median_force",SIZE,0,1,&TaskHandle_median_force);
	}

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		// Esperar a que el módulo HX711 esté listo
		if (xSemaphoreTake( sem_measure_force  ,  portMAX_DELAY )){

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

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){

			// Si ya se tienen los 20 valores, calcular la mediana como el valor medio
			// entre todos los valores medidos (esto permite evitar valores extremos que
			// en el promedio si se incluirian para el calculo)
			if (counter == AVERAGE_N){
				// Calcular la mediana de las AVERAGE_N mediciones
				median = f_values[counter/2];
				// Enviar la mediana por cola y eliminar las tareas de medicion y mediana
				xQueueSend(queue_force_average , &median,  portMAX_DELAY);
				vTaskDelete(TaskHandle_measure_force);
				vTaskDelete(NULL);
			}
			else{
				// Si todavia no se tienen los 20 valores, agregar el valor de fuerza
				// recibidoal arreglo y aumentar el contador
				if (f < OFFSET) {
					f_values[counter] = f;
					counter++;
				}
				// Volver a hacer otra medicion
				xSemaphoreGive( sem_measure_force );
			}
		}

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

	// Se crea la tarea que indica que el modulo hx711 esta listo para medir
	create_task(task_hx711_ready,"task_hx711_ready",SIZE,0,1,&TaskHandle_hx711_ready);

	unsigned long offset;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		// Si se recibe el valor de fuerza ya procesado por el calculo de la mediana,
		// se lo guarda en la  variable global OFFSET
		if(xQueueReceive(queue_force_average , &offset,  portMAX_DELAY)){
			OFFSET = offset;
			// Esto es solo para debuggear
			sprintf(str_aux, "Offset: %lu \r\n", offset);
			uartWriteString(UART_USB,str_aux);

			// Se crea la tarea que recibe un comando por wifi
			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}


void task_weight( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	// Se crea la tarea que indica que el modulo hx711 esta listo para medir
	create_task(task_hx711_ready,"task_hx711_ready",SIZE,0,1,&TaskHandle_hx711_ready);

	unsigned long fu = 0;
	char str_aux[80] = {};
	char fl_str_aux[64] = {};
	double p;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		// Si se recibe el valor del peso, ya promediado con la mediana, se lo guarda
		// en la variable global peso y se toma es evalor para calcular el peso en kg
		if(xQueueReceive(queue_force_average , &fu,  portMAX_DELAY)){

			PESO = fu;

			p = ((double)fu - (double)OFFSET) / SCALE;

			// Esto es solo para debuggear
			gcvt(p,10,fl_str_aux);
			sprintf(str_aux, "Offset: %lu, Fuerza: %lu\r\n", OFFSET, fu);
			uartWriteString(UART_USB,str_aux);
			uartWriteString(UART_USB, fl_str_aux);

			stdioPrintf(UART_USED, ">1%s<\n",fl_str_aux);

			// Se crea la tarea que recibe un comando por wifi
			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que calcula los parametros del salto
void task_calculate_jump_parameters( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_5;		// Tarea periodica cada 5 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	bool accelerating_downards = false;
	bool deaccelerating = false;
	bool accelerating_upwards = false;
	bool on_air = false;
	int time_on_air = 0;
	struct jump_parameters jp;
	const double weight = ((double)(PESO) - (double)(OFFSET)) / (double)SCALE;
	double zeroed_newton;

	uartWriteString( UART_USB, "task_calculate_jump_parameters\n");

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		// Se hace un peek para que los valores se reciban despues para imprimirse por wifi
		if(xQueuePeek(queue_jump , &zeroed_newton,  portMAX_DELAY)){
			// Etapa 0: todavia no empezo el salto
			if (accelerating_downards == false) {
				// Si se nota un valor en Newton menor a -20 (es como decir menor a 0, pero
				// se asegura que no haya una equivoacion porque la celda de carga midio mal)
				if(zeroed_newton <= -20.0) {
					accelerating_downards = true;
				}
			}
			// Etapa 1: acelerando hacia abajo
			if (accelerating_downards == true && deaccelerating == false) {
				// Si el valor en Newton es positivo, ya se comenzo la desaceleracion
				if (zeroed_newton > 5.0) {
					deaccelerating = true;
				}
			}
			// Etapa 2: desaceleracion y aceleracion hacia arriba
			if (deaccelerating == true && on_air == false) {
				// Si el valor en Newton es negativo, la persona ya esta en el aire
				if (zeroed_newton < 0) {
					on_air = true;
				}
			}
			// Etapa 3: En el aire
			if (on_air == true){
				// Si el valor es negativo (sigue en el aire), se aumenta el contador,
				// porque el tiempo entre cada medicion es siempre de 20 ms, entonces
				// el tiempo en el aire va a ser la cantidad de mediciones por 0,02 s
				if (zeroed_newton < 0){
					time_on_air++;
				// Si se mide un valor positivo de fuerza, el usuario ya cayo y se
				// pueden calcular los parametros
				} else {
					jp.t = time_on_air * FORCE_MEASUREMENT_PERIOD / 2;	// t es el tiempo hasta la altura maxima, por eso se lo divide por 2
					jp.vel = jp.t * GRAVITY;
					jp.height = (jp.vel * jp.vel)/ (2 * GRAVITY);
					jp.power = jp.vel * weight * GRAVITY;

					// Aca se deberia crear la tarea principal de impresion

//					create_task(print_measurements,"task_print_measurements",SIZE,0,1,&TaskHandle_print_measurements);
//					xQueueSend(&queue_jump_parameters , (void *) &jp,  portMAX_DELAY);

					vTaskDelete(NULL);
				}
			}
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}
//
//// Tarea que calcula los parametros del salto
//void task_calculate_jump_parameters( void* taskParmPtr )
//{
//    // ---------- CONFIGURACIONES ------------------------------
//	TickType_t xPeriodicity =  TASK_RATE_5;		// Tarea periodica cada 5 ms
//	TickType_t xLastWakeTime = xTaskGetTickCount();
//
//	//unsigned long jump_values[JUMP_N];
//	double force_sum_1 = 0;
//	double force_sum_2 = 0;
//	double force_sum_3 = 0;
//	char str_aux[50] = {};
//	bool accelerating_downards = false;
//	bool deaccelerating = false;
//	bool accelerating_upwards = false;
//	char vel_str_aux[8] = {};
////	char t_str_aux[64] = {};
////	char height_str_aux[64] = {};
////	char power_str_aux[64] = {};
//	struct jump_parameters jp;
//	const double gravity = 9.8;
//	const double time_diff = 0.02;
//	const double jump_constant = 1.5;
//	const double weight = ((double)(PESO) - (double)(OFFSET)) / (double)SCALE;
//
//	double zeroed_newton;
//
//	uartWriteString( UART_USB, "task_calculate_jump_parameters\n");
//
//    // ---------- REPETIR POR SIEMPRE --------------------------
//	while ( TRUE ){
//		gpioWrite( LED2 , ON );
//		vTaskDelay( 40 / portTICK_RATE_MS );
//		gpioWrite( LED2 , OFF );
//
//		if(xQueuePeek(queue_jump , &zeroed_newton,  portMAX_DELAY)){
////			for (size_t i = 0; i < sizeof(jump_values)/sizeof(unsigned long); i++) {
//			if (accelerating_downards == false) {
//				if(zeroed_newton <= -20.0) {
//					accelerating_downards = true;
//				}
//			}
//			if (accelerating_downards == true && deaccelerating == false) {
//				if (zeroed_newton > 5.0) {
//					deaccelerating = true;
//				} else {
//					force_sum_1 += fabs(zeroed_newton);
//				}
//			}
//			if (deaccelerating == true && accelerating_upwards == false) {
//				if ((force_sum_2 + zeroed_newton) < force_sum_1) {
//					force_sum_2 += zeroed_newton;
//				} else {
//					accelerating_upwards = true;
//				}
//			}
//			if (accelerating_downards == true && deaccelerating == true && accelerating_upwards == true){
//				if (zeroed_newton > 0.0){
//					force_sum_3 += zeroed_newton;
//				} else {
//					jp.vel = force_sum_3 * time_diff * jump_constant / weight;
//					jp.t =jp.vel / gravity;
//					jp.height = (jp.vel * jp.vel)/ (2 * gravity);
//					jp.power = jp.vel * weight * gravity;
//
//					create_task(print_measurements,"task_print_measurements",SIZE,0,1,&TaskHandle_print_measurements);
//					xQueueSend(&queue_jump_parameters , (void *) &jp,  portMAX_DELAY);
//
////					format(jp.vel,vel_str_aux,0);
////						format(jp.t,t_str_aux,0);
////						format(jp.height,height_str_aux,0);
////						format(jp.power,power_str_aux,0);
////						sprintf(str_aux, "vel: %s, t: %s, height: %s, power: %s \r\n", vel_str_aux, t_str_aux, height_str_aux, power_str_aux);
////						uartWriteString(UART_USB,str_aux);
////					sprintf(str_aux, "vel: %s \r\n", vel_str_aux);
////					uartWriteString(UART_USB,str_aux);
//
//					vTaskDelete(NULL);
//				}
//			}
//		}
////		}
//		// Delay periódico
//		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
//	}
//}

/*==================[fin del archivo]========================================*/
