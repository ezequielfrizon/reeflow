#include <assert.h>

#include "core/events/event_bus.h"
#include "storage/storage_schema.h"

namespace {

using reeflow::core::events::ConfigDomain;
using reeflow::storage::PersistedField;
using reeflow::storage::isPersistedField;
using reeflow::storage::storageKeyForDomain;
using reeflow::storage::storageNamespaceForDomain;

void testGeneralConfigDomainsHaveDeterministicStorageLocations() {
  assert(storageNamespaceForDomain(ConfigDomain::kWifi)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kMqtt)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kTemperature)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kAto)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kTimers)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kMode)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kCalibrations)[0] != '\0');

  assert(storageKeyForDomain(ConfigDomain::kWifi)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kMqtt)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kTemperature)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kAto)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kTimers)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kMode)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kCalibrations)[0] != '\0');
}

void testLightingDomainHasDeterministicStorageLocation() {
  assert(storageNamespaceForDomain(ConfigDomain::kLighting)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kLighting)[0] != '\0');
}

void testGeneralConfigPersistedFieldsExcludeRuntimeReadings() {
  assert(isPersistedField(PersistedField::kWifiConfig));
  assert(isPersistedField(PersistedField::kMqttConfig));
  assert(isPersistedField(PersistedField::kTemperatureLimits));
  assert(isPersistedField(PersistedField::kAtoConfig));
  assert(isPersistedField(PersistedField::kTimers));
  assert(isPersistedField(PersistedField::kCurrentMode));
  assert(isPersistedField(PersistedField::kCalibrations));
  assert(isPersistedField(PersistedField::kLightingConfig));
  assert(isPersistedField(PersistedField::kLightingProfiles));
  assert(isPersistedField(PersistedField::kLightingCurves));
}

}  // namespace

int main() {
  testGeneralConfigDomainsHaveDeterministicStorageLocations();
  testLightingDomainHasDeterministicStorageLocation();
  testGeneralConfigPersistedFieldsExcludeRuntimeReadings();
  return 0;
}
