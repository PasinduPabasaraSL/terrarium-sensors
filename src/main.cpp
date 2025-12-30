#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

#define SDA_PIN 26
#define SCL_PIN 27

BH1750 lightMeter;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin(SDA_PIN, SCL_PIN);

    if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
        Serial.println("[BH1750] ERROR: not found");
        while (true) delay(1000);
    }

    Serial.println("[BH1750] Ready");
}

void loop() {
    float lux = lightMeter.readLightLevel();

    if (lux < 0) {
        Serial.println("[BH1750] Read error");
    } else {
        Serial.print("Light: ");
        Serial.print(lux);
        Serial.println(" lx");
    }

    delay(1000);
}
