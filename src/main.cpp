#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#define BH1750_SDA 21
#define BH1750_SCL 22
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define SOIL_PIN 34

const int RELAY_PIN = 27;
const bool RELAY_ACTIVE_LOW = true;

#define SOIL_DRY 3500
#define SOIL_WET 1500

BH1750 lightMeter;
DHT dht(DHT_PIN, DHT_TYPE);

const char* ssid = "KGB_Mode";
const char* password = "87654321";
const char* serverUrl = "http://10.154.223.157:3000/api/sensors";

const uint32_t SEND_EVERY_MS = 2000;
const uint32_t DHT_EVERY_MS  = 5000;

uint32_t lastSendAt = 0;
uint32_t lastDhtAt  = 0;

float lastTemp = NAN;
float lastHum  = NAN;

const int SOIL_START_WATER_BELOW = 35;
const int SOIL_STOP_WATER_AT_OR_ABOVE = 45;

const uint32_t WATER_BURST_MS = 3000;
const uint32_t WATER_COOLDOWN_MS = 30000;

uint32_t pumpBurstStartedAt = 0;
uint32_t lastWateredAt = 0;
bool pumpIsOn = false;
bool wateringCycleActive = false;

void pumpWrite(bool on) {
  pumpIsOn = on;
  if (RELAY_ACTIVE_LOW) digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  else digitalWrite(RELAY_PIN, on ? HIGH : LOW);

  Serial.println(on ? "Pump ON" : "Pump OFF");
}

void pumpOff() { pumpWrite(false); }
void pumpOn()  { pumpWrite(true);  }

uint8_t detectBH1750Address() {
  const uint8_t addrs[] = {0x23, 0x5C};
  for (uint8_t i = 0; i < 2; i++) {
    Wire.beginTransmission(addrs[i]);
    if (Wire.endTransmission() == 0) return addrs[i];
  }
  return 0;
}

void printNetInfo() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
}

void wifiConnectOrRestart() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\nWiFi connect timeout. Restarting...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi connected.");
  printNetInfo();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RELAY_PIN, OUTPUT);
  pumpOff();

  delay(300);
  pumpOn();  delay(200);
  pumpOff(); delay(200);

  wifiConnectOrRestart();

  Wire.begin(BH1750_SDA, BH1750_SCL);
  Wire.setClock(100000);

  uint8_t bhAddr = detectBH1750Address();
  if (bhAddr == 0) {
    Serial.println("{\"error\":\"BH1750 not detected\"}");
    while (true) delay(1000);
  }

  Serial.print("[BH1750] Address: 0x");
  Serial.println(bhAddr, HEX);

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, bhAddr, &Wire)) {
    Serial.println("{\"error\":\"BH1750 init failed\"}");
    while (true) delay(1000);
  }

  dht.begin();

  lastWateredAt = millis();
  lastSendAt = millis();
  lastDhtAt = 0;
}

void loop() {
  const uint32_t nowMs = millis();

  if (WiFi.status() != WL_CONNECTED) {
    wifiConnectOrRestart();
    delay(50);
    return;
  }

  if (nowMs - lastDhtAt >= DHT_EVERY_MS) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      lastTemp = t;
      lastHum = h;
      Serial.printf("DHT OK: T=%.1fC H=%.1f%%\n", lastTemp, lastHum);
    } else {
      Serial.println("DHT read failed (keeping last values)");
    }

    lastDhtAt = nowMs;
  }

  if (nowMs - lastSendAt < SEND_EVERY_MS) return;
  lastSendAt = nowMs;

  if (isnan(lastTemp) || isnan(lastHum)) {
    Serial.println("No valid DHT values yet, skipping POST");
    return;
  }

  float lightLux = lightMeter.readLightLevel();
  if (lightLux < 1) lightLux = 0;

  int soilRaw = analogRead(SOIL_PIN);
  int soilPercent = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  if (!wateringCycleActive && soilPercent <= SOIL_START_WATER_BELOW) {
    wateringCycleActive = true;
  }
  if (wateringCycleActive && soilPercent >= SOIL_STOP_WATER_AT_OR_ABOVE) {
    wateringCycleActive = false;
    pumpOff();
  }

  if (wateringCycleActive) {
    if (pumpIsOn) {
      if (nowMs - pumpBurstStartedAt >= WATER_BURST_MS) {
        pumpOff();
        lastWateredAt = nowMs;
      }
    } else {
      if (nowMs - lastWateredAt >= WATER_COOLDOWN_MS) {
        pumpOn();
        pumpBurstStartedAt = nowMs;
      }
    }
  } else {
    pumpOff();
  }

  String payload = "{";
  payload += "\"temperature\":" + String(lastTemp, 1) + ",";
  payload += "\"humidity\":" + String(lastHum, 1) + ",";
  payload += "\"light\":" + String(lightLux, 1) + ",";
  payload += "\"soil\":" + String(soilPercent) + ",";
  payload += "\"pump\":" + String(pumpIsOn ? 1 : 0);
  payload += "}";

  Serial.println("Sending: " + payload);

  WiFiClient client;
  client.setTimeout(5000);

  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  if (!http.begin(client, serverUrl)) {
    Serial.println("HTTP begin() failed");
    http.end();
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  int code = http.POST((uint8_t*)payload.c_str(), payload.length());

  Serial.print("HTTP Response code: ");
  Serial.println(code);

  if (code < 0) {
    Serial.print("HTTP error: ");
    Serial.println(http.errorToString(code).c_str());
  } else {
    String resp = http.getString();
    Serial.print("Response: ");
    Serial.println(resp);
  }

  http.end();
}