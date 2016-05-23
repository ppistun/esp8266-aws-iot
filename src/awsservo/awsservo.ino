#include <Arduino.h>
#include <Stream.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//AWS
#include "src/common/sha256.h"
#include "src/common/Utils.h"
#include "src/common/AWSClient2.h"

//WEBSockets
#include <Hash.h>
#include <WebSocketsClient.h>

//MQTT PAHO
#include <SPI.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"

//servo
#include <Servo.h> 

#include "awsservoconfig.h"

int port = 443;

//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

//GPIO
#define GPIO_LED 5

ESP8266WiFiMulti WiFiMulti;

AWSWebSocketClient awsWSclient(1000);

IPStack ipstack(awsWSclient);
MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers> *client = NULL;

Servo servo;

//# of connections
long connection = 0;

//generate random mqtt clientID
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1)
    cID[i]=(char)random(1, 256);
  return cID;
}

//count messages arrived
int arrivedcount = 0;

//callback to handle mqtt messages
void messageArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;

  //draft test
  if (arrivedcount%2 == 0)
  {
    digitalWrite(GPIO_LED, HIGH);
    servo.write(5);
  }
  else
  {
    digitalWrite(GPIO_LED, LOW);
    servo.write(50);
  }

  Serial.print("Message ");
  Serial.print(++arrivedcount);
  Serial.print(" arrived: qos ");
  Serial.print(message.qos);
  Serial.print(", retained ");
  Serial.print(message.retained);
  Serial.print(", dup ");
  Serial.print(message.dup);
  Serial.print(", packetid ");
  Serial.println(message.id);
  Serial.print("Payload ");
  char* msg = new char[message.payloadlen+1]();
  memcpy (msg,message.payload,message.payloadlen);
  Serial.println(msg);
  delete msg;
}

//connects to websocket layer and mqtt layer
bool connect () {

    if (client == NULL) {
      client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
    } else {

      if (client->isConnected ()) {    
        client->disconnect ();
      }  
      delete client;
      client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
    }


    //delay is not necessary... it just help us to get a "trustful" heap space value
    delay (1000);
    Serial.print (millis ());
    Serial.print (" - conn: ");
    Serial.print (++connection);
    Serial.print (" - (");
    Serial.print (ESP.getFreeHeap ());
    Serial.println (")");




   int rc = ipstack.connect(aws_endpoint, port);
    if (rc != 1)
    {
      Serial.println("error connection to the websocket server");
      return false;
    } else {
      Serial.println("websocket layer connected");
    }


    Serial.println("MQTT connecting");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    char* clientID = generateClientID ();
    data.clientID.cstring = clientID;
    rc = client->connect(data);
    delete[] clientID;
    if (rc != 0)
    {
      Serial.print("error connection to MQTT server");
      Serial.println(rc);
      return false;
    }
    Serial.println("MQTT connected");
    return true;
}

//subscribe to a mqtt topic
void subscribe () {
    //subscript to a topic
    int rc = client->subscribe(aws_topic, MQTT::QOS0, messageArrived);
    if (rc != 0) {
      Serial.print("ERROR: rc from MQTT subscribe is ");
      Serial.println(rc);
      return;
    }
    Serial.println("MQTT subscribed");
}

unsigned long previousMillis = 0;
const long interval = 3000;
//send a message to a mqtt topic
void sendmessage() {
    //send a message
    MQTT::Message message;
    char buf[100];
    auto vcc = ESP.getVcc();
//    Serial.printf("VCC: %d\r\n", vcc);
//    Serial.printf("A0: %d\r\n", analogRead(A0));

//    strcpy(buf, "{\"state\":{\"reported\":{\"on\": false}, \"desired\":{\"on\": false}}}");
    sprintf(buf, "{\"state\":{\"reported\":{\"on\": false}, \"ALS\":%d}}", analogRead(A0));
    Serial.printf("SEND: %s", buf);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client->publish(aws_topic, message); 
}



void setup() {
    Serial.begin (115200);
    delay (2000);
    Serial.setDebugOutput(1);

    Serial.println ("Started. v0.1");

    pinMode(GPIO_LED, OUTPUT);
    
    servo.attach(14); 

    pinMode(A0, INPUT);

    //fill with ssid and wifi password
    WiFiMulti.addAP(wifi_ssid, wifi_password);
    Serial.println ("connecting to wifi");
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print (".");
    }
    Serial.println ("\nconnected");

    //fill AWS parameters    
    awsWSclient.setAWSRegion(aws_region);
    awsWSclient.setAWSDomain(aws_endpoint);
    awsWSclient.setAWSKeyID(aws_key);
    awsWSclient.setAWSSecretKey(aws_secret);
    awsWSclient.setUseSSL(true);

    if (connect ()){
      subscribe ();
      sendmessage ();
    }

}

void loop() {

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;   
    sendmessage();
  }


  //keep the mqtt up and running
  if (awsWSclient.connected ()) {    
      client->yield();
  } else {
    //handle reconnection
    if (connect ()){
      subscribe ();      
    }
  }

}
