#include "utilities.h"
#include "sapi.h"
#include "tasks_pressure.h"

// ****************************
// Sin PCB:
/*
#ifdef SENSOR
extern const int demuxY0;
extern const int demuxY1;
extern const int demuxY2;
extern const int demuxY3;
extern const int demuxY4;
extern const int demuxY5;
extern const int demuxY6;
extern const int demuxY7;
extern const int demuxY8;
extern const int demuxY9;
extern const int demuxY10;
extern const int demuxY11;
extern const int demuxY12;
extern const int demuxY13;
#endif
// ****************************

#ifdef SENSOR_PCB
extern const int demuxS0 = GPIO1;
extern const int demuxS1 = GPIO3;
extern const int demuxS2 = GPIO5;
extern const int demuxS3 = GPIO7;
extern const int demuxSIG = GPIO8;
#endif


extern const int muxS0; // GPIO25
extern const int muxS1; // GPIO28
extern const int muxS2; // GPIO27
extern const int muxS3; //GPIO29
extern const int muxSIG;
*/

void gpio_config(void)
{

	#ifdef SENSOR
	gpioInit(demuxY0, GPIO_OUTPUT);
	gpioInit(demuxY1, GPIO_OUTPUT);
	gpioInit(demuxY2, GPIO_OUTPUT);
	gpioInit(demuxY3, GPIO_OUTPUT);
	gpioInit(demuxY4, GPIO_OUTPUT);
	gpioInit(demuxY5, GPIO_OUTPUT);
	gpioInit(demuxY6, GPIO_OUTPUT);
	gpioInit(demuxY7, GPIO_OUTPUT);
	gpioInit(demuxY8, GPIO_OUTPUT);
	gpioInit(demuxY9, GPIO_OUTPUT);
	gpioInit(demuxY10, GPIO_OUTPUT);
	gpioInit(demuxY11, GPIO_OUTPUT);
	gpioInit(demuxY12, GPIO_OUTPUT);
	gpioInit(demuxY13, GPIO_OUTPUT);
	#endif

	#ifdef SENSOR_PCB
	gpioInit(demuxS0, GPIO_OUTPUT);
	gpioInit(demuxS1, GPIO_OUTPUT);
	gpioInit(demuxS2, GPIO_OUTPUT);
	gpioInit(demuxS3, GPIO_OUTPUT);
	gpioInit(demuxSIG, GPIO_OUTPUT);
	#endif

   gpioInit(muxS0, GPIO_OUTPUT);
   gpioInit(muxS1, GPIO_OUTPUT);
   gpioInit(muxS2, GPIO_OUTPUT);
   gpioInit(muxS3, GPIO_OUTPUT);
}
