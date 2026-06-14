#pragma once

#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_water_level_sensor.h"

namespace reeflow::test::fakes {

class FakeResilienceSensors {
 public:
  void startOnline() {
    temperature_.setValidTemperature(26.0F);
    waterLevel_.setValidLevel(55);
  }

  void failTemperature() { temperature_.setSensorNotFound(); }
  void recoverTemperature() { temperature_.setValidTemperature(26.0F); }
  void failWaterLevel() { waterLevel_.setSensorNotFound(); }
  void recoverWaterLevel() { waterLevel_.setValidLevel(55); }

  FakeTemperatureSensor& temperature() { return temperature_; }
  FakeWaterLevelSensor& waterLevel() { return waterLevel_; }

 private:
  FakeTemperatureSensor temperature_;
  FakeWaterLevelSensor waterLevel_;
};

}  // namespace reeflow::test::fakes
