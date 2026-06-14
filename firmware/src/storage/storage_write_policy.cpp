#include "storage/storage_write_policy.h"

#include <string.h>

namespace reeflow::storage {
namespace {

bool validName(const char* value, size_t maxLength) {
  return value != nullptr && value[0] != '\0' &&
         strnlen(value, maxLength + 1) <= maxLength;
}

bool validPayload(StoragePayload payload) {
  return payload.data != nullptr && payload.size > 0 &&
         payload.size <= kStorageMaxPayloadSize;
}

void copyText(char* destination, size_t capacity, const char* source) {
  strncpy(destination, source, capacity - 1);
  destination[capacity - 1] = '\0';
}

}  // namespace

StorageResult StorageWritePolicy::markChanged(StorageDomain domain,
                                              const char* namespaceName,
                                              const char* key,
                                              StoragePayload payload) {
  if (!validName(namespaceName, kStorageNamespaceMaxLength) ||
      !validName(key, kStorageKeyMaxLength) || !validPayload(payload)) {
    return StorageResult::kInvalidPayload;
  }

  DomainState* state = findOrCreateState(domain);
  if (state == nullptr) {
    return StorageResult::kWriteFailure;
  }

  if (state->payloadSize == payload.size &&
      memcmp(state->payload, payload.data, payload.size) == 0 &&
      strcmp(state->namespaceName, namespaceName) == 0 &&
      strcmp(state->key, key) == 0) {
    return StorageResult::kNoChange;
  }

  copyText(state->namespaceName, sizeof(state->namespaceName), namespaceName);
  copyText(state->key, sizeof(state->key), key);
  memcpy(state->payload, payload.data, payload.size);
  state->payloadSize = payload.size;
  state->dirty = true;
  return StorageResult::kSuccess;
}

StorageResult StorageWritePolicy::markClean(StorageDomain domain) {
  DomainState* state = findOrCreateState(domain);
  if (state == nullptr) {
    return StorageResult::kWriteFailure;
  }

  state->dirty = false;
  return StorageResult::kSuccess;
}

StorageResult StorageWritePolicy::flushDomain(StorageBackend& backend,
                                              StorageDomain domain) {
  DomainState* state = findOrCreateState(domain);
  if (state == nullptr) {
    return StorageResult::kWriteFailure;
  }

  if (!state->dirty) {
    return StorageResult::kNoChange;
  }

  const StorageResult openResult = backend.openNamespace(state->namespaceName);
  if (openResult != StorageResult::kSuccess) {
    return openResult;
  }

  const StorageResult result =
      backend.write(state->namespaceName, state->key,
                    {state->payload, state->payloadSize});
  if (result == StorageResult::kSuccess) {
    state->dirty = false;
  }

  return result;
}

StorageResult StorageWritePolicy::flushAll(StorageBackend& backend) {
  StorageResult lastResult = StorageResult::kNoChange;

  for (DomainState& state : states_) {
    if (!state.used || !state.dirty) {
      continue;
    }

    lastResult = flushDomain(backend, state.domain);
    if (lastResult != StorageResult::kSuccess) {
      return lastResult;
    }
  }

  return lastResult;
}

bool StorageWritePolicy::isDirty(StorageDomain domain) const {
  const DomainState* state = findState(domain);
  return state != nullptr && state->dirty;
}

uint8_t StorageWritePolicy::dirtyCount() const {
  uint8_t count = 0;
  for (const DomainState& state : states_) {
    if (state.used && state.dirty) {
      count += 1;
    }
  }

  return count;
}

StorageWritePolicy::DomainState* StorageWritePolicy::findOrCreateState(
    StorageDomain domain) {
  DomainState* existingState = const_cast<DomainState*>(findState(domain));
  if (existingState != nullptr) {
    return existingState;
  }

  for (DomainState& state : states_) {
    if (!state.used) {
      state.used = true;
      state.domain = domain;
      return &state;
    }
  }

  return nullptr;
}

const StorageWritePolicy::DomainState* StorageWritePolicy::findState(
    StorageDomain domain) const {
  for (const DomainState& state : states_) {
    if (state.used && state.domain == domain) {
      return &state;
    }
  }

  return nullptr;
}

}  // namespace reeflow::storage
