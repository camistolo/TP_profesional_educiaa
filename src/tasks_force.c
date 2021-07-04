#include "tasks_force.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
QueueHandle_t queue_measurement;
QueueHandle_t queue_tare;
QueueHandle_t queue_force;
QueueHandle_t queue_jump;
SemaphoreHandle_t sem_measurement;

extern void format( float valor, char *dst, uint8_t pos );
/*==================[definiciones de datos internos]=========================*/

volatile unsigned long OFFSET = 0;
bool tarado = false;
int new_measurement = 0;
float PESO;
extern int stage;
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

/*==================[definiciones de funciones internas]=====================*/
// Tarea que mide la fuerza
void task_measurement( void* taskParmPtr )
{
	// ---------- CONFIGURACIONES ------------------------------
		TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
		TickType_t xLastWakeTime = xTaskGetTickCount();
		TickType_t dif = *( (TickType_t*)  taskParmPtr );

		// Inicializaci�n de par�metros
		unsigned char i;
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


			// Esperar a que el m�dulo HX711 est� listo
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
				// Hacer medici�n final
				gpioWrite(ClockPin , ON);
				Count=Count^XOR_VALUE;
				gpioWrite(ClockPin , OFF);

				delay(100);

			xQueueSend(queue_measurement , &Count,  portMAX_DELAY);

			}

			// Delay peri�dico
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

		uartWriteString(UART,"waiting");

		if( !gpioRead(DataPin) ){
			// Indicar que el m�dulo ya est� listo para empezar a medir
			xSemaphoreGive( sem_measurement );
			//vTaskDelete(NULL);
			vTaskSuspend(NULL);
		}

		// Delay peri�dico
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
					if (!tarado){
						xQueueSend(queue_tare , &prom,  portMAX_DELAY);
					}
					else{
						xQueueSend(queue_force , &prom,  portMAX_DELAY);
					}
					vTaskSuspend(TaskHandle_measurement);
					vTaskSuspend(NULL);
				}
				else{
					sum += f;
					sprintf(str_aux, "f: %lu \r\n", f);
					uartWriteString(UART,str_aux);
					contador++;
					xSemaphoreGive( sem_measurement );
				}
			}

			// Delay peri�dico
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
	create_task(task_measurement,"task_measurement",SIZE,&tiempo_diff,PRIORITY+1,&TaskHandle_measurement);
	create_task(task_wait,"task_wait",SIZE,0,PRIORITY+1,&TaskHandle_wait);
	create_task(task_average,"task_average",SIZE,0,PRIORITY+1,&TaskHandle_average);

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

			if(xQueueReceive(queue_tare , &offset,  portMAX_DELAY)){			// Esperamos tecla
				OFFSET = offset;
				sprintf(str_aux, "Offset: %lu \r\n", offset);
				uartWriteString(UART,str_aux);
				tarado = true;
				gpioWrite( LED2, ON );
				vTaskDelay( 40 / portTICK_RATE_MS );
				gpioWrite( LED2 , OFF );
				vTaskDelete(NULL);
			}

			// Delay peri�dico
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
					uartWriteString(UART,str_aux);
					vTaskDelete(NULL);

			}

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}

}

// Tarea que setea el OFFSET
void task_jump( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  TASK_RATE_500;		// Tarea periodica cada 500 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	TickType_t tiempo_diff = 40 / portTICK_RATE_MS;

	uartWriteString(UART, "TAREA_JUMP");

	//unsigned long offset;
	float f = 0;
	TickType_t time_start;
	TickType_t time_end;
	float time_diff;
	float sup_1 = 0;
	float sup_2 = 0;
	float sup_3 = 0;
	char str_aux[50] = {};
	bool phase = 0;
	char fl_str_aux[64] = {};
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( TRUE ){

			gpioWrite( LED2 , ON );
			vTaskDelay( 40 / portTICK_RATE_MS );
			gpioWrite( LED2 , OFF );

			//Se resume la tarea de medicion para que devuelva un valor y comienzo a contar el tiempo
			vTaskResume(TaskHandle_measurement);
			new_measurement = 0;
			time_start = xTaskGetTickCount();

			if(xQueueReceive(queue_force , &f,  portMAX_DELAY)){			// Esperamos tecla
				time_end = xTaskGetTickCount();
				time_diff = (time_end - time_start)/1000;
				if (f < PESO && phase == 0){
					//uartWriteString(UART, "ETAPA 1");
					sup_1 = sup_1 + f*time_diff;
					phase = 1;
				}
				else if(f > PESO && phase == 1){
					//uartWriteString(UART, "ETAPA 2");
					if (sup_2 >= sup_1){
						phase = 2;
					}
					else{
						sup_2 = sup_2 + f*time_diff;
					}
				}
				else if(phase == 2){
					//uartWriteString(UART, "ETAPA 3");
					if (f <= PESO){
						vTaskSuspend(TaskHandle_measurement);
						xQueueSend(queue_jump , &sup_3,  portMAX_DELAY);
						format(sup_3,fl_str_aux,0);
						sprintf(str_aux, "SUP3: %s \r\n",fl_str_aux);
						uartWriteString(UART,str_aux);
						vTaskDelete(NULL);
					}
					else{
						sup_3 = sup_3 + f*time_diff;
					}
				}
				xSemaphoreGive( sem_measurement );
			}

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}

}

/*==================[fin del archivo]========================================*/
