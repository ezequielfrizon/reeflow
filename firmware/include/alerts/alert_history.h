#pragma once

#include <stddef.h>
#include <stdint.h>

#include "alerts/alert_types.h"

namespace reeflow::alerts {

enum class AlertHistoryResult : uint8_t {
  kRecorded,
  kRejected,
  kStoreUnavailable,
};

class AlertHistoryStore {
 public:
  virtual ~AlertHistoryStore() = default;

  virtual AlertHistoryResult append(const AlertHistoryEntry& entry) = 0;
};

class AlertHistoryBuffer final : public AlertHistoryStore {
 public:
  explicit AlertHistoryBuffer(size_t capacity = kMaxAlertHistoryEntries);

  AlertHistoryResult append(const AlertHistoryEntry& entry) override;
  size_t count() const;
  size_t capacity() const;
  bool full() const;
  const AlertHistoryEntry* latest() const;
  const AlertHistoryEntry* entryAt(size_t index) const;
  void clear();

 private:
  AlertHistoryEntry entries_[kMaxAlertHistoryEntries] = {};
  size_t capacity_;
  size_t start_;
  size_t count_;
};

void copyAlertReason(char* destination, size_t capacity, const char* reason);
AlertHistoryEntry makeAlertHistoryEntry(
    AlertCode code, AlertState state, AlertSource source,
    uint32_t occurredAtMillis, uint32_t repeatCount, const char* reason);

}  // namespace reeflow::alerts
