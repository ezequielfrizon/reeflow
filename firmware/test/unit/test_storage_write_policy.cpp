#include <assert.h>

#include "core/events/event_bus.h"
#include "fakes/fake_storage_backend.h"
#include "storage/storage_schema.h"
#include "storage/storage_write_policy.h"

namespace {

using reeflow::core::events::ConfigDomain;
using reeflow::storage::StorageResult;
using reeflow::storage::StorageWritePolicy;
using reeflow::storage::storageKeyForDomain;
using reeflow::storage::storageNamespaceForDomain;
using reeflow::test::fakes::FakeStorageBackend;

void testEffectiveChangeMarksDomainDirty() {
  StorageWritePolicy policy;
  const uint8_t payload[] = {1};

  assert(policy.markChanged(ConfigDomain::kWifi,
                            storageNamespaceForDomain(ConfigDomain::kWifi),
                            storageKeyForDomain(ConfigDomain::kWifi),
                            {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  assert(policy.isDirty(ConfigDomain::kWifi));
  assert(policy.dirtyCount() == 1);
}

void testIdenticalValueDoesNotCreateNewDirtyChange() {
  StorageWritePolicy policy;
  const uint8_t payload[] = {1, 2};

  assert(policy.markChanged(ConfigDomain::kMqtt,
                            storageNamespaceForDomain(ConfigDomain::kMqtt),
                            storageKeyForDomain(ConfigDomain::kMqtt),
                            {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kMqtt,
                            storageNamespaceForDomain(ConfigDomain::kMqtt),
                            storageKeyForDomain(ConfigDomain::kMqtt),
                            {payload, sizeof(payload)}) ==
         StorageResult::kNoChange);
  assert(policy.dirtyCount() == 1);
}

void testMultipleChangesAreCoalescedBeforeFlush() {
  FakeStorageBackend backend;
  StorageWritePolicy policy;
  const char* namespaceName = storageNamespaceForDomain(ConfigDomain::kAto);
  const char* key = storageKeyForDomain(ConfigDomain::kAto);
  const uint8_t firstPayload[] = {1};
  const uint8_t secondPayload[] = {2};

  assert(backend.openNamespace(namespaceName) == StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kAto, namespaceName, key,
                            {firstPayload, sizeof(firstPayload)}) ==
         StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kAto, namespaceName, key,
                            {secondPayload, sizeof(secondPayload)}) ==
         StorageResult::kSuccess);

  assert(policy.flushDomain(backend, ConfigDomain::kAto) ==
         StorageResult::kSuccess);
  assert(backend.writeCount(namespaceName, key) == 1);
  const auto readResult = backend.read(namespaceName, key);
  assert(readResult.result == StorageResult::kSuccess);
  assert(readResult.payload.data[0] == 2);
}

void testRepeatedFlushWithoutChangeDoesNotWriteAgain() {
  FakeStorageBackend backend;
  StorageWritePolicy policy;
  const char* namespaceName =
      storageNamespaceForDomain(ConfigDomain::kTemperature);
  const char* key = storageKeyForDomain(ConfigDomain::kTemperature);
  const uint8_t payload[] = {7};

  assert(backend.openNamespace(namespaceName) == StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kTemperature, namespaceName, key,
                            {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  assert(policy.flushDomain(backend, ConfigDomain::kTemperature) ==
         StorageResult::kSuccess);
  assert(policy.flushDomain(backend, ConfigDomain::kTemperature) ==
         StorageResult::kNoChange);
  assert(backend.writeCount(namespaceName, key) == 1);
  assert(!policy.isDirty(ConfigDomain::kTemperature));
}

void testFlushAllOnlyWritesDirtyDomains() {
  FakeStorageBackend backend;
  StorageWritePolicy policy;
  const char* wifiNamespace = storageNamespaceForDomain(ConfigDomain::kWifi);
  const char* wifiKey = storageKeyForDomain(ConfigDomain::kWifi);
  const char* modeNamespace = storageNamespaceForDomain(ConfigDomain::kMode);
  const char* modeKey = storageKeyForDomain(ConfigDomain::kMode);
  const uint8_t wifiPayload[] = {1};
  const uint8_t modePayload[] = {2};

  assert(backend.openNamespace(wifiNamespace) == StorageResult::kSuccess);
  assert(backend.openNamespace(modeNamespace) == StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kWifi, wifiNamespace, wifiKey,
                            {wifiPayload, sizeof(wifiPayload)}) ==
         StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kMode, modeNamespace, modeKey,
                            {modePayload, sizeof(modePayload)}) ==
         StorageResult::kSuccess);
  assert(policy.markClean(ConfigDomain::kMode) == StorageResult::kSuccess);

  assert(policy.flushAll(backend) == StorageResult::kSuccess);
  assert(backend.writeCount(wifiNamespace, wifiKey) == 1);
  assert(backend.writeCount(modeNamespace, modeKey) == 0);
}

void testWriteFailureKeepsDomainDirty() {
  FakeStorageBackend backend;
  StorageWritePolicy policy;
  const char* namespaceName =
      storageNamespaceForDomain(ConfigDomain::kCalibrations);
  const char* key = storageKeyForDomain(ConfigDomain::kCalibrations);
  const uint8_t payload[] = {5};

  assert(backend.openNamespace(namespaceName) == StorageResult::kSuccess);
  assert(policy.markChanged(ConfigDomain::kCalibrations, namespaceName, key,
                            {payload, sizeof(payload)}) ==
         StorageResult::kSuccess);
  backend.simulateWriteFailure();
  assert(policy.flushDomain(backend, ConfigDomain::kCalibrations) ==
         StorageResult::kWriteFailure);
  assert(policy.isDirty(ConfigDomain::kCalibrations));
}

}  // namespace

int main() {
  testEffectiveChangeMarksDomainDirty();
  testIdenticalValueDoesNotCreateNewDirtyChange();
  testMultipleChangesAreCoalescedBeforeFlush();
  testRepeatedFlushWithoutChangeDoesNotWriteAgain();
  testFlushAllOnlyWritesDirtyDomains();
  testWriteFailureKeepsDomainDirty();
  return 0;
}
