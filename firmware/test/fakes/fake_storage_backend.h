#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "storage/storage_backend.h"

namespace reeflow::test::fakes {

class FakeStorageBackend final : public reeflow::storage::StorageBackend {
 public:
  static constexpr size_t kMaxNamespaces = 8;
  static constexpr size_t kMaxEntries = 16;
  static constexpr size_t kMaxPayloadSize =
      reeflow::storage::kStorageMaxPayloadSize;

  reeflow::storage::StorageResult openNamespace(const char* name) override {
    if (!validName(name)) {
      return reeflow::storage::StorageResult::kWriteFailure;
    }

    Namespace* existingNamespace = findNamespace(name);
    if (existingNamespace != nullptr) {
      existingNamespace->exists = true;
      return reeflow::storage::StorageResult::kSuccess;
    }

    for (Namespace& namespaceEntry : namespaces_) {
      if (!namespaceEntry.used) {
        namespaceEntry.used = true;
        namespaceEntry.exists = true;
        copyName(namespaceEntry.name, sizeof(namespaceEntry.name), name);
        return reeflow::storage::StorageResult::kSuccess;
      }
    }

    return reeflow::storage::StorageResult::kWriteFailure;
  }

  bool namespaceExists(const char* name) const override {
    const Namespace* namespaceEntry = findNamespace(name);
    return namespaceEntry != nullptr && namespaceEntry->exists;
  }

  bool keyExists(const char* namespaceName, const char* key) const override {
    const Entry* entry = findEntry(namespaceName, key);
    return entry != nullptr && entry->exists;
  }

  reeflow::storage::StorageReadResult read(
      const char* namespaceName, const char* key) const override {
    if (failNextRead_) {
      failNextRead_ = false;
      return {reeflow::storage::StorageResult::kReadFailure, {}};
    }

    const Namespace* namespaceEntry = findNamespace(namespaceName);
    if (namespaceEntry == nullptr || !namespaceEntry->exists) {
      return {reeflow::storage::StorageResult::kNamespaceNotFound, {}};
    }

    const Entry* entry = findEntry(namespaceName, key);
    if (entry == nullptr || !entry->exists) {
      return {reeflow::storage::StorageResult::kKeyNotFound, {}};
    }

    if (entry->corrupted) {
      return {reeflow::storage::StorageResult::kInvalidPayload,
              {entry->payload, entry->payloadSize}};
    }

    return {reeflow::storage::StorageResult::kSuccess,
            {entry->payload, entry->payloadSize}};
  }

  reeflow::storage::StorageResult write(
      const char* namespaceName, const char* key,
      reeflow::storage::StoragePayload payload) override {
    if (failNextWrite_) {
      failNextWrite_ = false;
      return reeflow::storage::StorageResult::kWriteFailure;
    }

    if (!namespaceExists(namespaceName)) {
      return reeflow::storage::StorageResult::kNamespaceNotFound;
    }

    if (payload.size > kMaxPayloadSize ||
        (payload.size > 0 && payload.data == nullptr)) {
      return reeflow::storage::StorageResult::kInvalidPayload;
    }

    Entry* entry = findOrCreateEntry(namespaceName, key);
    if (entry == nullptr) {
      return reeflow::storage::StorageResult::kWriteFailure;
    }

    if (payload.size > 0) {
      memcpy(entry->payload, payload.data, payload.size);
    }
    entry->payloadSize = payload.size;
    entry->exists = true;
    entry->corrupted = false;
    entry->writeCount += 1;
    copyName(lastNamespace_, sizeof(lastNamespace_), namespaceName);
    copyName(lastKey_, sizeof(lastKey_), key);
    return reeflow::storage::StorageResult::kSuccess;
  }

  reeflow::storage::StorageResult remove(const char* namespaceName,
                                         const char* key) override {
    if (!namespaceExists(namespaceName)) {
      return reeflow::storage::StorageResult::kNamespaceNotFound;
    }

    Entry* entry = findEntry(namespaceName, key);
    if (entry == nullptr || !entry->exists) {
      return reeflow::storage::StorageResult::kKeyNotFound;
    }

    entry->exists = false;
    entry->payloadSize = 0;
    entry->corrupted = false;
    return reeflow::storage::StorageResult::kSuccess;
  }

  reeflow::storage::StorageResult resetNamespace(
      const char* namespaceName) override {
    Namespace* namespaceEntry = findNamespace(namespaceName);
    if (namespaceEntry == nullptr || !namespaceEntry->exists) {
      return reeflow::storage::StorageResult::kNamespaceNotFound;
    }

    for (Entry& entry : entries_) {
      if (entry.used && sameText(entry.namespaceName, namespaceName)) {
        entry.exists = false;
        entry.payloadSize = 0;
        entry.corrupted = false;
      }
    }

    return reeflow::storage::StorageResult::kResetApplied;
  }

  void simulateReadFailure() { failNextRead_ = true; }
  void simulateWriteFailure() { failNextWrite_ = true; }

  void simulateCorruptedPayload(const char* namespaceName, const char* key) {
    Entry* entry = findEntry(namespaceName, key);
    if (entry != nullptr) {
      entry->corrupted = true;
    }
  }

  uint16_t writeCount(const char* namespaceName, const char* key) const {
    const Entry* entry = findEntry(namespaceName, key);
    return entry == nullptr ? 0 : entry->writeCount;
  }

  const char* lastNamespace() const { return lastNamespace_; }
  const char* lastKey() const { return lastKey_; }

 private:
  struct Namespace {
    bool used;
    bool exists;
    char name[reeflow::storage::kStorageNamespaceMaxLength + 1];
  };

  struct Entry {
    bool used;
    bool exists;
    bool corrupted;
    char namespaceName[reeflow::storage::kStorageNamespaceMaxLength + 1];
    char key[reeflow::storage::kStorageKeyMaxLength + 1];
    uint8_t payload[kMaxPayloadSize];
    size_t payloadSize;
    uint16_t writeCount;
  };

  static bool validName(const char* value) {
    return value != nullptr && value[0] != '\0';
  }

  static bool sameText(const char* left, const char* right) {
    return strcmp(left, right) == 0;
  }

  static void copyName(char* destination, size_t capacity,
                       const char* source) {
    strncpy(destination, source, capacity - 1);
    destination[capacity - 1] = '\0';
  }

  Namespace* findNamespace(const char* name) {
    return const_cast<Namespace*>(
        static_cast<const FakeStorageBackend&>(*this).findNamespace(name));
  }

  const Namespace* findNamespace(const char* name) const {
    if (!validName(name)) {
      return nullptr;
    }

    for (const Namespace& namespaceEntry : namespaces_) {
      if (namespaceEntry.used && sameText(namespaceEntry.name, name)) {
        return &namespaceEntry;
      }
    }

    return nullptr;
  }

  Entry* findEntry(const char* namespaceName, const char* key) {
    return const_cast<Entry*>(
        static_cast<const FakeStorageBackend&>(*this).findEntry(namespaceName,
                                                                key));
  }

  const Entry* findEntry(const char* namespaceName, const char* key) const {
    if (!validName(namespaceName) || !validName(key)) {
      return nullptr;
    }

    for (const Entry& entry : entries_) {
      if (entry.used && sameText(entry.namespaceName, namespaceName) &&
          sameText(entry.key, key)) {
        return &entry;
      }
    }

    return nullptr;
  }

  Entry* findOrCreateEntry(const char* namespaceName, const char* key) {
    Entry* existingEntry = findEntry(namespaceName, key);
    if (existingEntry != nullptr) {
      return existingEntry;
    }

    if (!validName(key)) {
      return nullptr;
    }

    for (Entry& entry : entries_) {
      if (!entry.used) {
        entry.used = true;
        entry.exists = false;
        copyName(entry.namespaceName, sizeof(entry.namespaceName),
                 namespaceName);
        copyName(entry.key, sizeof(entry.key), key);
        return &entry;
      }
    }

    return nullptr;
  }

  Namespace namespaces_[kMaxNamespaces] = {};
  Entry entries_[kMaxEntries] = {};
  mutable bool failNextRead_ = false;
  bool failNextWrite_ = false;
  char lastNamespace_[reeflow::storage::kStorageNamespaceMaxLength + 1] = {};
  char lastKey_[reeflow::storage::kStorageKeyMaxLength + 1] = {};
};

}  // namespace reeflow::test::fakes
