#include "drivers/i2c/i2c_diagnostic_bus.h"

namespace reeflow::drivers {

I2cScanResult runI2cDiagnosticScan(I2cDiagnosticBus& bus, uint8_t sdaGpio,
                                   uint8_t sclGpio) {
  I2cScanResult result = {};
  result.sdaGpio = sdaGpio;
  result.sclGpio = sclGpio;

  bus.begin(sdaGpio, sclGpio);

  for (uint8_t address = kI2cScanFirstAddress; address <= kI2cScanLastAddress;
       ++address) {
    if (!bus.probe(address)) {
      continue;
    }

    if (result.deviceCount >= kI2cScanMaxDevices) {
      result.overflow = true;
      continue;
    }

    result.addresses[result.deviceCount] = address;
    ++result.deviceCount;
  }

  return result;
}

}  // namespace reeflow::drivers
