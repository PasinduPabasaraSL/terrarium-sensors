#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>

// Pins
#define BH1750_SDA 26
#define BH1750_SCL 27
#define DHT_PIN 13
#define DHT_TYPE DHT11
#define SOIL_PIN 34 // Analog input

// Sensor instances
BH1750 lightMeter;
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
    Serial.begin(115200);
    delay(2000);

    // Initialize I2C
    Wire.begin(BH1750_SDA, BH1750_SCL, 100000);

    // BH1750 init with retries
    int retries = 5;
    bool init_ok = false;
    while (retries-- > 0) {
        if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
            init_ok = true;
            break;
        }
        Serial.println("[BH1750] Init failed, retrying...");
        delay(500);
    }
    if (!init_ok) {
        Serial.println("[BH1750] ERROR: not found");
        while (true) delay(1000);
    }
    Serial.println("[BH1750] Ready");

    // Initialize DHT11
    dht.begin();
    Serial.println("[DHT11] Ready");
}

void loop() {
    // BH1750 Reading
    float lux = lightMeter.readLightLevel();
    if (lux < 0) {
        Serial.println("[BH1750] Read error");
    } else {
        Serial.print("Light: ");
        Serial.print(lux);
        Serial.println(" lx");
    }

    // DHT11 Reading
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (isnan(temp) || isnan(hum)) {
        Serial.println("[DHT11] Read error");
    } else {
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.print(" °C, Humidity: ");
        Serial.print(hum);
        Serial.println(" %");
    }

    // Soil Moisture Reading
    int soilRaw = analogRead(SOIL_PIN);
    int soilPercent = map(soilRaw, 4095, 0, 0, 100); // dry=0%, wet=100%
    Serial.print("Soil Moisture: ");
    Serial.print(soilPercent);
    Serial.println(" %");

    Serial.println("-----------------------------");
    delay(2000);
}
