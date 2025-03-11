#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace xdb401 {

class XDB401Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
  void set_pressure(sensor::Sensor *pressure) { pressure_ = pressure; }
  void set_fullscale_mpa(float fullscale) { fullscale_mpa_ = fullscale; }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;

 protected:
  void check_conversion_complete_(uint32_t start_time);
  void read_sensor_data_();

  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *pressure_{nullptr};

  float fullscale_mpa_{10.0};
};

}  // namespace xdb401
}  // namespace esphome
