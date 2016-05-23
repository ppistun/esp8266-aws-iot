import sys
import os
import socket
import ssl
import paho.mqtt.client as paho

def on_connect(client, userdata, flags, rc):
    print "Connection returned result: {}".format(rc)
	
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("#" , 1 )

def on_message(client, userdata, msg):
    print "->topic:", msg.topic, "->payload:{}".format(msg.payload)

mqttclient = paho.Client()
mqttclient.on_connect = on_connect
mqttclient.on_message = on_message

awshost = "EXAMPLE.iot.us-west-2.amazonaws.com"
awsport = 8883
clientId = "esp-servo"
thingName = "esp-servo"
caPath = "aws-iot-rootCA.crt"
certPath = "cert.pem"
keyPath = "privkey.pem"

mqttclient.tls_set(caPath, certfile=certPath, keyfile=keyPath, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2, ciphers=None)

mqttclient.connect(awshost, awsport, keepalive=60)

mqttclient.loop_forever()
