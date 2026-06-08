#pragma once

#include <stddef.h>

#include "modules/modes/mode_store.h"

namespace reeflow::test::fakes {

class FakeModeStore final : public modules::modes::ModeStore {
 public:
  modules::modes::ModeStoreLoadResult loadMode() override {
    ++loadCount_;
    return loadResult_;
  }

  modules::modes::ModeStoreSaveResult saveMode(
      modules::modes::OperationalMode mode) override {
    ++saveCount_;
    lastSavedMode_ = mode;

    if (!modules::modes::isCanonicalMode(mode)) {
      return modules::modes::ModeStoreSaveResult::kInvalidMode;
    }

    return saveResult_;
  }

  void simulateLoadedMode(modules::modes::OperationalMode mode) {
    loadResult_ = modules::modes::makeLoadedModeResult(mode);
  }

  void simulateNoSavedMode() {
    loadResult_ = modules::modes::makeModeNotFoundResult();
  }

  void simulateInvalidSavedMode() {
    loadResult_ = {modules::modes::ModeStoreLoadStatus::kInvalidMode,
                   static_cast<modules::modes::OperationalMode>(255)};
  }

  void simulateLoadFailure() {
    loadResult_ = {modules::modes::ModeStoreLoadStatus::kFailure,
                   modules::modes::NORMAL};
  }

  void simulateSaveFailure() {
    saveResult_ = modules::modes::ModeStoreSaveResult::kFailure;
  }

  void simulateSaveSuccess() {
    saveResult_ = modules::modes::ModeStoreSaveResult::kSuccess;
  }

  size_t loadCount() const {
    return loadCount_;
  }

  size_t saveCount() const {
    return saveCount_;
  }

  modules::modes::OperationalMode lastSavedMode() const {
    return lastSavedMode_;
  }

 private:
  modules::modes::ModeStoreLoadResult loadResult_ =
      modules::modes::makeModeNotFoundResult();
  modules::modes::ModeStoreSaveResult saveResult_ =
      modules::modes::ModeStoreSaveResult::kSuccess;
  modules::modes::OperationalMode lastSavedMode_ = modules::modes::NORMAL;
  size_t loadCount_ = 0;
  size_t saveCount_ = 0;
};

}  // namespace reeflow::test::fakes
