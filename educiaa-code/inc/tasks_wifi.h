#ifndef TASKS_WIFI_H
#define TASKS_WIFI_H

#include <stdio.h>

//#define SAPI_USE_INTERRUPTS
#define DELAY_TIME 10 // Wait time in ms
#define FRAME_MAX_SIZE 4 // Message received is like >n<, with n between 0 and 9.
#define WEIGHT_MEAS '1'
#define FORCE_MEAS '2'
#define PRESSURE_MEAS '3'
#define HEIGHT_MEAS '4'
#define ERROR_MEAS '5'
#define UART_USED UART_232

void task_wait_wifi( void* pvParameters );
void task_choose_meas( void* pvParameters );
void task_weight( void* pvParameters );
void task_force( void* pvParameters );
void task_pressure( void* pvParameters );
void task_height( void* pvParameters );
void task_send_wifi( void* pvParameters );

#endif // TASKS_WIFI_H
