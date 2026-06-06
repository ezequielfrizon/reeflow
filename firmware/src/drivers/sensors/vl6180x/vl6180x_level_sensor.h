#pragma once

#include <stdint.h>

#include "drivers/i2c/i2c_bus.h"
#include "modules/water_level/water_level_sensor.h"

namespace reeflow::drivers::sensors {

uint8_t vl6180xLevelSensorSdaGpio();
uint8_t vl6180xLevelSensorSclGpio();
uint8_t vl6180xLevelSensorI2cAddress();

class Vl6180xLevelSensor final
    : public modules::water_level::WaterLevelSensor {
 public:
  explicit Vl6180xLevelSensor(
      I2cBus& bus, uint8_t sdaGpio = vl6180xLevelSensorSdaGpio(),
      uint8_t sclGpio = vl6180xLevelSensorSclGpio(),
      uint8_t i2cAddress = vl6180xLevelSensorI2cAddress());

  modules::water_level::WaterLevelSensorReading readLevel() override;

  uint8_t sdaGpio() const;
  uint8_t sclGpio() const;
  uint8_t i2cAddress() const;

 private:
  void beginIfNeeded();

  I2cBus& bus_;
  uint8_t sdaGpio_;
  uint8_t sclGpio_;
  uint8_t i2cAddress_;
  bool initialized_;
};

}  // namespace reeflow::drivers::sensors
