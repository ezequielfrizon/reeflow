#include <assert.h>

#include "fakes/fake_water_level_sensor.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_sensor.h"

namespace {

using reeflow::modules::water_level::WaterLevelSensor;
using reeflow::modules::water_level::WaterLevelSensorReadStatus;
using reeflow::modules::water_level::isCanonicalWaterLevel;
using reeflow::modules::water_level::makeDefaultWaterLevelModuleConfig;
using reeflow::test::fakes::FakeWaterLevelSensor;

void assertReading(WaterLevelSensor& sensor,
                   WaterLevelSensorReadStatus expectedStatus,
                   int16_t expectedLogicalLevel) {
  const auto reading = sensor.readLevel();

  assert(reading.status == expectedStatus);
  assert(reading.logicalLevel == expectedLogicalLevel);
}

void testDefaultModuleConfigUsesPhase4Contract() {
  const auto config = makeDefaultWaterLevelModuleConfig();

  assert(config.readIntervalMillis == 5000);
  assert(config.sensorOfflineTimeoutMillis == 30000);
  assert(config.canonicalMinimumLevel == 0);
  assert(config.canonicalMaximumLevel == 100);
  assert(isCanonicalWaterLevel(0));
  assert(isCanonicalWaterLevel(100));
  assert(!isCanonicalWaterLevel(-1));
  assert(!isCanonicalWaterLevel(101));
}

void testFakeReturnsConfiguredValidReading() {
  FakeWaterLevelSensor sensor;

  sensor.setValidLevel(42);

  assertReading(sensor, WaterLevelSensorReadStatus::kValid, 42);
  assert(sensor.readCount() == 1);
}

void testFakeReturnsLowAndHighValidReadings() {
  FakeWaterLevelSensor sensor;

  sensor.setLowLevel();
  assertReading(sensor, WaterLevelSensorReadStatus::kValid,
                FakeWaterLevelSensor::kLowLogicalLevel);

  sensor.setHighLevel();
  assertReading(sensor, WaterLevelSensorReadStatus::kValid,
                FakeWaterLevelSensor::kHighLogicalLevel);
}

void testFakeReturnsSensorNotFound() {
  FakeWaterLevelSensor sensor;

  sensor.setSensorNotFound();

  assertReading(sensor, WaterLevelSensorReadStatus::kSensorNotFound, 0);
}

void testFakeReturnsReadError() {
  FakeWaterLevelSensor sensor;

  sensor.setReadError();

  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
}

void testFakeReturnsOutOfRangeReading() {
  FakeWaterLevelSensor sensor;

  sensor.setOutOfRangeLevel(101);

  assertReading(sensor, WaterLevelSensorReadStatus::kOutOfRange, 101);
}

void testValidFactoryRejectsOutOfRangeReading() {
  FakeWaterLevelSensor sensor;

  sensor.setValidLevel(101);

  assertReading(sensor, WaterLevelSensorReadStatus::kOutOfRange, 101);
}

void testFakeReturnsFailureAndRecoverySequence() {
  FakeWaterLevelSensor sensor;

  assert(sensor.queueReadError());
  assert(sensor.queueSensorNotFound());
  assert(sensor.queueOutOfRangeLevel(-1));
  assert(sensor.queueValidLevel(55));

  assert(sensor.remainingQueuedReadings() == 4);
  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
  assertReading(sensor, WaterLevelSensorReadStatus::kSensorNotFound, 0);
  assertReading(sensor, WaterLevelSensorReadStatus::kOutOfRange, -1);
  assertReading(sensor, WaterLevelSensorReadStatus::kValid, 55);
  assert(sensor.remainingQueuedReadings() == 0);
}

void testFakeCanRepresentFailuresSinceBootWithoutValidReading() {
  FakeWaterLevelSensor sensor;

  assertReading(sensor, WaterLevelSensorReadStatus::kSensorNotFound, 0);
  assert(sensor.queueSensorNotFound());
  assert(sensor.queueReadError());
  assert(sensor.queueOutOfRangeLevel(120));

  assertReading(sensor, WaterLevelSensorReadStatus::kSensorNotFound, 0);
  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
  assertReading(sensor, WaterLevelSensorReadStatus::kOutOfRange, 120);
  assert(sensor.readCount() == 4);
}

void testQueuedReadingsFallBackToConfiguredCurrentReading() {
  FakeWaterLevelSensor sensor;

  sensor.setValidLevel(60);
  assert(sensor.queueReadError());

  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
  assertReading(sensor, WaterLevelSensorReadStatus::kValid, 60);
}

}  // namespace

int main() {
  testDefaultModuleConfigUsesPhase4Contract();
  testFakeReturnsConfiguredValidReading();
  testFakeReturnsLowAndHighValidReadings();
  testFakeReturnsSensorNotFound();
  testFakeReturnsReadError();
  testFakeReturnsOutOfRangeReading();
  testValidFactoryRejectsOutOfRangeReading();
  testFakeReturnsFailureAndRecoverySequence();
  testFakeCanRepresentFailuresSinceBootWithoutValidReading();
  testQueuedReadingsFallBackToConfiguredCurrentReading();
  return 0;
}
