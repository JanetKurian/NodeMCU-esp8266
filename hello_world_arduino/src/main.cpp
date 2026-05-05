
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Hello ESP8266!");
}

void loop() {
    Serial.println("Printing every 1 second...");
    delay(1000);
}
