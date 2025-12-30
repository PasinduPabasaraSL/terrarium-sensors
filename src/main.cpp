#include <Arduino.h>

const int moisturePin = 34;

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(12);
    Serial.println("Soil Moisture Sensor Started");
}

void loop() {
    int rawValue = analogRead(moisturePin);

    Serial.print("Raw moisture value: ");
    Serial.println(rawValue);

    delay(1000);
}