#include <assert.h>

#include "contracts/i2c_contract.h"
#include "drivers/i2c/i2c_bus.h"
#include "drivers/sensors/vl6180x/vl6180x_level_sensor.h"
#include "fakes/fake_i2c_bus.h"
#include "modules/water_level/water_level_sensor.h"

namespace {

using reeflow::drivers::I2cOperationStatus;
using reeflow::drivers::sensors::Vl6180xLevelSensor;
using reeflow::drivers::sensors::vl6180xLevelSensorI2cAddress;
using reeflow::drivers::sensors::vl6180xLevelSensorSclGpio;
using reeflow::drivers::sensors::vl6180xLevelSensorSdaGpio;
using reeflow::modules::water_level::WaterLevelSensorReadStatus;
using reeflow::test::fakes::FakeI2cBus;

constexpr uint16_t kVl6180xSysrangeStartRegister = 0x0018;
constexpr uint8_t kVl6180xStartSingleShotRange = 0x01;
constexpr uint16_t kVl6180xResultRangeValueRegister = 0x0062;

void assertReading(Vl6180xLevelSensor& sensor,
                   WaterLevelSensorReadStatus expectedStatus,
                   int16_t expectedLogicalLevel) {
  const auto reading = sensor.readLevel();

  assert(reading.status == expectedStatus);
  assert(reading.logicalLevel == expectedLogicalLevel);
}

void testAdapterUsesOfficialPinoutAndAddressContracts() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);

  assert(vl6180xLevelSensorSdaGpio() == 21);
  assert(vl6180xLevelSensorSclGpio() == 22);
  assert(vl6180xLevelSensorI2cAddress() ==
         reeflow::contracts::kVl6180xI2cContract.expectedAddress);
  assert(sensor.sdaGpio() == 21);
  assert(sensor.sclGpio() == 22);
  assert(sensor.i2cAddress() == 0x29);
}

void testFakeI2cCanRepresentAddressFound() {
  FakeI2cBus bus;

  bus.setDevicePresent(true);

  assert(bus.probe(0x29));
  assert(bus.probeCount() == 1);
  assert(bus.lastProbedAddress() == 0x29);
}

void testFakeI2cCanRepresentAddressAbsent() {
  FakeI2cBus bus;

  bus.setDevicePresent(false);

  assert(!bus.probe(0x29));
  assert(bus.probeCount() == 1);
}

void testAdapterTranslatesValidRawRangeToLogicalLevel() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);
  bus.setRegister(kVl6180xResultRangeValueRegister, 55);

  assertReading(sensor, WaterLevelSensorReadStatus::kValid, 55);
  assert(bus.beginCount() == 1);
  assert(bus.lastSdaGpio() == 21);
  assert(bus.lastSclGpio() == 22);
  assert(bus.lastProbedAddress() == 0x29);
  assert(bus.lastWrittenRegister() == kVl6180xSysrangeStartRegister);
  assert(bus.lastWrittenValue() == kVl6180xStartSingleShotRange);
  assert(bus.lastReadRegister() == kVl6180xResultRangeValueRegister);
}

void testAdapterDoesNotBeginBusMoreThanOnce() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);
  bus.setRegister(kVl6180xResultRangeValueRegister, 40);

  assertReading(sensor, WaterLevelSensorReadStatus::kValid, 40);
  assertReading(sensor, WaterLevelSensorReadStatus::kValid, 40);
  assert(bus.beginCount() == 1);
  assert(bus.probeCount() == 2);
}

void testAdapterTranslatesSensorAbsent() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);

  bus.setDevicePresent(false);

  assertReading(sensor, WaterLevelSensorReadStatus::kSensorNotFound, 0);
  assert(bus.writeCount() == 0);
  assert(bus.readCount() == 0);
}

void testAdapterTranslatesWriteError() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);

  bus.setNextWriteStatus(I2cOperationStatus::kWriteError);

  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
  assert(bus.writeCount() == 1);
  assert(bus.readCount() == 0);
}

void testAdapterTranslatesReadError() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);

  bus.setNextReadStatus(I2cOperationStatus::kReadError);

  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
  assert(bus.writeCount() == 1);
  assert(bus.readCount() == 1);
}

void testAdapterTranslatesTimeoutAsReadError() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);

  bus.setNextReadStatus(I2cOperationStatus::kTimeout);

  assertReading(sensor, WaterLevelSensorReadStatus::kReadError, 0);
}

void testAdapterTranslatesAddressFailureDuringRegisterAccess() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);

  bus.setNextWriteStatus(I2cOperationStatus::kAddressNotFound);

  assertReading(sensor, WaterLevelSensorReadStatus::kSensorNotFound, 0);
}

void testAdapterTranslatesOutOfRangeRawRange() {
  FakeI2cBus bus;
  Vl6180xLevelSensor sensor(bus);
  bus.setRegister(kVl6180xResultRangeValueRegister, 150);

  assertReading(sensor, WaterLevelSensorReadStatus::kOutOfRange, 150);
}

void testFakeI2cSupportsReadBlockContract() {
  FakeI2cBus bus;
  uint8_t values[3] = {};

  bus.setRegister(0x0100, 1);
  bus.setRegister(0x0101, 2);
  bus.setRegister(0x0102, 3);

  const I2cOperationStatus status = bus.readBlock(0x29, 0x0100, values, 3);

  assert(status == I2cOperationStatus::kOk);
  assert(values[0] == 1);
  assert(values[1] == 2);
  assert(values[2] == 3);
  assert(bus.readBlockCount() == 1);
}

}  // namespace

int main() {
  testAdapterUsesOfficialPinoutAndAddressContracts();
  testFakeI2cCanRepresentAddressFound();
  testFakeI2cCanRepresentAddressAbsent();
  testAdapterTranslatesValidRawRangeToLogicalLevel();
  testAdapterDoesNotBeginBusMoreThanOnce();
  testAdapterTranslatesSensorAbsent();
  testAdapterTranslatesWriteError();
  testAdapterTranslatesReadError();
  testAdapterTranslatesTimeoutAsReadError();
  testAdapterTranslatesAddressFailureDuringRegisterAccess();
  testAdapterTranslatesOutOfRangeRawRange();
  testFakeI2cSupportsReadBlockContract();
  return 0;
}
