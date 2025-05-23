import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_PRESSURE,
    CONF_TEMPERATURE,
    DEVICE_CLASS_PRESSURE,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

DEPENDENCIES = ["i2c"]

CONF_FULLSCALE_MPA = "fullscale_mpa"

xdb401_ns = cg.esphome_ns.namespace("xdb401")
XDB401Component = xdb401_ns.class_(
    "XDB401Component", cg.PollingComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            # Component ID. Automatically generated.
            cv.GenerateID(): cv.declare_id(XDB401Component),
            # Temperature sensor configuration.
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Pressure sensor configuration.
            cv.Optional(CONF_PRESSURE): sensor.sensor_schema(
                unit_of_measurement="bar",
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_PRESSURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Full-scale range of the sensor in MPa.
            cv.Optional(CONF_FULLSCALE_MPA, default=10.0): cv.float_,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x7F))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if temperature_config := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(temperature_config)
        cg.add(var.set_temperature(sens))

    if pressure_config := config.get(CONF_PRESSURE):
        sens = await sensor.new_sensor(pressure_config)
        cg.add(var.set_pressure(sens))
    
    fullscale = config[CONF_FULLSCALE_MPA]
    cg.add(var.set_fullscale_mpa(fullscale))
