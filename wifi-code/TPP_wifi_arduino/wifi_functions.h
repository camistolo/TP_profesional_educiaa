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
const char force_meas = '2';
const char pressure_meas = '3';
const char height_meas = '4';
const char error_meas = '5';

//////////////////////////////////////////////////////////////////////////////////////

void server_config();
void check_client_connection();
void client_loop();
void serial_data();
void saveConfigCallback ();
void callback(String topic, byte* message, unsigned int length); 
void reconnect();

#endif WIFI_FUNCTIONS_H
