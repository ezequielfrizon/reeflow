#pragma once

#include "storage/storage_types.h"

namespace reeflow::storage {

class StorageBackend {
 public:
  virtual ~StorageBackend() = default;

  virtual StorageResult openNamespace(const char* name) = 0;
  virtual bool namespaceExists(const char* name) const = 0;
  virtual bool keyExists(const char* namespaceName, const char* key) const = 0;
  virtual StorageReadResult read(const char* namespaceName,
                                 const char* key) const = 0;
  virtual StorageResult write(const char* namespaceName, const char* key,
                              StoragePayload payload) = 0;
  virtual StorageResult remove(const char* namespaceName,
                               const char* key) = 0;
  virtual StorageResult resetNamespace(const char* namespaceName) = 0;
};

}  // namespace reeflow::storage
