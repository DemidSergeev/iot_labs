#include <Arduino.h>
#include "sta.hpp" 
#include "mqtt.hpp"
#include "mpu.hpp"
#include "rtos.hpp"

void setup() {
    Serial.begin(115200);
    while (!Serial) {
      delay(10);
    }
    Serial.println("[MAIN] Initialization started.");

    // 1. Подключение к WiFi
    if (!sta::connect_to_wifi()) {
        Serial.println("[MAIN] Error connecting to WiFi.");
        while (true) { delay(1000); }
    }

    // 2. Настройка MQTT
    mqtt::setupMqtt();
    mqtt::setCallback(rtos::mqttCallback);

    // 3. Инициализация датчика MPU6050
    if (!mpu::setupMpu()) {
        Serial.println("[MAIN] Ошибка инициализации MPU6050. Остановка.");
        while (true) { delay(1000); }
    }

    // 4. Настройка и запуск задач FreeRTOS
    rtos::setupRtos();

    Serial.println("[MAIN] Setup complete. Switching to FreeRTOS tasks.");
    
    vTaskDelete(NULL);
}

void loop() {
}