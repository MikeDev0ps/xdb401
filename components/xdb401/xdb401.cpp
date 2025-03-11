#include "xdb401.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace xdb401 {

static const char *const TAG = "xdb401.sensor";
static const uint8_t REG_CMD = 0x30;
static const uint8_t REG_PRESS_MSB = 0x06;
static const uint8_t REG_TEMP_MSB = 0x09;

void XDB401Component::setup() {
  ESP_LOGCONFIG(TAG, "Настройка XDB401...");
  uint8_t dummy;
  if (!this->read_bytes(REG_CMD, &dummy, 1)) {
    ESP_LOGE(TAG, "Ошибка связи с сенсором XDB401!");
    this->mark_failed();
  }
}

void XDB401Component::update() {
  uint8_t command = 0x0A;
  if (!this->write_bytes(REG_CMD, &command, 1)) {
    ESP_LOGE(TAG, "Не удалось отправить команду измерения в XDB401!");
    this->status_set_warning();
    return;
  }

  uint32_t start_time = millis();
  this->set_timeout("conversion", 5, [this, start_time]() { this->check_conversion_complete_(start_time); });
}

void XDB401Component::check_conversion_complete_(uint32_t start_time) {
  uint8_t status;
  if (!this->read_bytes(REG_CMD, &status, 1)) {
    ESP_LOGE(TAG, "Не удалось прочитать статус с XDB401!");
    this->status_set_warning();
    return;
  }

  if ((status & 0x08) != 0) {
    if (millis() - start_time < 50) {
      this->set_timeout("conversion", 5, [this, start_time]() { this->check_conversion_complete_(start_time); });
      return;
    } else {
      ESP_LOGE(TAG, "Таймаут измерения или сенсор не отвечает");
      this->status_set_warning();
      return;
    }
  }

  this->read_sensor_data_();
}

void XDB401Component::read_sensor_data_() {
  uint8_t data[5];
  if (!this->read_bytes(REG_PRESS_MSB, data, 5)) {
    ESP_LOGE(TAG, "Не удалось прочитать данные с XDB401");
    this->status_set_warning();
    return;
  }

  // ------------------------------
  // Преобразование давления:
  // Сырые данные (24-битное число) собираются из байтов data[0..2].
  long rawPressure = ((long)data[0] << 16) | ((long)data[1] << 8) | data[2];
  // Если значение больше 8388608, значит, число отрицательное – выполняем коррекцию.
  if (rawPressure > 8388608)
    rawPressure -= 16777216;
  
  // Определяем полный диапазон датчика: 
  // Для датчика с диапазоном 0–100 бар полный масштаб = 10 МПа.
  const float FULLSCALE_MPA = 10.0;
  // Преобразуем сырое значение в МПа: делим на 8388608 и умножаем на FULLSCALE_MPA.
  float pressure_MPa = (float)rawPressure / 8388608.0 * FULLSCALE_MPA;
  // Переводим МПа в бар (1 МПа = 10 бар).
  float pressure_bar = pressure_MPa * 10.0;
  
  // ------------------------------
  // Преобразование температуры:
  // Сырые данные температуры (2 байта) находятся в data[3] и data[4].
  uint16_t rawTemp = ((uint16_t)data[3] << 8) | data[4];
  // Если значение больше 32768, корректируем знак.
  if (rawTemp > 32768)
    rawTemp -= 65536;
  // Преобразование: умножаем на 128 и делим на 32768. Заметим, что 128/32768 == 1/256.
  float temperature_C = (float)rawTemp / 32768.0 * 128.0;

  ESP_LOGD(TAG, "XDB401: Давление=%.3f bar, Температура=%.2f °C", pressure_bar, temperature_C);

  if (this->pressure_ != nullptr)
    this->pressure_->publish_state(pressure_bar);
  if (this->temperature_ != nullptr)
    this->temperature_->publish_state(temperature_C);

  this->status_clear_warning();
}

void XDB401Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Сенсор XDB401:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Температура", this->temperature_);
  LOG_SENSOR("  ", "Давление", this->pressure_);
}

float XDB401Component::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace xdb401
}  // namespace esphome
