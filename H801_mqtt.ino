/*
 This bespoke "hack" code is for the H801 (LIXADA) control module. 
 The H801 module is based on an ESP8266EX and can drive a RGB LED strip of lights.  Select "Generic ESP8266 Module" in "Tools -> Board"

 Some background info can be found here
  https://tasmota.github.io/docs/devices/H801/#hardware
  https://www.inspectmygadgets.com/flashing-the-h801-led-controller-with-tasmota-firmware/

#######################################
# H801 actions 
#######################################
# Physical actions
#   - Remove all cables and power from H801
#   - Open up H801 case to access H801 PCB 
#   - Solder 6 header pins into H801 PCB
#   - Attach 3 female to female dupont cables to connect the H801 PCB to the TTL serial FTDI adaptor (as per advice from above links)
#   - Attach a single female to female dupont cable to short the 2 pins on the H801 PCB (that set the H801 PCB into programming mode on its power up)
#   - Attach a USB cable between your computer to the TTL serial FTDI adaptor
#   - Attach a female barrel power connector socket to the H801 PCB by hooking up the +ve and -ve power leads accordingly. You can then power the H801 using a standard 5V-2V brick power supply and it makes it easier to remove and attach power whenever you need to 
#   
# Software actions
#   - Execute Arduino IDE in your desktop
#   - add "https://arduino.esp8266.com/stable/package_esp8266com_index.json" to "File->Preferences->field "Additinnal Board Manager URLs"
#   - Load up this Arduino sketch into the Arduino IDE
#   - Setup Arduino IDE to use a board called "generic ESP8266 module"   
#   - Setup Arduino IDE to use the COM Port that points to the FDTI adapter
#   - Setup Arduino IDE to use a serial speed of 115200 
#   - Alter the following 5 parameters to suit i.e. ssid, password, mqtt_server, mqtt_user, mqtt_pass 
#   - If you want to control multiple H801 devices, ensure each H801 has a separate "mqtt_topic" e.g. Change the software for the 2nd H801 to be "ESP_RGB_2" etc
#   - Initiate Programming into the H801 by following the sequence below
#          - Remove FDTI adapter USB cable from Computer
#          - Remove power plug from female barrel socket
#          - Short the programming pins with a single dupont female to female cable          
#          - Insert power plug into female barrel socket (to power the H801 PCB) 
#          - Insert FDTI adapter USB cable into Computer
#          - Press the "Upload" button in your Arduino IDE
#          - N.B. It may be that you see dots and dashes and after a while the upload crashes. This part of the process is signifying that the arduino IDE is trying to connect to the H801 and it times out before success. 
#                 If this is happening to you, just try the upload again - But when you see the dots and dashes, unplug and replug the barrel power plug.
#                 This unplug and replug power action will kick the H801 to accept a connection for programming, and the process should then continue to completion. 
#                 If this still doesn't work, check you really did short the programming pins together for the H801 to accept reprogramming
#          - Check that uploading has finished OK
#          - Remove the single dupont female to female cable, its the one that shorts the programming pins together 
#          - unplug power from female barrel socket
#          - unplug DTI adapter USB cable from Computer
#          - Wait 2 seconds 
#          - Plug DTI adapter USB cable into Computer
#          - Plug power plug into female barrel socket
#          - View messages in the Arduinio IDE serial monitor (on your desktop)
#          - If all is OK, the red and green LEDS will both be lit, and the device will be ready to accept RGB MQTT messages 

 N.B. Remember you have to solder 6 header pins on the H801 PCB - this is so you can connect a cheap FDTI USB adapter to enable programming.
       Only 5 pins are actually used though 
       - 2 pins are for putting the H801 into programming mode (You connect them together with a female to female dupont cable)
       - 3 pins are for the Ground/TX/RX to a TTL/USB convertor 

#######################################
# Python program actions
#######################################
# This is a separate partner program called "mqtt_H801_send_in_network.py". Please go to this program for actions


############################################################################
#   H O W   T H I S  P R O G R A M   W O R K S 
#############################################################################
 When executing, this code 
  - Logs into the SSID network and if successful, the red   LED is lit
  - Logs into the MQTT broker  and if successful, the green LED is lit
  - Publishes a welcome message to the broker  
  - Subscribes to topic ESP_RGB_1. 
  - When a MQTT message is received For topic "ESP_RGB_1", this code retrieves the first 6 characters from the payload and uses it to formulate the RGB data 
  - Lights are changfed to suit
 It will reconnect to the server if the connection is lost using a blocking reconnect function. 
*/



// Update these with values suitable for your network.


const char* ssid        = "PUT YOUR BROADBAND SSID HERE";
const char* password    = "PUT YOUR BROADBAND PASSWORD HERE";
const char* mqtt_server = "PUT YOUR MQTT IP ADDRESS HERE e.g. 192.168.1.123";
const char* mqtt_user   = "PUT YOUR MQTT USER ID HERE";
const char* mqtt_pass   = "PUT YOUR MQTT PASSWORD HERE";


// Do not change mqtt_topic from "ESP_RGB_1" unless you want to control multiple H801 devices 
const char* mqtt_topic  = "ESP_RGB_1";

#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define PWM_VALUE 63
int gamma_table[PWM_VALUE+1] = {
    0, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10,
    11, 12, 13, 15, 17, 19, 21, 23, 26, 29, 32, 36, 40, 44, 49, 55,
    61, 68, 76, 85, 94, 105, 117, 131, 146, 162, 181, 202, 225, 250,
    279, 311, 346, 386, 430, 479, 534, 595, 663, 739, 824, 918, 1023
};

#define redPIN    15 // 12
#define greenPIN  13 // 15
#define bluePIN   12 // 13
#define LED1PIN    1 // onboard red   LED D1
#define LED2PIN    5 // onboard green LED D2

#define LED1off digitalWrite(LED1PIN,LOW)
#define LED1on  digitalWrite(LED1PIN,HIGH)
#define LED2off digitalWrite(LED2PIN,HIGH)
#define LED2on  digitalWrite(LED2PIN,LOW)

#define time_at_colour 1000 

int led_delay_red   = 0;
int led_delay_green = 0;
int led_delay_blue  = 0;

unsigned long TIME_LED_RED   = 0;
unsigned long TIME_LED_GREEN = 0;
unsigned long TIME_LED_BLUE  = 0;
int RED, GREEN, BLUE; 
int RED_A   = 0;
int GREEN_A = 0; 
int BLUE_A  = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
//#define LED_PIN 1
char msg[MSG_BUFFER_SIZE];
int value = 0;

//**************************************************************
// void setup_wifi()
//**************************************************************
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial1.println();
  Serial1.print("Connecting to ");
  Serial1.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial1.print(".");
  }

  randomSeed(micros());

  Serial1.println("");
  Serial1.print("WiFi connected - ");
  Serial1.print("IP address: ");
  Serial1.println(WiFi.localIP());
}

//**************************************************************
// void callback()
//**************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
  Serial1.print("Message arrived [");
  Serial1.print(topic);
  Serial1.print("] ");
  for (int i = 0; i < length; i++) {
    Serial1.print((char)payload[i]);
  }
  Serial1.println();

  String hexRGB = String((char*)payload);      
  // convert HEX to RGB
  Serial1.println();
  Serial1.print(hexRGB);
  Serial1.println();
        hexRGB.toUpperCase();
        char c[6];
        hexRGB.toCharArray(c,7);
        long r = convertToInt(c[0],c[1]); //red
        long g = convertToInt(c[2],c[3]); //green
        long b = convertToInt(c[4],c[5]); //blue 
//        long s = convertToInt(c[6],c[7]); //speed 
 //Serial1.println(r);      Serial1.println(g);     Serial1.println(b);     
        // set value of RGB controller
        int red = map(r,0,255,0,PWM_VALUE); 
        red = constrain(red,0,PWM_VALUE);
        int green = map(g,0,255,0,PWM_VALUE);
        green = constrain(green, 0, PWM_VALUE);
        int blue = map(b,0,255,0,PWM_VALUE);
        blue = constrain(blue,0,PWM_VALUE);
      
        RED   = gamma_table[red];
        GREEN = gamma_table[green];
        BLUE  = gamma_table[blue];
 //       Serial1.println(RED);      Serial1.println(GREEN);     Serial1.println(BLUE);    
        change_LED();

 while ((RED != RED_A) or (GREEN != GREEN_A) or (BLUE != BLUE_A)) {
  if(millis() - TIME_LED_RED >= led_delay_red){
    TIME_LED_RED = millis();
    LED_RED();
  };
//  }else{
  
  if(millis() - TIME_LED_GREEN >= led_delay_green){
    TIME_LED_GREEN = millis();
    LED_GREEN();
  };
//  }else{
  
  if(millis() - TIME_LED_BLUE >= led_delay_blue){
    TIME_LED_BLUE = millis();
    LED_BLUE();
  };
//  }}};
//  delayMicroseconds(200);

  };
  
//    LED1off;

}

//**************************************************************
// void reconnect()
//**************************************************************
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    LED1on; 
    LED2on;
    Serial1.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial1.println("connected");
      // Once connected, publish an announcement...
      client.publish("H801", "hello world");
      // ... and resubscribe
      client.subscribe(mqtt_topic);
      LED1off;
      LED2on;
    } else {
      Serial1.print("failed, rc=");
      Serial1.print(client.state());
      Serial1.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//**************************************************************
// void change_LED(speed=1)
//**************************************************************
void change_LED()
{
  int diff_red = abs(RED-RED_A);
  if(diff_red > 0){
    led_delay_red = time_at_colour / abs(RED-RED_A); 
  }else{
    led_delay_red = time_at_colour / 1023; 
  }
  
  int diff_green = abs(GREEN-GREEN_A);
  if(diff_green > 0){
    led_delay_green = time_at_colour / abs(GREEN-GREEN_A);
  }else{
    led_delay_green = time_at_colour / 1023; 
  }
  
  int diff_blue = abs(BLUE-BLUE_A);
  if(diff_blue > 0){
    led_delay_blue = time_at_colour / abs(BLUE-BLUE_A); 
  }else{
    led_delay_blue = time_at_colour / 1023; 
  }
  
}

//**************************************************************
// void LED_RED()
//**************************************************************
void LED_RED()
{
  if(RED != RED_A){
    if(RED_A > RED) RED_A = RED_A - 1;
    if(RED_A < RED) RED_A++;
    analogWrite(redPIN, RED_A);
  }
}

//**************************************************************
// void LED_GREEN
//**************************************************************
void LED_GREEN()
{
  if(GREEN != GREEN_A){
    if(GREEN_A > GREEN) GREEN_A = GREEN_A - 1;
    if(GREEN_A < GREEN) GREEN_A++;
    analogWrite(greenPIN, GREEN_A);
  }
}

//**************************************************************
// void LED_BLUE()
//**************************************************************  
void LED_BLUE()
{
  if(BLUE != BLUE_A){
    if(BLUE_A > BLUE) BLUE_A = BLUE_A - 1;
    if(BLUE_A < BLUE) BLUE_A++;
    analogWrite(bluePIN, BLUE_A);
  }
}

//**************************************************************
// void convertToInt()
//**************************************************************
int convertToInt(char upper,char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal >64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal >64 ? lVal - 55 : lVal - 48;
  return uVal + lVal;
}

//**************************************************************
// void setup()
//**************************************************************
void setup() {

  pinMode(LED1PIN, OUTPUT);  
  pinMode(LED2PIN, OUTPUT);  
  pinMode(redPIN, OUTPUT);
  pinMode(greenPIN, OUTPUT);
  pinMode(bluePIN, OUTPUT);

  digitalWrite(redPIN,LOW);
  digitalWrite(greenPIN,LOW);
  digitalWrite(bluePIN,LOW);

  LED1off;
  LED2off;

  Serial1.begin(115200);
 
  setup_wifi();
  LED1on;
  LED2off;

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //if you get here you have connected to the WiFi and MQTT
  LED1on;
  LED2on;

}

//**************************************************************
// void loop()
//**************************************************************
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
