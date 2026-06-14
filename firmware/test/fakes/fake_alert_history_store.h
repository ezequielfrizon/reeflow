#pragma once

#include "alerts/alert_history.h"

namespace reeflow::test::fakes {

class FakeAlertHistoryStore final : public alerts::AlertHistoryStore {
 public:
  alerts::AlertHistoryResult append(
      const alerts::AlertHistoryEntry& entry) override {
    appendCallCount += 1;

    if (nextResult != alerts::AlertHistoryResult::kRecorded) {
      return nextResult;
    }

    if (count_ >= capacity_) {
      fullWriteCount += 1;
      if (rejectWhenFull) {
        return alerts::AlertHistoryResult::kRejected;
      }
      start_ = (start_ + 1) % capacity_;
      count_ -= 1;
    }

    const size_t writeIndex = (start_ + count_) % capacity_;
    entries_[writeIndex] = entry;
    count_ += 1;
    return alerts::AlertHistoryResult::kRecorded;
  }

  void setCapacity(size_t capacity) {
    capacity_ = capacity == 0 || capacity > alerts::kMaxAlertHistoryEntries
                    ? alerts::kMaxAlertHistoryEntries
                    : capacity;
    clear();
  }

  void clear() {
    start_ = 0;
    count_ = 0;
  }

  size_t count() const { return count_; }
  size_t capacity() const { return capacity_; }
  bool full() const { return count_ == capacity_; }

  const alerts::AlertHistoryEntry* latest() const {
    if (count_ == 0) {
      return nullptr;
    }

    return &entries_[(start_ + count_ - 1) % capacity_];
  }

  alerts::AlertHistoryResult nextResult =
      alerts::AlertHistoryResult::kRecorded;
  bool rejectWhenFull = false;
  uint32_t appendCallCount = 0;
  uint32_t fullWriteCount = 0;

 private:
  alerts::AlertHistoryEntry entries_[alerts::kMaxAlertHistoryEntries] = {};
  size_t capacity_ = alerts::kMaxAlertHistoryEntries;
  size_t start_ = 0;
  size_t count_ = 0;
};

}  // namespace reeflow::test::fakes
