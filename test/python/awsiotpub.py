import os
import socket
import ssl
from time import sleep
from random import uniform
import paho.mqtt.client as paho

connected = False

def on_connect(client, userdata, flags, rc):
    global connected
    connected = True
    print("Connection returned result: " + str(rc) )

def on_message(client, userdata, msg):
    print("Topic:{} Data:{}".format(msg.topic, msg.payload)

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

#curl --tlsv1.2 --cert ./cert.pem --key ./privkey.pem --cacert ./aws-iot-rootCA.crt -X GET https://EXAMPLE.iot.us-west-2.amazonaws.com:8443/things/esp-servo/shadow

mqttclient.tls_set(caPath, certfile=certPath, keyfile=keyPath, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2, ciphers=None)

mqttclient.connect(awshost, awsport, keepalive=60)
        
mqttclient.loop_start()

while 1==1:
    if connected == True:
        while True:
            tempreading = uniform(20.0,25.0)
            shadow = '{"state":{"reported": {"tempreading": 42}}}'
            
            mqttclient.publish("$aws/things/esp-servo/shadow/update", shadow, qos=1)
            print "msg sent: {}".format(shadow )
            sleep(4)
    else:
        print "waiting for connection..."
        sleep(1)
