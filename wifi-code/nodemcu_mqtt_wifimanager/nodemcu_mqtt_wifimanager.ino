#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>
#include <PubSubClient.h> 

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Pegar la red wifi junto con su contraseña en el monitor serie. 
// Fibertel WiFi843 2.4GHz#0141600935#
// Fibertel WiFi582 2.4GHz#0142386309#
// Redmi-matu#matumatu#
// IPLANLiv- Matu 2C-2.4Ghz#nomeacuerdo#

#define Rx 3 // GPIO3
#define Tx 1 // GPIO1
#define WEIGHT_MEAS_STR "1"
#define FORCE_MEAS_STR "2"
#define PRESSURE_MEAS_STR "3"
#define HEIGHT_MEAS_STR "4"
#define ERROR_MEAS_STR "5"

// float sensor_data = 0;
int sensor_data = 0;
String sensor_data_str;

String ssid = "ssid"; 
String pass = "pass";
// Para no tener que poner siempre la clave wifi por puerto serie la dejo hardcodeada
//String ssid = "Fibertel WiFi843 2.4GHz"; 
//String pass = "0141600935";

int auxxx = 20;

bool connected_var = false; 
bool take_ssid = false;
int st=0;

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
// const char* mqtt_server = "192.168.0.91"; // Al estar corriendo el MQTT broker desde la PC poner acá la IP de la PC.
                                            // Abrir cmd y escribir ipconfig. Es la dirección IPv4 que aparece en la sección "Adaptador de LAN inalámbrica Wi-Fi:".
//const char* mqtt_server = "192.168.0.38";


// Para utilizar el programa completo abrir una terminal y escribir "node-red" y otra terminal con "mosquitto".
// Para correr mosquitto ir a "Program Files"/mosquitto y escibir "mosquitto -v"

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board
const int lamp = 4;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  pinMode(lamp, OUTPUT);

  pinMode(0, INPUT_PULLUP); // Flash button
  //wifiManager.resetSettings();
  
  Serial.begin(115200);
  //WiFi.mode(WIFI_STA);

    //read configuration from FS json
  //Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    //Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      //Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        //Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          //Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);

        } else {
          //Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    //Serial.println("failed to mount FS");
  }
  //end read



  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);

  

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);

  

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    //Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  //Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    //Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      //Serial.println("failed to open config file for writing");
    }
    //json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  // Serial.print("Message arrived on topic: ");
  // Serial.print(topic);
  // Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) 
  {
    // Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  // Serial.println();

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
        serial_print(WEIGHT_MEAS_STR);
        //Serial.print(">1<");
        //Serial.print(WEIGHT_MEAS);
      }
      else if(messageTemp == "medir fuerza")
      {
        serial_print(FORCE_MEAS_STR);
      }
      else if(messageTemp == "medir presion")
      {
        serial_print(PRESSURE_MEAS_STR);
      }
      else if(messageTemp == "medir altura")
      {
        serial_print(HEIGHT_MEAS_STR);
      }else
      {
        serial_print(ERROR_MEAS_STR);
      }
  }
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      //Serial.println("connected");  
      Serial.flush();
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/lamp");
      client.subscribe("room/indicate_measurement");
    } else {
      //Serial.print("failed, rc=");
      Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {

    
    if (digitalRead(0) == LOW)
    {
      wifiManager.resetSettings();
    }
  
    if (!client.connected()) {
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
    
    if(Serial.available()>0)
    {
      Serial.print("serial avalable");
      sensor_data_str = Serial.readString();
      //Serial.println(sensor_data_str);
      
      if(sensor_data_str.startsWith(">"))// && sensor_data_str.endsWith("<"))
      {
        int len = sensor_data_str.length();
        String meas_type = sensor_data_str.substring(1,2); // El primer dato que llega es el tipo de medicion
        sensor_data_str = sensor_data_str.substring(2,len-1);
        //static char sensor_data_temp[7];
        //dtostrf((float)sensor_data, 6, 2, sensor_data_temp);
        //client.publish("room/sensor_sim", sensor_data_temp);
        char copy[50];
        sensor_data_str.toCharArray(copy, 50);

        //if (meas_type == "1")
        if (meas_type == WEIGHT_MEAS_STR)
        {
          //client.publish("room/weight", copy);
          //auxxx = auxxx+1;
          //itoa(auxxx, copy, 10);
           client.publish("room/weight", copy);
        }else if (meas_type == FORCE_MEAS_STR){
          client.publish("room/force", copy);  
        }else if (meas_type == PRESSURE_MEAS_STR){
          client.publish("room/pressure", copy);  
        }else if (meas_type == HEIGHT_MEAS_STR){
          client.publish("room/height", copy);  
        }else
        {
          client.publish("room/sensor_sim", copy);    
        }
        //Serial.print("Sensor data: ");
        //Serial.println(sensor_data_str);
      }
      /*
      else{
        Serial.print("ERROR: ");
        Serial.println(sensor_data_str);
      }
      */
    }
}


void serial_print (String meas_str)
{
  Serial.print(">");
  Serial.print(meas_str);
  Serial.print("<");
}
