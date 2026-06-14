#include "storage/storage_events.h"

#include "storage/storage_schema.h"

namespace reeflow::storage {

const char* storageNamespaceForDomain(StorageDomain domain) {
  switch (domain) {
    case StorageDomain::kWifi:
      return "wifi";
    case StorageDomain::kMqtt:
      return "mqtt";
    case StorageDomain::kTemperature:
      return "temperature";
    case StorageDomain::kAto:
      return "ato";
    case StorageDomain::kTimers:
      return "timers";
    case StorageDomain::kMode:
      return "mode";
    case StorageDomain::kCalibrations:
      return "calibrations";
    case StorageDomain::kLighting:
      return "lighting";
    case StorageDomain::kNone:
      return "";
  }

  return "";
}

const char* storageKeyForDomain(StorageDomain domain) {
  return isPersistedDomain(domain) ? kStorageConfigKey : "";
}

const char* storageEventName(StorageEventType eventType) {
  switch (eventType) {
    case StorageEventType::kConfigSaved:
      return "CONFIG_SAVED";
    case StorageEventType::kConfigRestored:
      return "CONFIG_RESTORED";
    case StorageEventType::kConfigReset:
      return "CONFIG_RESET";
  }

  return "UNKNOWN_STORAGE_EVENT";
}

core::events::EventType storageCoreEventType(StorageEventType eventType) {
  switch (eventType) {
    case StorageEventType::kConfigSaved:
      return core::events::EventType::kConfigSaved;
    case StorageEventType::kConfigRestored:
      return core::events::EventType::kConfigRestored;
    case StorageEventType::kConfigReset:
      return core::events::EventType::kConfigReset;
  }

  return core::events::EventType::kConfigChanged;
}

core::events::Event makeStorageCoreEvent(StorageEventType eventType,
                                         const StorageEventData& data) {
  core::events::Event event = {};
  event.type = storageCoreEventType(eventType);
  event.configDomain = data.domain;
  event.payload = &data;
  return event;
}

StorageResult resetPersistedNamespace(StorageBackend& backend,
                                      core::events::EventBus& eventBus,
                                      StorageDomain domain,
                                      const char* namespaceName,
                                      uint32_t timestampMillis,
                                      const char* reason) {
  const StorageResult result = backend.resetNamespace(namespaceName);
  if (result == StorageResult::kResetApplied) {
    StorageEventData data = {};
    data.domain = domain;
    data.result = result;
    data.reason = reason;
    data.timestampMillis = timestampMillis;
    eventBus.publish(makeStorageCoreEvent(StorageEventType::kConfigReset,
                                          data));
  }

  return result;
}

StorageResult validatePayloadState(StoragePayloadState payloadState,
                                   uint16_t schemaVersion) {
  if (!schemaVersionIsSupported(schemaVersion)) {
    return StorageResult::kIncompatibleSchema;
  }

  switch (payloadState) {
    case StoragePayloadState::kValid:
      return StorageResult::kSuccess;
    case StoragePayloadState::kMissingSchema:
    case StoragePayloadState::kInvalidSchema:
      return StorageResult::kIncompatibleSchema;
    case StoragePayloadState::kCorruptedPayload:
      return StorageResult::kInvalidPayload;
  }

  return StorageResult::kInvalidPayload;
}

}  // namespace reeflow::storage
