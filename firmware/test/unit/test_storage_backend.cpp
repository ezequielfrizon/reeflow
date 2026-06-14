#include <assert.h>

#include "core/events/event_bus.h"
#include "fakes/fake_storage_backend.h"
#include "storage/storage_events.h"
#include "storage/storage_schema.h"
#include "storage/storage_types.h"

namespace {

using reeflow::core::events::ConfigDomain;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::storage::StorageResult;
using reeflow::storage::resetPersistedNamespace;
using reeflow::storage::storageKeyForDomain;
using reeflow::storage::storageNamespaceForDomain;
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

void testDeterministicNamespacesAndKeys() {
  assert(storageNamespaceForDomain(ConfigDomain::kWifi)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kMqtt)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kTemperature)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kAto)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kTimers)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kMode)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kCalibrations)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kLighting)[0] != '\0');
  assert(storageNamespaceForDomain(ConfigDomain::kNone)[0] == '\0');
  assert(storageKeyForDomain(ConfigDomain::kWifi)[0] != '\0');
  assert(storageKeyForDomain(ConfigDomain::kNone)[0] == '\0');
}

void testFakeBackendReadWriteAndMissingData() {
  FakeStorageBackend backend;
  const char* namespaceName = storageNamespaceForDomain(ConfigDomain::kWifi);
  const char* key = storageKeyForDomain(ConfigDomain::kWifi);

  assert(backend.read(namespaceName, key).result ==
         StorageResult::kNamespaceNotFound);
  assert(backend.openNamespace(namespaceName) == StorageResult::kSuccess);
  assert(backend.read(namespaceName, key).result ==
         StorageResult::kKeyNotFound);

  const uint8_t payload[] = {1, 2, 3};
  assert(backend.write(namespaceName, key, {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  assert(backend.read(namespaceName, key).result == StorageResult::kSuccess);
  assert(backend.writeCount(namespaceName, key) == 1);
}

void testFakeBackendFailuresAndCorruption() {
  FakeStorageBackend backend;
  const char* namespaceName = storageNamespaceForDomain(ConfigDomain::kMqtt);
  const char* key = storageKeyForDomain(ConfigDomain::kMqtt);
  const uint8_t payload[] = {4};

  assert(backend.openNamespace(namespaceName) == StorageResult::kSuccess);
  backend.simulateWriteFailure();
  assert(backend.write(namespaceName, key, {payload, sizeof(payload)}) ==
         StorageResult::kWriteFailure);

  assert(backend.write(namespaceName, key, {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  backend.simulateReadFailure();
  assert(backend.read(namespaceName, key).result ==
         StorageResult::kReadFailure);

  backend.simulateCorruptedPayload(namespaceName, key);
  assert(backend.read(namespaceName, key).result ==
         StorageResult::kInvalidPayload);
}

void testResetPublishesLocalConfigResetEvent() {
  FakeStorageBackend backend;
  EventBus bus;
  Recorder recorder = {};
  const char* namespaceName = storageNamespaceForDomain(ConfigDomain::kTimers);
  const char* key = storageKeyForDomain(ConfigDomain::kTimers);
  const uint8_t payload[] = {9};

  bus.subscribe(EventType::kConfigReset, recordEvent, &recorder);
  assert(backend.openNamespace(namespaceName) == StorageResult::kSuccess);
  assert(backend.write(namespaceName, key, {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);

  assert(resetPersistedNamespace(backend, bus, ConfigDomain::kTimers,
                                 namespaceName, 44, "unit-reset") ==
         StorageResult::kResetApplied);
  assert(recorder.count == 1);
  assert(recorder.event.type == EventType::kConfigReset);
  assert(recorder.event.configDomain == ConfigDomain::kTimers);
  assert(backend.read(namespaceName, key).result == StorageResult::kKeyNotFound);
}

}  // namespace

int main() {
  testDeterministicNamespacesAndKeys();
  testFakeBackendReadWriteAndMissingData();
  testFakeBackendFailuresAndCorruption();
  testResetPublishesLocalConfigResetEvent();
  return 0;
}
