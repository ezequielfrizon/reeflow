#include "diagnostics/i2c/vl6180x_i2c_address_diagnostics.h"

#include "contracts/i2c_contract.h"

namespace reeflow::diagnostics {

Vl6180xI2cAddressComparison compareVl6180xI2cAddress(
    const drivers::I2cScanResult& scanResult) {
  Vl6180xI2cAddressComparison comparison = {
      Vl6180xI2cAddressStatus::kNotFound,
      contracts::kVl6180xI2cContract.expectedAddress,
      0,
      false,
  };

  for (size_t index = 0; index < scanResult.deviceCount; ++index) {
    const uint8_t foundAddress = scanResult.addresses[index];

    if (foundAddress == comparison.expectedAddress) {
      comparison.status = Vl6180xI2cAddressStatus::kMatch;
      comparison.foundAddress = foundAddress;
      comparison.hasFoundAddress = true;
      return comparison;
    }

    if (!comparison.hasFoundAddress) {
      comparison.status = Vl6180xI2cAddressStatus::kMismatch;
      comparison.foundAddress = foundAddress;
      comparison.hasFoundAddress = true;
    }
  }

  return comparison;
}

const char* vl6180xI2cAddressStatusToText(Vl6180xI2cAddressStatus status) {
  switch (status) {
    case Vl6180xI2cAddressStatus::kMatch:
      return "MATCH";
    case Vl6180xI2cAddressStatus::kNotFound:
      return "NOT_FOUND";
    case Vl6180xI2cAddressStatus::kMismatch:
      return "MISMATCH";
  }

  return "NOT_FOUND";
}

}  // namespace reeflow::diagnostics
