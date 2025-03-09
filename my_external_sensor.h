// my_external_sensor.h
#pragma once
#include "esphome.h"

namespace my_external_sensor {

class MyExternalSensor : public PollingComponent, public sensor::Sensor {
 public:
  // Интервал опроса в миллисекундах (например, 5000 мс)
  MyExternalSensor() : PollingComponent(5000) {}

  void setup() override {
    // Дополнительная инициализация, если требуется
  }

  void update() override {
    // Задайте I2C-адрес датчика (например, 0x76)
    const uint8_t sensor_address = 0x76;

    // Отправка команды для начала измерения: записываем 0x0A в регистр 0x30
    Wire.beginTransmission(sensor_address);
    Wire.write(0x30);  // Регистр команды
    Wire.write(0x0A);  // Команда измерения
    Wire.endTransmission();

    delay(100);  // Ждём завершения измерения

    // Чтение 3 байт данных давления с регистра 0x06
    Wire.beginTransmission(sensor_address);
    Wire.write(0x06);
    Wire.endTransmission();
    Wire.requestFrom(sensor_address, (uint8_t)3);
    if (Wire.available() < 3) {
      ESP_LOGE("MyExternalSensor", "Не удалось считать данные давления");
      return;
    }
    uint32_t raw_pressure = ((uint32_t)Wire.read() << 16) | ((uint32_t)Wire.read() << 8) | Wire.read();

    // Чтение 2 байт данных температуры с регистра 0x09
    Wire.beginTransmission(sensor_address);
    Wire.write(0x09);
    Wire.endTransmission();
    Wire.requestFrom(sensor_address, (uint8_t)2);
    if (Wire.available() < 2) {
      ESP_LOGE("MyExternalSensor", "Не удалось считать данные температуры");
      return;
    }
    uint16_t raw_temperature = ((uint16_t)Wire.read() << 8) | Wire.read();

    // Преобразование данных давления
    const float fullscale_pressure = 1000.0f;  // Полная шкала, измените при необходимости
    float pressure_value;
    if (raw_pressure > 8388608UL) {
      pressure_value = (raw_pressure - 16777216UL) / 8388608.0f * fullscale_pressure;
    } else {
      pressure_value = raw_pressure / 8388608.0f * fullscale_pressure;
    }

    // Преобразование данных температуры
    float temperature_value;
    if (raw_temperature > 32768) {
      temperature_value = (raw_temperature - 65536) / 256.0f;
    } else {
      temperature_value = raw_temperature / 256.0f;
    }

    // Публикуем измеренное значение температуры
    publish_state(temperature_value);

    // Если требуется, можно добавить отдельный сенсор для давления,
    // либо передавать оба значения через один компонент.
    ESP_LOGD("MyExternalSensor", "Temperature: %.2f °C, Pressure: %.2f kPa", temperature_value, pressure_value);
  }
};

}  // namespace my_external_sensor
