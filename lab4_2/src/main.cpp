#include <Arduino.h>
#include "sta.hpp"
#include "mqtt.hpp"
#include "dht.hpp"

unsigned long fullLoopEndTime = 0, loopStartTime = 0;
constexpr unsigned long publishIntervalMillis = 2000;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MSG [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  if (!sta::connect_to_wifi()) {
    Serial.println("Failed to connect to WiFi. Halting execution.");
    while (true) {
      delay(1000);
    }
  }

  mqtt::setupMqtt();
  mqtt::setCallback(mqttCallback);

  dht::setupDHT();
}

void loop() {
  fullLoopEndTime = micros();
  // Serial.printf("[TIME] Full loop time: %lu microseconds\n", fullLoopEndTime - loopStartTime);
  loopStartTime = micros();

  if (!mqtt::client.connected()) {
    mqtt::connect();
  }
  mqtt::client.loop();

  static unsigned long lastPublishTime = 0;
  unsigned long currentTime = millis();

  // Publish every publishIntervalMillis milliseconds
  if (currentTime - lastPublishTime >= publishIntervalMillis) {
    float temperature, humidity, heatIndex;
    dht::readData(temperature, humidity, heatIndex);

    if (mqtt::client.connected()) {
      String message = String(temperature) + ":" + String(humidity) + ":" + String(heatIndex);
      Serial.printf("[MQTT] Publishing to topic %s: %s\n", mqtt::txTopic, message.c_str());
      if (mqtt::client.publish(mqtt::txTopic, message.c_str())) {
        lastPublishTime = currentTime;
      } else {
        Serial.println("[MQTT] Failed to publish message");
      }
    }
  }

  unsigned long userLoopFinishTime = micros();
  // Serial.printf("[TIME] User loop time: %lu microseconds\n", userLoopFinishTime - loopStartTime);
}
