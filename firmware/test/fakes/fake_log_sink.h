#pragma once

#include <string>
#include <vector>

#include "core/platform/log_sink.h"

namespace reeflow::test::fakes {

class FakeLogSink final : public core::platform::LogSink {
 public:
  void write(const char* message) override {
    messages_.push_back(message == nullptr ? "" : message);
  }

  const std::vector<std::string>& messages() const {
    return messages_;
  }

 private:
  std::vector<std::string> messages_;
};

}  // namespace reeflow::test::fakes
