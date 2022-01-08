#ifndef TASKS_PRESSURE_H
#define TASKS_PRESSURE_H

#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"

#define MAX_COL 14
#define MAX_ROW 14

//#define SENSOR_TEST 		// To test without sensor.
//#define SENSOR		// To test with sensor, but without PCB.
#define SENSOR_PCB	// To test the sensor with PCB.

#ifdef SENSOR_TEST
//#define UART_USED UART_USB
#define UART_USED UART_USB
#endif

#ifdef SENSOR
#define demuxY0 GPIO8
#define demuxY1 GPIO7
#define demuxY2 GPIO5
#define demuxY3 GPIO3
#define demuxY4 GPIO1
#define demuxY5 LCD1
#define demuxY6 LCD2
#define demuxY7 LCD3
#define demuxY8 LCDRS
#define demuxY9 LCD4
#define demuxY10 SPI_MISO
#define demuxY11 ENET_TXD1
#define demuxY12 ENET_TXD0
#define demuxY13 ENET_MDIO

#define UART_USED UART_232
#endif

#ifdef SENSOR_PCB
#define demuxS0 GPIO1
#define demuxS1 GPIO3
#define demuxS2 GPIO5
#define demuxS3 GPIO7
#define demuxSIG GPIO8

#define UART_USED UART_USB
#endif


#define muxS0 T_FIL0 // GPIO25
#define muxS1 T_FIL3 // GPIO28
#define muxS2 T_FIL2 // GPIO27
#define muxS3 T_COL0 //GPIO29
#define muxSIG CH3


void set_matrix_index( void* pvParameters );
void get_pressure_value( void* pvParameters );
//void print_matrix( void* pvParameters );

#ifndef SENSOR_TEST
void SetMuxChannel(int channel);
void SetDeMuxChannel(int channel);
#endif

#endif //TASKS_PRESSURE_H
