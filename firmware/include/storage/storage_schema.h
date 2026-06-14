#pragma once

#include <stddef.h>
#include <stdint.h>

#include "storage/storage_types.h"

namespace reeflow::storage {

enum class PersistedField {
  kWifiConfig,
  kMqttConfig,
  kTemperatureLimits,
  kAtoConfig,
  kTimers,
  kCurrentMode,
  kCalibrations,
  kLightingProfiles,
  kLightingCurves,
  kLightingConfig,
};

constexpr StorageDomain kPersistedDomains[] = {
    StorageDomain::kWifi,         StorageDomain::kMqtt,
    StorageDomain::kTemperature,  StorageDomain::kAto,
    StorageDomain::kTimers,       StorageDomain::kMode,
    StorageDomain::kCalibrations, StorageDomain::kLighting,
};

constexpr PersistedField kPersistedFields[] = {
    PersistedField::kWifiConfig,
    PersistedField::kMqttConfig,
    PersistedField::kTemperatureLimits,
    PersistedField::kAtoConfig,
    PersistedField::kTimers,
    PersistedField::kCurrentMode,
    PersistedField::kCalibrations,
    PersistedField::kLightingProfiles,
    PersistedField::kLightingCurves,
    PersistedField::kLightingConfig,
};

constexpr const char* kStorageConfigKey = "config";

constexpr size_t persistedDomainCount() {
  return sizeof(kPersistedDomains) / sizeof(kPersistedDomains[0]);
}

constexpr size_t persistedFieldCount() {
  return sizeof(kPersistedFields) / sizeof(kPersistedFields[0]);
}

inline bool isPersistedDomain(StorageDomain domain) {
  for (StorageDomain persistedDomain : kPersistedDomains) {
    if (persistedDomain == domain) {
      return true;
    }
  }

  return false;
}

inline bool isPersistedField(PersistedField field) {
  for (PersistedField persistedField : kPersistedFields) {
    if (persistedField == field) {
      return true;
    }
  }

  return false;
}

constexpr bool schemaVersionIsSupported(uint16_t version) {
  return version == kStorageSchemaVersion;
}

const char* storageNamespaceForDomain(StorageDomain domain);
const char* storageKeyForDomain(StorageDomain domain);
StorageResult validatePayloadState(StoragePayloadState payloadState,
                                   uint16_t schemaVersion);

}  // namespace reeflow::storage
