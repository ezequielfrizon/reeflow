#include "storage/preferences_storage_backend.h"

#include <Preferences.h>
#include <string.h>

namespace reeflow::storage {

bool PreferencesStorageBackend::validName(const char* value,
                                          size_t maxLength) {
  if (value == nullptr || value[0] == '\0') {
    return false;
  }

  return strnlen(value, maxLength + 1) <= maxLength;
}

StorageResult PreferencesStorageBackend::openNamespace(const char* name) {
  if (!validName(name, kStorageNamespaceMaxLength)) {
    return StorageResult::kWriteFailure;
  }

  Preferences preferences;
  if (!preferences.begin(name, false)) {
    return StorageResult::kWriteFailure;
  }

  preferences.end();
  return StorageResult::kSuccess;
}

bool PreferencesStorageBackend::namespaceExists(const char* name) const {
  if (!validName(name, kStorageNamespaceMaxLength)) {
    return false;
  }

  Preferences preferences;
  const bool opened = preferences.begin(name, true);
  if (opened) {
    preferences.end();
  }

  return opened;
}

bool PreferencesStorageBackend::keyExists(const char* namespaceName,
                                          const char* key) const {
  if (!validName(namespaceName, kStorageNamespaceMaxLength) ||
      !validName(key, kStorageKeyMaxLength)) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(namespaceName, true)) {
    return false;
  }

  const bool exists = preferences.isKey(key);
  preferences.end();
  return exists;
}

StorageReadResult PreferencesStorageBackend::read(
    const char* namespaceName, const char* key) const {
  if (!validName(namespaceName, kStorageNamespaceMaxLength) ||
      !validName(key, kStorageKeyMaxLength)) {
    return {StorageResult::kReadFailure, {}};
  }

  Preferences preferences;
  if (!preferences.begin(namespaceName, true)) {
    return {StorageResult::kNamespaceNotFound, {}};
  }

  if (!preferences.isKey(key)) {
    preferences.end();
    return {StorageResult::kKeyNotFound, {}};
  }

  const size_t payloadSize = preferences.getBytesLength(key);
  if (payloadSize == 0 || payloadSize > kStorageMaxPayloadSize) {
    preferences.end();
    return {StorageResult::kInvalidPayload, {}};
  }

  const size_t readSize =
      preferences.getBytes(key, readBuffer_, sizeof(readBuffer_));
  preferences.end();

  if (readSize != payloadSize) {
    return {StorageResult::kReadFailure, {}};
  }

  return {StorageResult::kSuccess, {readBuffer_, payloadSize}};
}

StorageResult PreferencesStorageBackend::write(const char* namespaceName,
                                               const char* key,
                                               StoragePayload payload) {
  if (!validName(namespaceName, kStorageNamespaceMaxLength) ||
      !validName(key, kStorageKeyMaxLength) || payload.data == nullptr ||
      payload.size == 0 || payload.size > kStorageMaxPayloadSize) {
    return StorageResult::kInvalidPayload;
  }

  Preferences preferences;
  if (!preferences.begin(namespaceName, false)) {
    return StorageResult::kWriteFailure;
  }

  const size_t writtenSize = preferences.putBytes(key, payload.data,
                                                  payload.size);
  preferences.end();

  return writtenSize == payload.size ? StorageResult::kSuccess
                                     : StorageResult::kWriteFailure;
}

StorageResult PreferencesStorageBackend::remove(const char* namespaceName,
                                                const char* key) {
  if (!validName(namespaceName, kStorageNamespaceMaxLength) ||
      !validName(key, kStorageKeyMaxLength)) {
    return StorageResult::kWriteFailure;
  }

  Preferences preferences;
  if (!preferences.begin(namespaceName, false)) {
    return StorageResult::kNamespaceNotFound;
  }

  if (!preferences.isKey(key)) {
    preferences.end();
    return StorageResult::kKeyNotFound;
  }

  const bool removed = preferences.remove(key);
  preferences.end();
  return removed ? StorageResult::kSuccess : StorageResult::kWriteFailure;
}

StorageResult PreferencesStorageBackend::resetNamespace(
    const char* namespaceName) {
  if (!validName(namespaceName, kStorageNamespaceMaxLength)) {
    return StorageResult::kWriteFailure;
  }

  Preferences preferences;
  if (!preferences.begin(namespaceName, false)) {
    return StorageResult::kNamespaceNotFound;
  }

  const bool cleared = preferences.clear();
  preferences.end();
  return cleared ? StorageResult::kResetApplied
                 : StorageResult::kWriteFailure;
}

}  // namespace reeflow::storage
