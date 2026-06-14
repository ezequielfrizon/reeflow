#include <assert.h>
#include <string.h>

#include "core/events/event_bus.h"
#include "fakes/fake_storage_backend.h"
#include "storage/storage_events.h"
#include "storage/storage_schema.h"
#include "storage/storage_service.h"

namespace {

using reeflow::core::events::ConfigDomain;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::storage::PersistedField;
using reeflow::storage::StorageEventData;
using reeflow::storage::StorageEventType;
using reeflow::storage::StoragePayload;
using reeflow::storage::StoragePayloadState;
using reeflow::storage::StorageResult;
using reeflow::storage::StorageService;
using reeflow::storage::kStorageSchemaVersion;
using reeflow::storage::isPersistedDomain;
using reeflow::storage::isPersistedField;
using reeflow::storage::makeStorageCoreEvent;
using reeflow::storage::schemaVersionIsSupported;
using reeflow::storage::storageCoreEventType;
using reeflow::storage::storageEventName;
using reeflow::storage::validatePayloadState;
using reeflow::test::fakes::FakeStorageBackend;

struct Recorder {
  Event event;
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  Recorder* recorder = static_cast<Recorder*>(context);
  recorder->event = event;
  recorder->count += 1;
  return true;
}

void testPersistedDomainsMatchPhase9Scope() {
  assert(isPersistedDomain(ConfigDomain::kWifi));
  assert(isPersistedDomain(ConfigDomain::kMqtt));
  assert(isPersistedDomain(ConfigDomain::kTemperature));
  assert(isPersistedDomain(ConfigDomain::kAto));
  assert(isPersistedDomain(ConfigDomain::kTimers));
  assert(isPersistedDomain(ConfigDomain::kMode));
  assert(isPersistedDomain(ConfigDomain::kCalibrations));
  assert(isPersistedDomain(ConfigDomain::kLighting));
  assert(!isPersistedDomain(ConfigDomain::kNone));
}

void testPersistedFieldsExcludeVolatileRuntimeState() {
  assert(isPersistedField(PersistedField::kWifiConfig));
  assert(isPersistedField(PersistedField::kMqttConfig));
  assert(isPersistedField(PersistedField::kTemperatureLimits));
  assert(isPersistedField(PersistedField::kAtoConfig));
  assert(isPersistedField(PersistedField::kTimers));
  assert(isPersistedField(PersistedField::kCurrentMode));
  assert(isPersistedField(PersistedField::kCalibrations));
  assert(isPersistedField(PersistedField::kLightingProfiles));
  assert(isPersistedField(PersistedField::kLightingCurves));
  assert(isPersistedField(PersistedField::kLightingConfig));
}

void testSchemaVersionAndPayloadValidation() {
  assert(kStorageSchemaVersion == 1);
  assert(schemaVersionIsSupported(1));
  assert(!schemaVersionIsSupported(2));
  assert(validatePayloadState(StoragePayloadState::kValid, 1) ==
         StorageResult::kSuccess);
  assert(validatePayloadState(StoragePayloadState::kMissingSchema, 1) ==
         StorageResult::kIncompatibleSchema);
  assert(validatePayloadState(StoragePayloadState::kInvalidSchema, 1) ==
         StorageResult::kIncompatibleSchema);
  assert(validatePayloadState(StoragePayloadState::kCorruptedPayload, 1) ==
         StorageResult::kInvalidPayload);
  assert(validatePayloadState(StoragePayloadState::kValid, 99) ==
         StorageResult::kIncompatibleSchema);
}

void testStorageEventsMapToLocalEventBus() {
  assert(strcmp(storageEventName(StorageEventType::kConfigSaved),
                "CONFIG_SAVED") == 0);
  assert(strcmp(storageEventName(StorageEventType::kConfigRestored),
                "CONFIG_RESTORED") == 0);
  assert(strcmp(storageEventName(StorageEventType::kConfigReset),
                "CONFIG_RESET") == 0);
  assert(storageCoreEventType(StorageEventType::kConfigSaved) ==
         EventType::kConfigSaved);
  assert(storageCoreEventType(StorageEventType::kConfigRestored) ==
         EventType::kConfigRestored);
  assert(storageCoreEventType(StorageEventType::kConfigReset) ==
         EventType::kConfigReset);

  EventBus bus;
  Recorder recorder = {};
  bus.subscribe(EventType::kConfigSaved, recordEvent, &recorder);

  StorageEventData data = {};
  data.domain = ConfigDomain::kWifi;
  data.result = StorageResult::kSuccess;
  data.reason = "unit";
  data.timestampMillis = 123;

  const auto result =
      bus.publish(makeStorageCoreEvent(StorageEventType::kConfigSaved, data));

  assert(result.deliveredCount == 1);
  assert(result.failedCount == 0);
  assert(recorder.count == 1);
  assert(recorder.event.type == EventType::kConfigSaved);
  assert(recorder.event.configDomain == ConfigDomain::kWifi);
  assert(recorder.event.payload == &data);
}

void testFakeStorageBackendReadWriteMissingAndCorruptedPayload() {
  FakeStorageBackend backend;
  StorageService service(backend);
  assert(&service.backend() == &backend);

  assert(backend.read("wifi", "config").result ==
         StorageResult::kNamespaceNotFound);
  assert(backend.openNamespace("wifi") == StorageResult::kSuccess);
  assert(backend.namespaceExists("wifi"));
  assert(backend.read("wifi", "config").result ==
         StorageResult::kKeyNotFound);

  const uint8_t payload[] = {1, 2, 3, 4};
  assert(backend.write("wifi", "config", {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  assert(backend.keyExists("wifi", "config"));
  assert(backend.writeCount("wifi", "config") == 1);
  assert(strcmp(backend.lastNamespace(), "wifi") == 0);
  assert(strcmp(backend.lastKey(), "config") == 0);

  const auto readResult = backend.read("wifi", "config");
  assert(readResult.result == StorageResult::kSuccess);
  assert(readResult.payload.size == sizeof(payload));
  assert(memcmp(readResult.payload.data, payload, sizeof(payload)) == 0);

  backend.simulateCorruptedPayload("wifi", "config");
  assert(backend.read("wifi", "config").result ==
         StorageResult::kInvalidPayload);
}

void testFakeStorageBackendFailuresAndReset() {
  FakeStorageBackend backend;
  assert(backend.openNamespace("mqtt") == StorageResult::kSuccess);

  const uint8_t payload[] = {9};
  backend.simulateWriteFailure();
  assert(backend.write("mqtt", "config", {payload, sizeof(payload)}) ==
         StorageResult::kWriteFailure);
  assert(backend.writeCount("mqtt", "config") == 0);

  assert(backend.write("mqtt", "config", {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);

  backend.simulateReadFailure();
  assert(backend.read("mqtt", "config").result ==
         StorageResult::kReadFailure);

  assert(backend.remove("mqtt", "config") == StorageResult::kSuccess);
  assert(backend.read("mqtt", "config").result == StorageResult::kKeyNotFound);

  assert(backend.write("mqtt", "config", {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  assert(backend.resetNamespace("mqtt") == StorageResult::kResetApplied);
  assert(backend.read("mqtt", "config").result == StorageResult::kKeyNotFound);
}

}  // namespace

int main() {
  testPersistedDomainsMatchPhase9Scope();
  testPersistedFieldsExcludeVolatileRuntimeState();
  testSchemaVersionAndPayloadValidation();
  testStorageEventsMapToLocalEventBus();
  testFakeStorageBackendReadWriteMissingAndCorruptedPayload();
  testFakeStorageBackendFailuresAndReset();
  return 0;
}
