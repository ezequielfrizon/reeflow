#include "drivers/sensors/vl6180x/vl6180x_level_sensor.h"

#include <stddef.h>
#include <string.h>

#include "contracts/hardware_pins.h"
#include "contracts/i2c_contract.h"
#include "modules/water_level/water_level_config.h"

namespace reeflow::drivers::sensors {
namespace {

constexpr uint16_t kVl6180xSysrangeStartRegister = 0x0018;
constexpr uint8_t kVl6180xStartSingleShotRange = 0x01;
constexpr uint16_t kVl6180xResultRangeValueRegister = 0x0062;

bool pinNameIs(const reeflow::contracts::HardwarePin& pin, const char* name) {
  return strcmp(pin.canonicalName, name) == 0;
}

uint8_t findI2cPinGpio(const char* canonicalName) {
  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role == reeflow::contracts::HardwarePinRole::kI2cBus &&
        pinNameIs(pin, canonicalName)) {
      return pin.gpio;
    }
  }

  return 0;
}

modules::water_level::WaterLevelSensorReading mapI2cFailure(
    I2cOperationStatus status) {
  if (status == I2cOperationStatus::kAddressNotFound) {
    return modules::water_level::makeSensorNotFoundWaterLevelReading();
  }

  return modules::water_level::makeWaterLevelReadError();
}

modules::water_level::WaterLevelSensorReading normalizeRawRange(
    uint8_t rawRange) {
  if (!modules::water_level::isCanonicalWaterLevel(
          static_cast<int16_t>(rawRange))) {
    return modules::water_level::makeOutOfRangeWaterLevelReading(rawRange);
  }

  return modules::water_level::makeValidWaterLevelReading(rawRange);
}

}  // namespace

uint8_t vl6180xLevelSensorSdaGpio() {
  return findI2cPinGpio("VL6180X SDA");
}

uint8_t vl6180xLevelSensorSclGpio() {
  return findI2cPinGpio("VL6180X SCL");
}

uint8_t vl6180xLevelSensorI2cAddress() {
  return reeflow::contracts::kVl6180xI2cContract.expectedAddress;
}

Vl6180xLevelSensor::Vl6180xLevelSensor(I2cBus& bus, uint8_t sdaGpio,
                                       uint8_t sclGpio, uint8_t i2cAddress)
    : bus_(bus),
      sdaGpio_(sdaGpio),
      sclGpio_(sclGpio),
      i2cAddress_(i2cAddress),
      initialized_(false) {}

modules::water_level::WaterLevelSensorReading Vl6180xLevelSensor::readLevel() {
  beginIfNeeded();

  if (!bus_.probe(i2cAddress_)) {
    return modules::water_level::makeSensorNotFoundWaterLevelReading();
  }

  const I2cOperationStatus startStatus = bus_.writeRegister(
      i2cAddress_, kVl6180xSysrangeStartRegister, kVl6180xStartSingleShotRange);
  if (startStatus != I2cOperationStatus::kOk) {
    return mapI2cFailure(startStatus);
  }

  uint8_t rawRange = 0;
  const I2cOperationStatus readStatus =
      bus_.readRegister(i2cAddress_, kVl6180xResultRangeValueRegister,
                        rawRange);
  if (readStatus != I2cOperationStatus::kOk) {
    return mapI2cFailure(readStatus);
  }

  return normalizeRawRange(rawRange);
}

uint8_t Vl6180xLevelSensor::sdaGpio() const {
  return sdaGpio_;
}

uint8_t Vl6180xLevelSensor::sclGpio() const {
  return sclGpio_;
}

uint8_t Vl6180xLevelSensor::i2cAddress() const {
  return i2cAddress_;
}

void Vl6180xLevelSensor::beginIfNeeded() {
  if (initialized_) {
    return;
  }

  bus_.begin(sdaGpio_, sclGpio_);
  initialized_ = true;
}

}  // namespace reeflow::drivers::sensors
