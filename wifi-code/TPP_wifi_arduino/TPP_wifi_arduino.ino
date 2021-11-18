// To use the firmware open a terminal and type "node-red". 
// On another terminal run mosquitto. 
// To do this go to "Program Files"/mosquitto and type "mosquitto -v"

#include <FS.h>                   //this needs to be first
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include "utilities.h"
#include "wifi_functions.h"

//////////////////////////////////////////////////////////////////////////////////////

void setup() 
{  
  pinMode(lamp, OUTPUT);
  pinMode(0, INPUT_PULLUP); // Flash button
  
  Serial.begin(115200);

  // Read configuration from FS json.
  if (SPIFFS.begin()) 
  {
    if (SPIFFS.exists("/config.json")) 
    {
      // File exists, reading and loading.
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) 
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) 
        {
          strcpy(mqtt_server, json["mqtt_server"]);
        } else {
          // Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    // Serial.println("failed to mount FS");
  }
  
  // Extra parameters.
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);

  // Set config save notify callback.
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  // Add extra parameters.
  wifiManager.addParameter(&custom_mqtt_server);

  // Fetches ssid and pass and tries to connect.
  // If it does not connect it starts an access point with the specified name "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration.
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) 
  {
    delay(3000);
    // Reset and try again.
    ESP.reset();
    delay(5000);
  }

  // Read updated parameters.
  strcpy(mqtt_server, custom_mqtt_server.getValue());

  // Save the custom parameters to FS.
  if (shouldSaveConfig) 
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) 
    {
      // Serial.println("failed to open config file for writing");
    }
    json.printTo(configFile);
    configFile.close();
  }
 
  server_config();
}

//////////////////////////////////////////////////////////////////////////////////////

void loop() 
{      
  // Reset credentials and IP.
  if (digitalRead(0) == LOW)
  {
    wifiManager.resetSettings();
  }

  check_client_connection();

  client_loop();

  serial_data();

}
