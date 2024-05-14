#include <Adafruit_BMP280.h>
#include <Arduino.h>
#include <Wire.h>
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include "Adafruit_HTS221.h"
#include "Adafruit_SHT4x.h"
#include <ArduinoMqttClient.h>
#include <WiFiMulti.h>
#include <queue>
using namespace std;
WiFiMulti wifiMulti;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
#define MAX_QUEUE_SIZE 2
#define WIFI_SSID "sompon_2.4G"
#define WIFI_PASSWORD "0815837095"

const char* topic_publish = "/hello/world";
char mqtt_user[] = "iotgroup3";
char mqtt_pass[] = "iotgroup3";

queue<float> HTStemperatureQueue;
queue<float> HTShumidityQueue;
queue<float> BMPPressureQueue;
Adafruit_HTS221 HTS221 = Adafruit_HTS221();
Adafruit_BMP280 BMP280 = Adafruit_BMP280();
Adafruit_SHT4x SHT4x = Adafruit_SHT4x();
const char broker[] = "192.168.1.144"; //IP address of the EMQX broker.
int        port     = 1883;
const char subscribe_topic[]  = "/hello";
const char publish_topic[]  = "/hello/world";

void setup() {
  Serial.begin(115200);
  
  // Setup WiFi
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  

  Serial.print("Connecting to WiFi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Wire.begin(41, 40, 100000);
  if (BMP280.begin(0x76)) { 
    Serial.println("BMP280 sensor ready");
  }
  if (HTS221.begin_I2C()) { 
    Serial.println("HTS221 sensor ready");
  }
  if (SHT4x.begin()) { 
    Serial.println("SHT4x sensor ready");
  }
  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);

  Serial.print("Attempting to connect to the MQTT broker.");

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");

  Serial.print("Subscribing to topic: ");
  Serial.println(subscribe_topic);

  // subscribe to a topic
  mqttClient.subscribe(subscribe_topic);

  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);

  Serial.print("Waiting for messages on topic: ");
  Serial.println(subscribe_topic);
  xTaskCreate(read_s, "read", 10000, NULL, 1, NULL);
  //thread 2
  xTaskCreate(push_n, "push", 10000, NULL, 1, NULL);
}
void read_s(void * parameter) {
  sensors_event_t temp, humid;
  while (1) {
    HTS221.getEvent(&humid, &temp); //use hts instend in new version
    float temphts = temp.temperature;
    float humidhts = humid.relative_humidity;
    float pressbmp = BMP280.readPressure();
    addToQueue(temphts, humidhts,pressbmp);
    printQueue();
    delay(10000);
  }
}

void addToQueue(float htstemp, float htshum, float bmppress) {
  HTStemperatureQueue.push(htstemp);
  HTShumidityQueue.push(htshum);
  BMPPressureQueue.push(bmppress);
}

void printQueue() {
  Serial.print(HTShumidityQueue.size());
  Serial.println(" Queue [HTS_temp, HTS_humid, BMP_pres]");
  Serial.print("Next Queue: ");
  Serial.print(HTStemperatureQueue.front());
  Serial.print(", ");
  Serial.print(HTShumidityQueue.front());
  Serial.print(", ");
  Serial.println(BMPPressureQueue.front());
}

void push_n(void * parameter) {
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4);
  while (1) {
    if (HTStemperatureQueue.size() != 0) {
      StaticJsonDocument<capacity> doc;
      JsonObject data = doc.createNestedObject("data");
      data["humid"] = HTShumidityQueue.front();
      data["temp"]  = HTStemperatureQueue.front();
      data["pres"] = BMPPressureQueue.front() ;
      HTStemperatureQueue.pop();
      HTShumidityQueue.pop();
      BMPPressureQueue.pop();
      Serial.print(F("Message send: "));
      Serial.println((doc.as<String>()).c_str());
      mqttClient.beginMessage(publish_topic);
      mqttClient.print((doc.as<String>()).c_str());
      mqttClient.endMessage();
    }
    delay(5000);
  }
}

void loop() {
  float tempsht, humidsht, tempbmp, pressbmp; 
  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    while (mqttClient.available()) {
      Serial.print((char)mqttClient.read());
    }
    Serial.println();
    
  // send message, the Print interface can be used to set the message contents
  delay(3000);


  }
}

