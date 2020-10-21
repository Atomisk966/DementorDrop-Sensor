// ***************************************
// ********** Global Variables ***********
// ***************************************


//Globals for Wifi Setup and OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//WiFi Credentials
#ifndef STASSID
/* My House */
//#define STASSID "ATTAPaQxEa"
//#define STAPSK  "u%5kf7aq%jm="
/* Warp House */
//#define STASSID "DoLGulDur"
//#define STAPSK  "warproom"
/* Woods */
#define STASSID "Pride Rock"
#define STAPSK  "mutualsnOOtbOOps"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

//Globals for MQTT
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#define MQTT_CONN_KEEPALIVE 300
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "atomisk966"
#define AIO_KEY         "e0c3d4198f7c42c6843cc67d87c9629d"

//Initialize and Subscribe to MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish dementor = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/DroppingDementor");

//MP3
#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>
SoftwareSerial mp3Serial(D3, D2); // RX, TX

//Globals for Relays
#define trigPin D7
#define echoPin  D8
float duration, distance;

//Globals for System
int difference = 0;
int greaterDifference = 0;
int lastSensorData = 0;
int reallyLastSensorData = 0;
int lastGreaterDifference = 0;
int warmingUpFlag = 0;


// ***************************************
// *************** Setup *****************
// ***************************************

void setup() {

  //Wifi
  wifiSetup();

  //Initialize Relays
  delay(1000);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //MP3
  Serial.println("Setting up software serial");
  mp3Serial.begin (9600);
  Serial.println("Setting up mp3 player");
  mp3_set_serial (mp3Serial);  
  // Delay is required before accessing player. From my experience it's ~1 sec
  delay(1000); 
  mp3_set_volume (30);
}


// ***************************************
// ************* Da Loop *****************
// ***************************************
void loop() {

  //Network Housekeeping
  ArduinoOTA.handle();
  MQTT_connect();

  //Sensor Checks
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  //Crunch numbers
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * .0343) / 2;
  difference = distance - lastSensorData;
  greaterDifference = reallyLastSensorData - distance;
  reallyLastSensorData = lastSensorData;
  lastSensorData = distance;
  Serial.print(distance);
  Serial.print("   1-Jump: ");
  Serial.print(difference);
  Serial.print("     2-Jump: ");
  Serial.println(greaterDifference);

  if (greaterDifference < -2000) {
    warmingUpFlag++;
    Serial.print("flag = ");
    Serial.println(warmingUpFlag);
  }

  if (warmingUpFlag > 2) {
    if (greaterDifference < -2000) {
      Serial.println("!!!SUCKERS DETECTED!!!");
      delay(350);
      dementor.publish(1);
      mp3_next();
      delay(10000);
      dementor.publish(2);
    }
  }

  lastGreaterDifference = greaterDifference;
  delay(150);
}


// ***************************************
// ********** Backbone Methods ***********
// ***************************************


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    Serial.println("Connected");
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
      Serial.println("Wait 10 min to reconnect");
      delay(600000);
    }
  }
  Serial.println("MQTT Connected!");
}

void wifiSetup() {
  //Initialize Serial
  Serial.begin(115200);
  Serial.println("Booting");

  //Initialize WiFi & OTA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("DroppingDementorEyes");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
