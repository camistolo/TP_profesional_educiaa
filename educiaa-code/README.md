***************14/10/20***************

En principio se tiene la tarea que recibe de wifi y crea la de medicion, que a su vez crea la de espera (estas ultimas dos con prioridad 2). La de medicion funciona una vez y dsps nunca mas, solo funciona la de espera. Voy a probar ponerle a la de medicion una prioridad mas alta.

Le puse prioridad 3 a la tarea de medicion y sigue solo funcionando la de esperar medicion. Eso no me molesta, pero igualmente nunca entra a estar listo para medir. Ese es el problema.

Lei que hay un problema que si sos muy lento, el datapin no se pone en 1 (https://forum.micropython.org/viewtopic.php?t=2678) asi que pase de periodicidad de 500 ms a 50 ms y voy a ver si eso lo resuelve.

SOLUCION (?): Yo ponia al principio el DATA pin en 1 para que despues el HX711 lo cambie pero no lo cambiaba. Lo deje de setear como 1 y ahi funciono, pero no se si esto esta bien.

***************
Ahora estoy intentando imprimir lo que se mide pero como no puse las cc (celdas de carga) en la plataforma, entonces no tienen peso, pero voy a ver si haciendo fuerza puedo cambiar el valor. Intent√© hacer fuerza pero sigue dando 0. No s√© si es porque no puse el DATA pin en 1 al principio o porque no hay fuerza suficiente.

***************3/11/20***************

CON DEBUGPRINTSTRING NO FUNCIONA PERO CON UARTWRITESTRING SI

***************8/11/20***************

Por quÈ se usa el mutex con ISR (en la p·gina dice que no se puede hacer esto).

***************9/11/20***************

CÛmo podemos modificar el programa para que las instrucciones sean m·s r·pidas?
Preguntar por ADC a freertos (es con interrupciones?).


define SAPI_USE_INTERRUPTS
PRUEBA: bluetooth terminal en el celu con la CIAA
bajar el rate y probar a ver si hay tiempo de ocio
subir el stack si hay muchos print
mandar puntero a la cola/sem crear
control RC: ADC