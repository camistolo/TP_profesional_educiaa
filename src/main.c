/* Copyright 2017-2018, Eric Pernia
 * All rights reserved.
 *
 * This file is part of sAPI Library.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*==================[inlcusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sapi.h"

#include "FreeRTOSConfig.h"
/*==================[definiciones y macros]==================================*/

#define DataPin GPIO8
#define ClockPin GPIO7
#define SCALE		17000

/*==================[definiciones de datos internos]=========================*/

double fuerza;
int cant_promediada = 5;
unsigned long Count = 0;
//volatile unsigned long OFFSET = 0;

/*==================[definiciones de datos externos]=========================*/
DEBUG_PRINT_ENABLE;

//Handle de la cola
QueueHandle_t cola_datos_calculados;
SemaphoreHandle_t sem_medir_fuerza;
//SemaphoreHandle_t sem_promediar_medicion;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/
TickType_t get_diff();
void clear_diff();

// Prototipo de funcion de la tarea
void tarea_Rx_WIFI( void* taskParmPtr );
void tarea_Tx_WIFI( void* taskParmPtr );
void tarea_medir_fuerza( void* taskParmPtr );
void tarea_esperar_medicion( void* taskParmPtr );


// Handles de las tareas
TaskHandle_t TaskHandle_esperar;
TaskHandle_t TaskHandle_medir;

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main( void )
{
    // ---------- CONFIGURACIONES ------------------------------
    // Inicializar y configurar la plataforma
    boardConfig();
    HX711Config();

    // FALTA VER EL TEMA DE LA TARA AL PRINCIPIO

    // UART for debug messages
    debugPrintConfigUart( UART_USB, 115200 );
    debugPrintlnString( "TP Profesional." );

    // Led para dar señal de vida
    gpioWrite( LED3, ON );

    // Creación de la cola
    cola_datos_calculados = xQueueCreate(cant_promediada,sizeof(unsigned int));

    // Creación del semáforo
    sem_medir_fuerza = xSemaphoreCreateBinary();
    //sem_promediar_medicion = xSemaphoreCreateCounting( cant_promediada , 0 );

    // Inicialización de variables
    fuerza = 0; // Variable que guarda el peso

    // Creación de las tareas
    BaseType_t res =
    xTaskCreate(
    	tarea_Rx_WIFI,                     // Funcion de la tarea a ejecutar
        ( const char * )"tarea_Rx",   // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
        0,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea
        0                           	// Puntero a la tarea creada en el sistema
    );

    if(res == pdFAIL)
    {
    	//error
    }

    res =
    xTaskCreate(
    	tarea_Tx_WIFI,                     // Funcion de la tarea a ejecutar
        ( const char * )"tarea_Tx",   // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
        0,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea
        0                           	// Puntero a la tarea creada en el sistema
    );

    if(res == pdFAIL)
    {
    	//error
    }

    	// Iniciar scheduler
    vTaskStartScheduler();

    // ---------- REPETIR POR SIEMPRE --------------------------
    while( TRUE )
    {
        // Si cae en este while 1 significa que no pudo iniciar el scheduler
    }

    // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
    // directamenteno sobre un microcontroladore y no es llamado por ningun
    // Sistema Operativo, como en el caso de un programa para PC.
    return 0;
}

/*==================[definiciones de funciones internas]=====================*/

/*==================[definiciones de funciones externas]=====================*/


void tarea_Rx_WIFI( void* taskParmPtr )
{
	fsmButtonInit();

	while( 1 )
	{
		fsmButtonUpdate( TEC1 );
	 	vTaskDelay( 1 / portTICK_RATE_MS );
	}
}

// Implementacion de funcion de la tarea
void tarea_Tx_WIFI( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();
	// ---------- REPETIR POR SIEMPRE --------------------------
    while( 1 )
    {
    	if(xQueueReceive(cola_datos_calculados , &fuerza,  portMAX_DELAY)){			// Esperamos tecla

			gpioWrite( LED1, ON );
			vTaskDelay(40 / portTICK_RATE_MS);
			gpioWrite( LED1, OFF );

			debugPrintString( "Fuerza: " );
			debugPrintlnUInt(fuerza);

			// Delay periódico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
    	}

    }
}

// Tarea que mide la fuerza
void tarea_medir_fuerza( void* taskParmPtr )
{
	// ---------- CONFIGURACIONES ------------------------------
		TickType_t xPeriodicity =  500 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
		TickType_t xLastWakeTime = xTaskGetTickCount();
		TickType_t dif = *( (TickType_t*)  taskParmPtr );

		// Inicialización de parámetros
		unsigned char i;

		//gpioWrite(DataPin,1);
		gpioWrite(ClockPin,0);

		// Crear tarea de espera
			BaseType_t res =
			xTaskCreate(
				tarea_esperar_medicion,                     	// Funcion de la tarea a ejecutar
				( const char * )"tarea_esperar",   	// Nombre de la tarea como String amigable para el usuario
				configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
				0,                	// Parametros de tarea
				tskIDLE_PRIORITY+2,         	// Prioridad de la tarea
				&TaskHandle_esperar                          	// Puntero a la tarea creada en el sistema
			);

			if(res == pdFAIL)
			{
				//error
			}

	    // ---------- REPETIR POR SIEMPRE --------------------------
		while ( 1 ){

			// Esperar a que el módulo HX711 esté listo
			if (xSemaphoreTake( sem_medir_fuerza  ,  portMAX_DELAY )){

				gpioWrite( LEDG , 1 );
				vTaskDelay( dif );
				gpioWrite( LEDG , 0 );

				// Medir 24 pulsos
				for (i=0;i<24;i++)
					{
						gpioWrite(ClockPin,1);
						Count=Count<<1;
						gpioWrite(ClockPin,0);
						if(gpioRead(DataPin)){

							//gpioWrite( LEDG , 1 );
							//vTaskDelay(40 / portTICK_RATE_MS);
							//gpioWrite( LEDG , 0 );
							Count++;
						}
					}
				// Hacer medición final
				gpioWrite(ClockPin,1);
				Count=Count^0x800000;
				gpioWrite(ClockPin,0);

				//fuerza = 10;

				xQueueSend(cola_datos_calculados , &Count,  portMAX_DELAY);

				vTaskDelete(NULL);
			}

			// Delay periódico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
		}

}

// Tarea que espera a que el HX711 este listo para medir
void tarea_esperar_medicion( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  50 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( 1 ){

			gpioWrite( LEDR , 1 );
			vTaskDelay( 40 / portTICK_RATE_MS );
			gpioWrite( LEDR , 0 );

			if( !gpioRead(DataPin) ){
				// Indicar que el módulo ya está listo para empezar a medir
				xSemaphoreGive( sem_medir_fuerza );
				vTaskDelete(NULL);
			}

			// Delay periódico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}

}


/*==================[fin del archivo]========================================*/
