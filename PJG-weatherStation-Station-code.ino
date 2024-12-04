#include <WiFi.h>
#include <WiFiManager.h>
#include <Adafruit_BME280.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include "arduino_secret.h"

#define RED 33
#define GREEN 25
#define BLUE 26

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme(SS, MOSI, MISO, SCK);
char serverName[] = URL;
char stationName[] = NAME;

WiFiManager wifiManager;
Ticker wifiCheckTicker;
Ticker ledTicker;
Ticker wifiLost;

bool ledState = false;

float temp;
int alt;
float pre;
float hum;
String mac;

void initLed() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

void initPortal() {
  ledTicker.attach(1.0, blinkYellow);
  wifiCheckTicker.attach(1.0, checkWiFi);
  wifiManager.setConnectTimeout(10);
  wifiManager.autoConnect(stationName);
}

void blinkYellow() {
  ledState = !ledState;
  digitalWrite(RED, ledState ? HIGH : LOW);
  digitalWrite(GREEN, ledState ? HIGH : LOW);
  digitalWrite(BLUE, LOW);
}

void checkWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecté au Wi-Fi !");
    wifiLost.attach(60.0, isWifiLost);
    ledTicker.detach();
    wifiCheckTicker.detach();
  }
}

void isWifiLost() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi perdue !");
    wifiLost.detach();
    tryToReconnect();
  }
}

void tryToReconnect() {
  int attempts = 0;
  while (attempts < 6 && WiFi.status() == WL_CONNECTED) {
    WiFi.begin();
    attempts++;
    delay(10000);
  }
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  wifiLost.attach(10.0, isWifiLost);
}

void checkCaptorStatus() {
  unsigned status_bme;
  status_bme = bme.begin();
  while (!status_bme) {
    blinkRed();
    status_bme = bme.begin();
  }
}

void getCaptorValue() {
  temp = bme.readTemperature();
  alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
  pre = bme.readPressure() / 100.0F;
  hum = bme.readHumidity();
  mac = WiFi.macAddress();
}

void blinkRed() {  // bme problem
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
  delay(1000);
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
  delay(1000);
}

String createJson(float temp, int alt, float pre, float hum, String mac) {
  String jsonPayload = "{";
  jsonPayload += "\"temperature\":\"";
  jsonPayload += String(temp);
  jsonPayload += "\",\"altitude\":\"";
  jsonPayload += String(alt);
  jsonPayload += "\",\"pressure\":\"";
  jsonPayload += String(pre);
  jsonPayload += "\",\"humidity\":\"";
  jsonPayload += String(hum);
  jsonPayload += "\",\"mac_address\":\"";
  jsonPayload += String(mac);
  jsonPayload += "\"}";
  return jsonPayload;
}

int sendHttpResquest(String jsonPayload, const char* serverName) {
  HTTPClient http;

  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);
  String response = http.getString();

  http.end();
  return httpResponseCode;
}

void constantGreen() {  // everything is fine
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, HIGH);
  digitalWrite(BLUE, LOW);
}

void constantYellow() {  // http error
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, HIGH);
  digitalWrite(BLUE, LOW);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  initLed();
  initPortal();
}

void loop() {

  checkCaptorStatus();

  getCaptorValue();

  String jsonPayload = createJson(temp, alt, pre, hum, mac);

  int httpResponseCode = sendHttpResquest(jsonPayload, serverName);

  if (httpResponseCode > 0) {
    constantGreen();
    delay(1800000);  //wait 30 minutes
  } else {
    constantYellow();
    delay(180000);  //wait 3 minutes
  }
}
