#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/events/event_bus.h"

namespace reeflow::storage {

using StorageDomain = core::events::ConfigDomain;

constexpr size_t kStorageNamespaceMaxLength = 15;
constexpr size_t kStorageKeyMaxLength = 31;
constexpr size_t kStorageReasonMaxLength = 40;
constexpr size_t kStorageMaxPayloadSize = 2048;
constexpr uint16_t kStorageSchemaVersion = 1;

enum class StorageResult {
  kSuccess,
  kKeyNotFound,
  kNamespaceNotFound,
  kInvalidPayload,
  kIncompatibleSchema,
  kReadFailure,
  kWriteFailure,
  kNoChange,
  kResetApplied,
  kIgnoredByWritePolicy,
};

enum class StoragePayloadState {
  kValid,
  kMissingSchema,
  kInvalidSchema,
  kCorruptedPayload,
};

struct StoragePayload {
  const uint8_t* data;
  size_t size;
};

struct StorageReadResult {
  StorageResult result;
  StoragePayload payload;
};

struct StorageEventData {
  StorageDomain domain;
  StorageResult result;
  const char* reason;
  uint32_t timestampMillis;
};

constexpr bool storageResultIsSuccess(StorageResult result) {
  return result == StorageResult::kSuccess ||
         result == StorageResult::kResetApplied ||
         result == StorageResult::kNoChange;
}

}  // namespace reeflow::storage
