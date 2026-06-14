#pragma once

#include "core/events/event_bus.h"
#include "storage/storage_backend.h"
#include "storage/storage_types.h"

namespace reeflow::storage {

enum class StorageEventType {
  kConfigSaved,
  kConfigRestored,
  kConfigReset,
};

const char* storageEventName(StorageEventType eventType);
core::events::EventType storageCoreEventType(StorageEventType eventType);
core::events::Event makeStorageCoreEvent(StorageEventType eventType,
                                         const StorageEventData& data);
StorageResult resetPersistedNamespace(StorageBackend& backend,
                                      core::events::EventBus& eventBus,
                                      StorageDomain domain,
                                      const char* namespaceName,
                                      uint32_t timestampMillis,
                                      const char* reason);

}  // namespace reeflow::storage
