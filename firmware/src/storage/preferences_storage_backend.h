#pragma once

#include <stddef.h>
#include <stdint.h>

#include "storage/storage_backend.h"

namespace reeflow::storage {

class PreferencesStorageBackend final : public StorageBackend {
 public:
  StorageResult openNamespace(const char* name) override;
  bool namespaceExists(const char* name) const override;
  bool keyExists(const char* namespaceName, const char* key) const override;
  StorageReadResult read(const char* namespaceName,
                         const char* key) const override;
  StorageResult write(const char* namespaceName, const char* key,
                      StoragePayload payload) override;
  StorageResult remove(const char* namespaceName, const char* key) override;
  StorageResult resetNamespace(const char* namespaceName) override;

 private:
  static bool validName(const char* value, size_t maxLength);
  mutable uint8_t readBuffer_[kStorageMaxPayloadSize] = {};
};

}  // namespace reeflow::storage
