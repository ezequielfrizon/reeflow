#include "drivers/initial_safe_state.h"

#include "config/pwm_bringup_config.h"
#include "contracts/hardware_pins.h"
#include "contracts/relay_contract.h"

namespace reeflow::drivers {
namespace {

GpioLevel gpioLogicLevelToPortLevel(contracts::GpioLogicLevel level) {
  switch (level) {
    case contracts::GpioLogicLevel::kLow:
      return GpioLevel::kLow;
    case contracts::GpioLogicLevel::kHigh:
      return GpioLevel::kHigh;
  }

  return GpioLevel::kLow;
}

void applyOutputLevel(GpioPort& gpioPort, uint8_t gpio, GpioLevel level) {
  gpioPort.write(gpio, level);
  gpioPort.setMode(gpio, GpioMode::kOutput);
  gpioPort.write(gpio, level);
}

}  // namespace

InitialSafeStateResult applyInitialSafeHardwareState(GpioPort& gpio,
                                                     PwmLedcPort& pwm) {
  InitialSafeStateResult result = {};
  const GpioLevel relayOffLevel =
      gpioLogicLevelToPortLevel(contracts::kRelayElectricalContract.physicalOffLevel);
  const config::PwmBringupParameters& pwmParameters =
      config::pwmBringupParameters();

  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];

    switch (pin.role) {
      case contracts::HardwarePinRole::kRelayOutput:
        applyOutputLevel(gpio, pin.gpio, relayOffLevel);
        ++result.relaysConfigured;
        break;
      case contracts::HardwarePinRole::kPwmOutput:
        {
          config::PwmBringupChannel channel = {};
          for (size_t pwmIndex = 0; pwmIndex < config::pwmBringupChannelCount();
               ++pwmIndex) {
            if (config::getPwmBringupChannel(pwmIndex, channel) &&
                channel.pin != nullptr && channel.pin->gpio == pin.gpio) {
              pwm.setup(channel.ledcChannel, pwmParameters.frequencyHz,
                        pwmParameters.resolutionBits);
              pwm.attachPin(pin.gpio, channel.ledcChannel);
              pwm.write(channel.ledcChannel, pwmParameters.dutyZero);
              break;
            }
          }
        }
        ++result.pwmChannelsConfigured;
        break;
      case contracts::HardwarePinRole::kSensorBus:
      case contracts::HardwarePinRole::kI2cBus:
        gpio.setMode(pin.gpio, GpioMode::kInput);
        ++result.sensorBusPinsPrepared;
        break;
    }
  }

  return result;
}

}  // namespace reeflow::drivers
