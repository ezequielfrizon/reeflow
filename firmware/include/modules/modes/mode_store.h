#pragma once

#include "modules/modes/mode_types.h"

namespace reeflow::modules::modes {

enum class ModeStoreLoadStatus {
  kLoaded,
  kNotFound,
  kInvalidMode,
  kFailure,
};

enum class ModeStoreSaveResult {
  kSuccess,
  kInvalidMode,
  kFailure,
};

struct ModeStoreLoadResult {
  ModeStoreLoadStatus status;
  OperationalMode mode;
};

class ModeStore {
 public:
  virtual ~ModeStore() = default;
  virtual ModeStoreLoadResult loadMode() = 0;
  virtual ModeStoreSaveResult saveMode(OperationalMode mode) = 0;
};

class VolatileModeStore final : public ModeStore {
 public:
  ModeStoreLoadResult loadMode() override {
    if (!hasMode_) {
      return {ModeStoreLoadStatus::kNotFound, NORMAL};
    }

    return {isCanonicalMode(mode_) ? ModeStoreLoadStatus::kLoaded
                                   : ModeStoreLoadStatus::kInvalidMode,
            mode_};
  }

  ModeStoreSaveResult saveMode(OperationalMode mode) override {
    if (!isCanonicalMode(mode)) {
      return ModeStoreSaveResult::kInvalidMode;
    }

    mode_ = mode;
    hasMode_ = true;
    return ModeStoreSaveResult::kSuccess;
  }

 private:
  bool hasMode_ = false;
  OperationalMode mode_ = NORMAL;
};

constexpr ModeStoreLoadResult makeModeNotFoundResult() {
  return {ModeStoreLoadStatus::kNotFound, NORMAL};
}

constexpr ModeStoreLoadResult makeLoadedModeResult(OperationalMode mode) {
  return {isCanonicalMode(mode) ? ModeStoreLoadStatus::kLoaded
                                : ModeStoreLoadStatus::kInvalidMode,
          mode};
}

}  // namespace reeflow::modules::modes
