#pragma once

#include <stddef.h>
#include <stdint.h>

#include "storage/storage_backend.h"
#include "storage/storage_types.h"

namespace reeflow::storage {

struct StorageWritePolicyDecision {
  StorageDomain domain;
  StorageResult result;
};

class StorageWritePolicy {
 public:
  StorageResult markChanged(StorageDomain domain, const char* namespaceName,
                            const char* key, StoragePayload payload);
  StorageResult markClean(StorageDomain domain);
  StorageResult flushDomain(StorageBackend& backend, StorageDomain domain);
  StorageResult flushAll(StorageBackend& backend);

  bool isDirty(StorageDomain domain) const;
  uint8_t dirtyCount() const;

 private:
  struct DomainState {
    bool used;
    bool dirty;
    StorageDomain domain;
    char namespaceName[kStorageNamespaceMaxLength + 1];
    char key[kStorageKeyMaxLength + 1];
    uint8_t payload[kStorageMaxPayloadSize];
    size_t payloadSize;
  };

  DomainState* findOrCreateState(StorageDomain domain);
  const DomainState* findState(StorageDomain domain) const;

  DomainState states_[8] = {};
};

}  // namespace reeflow::storage
