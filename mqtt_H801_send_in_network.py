## https://www.emqx.com/en/blog/how-to-use-mqtt-in-python#full-python-mqtt-code-example


#######################################
# What to do with this program
#######################################
#  - Alter three parameters to suit - mqtt_broker, mqtt_username and mqtt_password  
#  - Execute this program in a computer that is signed into your broadband network 

#######################################
# H801 actions 
#######################################
#  Follow instructions in the Arduino "partner" program called "H801_mqtt.ino" 



import random
import time

from paho.mqtt import client as mqtt_client

mqtt_broker   = 'PUT YOUR MQTT IP ADDRESS HERE e.g. 192.168.1.123'
mqtt_port     = 1883
mqtt_username = 'PUT YOUR MQTT USER ID HERE'
mqtt_password = 'PUT YOUR MQTT PASSWORD HERE'


# N.B. This needs to be the same value that is also programmed in your H801
mqtt_topic    = 'ESP_RGB_1'

# Generate a Client ID with the publish prefix.
client_id = f'publish-{random.randint(0, 1000)}'

###########################################################
# connect_mqtt()
###########################################################
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

###########################################################
# publish()
###########################################################
def publish(client):
    msg_count = 1
    while True:
        time.sleep(1)
        msg = f"messages: {msg_count}"

        if msg_count   == 1:
            msg = 'ff0000';     # Red
        elif msg_count == 2:
            msg = '00ff00';     # Green
        elif msg_count == 3:
            msg = '0000ff';     # Blue
        elif msg_count == 4:
            msg = '00ffff';     # mix 1
        elif msg_count == 5:
            msg = 'ffff00';     # mix 2
        elif msg_count == 6:
            msg = 'ff00ff';     # mix 3
        elif msg_count == 7:
            msg = 'ffffff';     # White
        elif msg_count == 8:
            msg = '000000'     # All lights off

        result = client.publish(mqtt_topic, msg)
        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{msg}` to topic `{mqtt_topic}`")
        else:
            print(f"Failed to send message to topic {mqtt_topic}")
        msg_count += 1
        if msg_count > 8:
            msg_count = 1

###########################################################
# main()
###########################################################
def main():
    client = connect_mqtt()
    client.loop_start()
    publish(client)
    client.loop_stop()

###########################################################
# P R O G R A M   S T A R T S   H E R E publish()
###########################################################
if __name__ == '__main__':
    main()