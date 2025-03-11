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
  ESP_LOGCONFIG(TAG, "Configuring XDB401...");
  uint8_t dummy;
  if (!this->read_bytes(REG_CMD, &dummy, 1)) {
    ESP_LOGE(TAG, "Communication error with XDB401 sensor!");
    this->mark_failed();
  }
}

void XDB401Component::update() {
  uint8_t command = 0x0A;
  if (!this->write_bytes(REG_CMD, &command, 1)) {
    ESP_LOGE(TAG, "Failed to send measurement command to XDB401!");
    this->status_set_warning();
    return;
  }

  uint32_t start_time = millis();
  this->set_timeout("conversion", 5, [this, start_time]() { this->check_conversion_complete_(start_time); });
}

void XDB401Component::check_conversion_complete_(uint32_t start_time) {
  uint8_t status;
  if (!this->read_bytes(REG_CMD, &status, 1)) {
    ESP_LOGE(TAG, "Failed to read status from XDB401!");
    this->status_set_warning();
    return;
  }

  if ((status & 0x08) != 0) {
    if (millis() - start_time < 50) {
      this->set_timeout("conversion", 5, [this, start_time]() { this->check_conversion_complete_(start_time); });
      return;
    } else {
      ESP_LOGE(TAG, "Measurement timeout or sensor not responding");
      this->status_set_warning();
      return;
    }
  }

  this->read_sensor_data_();
}

void XDB401Component::read_sensor_data_() {
  uint8_t data[5];
  if (!this->read_bytes(REG_PRESS_MSB, data, 5)) {
    ESP_LOGE(TAG, "Failed to read data from XDB401");
    this->status_set_warning();
    return;
  }

// ------------------------------
// Pressure conversion:
// Raw data (24-bit number) is assembled from the bytes data[0..2].
long rawPressure = ((long)data[0] << 16) | ((long)data[1] << 8) | data[2];
// If the value is greater than 8388608, the number is negative – apply correction.
if (rawPressure > 8388608)
  rawPressure -= 16777216;

// Determine the sensor's full scale:
// For a sensor with a range of 0–100 bar, the full scale = 10 MPa.
const float FULLSCALE_MPA = 10.0;
// Convert the raw value to MPa: divide by 8388608 and multiply by FULLSCALE_MPA.
float pressure_MPa = (float)rawPressure / 8388608.0 * FULLSCALE_MPA;
// Convert MPa to bar (1 MPa = 10 bar).
float pressure_bar = pressure_MPa * 10.0;

// ------------------------------
// Temperature conversion:
// Raw temperature data (2 bytes) is located in data[3] and data[4].
uint16_t rawTemp = ((uint16_t)data[3] << 8) | data[4];
// If the value is greater than 32768, adjust the sign.
if (rawTemp > 32768)
  rawTemp -= 65536;
// Conversion: multiply by 128 and divide by 32768. Note that 128/32768 == 1/256.
float temperature_C = (float)rawTemp / 32768.0 * 128.0;


  ESP_LOGD(TAG, "XDB401: Pressure=%.3f bar, Temperature=%.2f °C", pressure_bar, temperature_C);

  if (this->pressure_ != nullptr)
    this->pressure_->publish_state(pressure_bar);
  if (this->temperature_ != nullptr)
    this->temperature_->publish_state(temperature_C);

  this->status_clear_warning();
}

void XDB401Component::dump_config() {
  ESP_LOGCONFIG(TAG, "XDB401 Sensor:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Temperature", this->temperature_);
  LOG_SENSOR("  ", "Pressure", this->pressure_);
}

float XDB401Component::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace xdb401
}  // namespace esphome
