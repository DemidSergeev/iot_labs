#include <Arduino.h>
#include "DHT.h"
DHT dht_sensor(4, DHT22);

void setup() {
  Serial.begin(500000);
  Serial.println(F("DHT22 test!"));

  dht_sensor.begin();
}

void loop() {
  delay(2000);
  float h = dht_sensor.readHumidity();
  float t = dht_sensor.readTemperature();
  float hic = dht_sensor.computeHeatIndex(t, h, false);

  Serial.print(F("H: "));
  Serial.print(h);
  Serial.print(F(" %  T: "));
  Serial.print(t);
  Serial.print(F(" °C  Heat index: "));
  Serial.print(hic);
  Serial.println(F(" °C "));
}
