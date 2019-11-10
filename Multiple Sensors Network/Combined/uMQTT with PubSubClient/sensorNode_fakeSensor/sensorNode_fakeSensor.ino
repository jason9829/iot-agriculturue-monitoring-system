/****************************************************************
 * Author  : Jason Leong Xie Wei
 * Contact : jason9829@live.com
 * Title : Send sensor readings to a centralised node
 * Hardware : NodeMCU ESP8266
 * Library Version:
 *  ArduinoJson : Version 5.13.5
 *  ThingsBoard : Version 0.2.0
 *  PubSubClient : Version 2.7.0
 ****************************************************************/
#include <ArduinoJson.h>
#include <ThingsBoard.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <stdlib.h>
#include <Ticker.h>

// Definition for WiFi
#define WIFI_AP "YOUR_WIFI_SSID_HERE"         // WiFi SSID
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD_HERE"         // WiFi PASSWORD

#define TOKEN  "ADDRESS_TOKEN"                         // Device's Token address created on ThingsBoard
#define PARAMETER_SOIL_MOSITURE  "SoilMoisture"       // Parameter of device's widget on ThingsBoard
#define PARAMETER_TEMPERATURE  "Temperature"  

const char * host = "IP_ADDRESS_SERVER";     // IP Server


// Definition for ADC
#define ADC_10_BIT_RESOLUTION   1024     // resolution of ESP8266 ADC

// Global variable
int status = WL_IDLE_STATUS;

WiFiClient wifiClient;                   // Wifi clients created to connect to internet and 
PubSubClient client(wifiClient);         // ThingsBoard

const char* mqtt_server = "ESP8266_SERVER_IP_ADDRESS";

const int httpPort = 80;                 // client port

char thingsboardServer[] = "YOUR_THINGSBOARD_HOST_OR_IP_HERE";   // ip or host of ThingsBoard 

char rpcCommand[128] ;          // rpc message key-in at ThingsBoard RPC remote shell
String replyFromServer;         // Reply message from server after received data
#define MAX_MQTT_MESSAGE_LENGTH     128   // Size of message to be sent to server node
/*
 * @desc: bypass the json string and get command type in rpc remote shell
 * @param: message from rpc remote shell, bypass message length
 * @retval: pointer to the bypassed input message
 */
char *getRpcCommandInStr(char *str, int bypassLength){
  while(bypassLength != 0){
    str++;
    bypassLength--;
  }
  return str;
}

/* 
 *  @desc: Control sampling rate based on server reply
 *  @param: server replay for rpc
 */
void rpcHandler(){
  char *temp;
  Serial.println("\nrpc-Command from server");

  // If user didn't type anything on rpc remote shell on ThingsBoard, the default message is
  // "No rpc command from ThingsBoard", else the message will be something else such as
  // Received data{"method":"sendCommand","params":{"command":"characters type in remote shell"}}
  if(strstr(replyFromServer.c_str(), "No rpc command from ThingsBoard") == NULL){
    // strcpy(rpcCommand,temp);
    Serial.println(replyFromServer);
    // Example of server reply
    // Received data{"method":"sendCommand","params":{"command":"characters type in remote shell"}}
    // Thus to get the first characters typed, the string need to bypass 58 characters to reach "
    temp = getRpcCommandInStr((char *)(replyFromServer.c_str()), 58); 
    Serial.println(temp);
  }
  else
    Serial.println("Nothing is typed at rpc remote shell of ThingsBoard");
  } 


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  client.publish("toServer", "Received command from server");
      // ... and resubscribe
  client.subscribe("fromServer");

}

/*******************WiFi functions****************/  
/*
 * @desc: Connect device to WiFi
 */  
void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

/*
 * @desc: Connect device to ThingsBoard/ Reconnect to WiFi
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("toServer", "I'm connected");
      // ... and resubscribe
      client.subscribe("fromServer");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// Main functions
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //delay(10);
  InitWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void sendSensorReadingsToServerNode(int soilMoisture,  int temperature){
  char messsage[MAX_MQTT_MESSAGE_LENGTH];
  sprintf(messsage, "Soil Mositure 1: %i, Temperature 1: %i", \
  soilMoisture, temperature);
  client.publish("toServer", messsage);
      // ... and resubscribe
  client.subscribe("fromServer");
  }

void bypassCharactersInStr(char **str, int length){
  while(length != 0){
    ++*str;
    length--;
  }
}

void skipWhiteSpaces(char **str){
    while(**str == ' ')
        ++*str;
}
int getSensorNodeNumber(char *message, char *parameter){
    int sensorNodeNumber;
    int parameterLength = strlen(parameter);
    
    // move to the last character of matched string        
    bypassCharactersInStr(&message, parameterLength-1); // -1 minus the '/0'
    skipWhiteSpaces(&message);
    sensorNodeNumber = (*message) - '0'; // -0 based on ASCII table
    return sensorNodeNumber;
}
void loop() {
  // put your main code here, to run repeatedly:
 if ( !client.connected() ) {
    reconnect();
  }
  delay(5000);
  sendSensorReadingsToServerNode(50,50);
// sendFakeDataToServerNode(50);
  //uploadReadingsToThingsBoard(random(12,25), PARAMETER_SOIL_MOSITURE);
  //sendFakeDataToServerNode(10);
 // uploadReadingsToThingsBoard(random(50,70), PARAMETER_TEMPERATURE);

  client.loop();
}


// References 
// [1.] Arduino | Communication Two LoLin NodeMCU V3 ESP8266 (Client Server) for Controlling LED
//      https://www.youtube.com/watch?v=O-aOnZViBzs&t=317s
// [2.] Temperature upload over MQTT using ESP8266 and DHT22 sensor
//      https://thingsboard.io/docs/samples/esp8266/temperature/
// [3.] Arduino and Soil Moisture Sensor -Interfacing Tutorial
//      http://www.circuitstoday.com/arduino-soil-moisture-sensor