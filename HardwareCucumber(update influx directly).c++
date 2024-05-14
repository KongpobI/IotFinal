#include <Arduino.h>
#include "Adafruit_HTS221.h"
#include "Adafruit_BMP280.h"
#include "Adafruit_SHT4x.h"
#include <WiFi.h>
#include <WiFiMulti.h>
static uint8_t humidsht = 0;
static uint64_t pressbmp = 0;
static uint8_t tempsht = 0;
static uint8_t tempbmp = 0;
static float influxdb_send_timestamp = 0u;
WiFiMulti wifiMulti;
#include <WiFiClient.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
Adafruit_HTS221 HTS221 = Adafruit_HTS221();
Adafruit_BMP280 BMP280 = Adafruit_BMP280();
Adafruit_SHT4x SHT4x = Adafruit_SHT4x();
// WiFi credentials
#define WIFI_SSID "@HACHI n53912"
#define WIFI_PASSWORD "n539123476"
#define INFLUXDB_SEND_TIME              (10000u)
#define INFLUXDB_URL "https://iot-group3-service1.iotcloudserve.net"
#define INFLUXDB_TOKEN "k8NAhIPxQhDJs_UdDwwAHiQg0t_MoXsEzQ3u2rk-0Try_FOXG9Q8bM56nahw6YgTKM_IvGEpp3qFR8ZvvpX6kg=="
#define INFLUXDB_ORG "f0f016df6c65b7a4"
#define INFLUXDB_BUCKET "iotgroup3"
#define DEVICE                          "ESP32"
// Declare WiFiMulti instance globally
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point sensor("Sensor_Data");
#define TZ_INFO "EST-14"
void setup() {
  Serial.begin(115200);
  
  // Setup WiFi
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nWiFi connected.");
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
  client.setInsecure();
  InfluxDB_TaskInit();
}
void loop() {
  Sensor_TaskMng();
  InfluxDB_TaskMng();
  delay(10000);  // Example delay
}

static void Sensor_TaskMng( void ) {
  float stempsht, shumidsht, stempbmp, spressbmp;
  sensors_event_t temp, humid;
  SHT4x.getEvent(&humid, &temp);
  stempsht = temp.temperature;
  shumidsht = humid.relative_humidity;
  stempbmp = BMP280.readTemperature();
  spressbmp = BMP280.readPressure();
  tempsht =(float_t)stempsht;
  humidsht =(float_t)shumidsht;
  tempbmp =(float_t)stempbmp;
  pressbmp =(float_t)spressbmp;
}
static void InfluxDB_TaskInit( void ) {
  // Add constant tags - only once
  sensor.addTag("device", DEVICE);

  // Accurate time is necessary for certificate validation & writing in batches
  // For the fastest time sync find NTP servers in your area: 
  // https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) 
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
static void InfluxDB_TaskMng( void ) {
  // Store measured value into point
  sensor.clearFields();
  // Report RSSI of currently connected network
  sensor.addField( "rssi", WiFi.RSSI() );
  // add temperature and humidity values also
  sensor.addField( "temperature", tempsht );
  sensor.addField( "humidity", humidsht );
  sensor.addField( "pressure", pressbmp );
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    
  }
}