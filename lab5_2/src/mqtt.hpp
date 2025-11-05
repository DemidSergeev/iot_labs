#ifndef MQTT_HPP
#define MQTT_HPP

#include "PubSubClient.h"

namespace mqtt {

const char* brokerHost = "192.168.0.102";
const uint16_t brokerPort = 1883;
const char* txTopic = "esp32/0ad3/tx";
const char* rxTopic = "esp32/0ad3/rx";
const char* user = "admin";
const char* password = "admin";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void connect();

void setupMqtt() {
    client.setServer(brokerHost, brokerPort);
    Serial.printf("[MQTT] Server configured: %s:%d\n", brokerHost, brokerPort);
    connect();
}

void setCallback(MQTT_CALLBACK_SIGNATURE) {
    client.setCallback(callback);
    Serial.println("[MQTT] Callback set.");
}

void connect() {
    if (!client.connected()) {
        Serial.println("[MQTT] Connecting to MQTT broker...");
        // Create a random client ID
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str(), user, password)) {
            Serial.println("[MQTT] Connected to MQTT broker.");
            client.subscribe(rxTopic);
            Serial.printf("[MQTT] Subscribed to topic: %s\n", rxTopic);
        } else {
            Serial.printf("[MQTT] Failed to connect to MQTT broker, state: %d\n", client.state());
            delay(5000);  // Wait before next retry
        }
    }
}

} // namespace mqtt

#endif // MQTT_HPP