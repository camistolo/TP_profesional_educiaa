/* Copyright 2020, Franco Bucafusco
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

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "protocol.h"
#include "semphr.h"

#define FRAME_MAX_SIZE  200

uartMap_t         uart_used;
SemaphoreHandle_t new_frame_signal;
SemaphoreHandle_t sem_tx;
SemaphoreHandle_t mutex;


char buffer[FRAME_MAX_SIZE];
uint16_t index;

char* buffer_tx ;
uint16_t size_tx;
uint16_t counter_tx;

void protocol_tx_event( void *noUsado )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uartTxWrite( uart_used, buffer_tx[counter_tx] );

    counter_tx++;

    if( counter_tx==size_tx )
    {
        /* deshabilito la isr de transmision */
        uartCallbackClr( uart_used, UART_TRANSMITER_FREE );

        xSemaphoreGiveFromISR( sem_tx, &xHigherPriorityTaskWoken );
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void protocol_rx_event( void *noUsado )
{
    ( void* ) noUsado;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* leemos el caracter recibido */
    char c = uartRxRead( UART_232 );

    BaseType_t signaled = xSemaphoreTakeFromISR( mutex, &xHigherPriorityTaskWoken );

    if( signaled )
    {
        if( FRAME_MAX_SIZE-1==index )
        {
            /* reinicio el paquete */
            index = 0;
        }

        if( c=='>' )
        {
            if( index==0 )
            {
                /* 1er byte del frame*/
            }
            else
            {
                /* fuerzo el arranque del frame (descarto lo anterior)*/
                index = 0;
            }

            buffer[index] = c;

            /* incremento el indice */
            index++;
        }
        else if( c=='<' )
        {
            /* solo cierro el fin de frame si al menos se recibio un start.*/
            if( index>=1 )
            {
                /* se termino el paquete - guardo el dato */
                buffer[index] = c;

                /* incremento el indice */
                index++;

                /* Deshabilito todas las interrupciones de UART_USB */
                uartCallbackClr( uart_used, UART_RECEIVE );

                /* señalizo a la aplicacion */
                xSemaphoreGiveFromISR( new_frame_signal, &xHigherPriorityTaskWoken );
            }
            else
            {
                /* no hago nada, descarto el byte */
            }
        }
        else
        {
            /* solo cierro el fin de frame si al menos se recibio un start.*/
            if( index>=1 )
            {
                /* guardo el dato */
                buffer[index] = c;

                /* incremento el indice */
                index++;
            }
            else
            {
                /* no hago nada, descarto el byte */
            }
        }

        xSemaphoreGiveFromISR( mutex, &xHigherPriorityTaskWoken );
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void procotol_x_init( uartMap_t uart, uint32_t baudRate )
{
    /* CONFIGURO EL DRIVER */

    uart_used = uart;

    /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
    uartConfig( uart, baudRate );

    /* Seteo un callback al evento de recepcion y habilito su interrupcion */
    uartCallbackSet( uart, UART_RECEIVE, protocol_rx_event, NULL );

    /* Habilito todas las interrupciones de UART_USB */
    uartInterrupt( uart, true );

    /* CONFIGURO LA PARTE LOGICA */
    index = 0;
    new_frame_signal = xSemaphoreCreateBinary();
    sem_tx = xSemaphoreCreateBinary();
    mutex = xSemaphoreCreateMutex();
}

void protocol_wait_frame()
{
    xSemaphoreTake( new_frame_signal, portMAX_DELAY );
    xSemaphoreTake( mutex, 0 );
}

void  protocol_get_frame_ref( char** data, uint16_t* size )
{
    *data = buffer;
    *size = index;
}

void protocol_discard_frame()
{
    /* indico que se puede inciar un paquete nuevo */
    index = 0;

    /* libero la seccion critica, para que el handler permita ejecutarse */
    xSemaphoreGive( mutex );

    /* limpio cualquier interrupcion que hay ocurrido durante el procesamiento */
    uartClearPendingInterrupt( uart_used );

    /* habilito isr rx  de UART_USB */
    uartCallbackSet( uart_used, UART_RECEIVE, protocol_rx_event, NULL );
}

void protocol_transmit_frame( char* data, uint16_t size )
{
    buffer_tx = data ;
    size_tx = size;

    counter_tx = 0;

    uartCallbackSet( uart_used, UART_TRANSMITER_FREE, protocol_tx_event, NULL );

    /* dispara la 1ra interrupcion */
    uartSetPendingInterrupt( uart_used );

    xSemaphoreTake( sem_tx, portMAX_DELAY );
}
