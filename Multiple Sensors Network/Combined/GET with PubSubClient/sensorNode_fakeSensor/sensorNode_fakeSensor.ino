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
ThingsBoard tb(wifiClient);

const int httpPort = 80;                 // client port

char thingsboardServer[] = "YOUR_THINGSBOARD_HOST_OR_IP_HERE";   // ip or host of ThingsBoard 

char rpcCommand[128] ;          // rpc message key-in at ThingsBoard RPC remote shell
String replyFromServer;         // Reply message from server after received data

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


/*
 * @desc: send fake data to server node
 * @param: soil sensor reading
 */
void sendFakeDataToServerNode(int data){
  Serial.println("\nSending data to server.");

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  if (!client.connect(host, httpPort)) {    // Connect to server (return 1 if connected)
    Serial.println("Connection failed");
    return;
  }
  // We now create a URI for the request  
  Serial.print("Sending: ");
  Serial.print(data);
  Serial.print("\n");

  // This will send the request to the server
  client.print(String("GET ") + data + "\r\n"); // send request(data) to server node
                                                        // it's important to sent "\r\n" else 
                                                        // server reply wont received.
  Serial.print("Server IP: ");
  Serial.print(client.remoteIP());          // Display connected server's IP
  Serial.print('\n');
  unsigned long timeout = millis();
  while (client.available() == 0) {         // wait until server is available
    if (millis() - timeout > 5000) {      
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  Serial.print("Server Reply = "); 
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    replyFromServer = client.readStringUntil('\r');
    Serial.println(replyFromServer);
  }
}

/*
 * @desc: Upload sensor reading to ThingsBoard and display on widget
 * @param: sensor reading, parameter created on ThingsBoard
 */
void uploadReadingsToThingsBoard(int sensorData, char *deviceParameter){
  Serial.println("Sending data to ThingsBoard:");
  Serial.print(deviceParameter);
  Serial.print(sensorData);
  Serial.print(" %\t");

  tb.sendTelemetryFloat(deviceParameter, sensorData);   // Upload data to ThingsBoard
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
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect("ESP8266 Device", TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}


// Main functions
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //delay(10);
  InitWiFi();
  client.setServer( thingsboardServer, 1883 );
}

void loop() {
  // put your main code here, to run repeatedly:
 if ( !tb.connected() ) {
    reconnect();
  }
  sendFakeDataToServerNode(50);
  uploadReadingsToThingsBoard(random(12,25), PARAMETER_SOIL_MOSITURE);
  sendFakeDataToServerNode(10);
  uploadReadingsToThingsBoard(random(50,70), PARAMETER_TEMPERATURE);

  tb.loop();
}


// References 
// [1.] Arduino | Communication Two LoLin NodeMCU V3 ESP8266 (Client Server) for Controlling LED
//      https://www.youtube.com/watch?v=O-aOnZViBzs&t=317s
// [2.] Temperature upload over MQTT using ESP8266 and DHT22 sensor
//      https://thingsboard.io/docs/samples/esp8266/temperature/
// [3.] Arduino and Soil Moisture Sensor -Interfacing Tutorial
//      http://www.circuitstoday.com/arduino-soil-moisture-sensor