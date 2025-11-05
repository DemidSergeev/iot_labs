#ifndef RTOS_HPP
#define RTOS_HPP

#include <Arduino.h>
#include "mpu.hpp"
#include "mqtt.hpp"

namespace rtos {

mpu::MpuData sharedBuffer; // Буфер для данных с акселерометра
SemaphoreHandle_t bufferMutex; // Мьютекс для защиты буфера

TaskHandle_t mqttTaskHandle; // Дескриптор задачи MQTT (Ядро 0)
TaskHandle_t mpuTaskHandle;  // Дескриптор задачи MPU (Ядро 1)

const TickType_t PUBLISH_INTERVAL = pdMS_TO_TICKS(2000);

/**
 * @brief Callback для входящих MQTT сообщений.
 * Вызывается из mqtt::client.loop() в задаче taskCore0_MQTT.
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[RTOS-MQTT] Message received[");
    Serial.print(topic);
    Serial.print("]: ");
    
    // Здесь может быть обработка команды
    // Например, изменение режима работы MPU
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

/**
 * @brief Задача для Ядра 0: Управление MQTT.
 * - Поддерживает соединение с брокером.
 * - Принимает команды (через callback).
 * - Публикует данные из общего буфера.
 */
void taskCore0_MQTT(void *pvParameters) {
    Serial.println("[RTOS] RTOS task on Core 0 started.");
    delay(10);
    TickType_t lastPublishTime = xTaskGetTickCount();

    // Измерение времени выполнения (микросекунды)
    uint64_t totalExecMicros = 0;
    uint32_t maxExecMicros = 0;
    uint32_t iterCount = 0;
    const uint32_t REPORT_EVERY = 100;

    for (;;) {
        // Запуск таймера активной работы
        uint32_t start = micros();

        if (!mqtt::client.connected()) {
            mqtt::connect();
        }
        mqtt::client.loop();

        if ((xTaskGetTickCount() - lastPublishTime) >= PUBLISH_INTERVAL) {
            
            if (mqtt::client.connected()) {
                mpu::MpuData localCopy;

                // Блокируем мьютекс для безопасного чтения
                if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    localCopy = sharedBuffer; // Копируем данные
                    xSemaphoreGive(bufferMutex); // Освобождаем мьютекс
                } else {
                    Serial.println("[RTOS-MQTT] Error acquiring mutex for reading.");
                }

                // Формируем сообщение в JSON
                String message = "{\"ax\": " + String(localCopy.ax) + 
                                 ", \"ay\": " + String(localCopy.ay) + 
                                 ", \"az\": " + String(localCopy.az) + "}";

                // Публикуем
                Serial.printf("[RTOS-MQTT] Publishing to topic %s: %s\n", mqtt::txTopic, message.c_str());
                if (mqtt::client.publish(mqtt::txTopic, message.c_str())) {
                    lastPublishTime = xTaskGetTickCount();
                } else {
                    Serial.println("[RTOS-MQTT] Error publishing.");
                }
            }
        }

        // Измеряем время выполнения
        uint32_t elapsed = micros() - start;
        totalExecMicros += elapsed;
        if (elapsed > maxExecMicros) maxExecMicros = elapsed;
        iterCount++;

        if (iterCount >= REPORT_EVERY) {
            uint64_t avg = (iterCount > 0) ? (totalExecMicros / iterCount) : 0;
            Serial.printf("[RTOS-MQTT] Core0 task execution time: avg=%llu us max=%u us (over %u iters)\n",
                          (unsigned long long)avg, maxExecMicros, iterCount);
            // сброс статистики
            totalExecMicros = 0;
            maxExecMicros = 0;
            iterCount = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Даем поработать другим процессам
    }
}

/**
 * @brief Задача для Ядра 1: Чтение данных с MPU6050.
 * - Считывает данные с датчика.
 * - Записывает их в общий буфер под мьютексом.
 */
void taskCore1_MPU(void *pvParameters) {
    Serial.println("[RTOS] RTOS task on Core 1 started.");
    delay(10);

    mpu::MpuData tempData; // Локальный буфер для чтения

    // Измерение времени выполнения (микросекунды)
    uint64_t totalExecMicros = 0;
    uint32_t maxExecMicros = 0;
    uint32_t iterCount = 0;
    const uint32_t REPORT_EVERY = 100;

    for (;;) {
        // Запуск таймера активной работы
        uint32_t start = micros();

        // 1. Чтение данных с датчика
        if (mpu::readMockData(tempData)) {
            Serial.printf("[RTOS-MPU] Read: ax=%.2f, ay=%.2f, az=%.2f\n", 
                          tempData.ax, tempData.ay, tempData.az);
            
            // 2. Запись в общий буфер с блокировкой
            if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                sharedBuffer = tempData; // Обновляем общие данные
                xSemaphoreGive(bufferMutex); // Освобождаем
            } else {
                 Serial.println("[RTOS-MPU] Error acquiring mutex for writing.");
            }
        } else {
            Serial.println("[RTOS-MPU] Error reading MPU data.");
        }

        // Измерение времени выполнения
        uint32_t elapsed = micros() - start;
        totalExecMicros += elapsed;
        if (elapsed > maxExecMicros) maxExecMicros = elapsed;
        iterCount++;

        if (iterCount >= REPORT_EVERY) {
            uint64_t avg = (iterCount > 0) ? (totalExecMicros / iterCount) : 0;
            Serial.printf("[RTOS-MPU] Core1 task execution time: avg=%llu us max=%u us (over %u iters)\n",
                          (unsigned long long)avg, maxExecMicros, iterCount);
            // сброс статистики
            totalExecMicros = 0;
            maxExecMicros = 0;
            iterCount = 0;
        }

        // Опрашиваем датчик с заданным интервалом
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Инициализация мьютекса и создание задач FreeRTOS.
 */
void setupRtos() {
    Serial.println("[RTOS] RTOS setup started.");

    // Создаем мьютекс
    bufferMutex = xSemaphoreCreateMutex();
    if (bufferMutex == NULL) {
        Serial.println("[RTOS] Error creating mutex!");
        // Остановка, если мьютекс не создан
        while(true);
    }

    // Создаем задачу MQTT на Ядре 0
    xTaskCreatePinnedToCore(
        taskCore0_MQTT,   // Функция задачи
        "MQTT_Task",      
        10000,            
        NULL,             
        1,                
        &mqttTaskHandle,  
        0                 // Ядро 0
    );

    // Создаем задачу MPU на Ядре 1
    xTaskCreatePinnedToCore(
        taskCore1_MPU,    // Функция задачи
        "MPU_Task",       
        4096,             
        NULL,             
        1,                
        &mpuTaskHandle,   
        1                 // Ядро 1
    );
}

} // namespace rtos

#endif // RTOS_HPP