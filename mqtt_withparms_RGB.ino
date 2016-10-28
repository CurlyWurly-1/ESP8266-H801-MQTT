/*
 * This bespoke "hack" code is for the H801 (LIXADA) control module. 
 * The H801 module is based on an ESP8266 and can drive a RGB LED strip of lights. 
 * - This code responds to MQTT messages of the form ffffff to topic ESP_RGB_1 
 * e.g. 00cc00  or ff00cc for RGB control
 * 
 * N.B. You have to solder 6 header pins on the H801 - this is so you can connect a cheap FDTI USB for programming.
 */
#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

void LED_RED();
void LED_GREEN();
void LED_BLUE();
void change_LED();
int convertToInt(char upper,char lower);

#define PWM_VALUE 63
int gamma_table[PWM_VALUE+1] = {
    0, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10,
    11, 12, 13, 15, 17, 19, 21, 23, 26, 29, 32, 36, 40, 44, 49, 55,
    61, 68, 76, 85, 94, 105, 117, 131, 146, 162, 181, 202, 225, 250,
    279, 311, 346, 386, 430, 479, 534, 595, 663, 739, 824, 918, 1023
};


// RGB FET
#define redPIN    15 //12
#define greenPIN  13 //15
#define bluePIN   12 //13

// onboard green LED D1
#define LEDPIN    5
// onboard red LED D2
#define LED2PIN   1

// note 
// TX GPIO2 @Serial1 (Serial ONE)
// RX GPIO3 @Serial    


#define LEDoff digitalWrite(LEDPIN,HIGH)
#define LEDon digitalWrite(LEDPIN,LOW)

#define LED2off digitalWrite(LED2PIN,HIGH)
#define LED2on digitalWrite(LED2PIN,LOW)

int led_delay_red = 0;
int led_delay_green = 0;
int led_delay_blue = 0;
#define time_at_colour 1000 
unsigned long TIME_LED_RED = 0;
unsigned long TIME_LED_GREEN = 0;
unsigned long TIME_LED_BLUE = 0;
int RED, GREEN, BLUE; 
int RED_A = 0;
int GREEN_A = 0; 
int BLUE_A = 0;


//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
char mqtt_server[40] = "10.1.10.22";
char mqtt_port[6] = "1883";
//char blynk_token[33] = "YOUR_BLYNK_TOKEN";
//default custom static IP
char device_topic[60] = "ESP_RGB_1";
char static_ip[16] = "10.1.10.141";
char static_gw[16] = "10.1.10.1";
char static_sn[16] = "255.255.255.0";
char config_file[25] = "/config.json";
boolean clean_reset = false;  // should be false for production use!

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial1.println("Should save config");
  shouldSaveConfig = true;
}


void callback(char* topic, byte* payload, unsigned int length) {
  LEDon;
  Serial1.print("Message arrived [");
  Serial1.print(topic);
  Serial1.print("] ");
  for (int i = 0; i < length; i++) {
    Serial1.print((char)payload[i]);
  }
  Serial1.println();

//       String hexRGB =(char)payload[0,1];
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
 //Serial1.println(r);      Serial1.println(g);     Serial1.println(b);     
        // set value of RGB controller
        int red = map(r,0,255,0,PWM_VALUE); 
        red = constrain(red,0,PWM_VALUE);
        int green = map(g,0,255,0,PWM_VALUE);
        green = constrain(green, 0, PWM_VALUE);
        int blue = map(b,0,255,0,PWM_VALUE);
        blue = constrain(blue,0,PWM_VALUE);
      
        RED = gamma_table[red];
        GREEN = gamma_table[green];
        BLUE = gamma_table[blue];
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
  
    LEDoff;

//        Serial1.println(RED_A);      Serial1.println(GREEN_A);     Serial1.println(BLUE_A);  

  LEDoff;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    LEDon; 
    LED2on;
    Serial1.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial1.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", device_topic);
      // ... and resubscribe
      Serial1.printf("Subscribing to %s\n", device_topic);
      client.subscribe(device_topic);
      LEDoff;
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

void setup() {
  LEDon;
  LED2on;
  // put your setup code here, to run once:
  pinMode(LEDPIN, OUTPUT);  
  pinMode(LED2PIN, OUTPUT);  
  pinMode(redPIN, OUTPUT);
  pinMode(greenPIN, OUTPUT);
  pinMode(bluePIN, OUTPUT);
  
  
  Serial1.begin(115200);
  Serial1.println();

  LEDoff;
  LED2off;

  //clean FS, for testing
  if (clean_reset) {
    Serial1.println("clean_reset is true, formatting file system...");
    SPIFFS.format();
  }
  
  //read configuration from FS json
  Serial1.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial1.println("mounted file system");
    if (SPIFFS.exists(config_file)) {
      //file exists, reading and loading
      Serial1.println("reading config file");
      File configFile = SPIFFS.open(config_file, "r");
      if (configFile) {
        Serial1.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial1.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(device_topic, json["device_topic"]);
//          strcpy(blynk_token, json["blynk_token"]);

          if(json["ip"]) {
            Serial1.println("setting custom ip from config");
            //static_ip = json["ip"];
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            //strcat(static_ip, json["ip"]);
            //static_gw = json["gateway"];
            //static_sn = json["subnet"];
            Serial1.println(static_ip);
/*            Serial1.println("converting ip");
            IPAddress ip = ipFromCharArray(static_ip);
            Serial1.println(ip);*/
          } else {
            Serial1.println("no custom ip in config");
          }
        } else {
          Serial1.println("failed to load json config");
        }
      }
    }
  } else {
    Serial1.println("failed to mount FS");
  }
  //end read
  Serial1.println(static_ip);
//  Serial1.println(blynk_token);
  Serial1.println(mqtt_server);


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_device_topic("topic", "device topic", device_topic, 60);
//  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 34);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_device_topic);
//  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  if (clean_reset) {
    Serial1.println("clean_reset is true, resetting WifiManager...");
    wifiManager.resetSettings();
  }
  
  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  LEDon;
  LED2off;
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial1.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  LEDon;
  LED2on;
  Serial1.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(device_topic, custom_device_topic.getValue());
//  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial1.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["device_topic"] = device_topic;
//    json["blynk_token"] = blynk_token;

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open(config_file, "w");
    if (!configFile) {
      Serial1.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial1.println("local ip");
  Serial1.println(WiFi.localIP());
  Serial1.println(WiFi.gatewayIP());
  Serial1.println(WiFi.subnetMask());
  Serial1.println(device_topic);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}



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

void LED_RED()
{
  if(RED != RED_A){
    if(RED_A > RED) RED_A = RED_A - 1;
    if(RED_A < RED) RED_A++;
    analogWrite(redPIN, RED_A);
  }
}

void LED_GREEN()
{
  if(GREEN != GREEN_A){
    if(GREEN_A > GREEN) GREEN_A = GREEN_A - 1;
    if(GREEN_A < GREEN) GREEN_A++;
    analogWrite(greenPIN, GREEN_A);
  }
}
  
void LED_BLUE()
{
  if(BLUE != BLUE_A){
    if(BLUE_A > BLUE) BLUE_A = BLUE_A - 1;
    if(BLUE_A < BLUE) BLUE_A++;
    analogWrite(bluePIN, BLUE_A);
  }
}

int convertToInt(char upper,char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal >64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal >64 ? lVal - 55 : lVal - 48;
  return uVal + lVal;
}


void loop() {
  // put your main code here, to run repeatedly:


  if (!client.connected()) {
    reconnect();
  }
  client.loop();

//  long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 75, "hello world #%ld", value);
//    Serial1.print("Publish message: ");
//    Serial1.println(msg);
//    client.publish("outTopic", msg);
//  }
}
