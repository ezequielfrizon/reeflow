#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::contracts {

enum class HardwarePinRole {
  kSensorBus,
  kI2cBus,
  kRelayOutput,
  kPwmOutput,
};

struct HardwarePin {
  HardwarePinRole role;
  const char* group;
  const char* canonicalName;
  const char* signal;
  uint8_t gpio;
  const char* observation;
};

constexpr HardwarePin kHardwareV1Pins[] = {
    {HardwarePinRole::kSensorBus, "Sensor", "DS18B20", "OneWire data", 4, ""},
    {HardwarePinRole::kI2cBus, "I2C", "VL6180X SDA", "I2C SDA", 21, ""},
    {HardwarePinRole::kI2cBus, "I2C", "VL6180X SCL", "I2C SCL", 22, ""},
    {HardwarePinRole::kRelayOutput, "Relay", "Relay 1 Recalque", "Relay output", 16, ""},
    {HardwarePinRole::kRelayOutput, "Relay", "Relay 2 Aquecedor", "Relay output", 17, ""},
    {HardwarePinRole::kRelayOutput, "Relay", "Relay 3 ATO", "Relay output", 18, ""},
    {HardwarePinRole::kRelayOutput, "Relay", "Relay 4 Reserva", "Relay output", 19, ""},
    {HardwarePinRole::kPwmOutput, "PWM", "PWM Branco", "LEDC output", 25, ""},
    {HardwarePinRole::kPwmOutput, "PWM", "PWM Azul", "LEDC output", 26, ""},
    {HardwarePinRole::kPwmOutput, "PWM", "PWM Royal Blue", "LEDC output", 27, ""},
    {HardwarePinRole::kPwmOutput, "PWM", "PWM Moonlight/UV", "LEDC output", 14,
     "Hardware V1 lists Moonlight; lighting spec lists UV."},
    {HardwarePinRole::kPwmOutput, "PWM", "PWM Reserva", "LEDC output", 13, ""},
};

constexpr size_t kHardwareV1PinCount =
    sizeof(kHardwareV1Pins) / sizeof(kHardwareV1Pins[0]);

}  // namespace reeflow::contracts
