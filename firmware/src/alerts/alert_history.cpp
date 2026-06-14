#include "alerts/alert_history.h"

#include <string.h>

#include "alerts/alert_registry.h"

namespace reeflow::alerts {

AlertHistoryBuffer::AlertHistoryBuffer(size_t capacity)
    : capacity_(capacity == 0 || capacity > kMaxAlertHistoryEntries
                    ? kMaxAlertHistoryEntries
                    : capacity),
      start_(0),
      count_(0) {}

AlertHistoryResult AlertHistoryBuffer::append(
    const AlertHistoryEntry& entry) {
  if (!isKnownAlertCode(entry.code) || capacity_ == 0) {
    return AlertHistoryResult::kRejected;
  }

  const size_t writeIndex =
      count_ < capacity_ ? (start_ + count_) % capacity_ : start_;
  entries_[writeIndex] = entry;

  if (count_ < capacity_) {
    count_ += 1;
  } else {
    start_ = (start_ + 1) % capacity_;
  }

  return AlertHistoryResult::kRecorded;
}

size_t AlertHistoryBuffer::count() const {
  return count_;
}

size_t AlertHistoryBuffer::capacity() const {
  return capacity_;
}

bool AlertHistoryBuffer::full() const {
  return count_ == capacity_;
}

const AlertHistoryEntry* AlertHistoryBuffer::latest() const {
  if (count_ == 0) {
    return nullptr;
  }

  const size_t index = (start_ + count_ - 1) % capacity_;
  return &entries_[index];
}

const AlertHistoryEntry* AlertHistoryBuffer::entryAt(size_t index) const {
  if (index >= count_) {
    return nullptr;
  }

  return &entries_[(start_ + index) % capacity_];
}

void AlertHistoryBuffer::clear() {
  start_ = 0;
  count_ = 0;
}

void copyAlertReason(char* destination, size_t capacity, const char* reason) {
  if (destination == nullptr || capacity == 0) {
    return;
  }

  if (reason == nullptr) {
    destination[0] = '\0';
    return;
  }

  strncpy(destination, reason, capacity - 1);
  destination[capacity - 1] = '\0';
}

AlertHistoryEntry makeAlertHistoryEntry(
    AlertCode code, AlertState state, AlertSource source,
    uint32_t occurredAtMillis, uint32_t repeatCount, const char* reason) {
  AlertHistoryEntry entry = {};
  const AlertDefinition* definition = alertDefinition(code);

  entry.code = code;
  entry.recoveryCode = recoveryForAlert(code);
  entry.category = definition == nullptr ? AlertCategory::kInfo
                                         : definition->category;
  entry.priority = definition == nullptr ? AlertPriority::kInfo
                                         : definition->defaultPriority;
  entry.state = state;
  entry.source = source;
  entry.occurredAtMillis = occurredAtMillis;
  entry.repeatCount = repeatCount;
  copyAlertReason(entry.reason, sizeof(entry.reason), reason);
  return entry;
}

}  // namespace reeflow::alerts
