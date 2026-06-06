#pragma once

namespace reeflow::modules::temperature {

enum class TemperatureSensorReadStatus {
  kValid,
  kSensorNotFound,
  kReadError,
  kConversionPending,
};

struct TemperatureSensorReading {
  TemperatureSensorReadStatus status;
  float temperatureCelsius;
};

constexpr TemperatureSensorReading makeValidTemperatureReading(
    float temperatureCelsius) {
  return {TemperatureSensorReadStatus::kValid, temperatureCelsius};
}

constexpr TemperatureSensorReading makeSensorNotFoundReading() {
  return {TemperatureSensorReadStatus::kSensorNotFound, 0.0F};
}

constexpr TemperatureSensorReading makeTemperatureReadError() {
  return {TemperatureSensorReadStatus::kReadError, 0.0F};
}

constexpr TemperatureSensorReading makeTemperatureConversionPending() {
  return {TemperatureSensorReadStatus::kConversionPending, 0.0F};
}

class TemperatureSensor {
 public:
  virtual ~TemperatureSensor() = default;

  virtual TemperatureSensorReading readTemperature() = 0;
};

}  // namespace reeflow::modules::temperature
