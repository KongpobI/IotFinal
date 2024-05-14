#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include "Adafruit_HTS221.h"
#include "Adafruit_BMP280.h"
#include <queue>
using namespace std;
#define MAX_QUEUE_SIZE 2
#define UPDATEDATA   "@shadow/data/update"
const char* ssid = "Family_2.4GHz";
const char* password = "0123456789";
const char* mqtt_server = "broker.netpie.io";
const char* client_id = "fe3f78ac-9f36-4e07-82bd-f651964928cf";
const char* token     = "URcnMeBAapcJd7dxrLF1MATX1MdkxxER";    
const char* secret    = "teWsmuqojdiDeQioSxzLX6idCDadesmP";  

queue<float> HTStemperatureQueue;
queue<float> HTShumidityQueue;
queue<float> BMPtemperatureQueue;
queue<float> BMPPressureQueue;
Adafruit_HTS221 HTS221 = Adafruit_HTS221(); //************************************************************************
Adafruit_BMP280 BMP280 = Adafruit_BMP280();
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
   // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Wire.begin(41, 40, 100000);
  if (BMP280.begin(0x76)) { 
    Serial.println("BMP280 sensor ready");
  }
  if (HTS221.begin_I2C()) { 
    Serial.println("HTS221 sensor ready");
  }
  client.setServer(mqtt_server, 1883);

  // thread 1
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
    float tempbmp = BMP280.readTemperature();
    float pressbmp = BMP280.readPressure();
    addToQueue(temphts, humidhts,tempbmp,pressbmp);
    printQueue();
    delay(60000);
  }
}

void push_n(void * parameter) {
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4);
  while (1) {
    if (HTStemperatureQueue.size() != 0) {
      while (!client.connected()) {
        Serial.println(F("Attempting MQTT connection..."));
        if (client.connect(client_id, token, secret)) {
          Serial.println(F("connected"));
        } else {
          Serial.println(F("failed, rc="));
          Serial.println(client.state());
          Serial.println(F(" try again in 5 seconds"));
          delay(5000);
        }
      }
      StaticJsonDocument<capacity> doc;
      JsonObject data = doc.createNestedObject("data");
      data["humid"] = HTShumidityQueue.front();
      data["temp"]  = (HTStemperatureQueue.front() + BMPtemperatureQueue.front())/2;
      data["pres"] = BMPPressureQueue.front() ;
      HTStemperatureQueue.pop();
      HTShumidityQueue.pop();
      BMPtemperatureQueue.pop();
      BMPPressureQueue.pop();
      Serial.print(F("Message send: "));
      Serial.println((doc.as<String>()).c_str());
      client.publish(UPDATEDATA, (doc.as<String>()).c_str());
    }
    delay(5000);
  }
}

void addToQueue(float htstemp, float htshum, float bmptemp, float bmppress) {
  HTStemperatureQueue.push(htstemp);
  HTShumidityQueue.push(htshum);
  BMPtemperatureQueue.push(bmptemp);
  BMPPressureQueue.push(bmppress);
}

void printQueue() {
  Serial.print(HTShumidityQueue.size());
  Serial.println(" Queue [HTS_temp, HTS_humid, BMP_temp, BMP_pres]");
  Serial.print("Next Queue: ");
  Serial.print(HTStemperatureQueue.front());
  Serial.print(", ");
  Serial.print(HTShumidityQueue.front());
  Serial.print(", ");
  Serial.print(BMPtemperatureQueue.front());
  Serial.print(", ");
  Serial.println(BMPPressureQueue.front());
}

void loop() {
  
}


