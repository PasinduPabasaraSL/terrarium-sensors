#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>

// Pins
#define BH1750_SDA 26
#define BH1750_SCL 27
#define DHT_PIN 13
#define DHT_TYPE DHT11

// Sensor instances
BH1750 lightMeter;
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
    Serial.begin(115200);
    delay(2000); // let ESP32 stabilize

    // Initialize I2C for BH1750
    Wire.begin(BH1750_SDA, BH1750_SCL, 100000); // 100 kHz

    // Retry BH1750 init to avoid undefined error
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
    // Read BH1750
    float lux = lightMeter.readLightLevel();
    if (lux < 0) {
        Serial.println("[BH1750] Read error");
    } else {
        Serial.print("Light: ");
        Serial.print(lux);
        Serial.println(" lx");
    }

    // Read DHT11
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

    delay(2000); // 2 seconds between readings
}
