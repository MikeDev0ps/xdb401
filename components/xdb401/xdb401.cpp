#include "xdb401.h"
#include "esphome/core/log.h"

namespace esphome {
namespace xdb401 {

static const char *const TAG = "xdb401.sensor";
// Определения регистров по скетчу XDB401.txt
static const uint8_t REG_CMD = 0x30;
static const uint8_t REG_PRESS_MSB = 0x06;
static const uint8_t REG_TEMP_MSB = 0x09;

void XDB401Component::setup() {
  ESP_LOGCONFIG(TAG, "Настройка XDB401...");
  // Проверка связи: считываем статус из регистра REG_CMD.
  uint8_t dummy;
  if (!this->read_bytes(REG_CMD, &dummy, 1)) {
    ESP_LOGE(TAG, "Ошибка связи с сенсором XDB401!");
    this->mark_failed();
  }
}

void XDB401Component::update() {
  // 1. Отправляем команду 0x0A для старта одновременного измерения температуры и давления.
  uint8_t cmd[2] = { REG_CMD, 0x0A };
  if (!this->write_array(cmd, 2)) {
    ESP_LOGE(TAG, "Не удалось отправить команду измерения в XDB401!");
    this->status_set_warning();
    return;
  }

  uint32_t start_time = millis();
  // 2. Запускаем проверку готовности измерения через 5 мс.
  this->set_timeout("conversion", 5, [this, start_time]() { this->check_conversion_complete_(start_time); });
}

void XDB401Component::check_conversion_complete_(uint32_t start_time) {
  uint8_t status;
  if (!this->read_bytes(REG_CMD, &status, 1)) {
    ESP_LOGE(TAG, "Не удалось прочитать статус с XDB401!");
    this->status_set_warning();
    return;
  }

  // Проверяем бит 3 (0x08). Если он установлен, измерение ещё не завершено.
  if ((status & 0x08) != 0) {
    if (millis() - start_time < 50) {
      // Если не вышел таймаут, проверяем снова через 5 мс.
      this->set_timeout("conversion", 5, [this, start_time]() { this->check_conversion_complete_(start_time); });
      return;
    } else {
      ESP_LOGE(TAG, "Таймаут измерения или сенсор не отвечает");
      this->status_set_warning();
      return;
    }
  }

  // Измерение завершено – переходим к считыванию данных.
  this->read_sensor_data_();
}

void XDB401Component::read_sensor_data_() {
  uint8_t data[5];
  // 3. Считываем 5 байт, начиная с регистра давления (0x06): 3 байта давления и 2 байта температуры.
  if (!this->read_bytes(REG_PRESS_MSB, data, 5)) {
    ESP_LOGE(TAG, "Не удалось прочитать данные с XDB401");
    this->status_set_warning();
    return;
  }

  // 4. Объединяем байты давления в 24-битное знаковое значение.
  long rawPressure = ((long)data[0] << 16) | ((long)data[1] << 8) | data[2];
  if (rawPressure & 0x00800000) {  // Проверка знакового бита
    rawPressure |= 0xFF000000;
  }

  // 5. Объединяем байты температуры в 16-битное знаковое значение.
  int16_t rawTemp = (int16_t)((data[3] << 8) | data[4]);

  // 6. Преобразуем сырые данные в физические величины.
  float pressure_Pa = (float)rawPressure / 2.0;
  float pressure_hPa = pressure_Pa / 100.0;  // перевод в гектопаскали
  float temperature_C = (float)rawTemp / 256.0;

  ESP_LOGD(TAG, "XDB401: Давление=%.1f hPa, Температура=%.2f °C", pressure_hPa, temperature_C);

  // 7. Публикуем состояния датчиков.
  if (this->pressure_ != nullptr)
    this->pressure_->publish_state(pressure_hPa);
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
