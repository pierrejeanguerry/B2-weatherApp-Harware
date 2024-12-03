#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define SECRET_SSID "******"
#define SECRET_PASS "******"

// pins:
// capteur coté esp // capteur coté capteur
// rouge: V3        // rouge: VCC
//bleu: GND         // bleu: GND
// vert: D5         // jaune: SCL
// jaune: D18       // gris: SDA
// orange: D19      // vert: CSB
// gris: D23        // orange: SDO

// led coté esp     // coté led:
// bleu: GND        // bleu: GND
// jaune: D26       // jaune:B
// vert: D25        // vert: G
// orange: D33      // violet: R

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

const int red = 33;
const int green = 25;
const int blue = 26;

Adafruit_BME280 bme(SS, MOSI, MISO, SCK);

char serverAddress[] = "51.159.188.157";
int port = 8000;


WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  digitalWrite(blue, LOW);

  unsigned status_bme;
    
  status_bme = bme.begin(); 
  while (!status_bme) { //blinking in red if captor error
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
    digitalWrite(blue, LOW);
    delay(1000);
    digitalWrite(red, LOW);
    digitalWrite(green, LOW);
    digitalWrite(blue, LOW);
    delay(1000);
    status_bme = bme.begin(); 
  }

  while ( WiFi.status() != WL_CONNECTED ) {
    digitalWrite(red, HIGH); //blinking in yellow is Tring to connect to wifi
    digitalWrite(green, HIGH);
    digitalWrite(blue, LOW);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    delay(2000);
    digitalWrite(red, LOW);
    digitalWrite(green, LOW);
    digitalWrite(blue, LOW);
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED ){
    digitalWrite(red, LOW);
    digitalWrite(green, HIGH);
    digitalWrite(blue, LOW);
  }

}

void loop() {
  unsigned status_bme;
  while (!status_bme) { //blinking in red if captor error
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
    digitalWrite(blue, LOW);
    delay(1000);
    digitalWrite(red, LOW);
    digitalWrite(green, LOW);
    digitalWrite(blue, LOW);
    delay(1000);
    status_bme = bme.begin(); 
  }
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  digitalWrite(blue, LOW);
  String contentType = "application/ld+json";
  DynamicJsonDocument doc(1024);
  String json;
  float temp = bme.readTemperature();
  int alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float pre = bme.readPressure() / 100.0F;
  float hum = bme.readHumidity();
  String mac = WiFi.macAddress();

  doc["temperature"] = String(temp, 1);
  doc["altitude"] = String(alt);
  doc["pressure"] = String(pre, 2);
  doc["humidity"] = String(hum, 1);
  doc["mac_address"] = mac;

  serializeJson(doc, json);
  Serial.println(json);
  client.post("/api/readings", contentType, json);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  
}
