#ifndef WIFI_FUNCTIONS_H
#define WIFI_FUNCTIONS_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <Arduino.h>
#include "utilities.h"

//////////////////////////////////////////////////////////////////////////////////////

//flag for saving data
extern bool shouldSaveConfig;

extern char mqtt_server[40];
extern String sensor_data_str;

extern WiFiManager wifiManager;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board for debugging
const int lamp = 4;

const char weight_meas = '1';
const char jump_meas = '3';
const char error_meas = '5';

const char pressure_row_1 = 'A';
const char pressure_row_2 = 'B';
const char pressure_row_3 = 'C';
const char pressure_row_4 = 'D';
const char pressure_row_5 = 'E';
const char pressure_row_6 = 'F';
const char pressure_row_7 = 'G';
const char pressure_row_8 = 'H';
const char pressure_row_9 = 'I';
const char pressure_row_10 = 'J';
const char pressure_row_11 = 'K';
const char pressure_row_12 = 'L';
const char pressure_row_13 = 'M';
const char pressure_row_14 = 'N';

//////////////////////////////////////////////////////////////////////////////////////

void server_config();
void check_client_connection();
void client_loop();
void serial_data();
void saveConfigCallback ();
void callback(String topic, byte* message, unsigned int length); 
void reconnect();

#endif WIFI_FUNCTIONS_H
