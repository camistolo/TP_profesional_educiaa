// To use the firmware open a terminal and type "node-red". 
// On another terminal run mosquitto. To do this go to "Program Files"/mosquitto and type "mosquitto -v"

#include <FS.h>                   //this needs to be first
#include <ESP8266WiFi.h>
#include <PubSubClient.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#define Rx 3 // GPIO3
#define Tx 1 // GPIO1
#define WEIGHT_MEAS_STR "1"
#define FORCE_MEAS_STR "2"
#define PRESSURE_MEAS_STR "3"
#define HEIGHT_MEAS_STR "4"
#define ERROR_MEAS_STR "5"

const char weight_meas = '1';
const char force_meas = '2';
const char pressure_meas = '3';
const char height_meas = '4';
const char error_meas = '5';

char mqtt_server[40];

//flag for saving data
bool shouldSaveConfig = false;

String sensor_data_str;

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board for debugging
const int lamp = 4;


// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() 
{
  pinMode(lamp, OUTPUT);
  pinMode(0, INPUT_PULLUP); // Flash button
  
  Serial.begin(115200);

  //Read configuration from FS json.
  if (SPIFFS.begin()) 
  {
    if (SPIFFS.exists("/config.json")) 
    {
      //File exists, reading and loading.
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

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() 
{    
  // Reset credentials and IP.
  if (digitalRead(0) == LOW)
  {
    wifiManager.resetSettings();
  }

  if (!client.connected()) 
  {
    Serial.print("client connected");
    reconnect();
    Serial.flush();
  }
  
  if(!client.loop())
  {
    Serial.print("client loop");
    client.connect("ESP8266Client");
    Serial.flush();
  }

  // New data to read.
  if(Serial.available()>0)
  {
    sensor_data_str = Serial.readString();
    
    if(sensor_data_str.startsWith(">"))// && sensor_data_str.endsWith("<"))
    {
      int len = sensor_data_str.length();
      String meas_type = sensor_data_str.substring(1,2); // The first character (after the ">") is the measurement type.
      sensor_data_str = sensor_data_str.substring(2,len-1);
      
      char copy[50];
      sensor_data_str.toCharArray(copy, 50);

      switch(meas_type.charAt(0))
      {
        case weight_meas: 
          client.publish("room/weight", copy);
          break;
        case force_meas: 
          client.publish("room/force", copy);
          break;
        case pressure_meas: 
          client.publish("room/pressure", copy);
          break;
        case height_meas: 
          client.publish("room/height", copy);
          break;
        default:
          client.publish("room/sensor_sim", copy);    
          break;
      }
    }
  }
}


// Callback notifying us of the need to save config
void saveConfigCallback () 
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to.
void callback(String topic, byte* message, unsigned int length) 
{
  String messageTemp;
  
  for (int i = 0; i < length; i++) 
  {
    messageTemp += (char)message[i];
  }

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="room/lamp"){
      //Serial.print("Changing Room lamp to: ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.println("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.println("Off");
      }
  }
  
  if(topic=="room/indicate_measurement")
  {
      if(messageTemp == "medir peso")
      {
        serial_print(weight_meas);
        //serial_print(WEIGHT_MEAS_STR);
      }
      else if(messageTemp == "medir fuerza")
      {
        serial_print(force_meas);
        //serial_print(FORCE_MEAS_STR);
      }
      else if(messageTemp == "medir presion")
      {
        serial_print(pressure_meas);
        //serial_print(PRESSURE_MEAS_STR);
      }
      else if(messageTemp == "medir altura")
      {
        serial_print(height_meas);
        //serial_print(HEIGHT_MEAS_STR);
      }else
      {
        serial_print(error_meas);
        //serial_print(ERROR_MEAS_STR);
      }
  }
}

// This functions reconnects your ESP8266 to your MQTT broker.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {  
    if (client.connect("ESP8266Client")) 
    {
      Serial.flush();
      client.subscribe("room/lamp");
      client.subscribe("room/indicate_measurement");
    } else {
      Serial.print(client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



//void serial_print (String meas_str)
void serial_print (char meas_str)
{
  Serial.print(">");
  Serial.print(meas_str);
  Serial.print("<");
}
