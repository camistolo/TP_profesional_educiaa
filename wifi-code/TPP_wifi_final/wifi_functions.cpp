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
    //sensor_data_str = Serial.readStringUntil('#');
    
    if(sensor_data_str.startsWith(">") && sensor_data_str.endsWith("<"))
    {
      int len = sensor_data_str.length();

      // The first character (after the ">") is the measurement type.
      String meas_type = sensor_data_str.substring(1,2); 
      sensor_data_str = sensor_data_str.substring(2,len-1);
      
      char copy[1000];
      sensor_data_str.toCharArray(copy, 1000);

      switch(meas_type.charAt(0))
      {
        case pressure_row_1: 
          client.publish("row_1", copy, true);
          break;
        case pressure_row_2: 
          client.publish("row_2", copy, true);
          break;
        case pressure_row_3: 
          client.publish("row_3", copy, true);
          break;
        case pressure_row_4: 
          client.publish("row_4", copy, true);
          break;
        case pressure_row_5: 
          client.publish("row_5", copy, true);
          break;
        case pressure_row_6: 
          client.publish("row_6", copy, true);
          break;
        case pressure_row_7: 
          client.publish("row_7", copy, true);
          break;
        case pressure_row_8: 
          client.publish("row_8", copy, true);
          break;
        case pressure_row_9: 
          client.publish("row_9", copy, true);
          break;
        case pressure_row_10: 
          client.publish("row_10", copy, true);
          break;
        case pressure_row_11: 
          client.publish("row_11", copy, true);
          break;
        case pressure_row_12: 
          client.publish("row_12", copy, true);
          break;
        case pressure_row_13: 
          client.publish("row_13", copy, true);
          break;
        case pressure_row_14: 
          client.publish("row_14", copy, true);
          break;              
        case weight_meas: 
          client.publish("weight", copy, true);
          break;
        case jump_meas: 
          client.publish("jump", copy, true); 
          break;
        default:
          client.publish("error", copy, true);    
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
  if(topic=="lamp"){
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.println("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.println("Off");
      }
  }
  
  if(topic=="indicate_measurement")
  {
      if(messageTemp == "medir peso")
      {
        serial_print(weight_meas);
      }
      else if(messageTemp == "medir salto")
      {
        serial_print(jump_meas);
      }
      else
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
      client.subscribe("lamp");
      //client.subscribe("room/indicate_measurement");
      client.subscribe("indicate_measurement");
    } else {
      Serial.print(client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
