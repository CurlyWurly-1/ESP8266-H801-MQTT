# ESP8266-H801-MQTT
ESP8266 / H801 RGB controller using MQTT and WiFi - With programming via the Arduino IDE 

I have had to re-visit this repository because the wifi manager library seems to have connection problems with the ESP8166EX chip

I have the left the original sketch in, just in case it gets fixed, but in the mean time, I have added the following 2 programs which should be OK
 - mqtt_H801_send_in_network.py 
   - _This is an example python program that you can execute in your desktop to control the LED strip via MQTT_
   - _Please see the program comments on how to use_
   - _Reference info can be found here_
     - https://www.emqx.com/en/blog/how-to-use-mqtt-in-python#full-python-mqtt-code-example
     
   
 - H801_MQTT.ino 
   - _This is the Arduino code that you need to flash into the H801_
   - _Please see the program comments on how to use_
   - _Reference info can be found here_
     - https://www.inspectmygadgets.com/flashing-the-h801-led-controller-with-tasmota-firmware/
     - https://tasmota.github.io/docs/devices/H801/#hardware  

# Old References (no longer needed)
This is based on the following sources

https://eryk.io/2015/10/esp8266-based-wifi-rgb-controller-h801/

https://github.com/tzapu/WiFiManager

https://github.com/knolleary/pubsubclient
