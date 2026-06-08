#pragma once

#include <stdint.h>

#include "core/state/system_state.h"

namespace reeflow::modules::modes {

using OperationalMode = core::state::OperationalMode;

constexpr OperationalMode NORMAL = OperationalMode::kNormal;
constexpr OperationalMode FEEDING = OperationalMode::kFeeding;
constexpr OperationalMode TPA = OperationalMode::kTpa;
constexpr OperationalMode MAINTENANCE = OperationalMode::kMaintenance;

enum class ModeCommandSource {
  kLocal,
};

enum class ModeCommandReason {
  kUserRequest,
  kAutomaticReturn,
  kBootRestore,
};

struct ModeCommand {
  OperationalMode requestedMode;
  ModeCommandSource source;
  uint32_t requestedAtMillis;
  ModeCommandReason reason;
};

constexpr bool isCanonicalMode(OperationalMode mode) {
  return mode == NORMAL || mode == FEEDING || mode == TPA ||
         mode == MAINTENANCE;
}

constexpr ModeCommand makeLocalModeCommand(OperationalMode requestedMode,
                                           uint32_t requestedAtMillis,
                                           ModeCommandReason reason =
                                               ModeCommandReason::kUserRequest) {
  return {requestedMode, ModeCommandSource::kLocal, requestedAtMillis, reason};
}

inline const char* modeName(OperationalMode mode) {
  switch (mode) {
    case OperationalMode::kNormal:
      return "NORMAL";
    case OperationalMode::kFeeding:
      return "FEEDING";
    case OperationalMode::kTpa:
      return "TPA";
    case OperationalMode::kMaintenance:
      return "MAINTENANCE";
  }

  return "UNKNOWN_MODE";
}

}  // namespace reeflow::modules::modes
