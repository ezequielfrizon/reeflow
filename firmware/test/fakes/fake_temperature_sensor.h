#pragma once

#include <stddef.h>

#include "modules/temperature/temperature_sensor.h"

namespace reeflow::test::fakes {

class FakeTemperatureSensor final
    : public modules::temperature::TemperatureSensor {
 public:
  static constexpr float kHighTemperatureCelsius = 29.0F;
  static constexpr float kLowTemperatureCelsius = 24.0F;
  static constexpr size_t kMaxQueuedReadings = 16;

  modules::temperature::TemperatureSensorReading readTemperature() override {
    ++readCount_;

    if (nextQueuedReading_ < queuedReadingCount_) {
      return queuedReadings_[nextQueuedReading_++];
    }

    return currentReading_;
  }

  void setValidTemperature(float temperatureCelsius) {
    currentReading_ =
        modules::temperature::makeValidTemperatureReading(temperatureCelsius);
  }

  void setHighTemperature() {
    setValidTemperature(kHighTemperatureCelsius);
  }

  void setLowTemperature() {
    setValidTemperature(kLowTemperatureCelsius);
  }

  void setSensorNotFound() {
    currentReading_ = modules::temperature::makeSensorNotFoundReading();
  }

  void setReadError() {
    currentReading_ = modules::temperature::makeTemperatureReadError();
  }

  void setConversionPending() {
    currentReading_ =
        modules::temperature::makeTemperatureConversionPending();
  }

  bool queueValidTemperature(float temperatureCelsius) {
    return queueReading(
        modules::temperature::makeValidTemperatureReading(temperatureCelsius));
  }

  bool queueHighTemperature() {
    return queueValidTemperature(kHighTemperatureCelsius);
  }

  bool queueLowTemperature() {
    return queueValidTemperature(kLowTemperatureCelsius);
  }

  bool queueSensorNotFound() {
    return queueReading(modules::temperature::makeSensorNotFoundReading());
  }

  bool queueReadError() {
    return queueReading(modules::temperature::makeTemperatureReadError());
  }

  bool queueConversionPending() {
    return queueReading(
        modules::temperature::makeTemperatureConversionPending());
  }

  void clearQueuedReadings() {
    queuedReadingCount_ = 0;
    nextQueuedReading_ = 0;
  }

  size_t readCount() const {
    return readCount_;
  }

  size_t queuedReadingCount() const {
    return queuedReadingCount_;
  }

  size_t remainingQueuedReadings() const {
    return queuedReadingCount_ - nextQueuedReading_;
  }

 private:
  bool queueReading(
      modules::temperature::TemperatureSensorReading reading) {
    if (queuedReadingCount_ >= kMaxQueuedReadings) {
      return false;
    }

    queuedReadings_[queuedReadingCount_++] = reading;
    return true;
  }

  modules::temperature::TemperatureSensorReading currentReading_ =
      modules::temperature::makeSensorNotFoundReading();
  modules::temperature::TemperatureSensorReading
      queuedReadings_[kMaxQueuedReadings] = {};
  size_t queuedReadingCount_ = 0;
  size_t nextQueuedReading_ = 0;
  size_t readCount_ = 0;
};

}  // namespace reeflow::test::fakes
