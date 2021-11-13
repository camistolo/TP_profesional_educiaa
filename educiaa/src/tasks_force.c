#include "tasks_force.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
QueueHandle_t queue_measurement;
QueueHandle_t queue_force;
QueueHandle_t queue_jump;
SemaphoreHandle_t sem_measurement;

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
TaskHandle_t TaskHandle_wait;
TaskHandle_t TaskHandle_measurement;
TaskHandle_t TaskHandle_average;
TaskHandle_t TaskHandle_weight;

/*==================[definiciones de funciones internas]=====================*/
// Tarea que mide la fuerza
void task_measurement( void* taskParmPtr )
{
	// ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	// ---------- CONFIGURACIONES ------------------------------
	measurement_mode_t* mode = (measurement_mode_t*) taskParmPtr;

	// Inicialización de parámetros
	//size_t i;
	unsigned long Count = 0;
	char str_aux[50] = {};

	//gpioWrite(DataPin,1);
	gpioWrite(ClockPin , 0);

	if (mode == AVERAGE_MODE) {
		create_task(task_average,"task_average",SIZE,0,1,&TaskHandle_average);
	}

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		// Esperar a que el módulo HX711 esté listo
		if (xSemaphoreTake( sem_measurement  ,  portMAX_DELAY )){

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
			vTaskDelay( TASK_RATE_50 );

			// Enviar medicion a la tarea correspondiente
			xQueueSend(queue_measurement , &Count,  portMAX_DELAY);
			// A esta tarea la elimino en la tarea que recibe la medicion porque si se trata de una medicion
			// con promedio, entonces se va a tener que hacer un back and forth de esta tarea con la de
			// average y no tiene sentido eliminarla y crearla constantemente.
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que espera a que el HX711 este listo para medir
void task_wait( void* taskParmPtr )
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
			create_task(task_measurement,"task_measurement",SIZE,&mode,1,&TaskHandle_measurement);
			// Indicar que el modulo ya esta listo para empezar a medir
			xSemaphoreGive( sem_measurement );
			vTaskDelete(NULL);
		}

		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

// Tarea que promedia los valores medidos
void task_average( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	unsigned long f;
	unsigned long sum = 0;
	int counter = 0;
	unsigned long avg;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LEDB , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LEDB , OFF );

		if(xQueueReceive(queue_measurement , &f,  portMAX_DELAY)){
			if (counter == AVERAGE_N){
				// Calcular el promedio de las AVERAGE_N mediciones
				avg = sum / AVERAGE_N;
				// Enviar el promedio por cola
				xQueueSend(queue_force , &avg,  portMAX_DELAY);
				vTaskDelete(TaskHandle_measurement);
				vTaskDelete(NULL);
			}
			else{
				sum += f;
				//sprintf(str_aux, "f: %lu \r\n", f);
				//uartWriteString(UART_USB,str_aux);
				counter++;
				// Volver a hacer otra medicion
				xSemaphoreGive( sem_measurement );
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

	//Creo la tarea de medicion
	create_task(task_wait,"task_wait",SIZE,0,1,&TaskHandle_wait);

	unsigned long offset;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		if(xQueueReceive(queue_force , &offset,  portMAX_DELAY)){			// Esperamos tecla
			OFFSET = offset;
			sprintf(str_aux, "Offset: %lu \r\n", offset);
			uartWriteString(UART_USB,str_aux);
			//gpioWrite( LED2, ON );
			//vTaskDelay( 40 / portTICK_RATE_MS );
			//gpioWrite( LED2 , OFF );

			procotol_x_init( UART_232, 115200 );

			create_task(wait_frame,"wait_frame",SIZE,0,1,NULL);

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
	create_task(task_wait,"task_wait",SIZE,0,1,&TaskHandle_wait);

	unsigned long fu = 0;
	char str_aux[50] = {};
	char fl_str_aux[64] = {};
	float p;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		//gpioWrite( LED2 , ON );
		//vTaskDelay( 40 / portTICK_RATE_MS );
		//gpioWrite( LED2 , OFF );

		if(xQueueReceive(queue_force , &fu,  portMAX_DELAY)){			// Esperamos tecla
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
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	uartWriteString(UART_USB, "TAREA_JUMP");

	//unsigned long offset;
	unsigned long f = 0;
	TickType_t time_start;
	TickType_t time_end;
	double force;
	double time_diff;
	double sup_1 = 0;
	double sup_2 = 0;
	double sup_3 = 0;
	char str_aux[50] = {};
	bool higher=false;
	char fl_str_aux[64] = {};

	measurement_mode_t mode = SIMPLE_MODE;
	create_task(task_measurement,"task_measurement",SIZE,&mode,1,&TaskHandle_measurement);

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		//time_start = xTaskGetTickCount();

		if(xQueueReceive(queue_measurement , &f,  portMAX_DELAY)){	// Esperamos tecla
			//time_end = xTaskGetTickCount();
			//time_diff = (time_end - time_start)/1000;
			time_diff = 0.05; // 50 ms (del delay de la tarea de medicion)
			force = (f - OFFSET) / SCALE;
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
					sup_2 += force*time_diff;
				}else if (force > PESO){
					sup_3 += force * time_diff;
				}else{
					vTaskDelete(TaskHandle_measurement);

					format(sup_3,fl_str_aux,0);
					sprintf(str_aux, "SUP3: %s \r\n",fl_str_aux);
					uartWriteString(UART_USB,str_aux);

					create_task(task_jump_parameters,"task_jump_parameters",SIZE,0,1,NULL);
					xQueueSend(queue_jump , &sup_3,  portMAX_DELAY);

					vTaskDelete(NULL);
				}
			}
			xSemaphoreGive( sem_measurement );
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

	TickType_t tiempo_diff = 40 / portTICK_RATE_MS;

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
			height = (vel*vel)/ (2 * gravity);
			power = vel * PESO;

			sprintf(str_aux, "vel: %d, t: %d, h: %d, pot: %d \r\n", vel, t, height, power);
			uartWriteString(UART_USB,str_aux);
		}
		// Delay periódico
		vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
	}
}

/*==================[fin del archivo]========================================*/
