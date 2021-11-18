#include "wifi_functions.h"

//////////////////////////////////////////////////////////////////////////////////////

bool shouldSaveConfig = false;
char mqtt_server[40];
String sensor_data_str;

// Initializes the espClient.
WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

//////////////////////////////////////////////////////////////////////////////////////

void server_config()
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

//////////////////////////////////////////////////////////////////////////////////////

void check_client_connection()
{
  if (!client.connected()) 
  {
    Serial.print("client connected");
    reconnect();
    Serial.flush();
  }
}

//////////////////////////////////////////////////////////////////////////////////////

void client_loop()
{
  if(!client.loop())
  {
    Serial.print("client loop");
    client.connect("ESP8266Client");
    Serial.flush();
  }
}

//////////////////////////////////////////////////////////////////////////////////////

void serial_data()
{
    // New data to read.
  if(Serial.available()>0)
  {
    //sensor_data_str = Serial.readString();
    sensor_data_str = Serial.readStringUntil('\n');
    
    if(sensor_data_str.startsWith(">") && sensor_data_str.endsWith("<"))
    {
      int len = sensor_data_str.length();

      // The first character (after the ">") is the measurement type.
      String meas_type = sensor_data_str.substring(1,2); 
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

//////////////////////////////////////////////////////////////////////////////////////

// Callback notifying us of the need to save config
void saveConfigCallback () 
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//////////////////////////////////////////////////////////////////////////////////////

// This functions is executed when some device publishes a 
// message to a topic that your ESP8266 is subscribed to.
void callback(String topic, byte* message, unsigned int length) 
{
  String messageTemp;
  
  for (int i = 0; i < length; i++) 
  {
    messageTemp += (char)message[i];
  }

  // LED for debugging correct WiFi operation.
  if(topic=="room/lamp"){
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
      }
      else if(messageTemp == "medir fuerza")
      {
        serial_print(force_meas);
      }
      else if(messageTemp == "medir presion")
      {
        serial_print(pressure_meas);
      }
      else if(messageTemp == "medir altura")
      {
        serial_print(height_meas);
      }else
      {
        serial_print(error_meas);
      }
  }
}

//////////////////////////////////////////////////////////////////////////////////////

// This functions reconnects your ESP8266 to your MQTT broker.
void reconnect() 
{
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
