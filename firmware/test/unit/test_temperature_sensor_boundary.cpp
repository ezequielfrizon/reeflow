#include <assert.h>

#include "fakes/fake_temperature_sensor.h"
#include "modules/temperature/temperature_sensor.h"

namespace {

using reeflow::modules::temperature::TemperatureSensor;
using reeflow::modules::temperature::TemperatureSensorReadStatus;
using reeflow::test::fakes::FakeTemperatureSensor;

void assertReading(TemperatureSensor& sensor,
                   TemperatureSensorReadStatus expectedStatus,
                   float expectedTemperatureCelsius) {
  const auto reading = sensor.readTemperature();

  assert(reading.status == expectedStatus);
  assert(reading.temperatureCelsius == expectedTemperatureCelsius);
}

void testFakeReturnsConfiguredValidReading() {
  FakeTemperatureSensor sensor;

  sensor.setValidTemperature(26.5F);

  assertReading(sensor, TemperatureSensorReadStatus::kValid, 26.5F);
  assert(sensor.readCount() == 1);
}

void testFakeReturnsHighAndLowValidReadings() {
  FakeTemperatureSensor sensor;

  sensor.setHighTemperature();
  assertReading(sensor, TemperatureSensorReadStatus::kValid,
                FakeTemperatureSensor::kHighTemperatureCelsius);

  sensor.setLowTemperature();
  assertReading(sensor, TemperatureSensorReadStatus::kValid,
                FakeTemperatureSensor::kLowTemperatureCelsius);
}

void testFakeReturnsSensorNotFound() {
  FakeTemperatureSensor sensor;

  sensor.setSensorNotFound();

  assertReading(sensor, TemperatureSensorReadStatus::kSensorNotFound, 0.0F);
}

void testFakeReturnsReadError() {
  FakeTemperatureSensor sensor;

  sensor.setReadError();

  assertReading(sensor, TemperatureSensorReadStatus::kReadError, 0.0F);
}

void testFakeReturnsFailureAndRecoverySequence() {
  FakeTemperatureSensor sensor;

  assert(sensor.queueReadError());
  assert(sensor.queueSensorNotFound());
  assert(sensor.queueValidTemperature(26.0F));

  assert(sensor.remainingQueuedReadings() == 3);
  assertReading(sensor, TemperatureSensorReadStatus::kReadError, 0.0F);
  assertReading(sensor, TemperatureSensorReadStatus::kSensorNotFound, 0.0F);
  assertReading(sensor, TemperatureSensorReadStatus::kValid, 26.0F);
  assert(sensor.remainingQueuedReadings() == 0);
}

void testFakeCanRepresentFailuresSinceBootWithoutValidReading() {
  FakeTemperatureSensor sensor;

  assertReading(sensor, TemperatureSensorReadStatus::kSensorNotFound, 0.0F);
  assert(sensor.queueSensorNotFound());
  assert(sensor.queueReadError());

  assertReading(sensor, TemperatureSensorReadStatus::kSensorNotFound, 0.0F);
  assertReading(sensor, TemperatureSensorReadStatus::kReadError, 0.0F);
  assert(sensor.readCount() == 3);
}

void testQueuedReadingsFallBackToConfiguredCurrentReading() {
  FakeTemperatureSensor sensor;

  sensor.setValidTemperature(25.5F);
  assert(sensor.queueReadError());

  assertReading(sensor, TemperatureSensorReadStatus::kReadError, 0.0F);
  assertReading(sensor, TemperatureSensorReadStatus::kValid, 25.5F);
}

}  // namespace

int main() {
  testFakeReturnsConfiguredValidReading();
  testFakeReturnsHighAndLowValidReadings();
  testFakeReturnsSensorNotFound();
  testFakeReturnsReadError();
  testFakeReturnsFailureAndRecoverySequence();
  testFakeCanRepresentFailuresSinceBootWithoutValidReading();
  testQueuedReadingsFallBackToConfiguredCurrentReading();
  return 0;
}
