#pragma once

#include <stdint.h>

#include "core/platform/time_source.h"
#include "drivers/onewire/onewire_bus.h"
#include "modules/temperature/temperature_sensor.h"

namespace reeflow::drivers::sensors {

constexpr uint32_t kDs18b20TemperatureConversionWaitMillis = 750;

uint8_t ds18b20TemperatureSensorGpio();

class Ds18b20TemperatureSensor final
    : public modules::temperature::TemperatureSensor {
 public:
  Ds18b20TemperatureSensor(
      OneWireBus& bus, const core::platform::TimeSource& timeSource,
      uint32_t conversionWaitMillis =
          kDs18b20TemperatureConversionWaitMillis,
      uint8_t gpio = ds18b20TemperatureSensorGpio());

  modules::temperature::TemperatureSensorReading readTemperature() override;

  uint8_t gpio() const;

 private:
  modules::temperature::TemperatureSensorReading startConversion();
  modules::temperature::TemperatureSensorReading readCompletedConversion();
  void startNextConversionIfPresent();

  OneWireBus& bus_;
  const core::platform::TimeSource& timeSource_;
  uint32_t conversionWaitMillis_;
  uint8_t gpio_;
  bool conversionPending_;
  uint32_t conversionStartedAt_;
};

}  // namespace reeflow::drivers::sensors
