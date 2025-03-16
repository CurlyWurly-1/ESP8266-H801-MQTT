## https://www.emqx.com/en/blog/how-to-use-mqtt-in-python#full-python-mqtt-code-example

#######################################
# H801 actions 
#######################################
#  Follow instructions in the Arduino "partner" program called "H801.ino" 
#   - Remove all cables and power from H801
#   - Open up case 
#   - Solder header pins
#   - Attach 3 Male to male dupont cables from H801 to TTL serial FTDI adaptor
#   - Attach a single male to male dupont cable across the 2 pins that set the H801 in programming mode
#   - Insert USB cable from your computer to TTL serial FTDI adaptor
#   - Execute Arduino IDE and following uistructinos   
#  Setup the H801 
#   - To log into your broadband network
#   - To log into your MQTT broker
#   - To subscribe to topic 'ESP_RGB_1' 
#  Put power into H801

#######################################
# What to do with this program
#######################################
#  Amend this python program
#   - Enter details to log into your MQTT broker  
#  Execute this python code 
#   - Make sure you execute this program in a computer that is signed into your broadband network 


import random
import time

from paho.mqtt import client as mqtt_client


mqtt_broker   = '192.168.1.170'
mqtt_port     = 1883
mqtt_username = 'mqtt_user'
mqtt_password = 'D1strict!1'

# N.B. This needs to be the same value that is also programmed in your H801
mqtt_topic    = 'ESP_RGB_1'

# Generate a Client ID with the publish prefix.
client_id = f'publish-{random.randint(0, 1000)}'

def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

#    client = mqtt_client.Client(client_id)
    client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION1, client_id)
    client.username_pw_set(mqtt_username, mqtt_password)
    client.on_connect = on_connect
    client.connect(mqtt_broker, mqtt_port)
    return client


def publish(client):
    msg_count = 1
    while True:
        time.sleep(0.5)
#        msg = f"messages: {msg_count}"
# Red
#        msg = 'ff0000'
# Green
#        msg = '00ff00'
# Blue
#        msg = '0000ff'
# All lights off
        msg = '000000'

        result = client.publish(topic, msg)
        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{msg}` to topic `{topic}`")
        else:
            print(f"Failed to send message to topic {topic}")
        msg_count += 1
        if msg_count > 5:
            break


def run():
    client = connect_mqtt()
    client.loop_start()
    publish(client)
    client.loop_stop()


if __name__ == '__main__':
    run()