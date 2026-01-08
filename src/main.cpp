#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>

/* -------------------- Pins -------------------- */
#define BH1750_SDA 26
#define BH1750_SCL 27

#define DHT_PIN 13
#define DHT_TYPE DHT11

#define SOIL_PIN 34

/* -------------------- Soil calibration -------------------- */
#define SOIL_DRY 3500   // adjust after testing
#define SOIL_WET 1500   // adjust after testing

/* -------------------- Sensor objects -------------------- */
BH1750 lightMeter;
DHT dht(DHT_PIN, DHT_TYPE);

/* -------------------- Setup -------------------- */
void setup() {
  Serial.begin(115200);
  delay(2000);

  /* I2C init */
  Wire.begin(BH1750_SDA, BH1750_SCL);

  /* BH1750 init with retry */
  bool bh1750Ready = false;
  for (int i = 0; i < 5; i++) {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23, &Wire)) {
      bh1750Ready = true;
      break;
    }
    delay(500);
  }

  if (!bh1750Ready) {
    Serial.println("{\"error\":\"BH1750 not detected\"}");
    while (true) delay(1000);
  }

  /* DHT init */
  dht.begin();
}

/* -------------------- Loop -------------------- */
void loop() {
  /* Read sensors */
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float lightLux = lightMeter.readLightLevel();
  int soilRaw = analogRead(SOIL_PIN);

  /* Validate readings */
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("{\"error\":\"DHT read failed\"}");
    delay(2000);
    return;
  }

  if (lightLux < 1) {
    lightLux = 0;
  }

  /* Soil percentage */
  int soilPercent = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  /* Output JSON */
  Serial.print("{");
  Serial.print("\"temperature\":"); Serial.print(temperature, 1); Serial.print(",");
  Serial.print("\"humidity\":"); Serial.print(humidity, 1); Serial.print(",");
  Serial.print("\"light\":"); Serial.print(lightLux, 1); Serial.print(",");
  Serial.print("\"soil\":"); Serial.print(soilPercent);
  Serial.println("}");

  delay(2000);
}
