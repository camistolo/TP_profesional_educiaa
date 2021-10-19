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
int new_measurement = 0;
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
	TickType_t dif = *( (TickType_t*)  taskParmPtr );

	// Inicialización de parámetros
	size_t i;
	unsigned long Count = 0;
	char str_aux[50] = {};

	//gpioWrite(DataPin,1);
	gpioWrite(ClockPin , 0);

	//create_task(task_wait,"task_wait",SIZE,0,PRIORITY+1,&TaskHandle_wait);
	//create_task(task_average,"task_average",SIZE,0,PRIORITY+1,NULL);

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		//sprintf(str_aux, "NEW_MEAS: %d \r\n", new_measurement);
		//uartWriteString(UART,str_aux);
		if (new_measurement == 0){
			vTaskResume(TaskHandle_wait);
			vTaskResume(TaskHandle_average);
			new_measurement = 1;
		}
		//Resumo las tareas de espera y promedio de fuerza

		// Esperar a que el módulo HX711 esté listo
		if (xSemaphoreTake( sem_measurement  ,  portMAX_DELAY )){
			gpioWrite( LEDG , ON );
			vTaskDelay( dif );
			gpioWrite( LEDG , OFF );

			Count = 0;

			// Medir 24 pulsos
			for (i = 0;i < GAIN_128;i++)
			{
				gpioWrite(ClockPin , ON);
				Count=Count<<1;
				gpioWrite(ClockPin , OFF);
				if(gpioRead(DataPin)){
						Count++;
				}
			}
			// Hacer medición final
			gpioWrite(ClockPin , ON);
			Count=Count^XOR_VALUE;
			gpioWrite(ClockPin , OFF);
			delay(50);
			xQueueSend(queue_measurement , &Count,  portMAX_DELAY);
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

		gpioWrite( LEDR , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LEDR , OFF );

		uartWriteString(UART_USB,"waiting");

		if( !gpioRead(DataPin) ){
			// Indicar que el módulo ya está listo para empezar a medir
			xSemaphoreGive( sem_measurement );
			//vTaskDelete(NULL);
			vTaskSuspend(NULL);
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
	int contador = 0;
	unsigned long prom;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		gpioWrite( LEDB , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LEDB , OFF );

		if(xQueueReceive(queue_measurement , &f,  portMAX_DELAY)){
			if (contador == CANT_MEDICIONES){
				prom = sum / CANT_MEDICIONES;
				contador = 0;
				sum = 0;
				xQueueSend(queue_force , &prom,  portMAX_DELAY);
				vTaskSuspend(TaskHandle_measurement);
				vTaskSuspend(NULL);
			}
			else{
				sum += f;
				sprintf(str_aux, "f: %lu \r\n", f);
				uartWriteString(UART_USB,str_aux);
				contador++;
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

	TickType_t tiempo_diff = 40 / portTICK_RATE_MS;

	//Creo las tareas de fuerza
	create_task(task_measurement,"task_measurement",SIZE,&tiempo_diff,1,&TaskHandle_measurement);
	create_task(task_wait,"task_wait",SIZE,0,1,&TaskHandle_wait);
	create_task(task_average,"task_average",SIZE,0,1,&TaskHandle_average);

	//Suspendo las tareas que no quiero que funcionen primero
	vTaskSuspend(TaskHandle_wait);
	vTaskSuspend(TaskHandle_average);

	unsigned long offset;
	char str_aux[50] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		if(xQueueReceive(queue_force , &offset,  portMAX_DELAY)){			// Esperamos tecla
			OFFSET = offset;
			sprintf(str_aux, "Offset: %lu \r\n", offset);
			uartWriteString(UART_USB,str_aux);
			gpioWrite( LED2, ON );
			vTaskDelay( 40 / portTICK_RATE_MS );
			gpioWrite( LED2 , OFF );

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

	TickType_t tiempo_diff = 40 / portTICK_RATE_MS;

	new_measurement = 0;
	vTaskResume(TaskHandle_measurement);

	unsigned long fu = 0;
	char str_aux[50] = {};
	char fl_str_aux[64] = {};
	float p;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

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

	TickType_t tiempo_diff = 40 / portTICK_RATE_MS;

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

	//Se resume la tarea de medicion para que devuelva un valor y comienzo a contar el tiempo
	vTaskResume(TaskHandle_measurement);

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){
		gpioWrite( LED2 , ON );
		vTaskDelay( 40 / portTICK_RATE_MS );
		gpioWrite( LED2 , OFF );

		//time_start = xTaskGetTickCount();

		if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){			// Esperamos tecla
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
					vTaskSuspend(TaskHandle_measurement);
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
