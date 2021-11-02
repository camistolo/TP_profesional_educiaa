# Diseño actual
 Pasos que se llevan a cabo actualmente

1. Se crea la tarea de tara **task_tare**
    1. La tarea de tara crea las tareas **task_wait**, **task_average** y **task_measurement**.
    2. La tarea de medición espera a que **task_wait** le diga a través de un semáforo que ya puede medirse.
    3. Se intercala entre las tareas de medición y promedio (mediante una cola y un semáforo) para obtener un resultado preciso.
    4. Se devuelve el valor promedio final a la tarea de tara mediante una cola, se habilita la interrupción de wifi y s eleimina la tarea de tara.
2. La interrupción de wifi espera a recibir algún valor por wifi.
    1. Si recibe que se mida el peso, se crea la tarea **task_weight**, y si recibe que se mida el salto, se crea la tarea **task_jump**.
3. Ambas tareas (de peso y de salto) calculan lo correspondiente y luego, vuelven a llamar a la función **protocol_x_init** que habilita la interrupción de wifi. 

# Cosas a modificar
- Crear todas las tareas en el main, de forma tal de asegurar que todas las tareas son creadas correctamente al principio [TO DO].
- Separar las funciones de wifi en una parte que se encargue de recibir datos y otra que se encargue de enviar datos [IN PROGRESS].
- Cambiar el uso de *vTaskResume* y *vTaskSuspend* por semáforos/colas [TO DO].
- Juntar todas las variables en una o dos estructuras que condensen toda la información [TO DO].

# Errores que se están obteniendo actualmente
- Pasos que se siguieron: *Se tara a la plataforma sin inconvenientes, se manda por wifi a través de node-red para medir algo (ya sea peso o fuerza, es lo mismo) y cuando se quiere volver a mandar una instrucción por wifi mediante node-red, el programa no recibe nada.*
- Error que apareció: 
```
Error: JTAG-DP STICKY ERROR
Error: MEM_AP_CSW 0x23000062, MEM_AP_TAR 0x10008004
Error: Failed to read memory at 0x10008004
Error: JTAG-DP STICKY ERROR
Error: MEM_AP_CSW 0x23000050, MEM_AP_TAR 0xa5a5a5a6
Error: Failed to read memory at 0xa5a5a5a6
```
