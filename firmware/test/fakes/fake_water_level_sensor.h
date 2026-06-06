#pragma once

#include <stddef.h>

#include "modules/water_level/water_level_sensor.h"

namespace reeflow::test::fakes {

class FakeWaterLevelSensor final
    : public modules::water_level::WaterLevelSensor {
 public:
  static constexpr uint16_t kLowLogicalLevel = 0;
  static constexpr uint16_t kHighLogicalLevel = 100;
  static constexpr size_t kMaxQueuedReadings = 16;

  modules::water_level::WaterLevelSensorReading readLevel() override {
    ++readCount_;

    if (nextQueuedReading_ < queuedReadingCount_) {
      return queuedReadings_[nextQueuedReading_++];
    }

    return currentReading_;
  }

  void setValidLevel(uint16_t logicalLevel) {
    currentReading_ =
        modules::water_level::makeValidWaterLevelReading(logicalLevel);
  }

  void setLowLevel() {
    setValidLevel(kLowLogicalLevel);
  }

  void setHighLevel() {
    setValidLevel(kHighLogicalLevel);
  }

  void setSensorNotFound() {
    currentReading_ =
        modules::water_level::makeSensorNotFoundWaterLevelReading();
  }

  void setReadError() {
    currentReading_ = modules::water_level::makeWaterLevelReadError();
  }

  void setOutOfRangeLevel(int16_t logicalLevel) {
    currentReading_ =
        modules::water_level::makeOutOfRangeWaterLevelReading(logicalLevel);
  }

  bool queueValidLevel(uint16_t logicalLevel) {
    return queueReading(
        modules::water_level::makeValidWaterLevelReading(logicalLevel));
  }

  bool queueLowLevel() {
    return queueValidLevel(kLowLogicalLevel);
  }

  bool queueHighLevel() {
    return queueValidLevel(kHighLogicalLevel);
  }

  bool queueSensorNotFound() {
    return queueReading(
        modules::water_level::makeSensorNotFoundWaterLevelReading());
  }

  bool queueReadError() {
    return queueReading(modules::water_level::makeWaterLevelReadError());
  }

  bool queueOutOfRangeLevel(int16_t logicalLevel) {
    return queueReading(
        modules::water_level::makeOutOfRangeWaterLevelReading(logicalLevel));
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
  bool queueReading(modules::water_level::WaterLevelSensorReading reading) {
    if (queuedReadingCount_ >= kMaxQueuedReadings) {
      return false;
    }

    queuedReadings_[queuedReadingCount_++] = reading;
    return true;
  }

  modules::water_level::WaterLevelSensorReading currentReading_ =
      modules::water_level::makeSensorNotFoundWaterLevelReading();
  modules::water_level::WaterLevelSensorReading
      queuedReadings_[kMaxQueuedReadings] = {};
  size_t queuedReadingCount_ = 0;
  size_t nextQueuedReading_ = 0;
  size_t readCount_ = 0;
};

}  // namespace reeflow::test::fakes
