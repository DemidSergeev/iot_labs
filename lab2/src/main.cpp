#include <Arduino.h>

constexpr int ledPin = 2, ledPinChannel = 0;
constexpr int brightnessStep = 3;
int delayMs = 10;
unsigned long loopStartTime = 0, fullLoopEndTime = 0;

void setup() {
  Serial.begin(115200);
  ledcSetup(ledPinChannel, 5000, 8);
  ledcAttachPin(ledPin, ledPinChannel);
}

void loop() {
  fullLoopEndTime = micros();
  Serial.printf("Full loop time: %lu microseconds\n", fullLoopEndTime - loopStartTime);
  loopStartTime = micros();

  for (int brightness = 0; brightness <= 255; brightness += brightnessStep) {
      ledcWrite(ledPinChannel, brightness);
      delay(delayMs);
  }
  
  delay(200);
  
  for (int brightness = 255; brightness >= 0; brightness -= brightnessStep) {
      ledcWrite(ledPinChannel, brightness);
      delay(delayMs);
  }
  
  delay(200);

  unsigned long userLoopFinishTime = micros();
  Serial.printf("User loop time: %lu microseconds\n", userLoopFinishTime - loopStartTime);
}