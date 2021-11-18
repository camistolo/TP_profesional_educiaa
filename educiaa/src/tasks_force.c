#include "tasks_force.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
QueueHandle_t queue_force;
QueueHandle_t queue_force_average;
QueueHandle_t queue_jump;
SemaphoreHandle_t sem_measure_force;

extern void format( float valor, char *dst, uint8_t pos );
/*==================[definiciones de datos internos]=========================*/

volatile unsigned long OFFSET = 0;
float PESO;
//extern int stage;
/*
typedef enum
{
    STAGE_TARE,
    STAGE_WEIGHT,
    STAGE_FORCE,
    STAGE_PRESSURE
} platformStage_t;

struct platformState{
	platformStage_t pStage;	//etapa de medicion (tara:0, peso:1, fuerza:2, presion:3)
	unsigned long offset;	//valor de tara
	int measurementsNumber;	//cantidad de mediciones realizadas en cada instante
	bool UARTSent;	//indica si se envio por UART o no lo que se deseaba
	unsigned long force;
	TickType_t timePassed;
} platformState_t;

struct platformMeasurements{
	float weight;
	float takeoffSpeed;
	float jumpTime;
	float jumpHeight;
	float jumpPower;
	float rightPressure[24][14]; //memset(arr, 0, sizeof arr);
	float leftPressure[24][14]; //memset(arr, 0, sizeof arr);
} platformMeasurements_t;*/



// Modo de medicion
typedef enum {
	AVERAGE_MODE,
	SIMPLE_MODE
} measurement_mode_t;

// Handles de las tareas
TaskHandle_t TaskHandle_hx711_ready;
TaskHandle_t TaskHandle_measure_force;
TaskHandle_t TaskHandle_average_force;
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
	TickType_t xPeriodicity =  TASK_RATE_20;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	// ---------- CONFIGURACIONES ------------------------------
	measurement_mode_t* mode = (measurement_mode_t*) taskParmPtr;

	// Inicialización de parámetros
	//size_t i;
	unsigned long Count = 0;
	char str_aux[50] = {};
	float time_diff;
	uint32_t Counter = 0;

	//gpioWrite(DataPin,1);
	gpioWrite(ClockPin , 0);

	SystemCoreClockUpdate();
	cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);

	if (*mode == AVERAGE_MODE) {
		create_task(task_average_force,"task_average_force",SIZE,0,1,&TaskHandle_average_force);
	}

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		uartWriteString(UART_USB, "1");

		// Esperar a que el módulo HX711 esté listo
		if (xSemaphoreTake( sem_measure_force  ,  portMAX_DELAY )){

			//time_start = xTaskGetTickCount();
			cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);

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
			Counter = cyclesCounterRead();
			time_diff = (float)Counter/((float)EDU_CIAA_NXP_CLOCK_SPEED/1000000.0);

			gcvt(time_diff,10,str_aux);
			//sprintf(str_aux, "TIME: %f \r\n",time_diff);
//			uartWriteString(UART_USB,str_aux);

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

// Tarea que promedia los valores medidos
void task_average_force( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_1;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	unsigned long f;
	unsigned long sum = 0;
	int counter = 0;
	unsigned long avg;
	char str_aux[50] = {};

	SystemCoreClockUpdate();
	cyclesCounterConfig(EDU_CIAA_NXP_CLOCK_SPEED);

	float time_diff;
	uint32_t Counter = 0;

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LEDB , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LEDB , OFF );

		uartWriteString(UART_USB, "2");

		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){

			cyclesCounterReset();

			if (counter == AVERAGE_N){
				// Calcular el promedio de las AVERAGE_N mediciones
				avg = sum / AVERAGE_N;
				// Enviar el promedio por cola
				xQueueSend(queue_force_average , &avg,  portMAX_DELAY);
				vTaskDelete(TaskHandle_measure_force);
				vTaskDelete(NULL);
			}
			else{
				sum += f;
				//sprintf(str_aux, "f: %lu \r\n", f);
				//uartWriteString(UART_USB,str_aux);
				counter++;
				// Volver a hacer otra medicion
				xSemaphoreGive( sem_measure_force );
			}
		}

		Counter = cyclesCounterRead();
		time_diff = (float)Counter/((float)EDU_CIAA_NXP_CLOCK_SPEED/1000000.0);
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
	char str_aux[50] = {};
	char fl_str_aux[64] = {};
	float p;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LED2 , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LED2 , OFF );

		if(xQueueReceive(queue_force_average , &fu,  portMAX_DELAY)){			// Esperamos tecla
			gpioWrite( LED1, ON );
			vTaskDelay(40 / portTICK_RATE_MS);
			gpioWrite( LED1, OFF );

			p = (fu - OFFSET) / SCALE;

			if (p > 300){
				p = 0;
			}

			PESO = p;

			format(p,fl_str_aux,0);
			sprintf(str_aux, "Offset: %lu, Fuerza: %lu, Peso: %s \r\n", OFFSET, fu, fl_str_aux);
			uartWriteString(UART_USB,str_aux);

			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que calcula el valor principal del salto
void task_jump( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_5;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uartWriteString(UART_USB, "TAREA_JUMP");

	//unsigned long offset;
	unsigned long f = 0;
	TickType_t time_start;
	TickType_t time_end;
	double time_diff;
	double force;
	double sup_1 = 0;
	double sup_2 = 0;
	double sup_3 = 0;
	char str_aux[50] = {};
	bool higher = false;
	char fl_str_aux[64] = {};

	measurement_mode_t mode = SIMPLE_MODE;
	create_task(task_measure_force,"task_measure_force",SIZE,&mode,1,&TaskHandle_measure_force);

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		time_start = xTaskGetTickCount();
		xSemaphoreGive( sem_measure_force );

		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){	// Esperamos tecla
			time_end = xTaskGetTickCount();
			time_diff = (time_end - time_start) / configTICK_RATE_HZ;
			//time_diff = 0.05; // 50 ms (del delay de la tarea de medicion)
			force = (f - OFFSET) / SCALE;
			sprintf(str_aux, "TIME: %f \r\n",time_diff);
			uartWriteString(UART_USB,str_aux);
			if (higher == false)
			{
				if (force >= PESO)
				{
					higher = true;
				}else{
					//uartWriteString(UART, "ETAPA 1");
					sup_1 += force * time_diff;
				}
			}else{
				if (sup_2 < sup_1)
				{
					sup_2 += force * time_diff;
				}else if (force > PESO){
					sup_3 += force * time_diff;
				}else{
					vTaskDelete(TaskHandle_measure_force);
					format(sup_3,fl_str_aux,0);
					sprintf(str_aux, "SUP3: %s \r\n",fl_str_aux);
					uartWriteString(UART_USB,str_aux);

					create_task(task_jump_parameters,"task_jump_parameters",SIZE,0,1,NULL);
					xQueueSend(queue_jump , &sup_3,  portMAX_DELAY);

					vTaskDelete(NULL);
				}
			}
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que calcula los parametros del salto
void task_jump_parameters( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uartWriteString(UART_USB, "TAREA_JUMP_PARAMETERS");

	double sup = 0;
	double vel = 0;
	double t = 0;
	double height = 0;
	double power = 0;
	double gravity = 9.81;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		if(xQueueReceive(queue_jump , &sup,  portMAX_DELAY)){			// Esperamos tecla
			vel = sup / PESO;
			t = 2 * (vel / gravity);
			height = (vel * vel)/ (2 * gravity);
			power = vel * PESO;

			sprintf(str_aux, "vel: %d, t: %d, h: %d, pot: %d \r\n", vel, t, height, power);
			uartWriteString(UART_USB,str_aux);

			create_task(task_receive_wifi,"task_receive_wifi",SIZE,0,1,NULL);

			vTaskDelete(NULL);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

/*==================[fin del archivo]========================================*/
