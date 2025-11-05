#ifndef MPU_HPP
#define MPU_HPP

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

namespace mpu {

Adafruit_MPU6050 mpu;

/**
 * @brief Структура для хранения данных с MPU6050.
 */
struct MpuData {
    float ax, ay, az; // Ускорение
    float gx, gy, gz; // Гироскоп
    float temp;       // Температура
};

/**
 * @brief Инициализация датчика MPU6050.
 * @return true в случае успеха, false в случае ошибки.
 */
bool setupMpu() {
    Serial.println("[MPU] Initializing MPU6050...");
    if (!mpu.begin()) {
        Serial.println("[MPU] MPU6050 not found!");
        return false;
    }
    
    // Настройка диапазонов (пример)
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("[MPU] MPU6050 initialized.");
    return true;
}

bool readData(MpuData& data) {
    sensors_event_t a, g, temp;
    
    // Получаем события датчика
    if (!mpu.getEvent(&a, &g, &temp)) {
        Serial.println("[MPU] Error reading data from MPU6050.");
        return false;
    }

    // Заполняем структуру
    data.ax = a.acceleration.x;
    data.ay = a.acceleration.y;
    data.az = a.acceleration.z;

    data.gx = g.gyro.x;
    data.gy = g.gyro.y;
    data.gz = g.gyro.z;
    
    data.temp = temp.temperature;
    
    return true;
}

bool readMockData(MpuData& data) {
    data.ax = (float)random(-100, 100) / 10.0f;
    data.ay = (float)random(-100, 100) / 10.0f;
    data.az = (float)random(-100, 100) / 10.0f;
    data.gx = 0.0f;
    data.gy = 0.0f;
    data.gz = 0.0f;
    data.temp = 25.0f;
    return true;
}

} // namespace mpu

#endif // MPU_HPP