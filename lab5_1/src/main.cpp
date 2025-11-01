#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// Создание объекта MPU6050
// (Использует Wire.h по умолчанию для I2C)
Adafruit_MPU6050 mpu;

void setup(void) {
  // Инициализация последовательного порта
  Serial.begin(115200);
  while (!Serial) delay(10); // Ждем подключения (необходимо для некоторых плат, как ESP32/ESP8266)

  Serial.println("Adafruit MPU6050 Test!");

  // Инициализация MPU6050
  if (!mpu.begin()) {
    Serial.println("Не удалось найти чип MPU6050. Проверьте подключение.");
    // Бесконечный цикл, если инициализация не удалась
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 успешно инициализирован!");
  
  // Установка диапазона гироскопа (необязательно)
  // Диапазоны: MPU6050_RANGE_250_DEG, MPU6050_RANGE_500_DEG, MPU6050_RANGE_1000_DEG, MPU6050_RANGE_2000_DEG
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Диапазон Гироскопа установлен на: ");
  Serial.print(mpu.getGyroRange());
  Serial.println(" deg/s");

  // Установка диапазона акселерометра (необязательно)
  // Диапазоны: MPU6050_RANGE_2_G, MPU6050_RANGE_4_G, MPU6050_RANGE_8_G, MPU6050_RANGE_16_G
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Диапазон Акселерометра установлен на: ");
  Serial.print(mpu.getAccelerometerRange());
  Serial.println(" G");
  
  // Установка полосы пропускания (DLPF) (необязательно)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Полоса пропускания установлена на: ");
  Serial.print(mpu.getFilterBandwidth());
  Serial.println(" Hz");

  delay(100);
}

void loop() {
  // 'event' - это структура, которая содержит данные с акселерометра
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Вывод данных акселерометра (ускорение в м/с^2)
  Serial.print("Accel X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.print(" m/s^2");

  // Вывод данных гироскопа (угловая скорость в рад/с)
  Serial.print(" | Gyro X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.print(" rad/s");

  // Вывод температуры (в Цельсиях)
  Serial.print(" | Temp: ");
  Serial.print(temp.temperature);
  Serial.println(" C");

  delay(100);
}