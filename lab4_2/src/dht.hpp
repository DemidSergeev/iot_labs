#include "DHT.h"

namespace dht {

const uint8_t DHT_PIN = 4;
const uint8_t DHT_TYPE = DHT22;

DHT dht(DHT_PIN, DHT_TYPE);

void setupDHT() {
    dht.begin();
    Serial.println("[DHT] Sensor initialized.");
}

void readData(float& temperature, float& humidity, float& heatIndex) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    heatIndex = dht.computeHeatIndex(temperature, humidity, false);
}

} // namespace dht