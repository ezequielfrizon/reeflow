#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/state/system_state.h"

namespace reeflow::modules::lighting {

using LightingMode = core::state::LightingMode;

constexpr LightingMode MANUAL = LightingMode::kManual;
constexpr LightingMode AUTOMATIC = LightingMode::kAutomatic;
constexpr LightingMode ACCLIMATION = LightingMode::kAcclimation;

constexpr uint8_t kLightingFunctionalChannelCount = 4;
constexpr uint16_t kLightingMinMinuteOfDay = 0;
constexpr uint16_t kLightingMaxMinuteOfDay = 1439;
constexpr uint16_t kLightingMinutesPerDay = 1440;
constexpr uint8_t kLightingPwmResolutionBits = 8;
constexpr uint16_t kLightingMinDuty = 0;
constexpr uint16_t kLightingMaxDuty = 255;
constexpr uint8_t kLightingMinIntensityPercent = 0;
constexpr uint8_t kLightingMaxIntensityPercent = 100;
constexpr uint8_t kLightingMaxProfilesInMemory = 4;
constexpr uint8_t kLightingMaxCurvePoints = 8;
constexpr size_t kLightingProfileNameStorageLength =
    core::state::kProfileNameMaxLength;
constexpr size_t kLightingProfileNameMaxLength =
    kLightingProfileNameStorageLength - 1;

enum class LightingChannel {
  kWhite,
  kBlue,
  kRoyalBlue,
  kUv,
  kUnknown,
};

constexpr LightingChannel WHITE = LightingChannel::kWhite;
constexpr LightingChannel BLUE = LightingChannel::kBlue;
constexpr LightingChannel ROYAL_BLUE = LightingChannel::kRoyalBlue;
constexpr LightingChannel UV = LightingChannel::kUv;

using LightingChannelMask = uint8_t;

constexpr LightingChannelMask kLightingChannelMaskNone = 0;
constexpr LightingChannelMask kLightingChannelMaskWhite = 1U << 0;
constexpr LightingChannelMask kLightingChannelMaskBlue = 1U << 1;
constexpr LightingChannelMask kLightingChannelMaskRoyalBlue = 1U << 2;
constexpr LightingChannelMask kLightingChannelMaskUv = 1U << 3;
constexpr LightingChannelMask kLightingChannelMaskAll =
    kLightingChannelMaskWhite | kLightingChannelMaskBlue |
    kLightingChannelMaskRoyalBlue | kLightingChannelMaskUv;

enum class LightingCommandSource {
  kLocal,
};

enum class LightingCommandReason {
  kUserRequest,
  kProfileSelection,
  kModeSelection,
  kSafetyOff,
};

enum class LightingCommandTarget {
  kMode,
  kProfile,
  kChannelDuty,
  kChannelIntensity,
  kAllChannelsOff,
};

struct LightingCommand {
  LightingMode requestedMode;
  LightingCommandTarget target;
  LightingChannel channel;
  uint16_t requestedDuty;
  uint8_t requestedIntensityPercent;
  char targetProfile[kLightingProfileNameStorageLength];
  LightingCommandSource source;
  uint32_t requestedAtMillis;
  LightingCommandReason reason;
};

constexpr bool isCanonicalLightingMode(LightingMode mode) {
  return mode == MANUAL || mode == AUTOMATIC || mode == ACCLIMATION;
}

constexpr bool isCanonicalLightingChannel(LightingChannel channel) {
  return channel == WHITE || channel == BLUE || channel == ROYAL_BLUE ||
         channel == UV;
}

constexpr bool validLightingMinuteOfDay(uint16_t minuteOfDay) {
  return minuteOfDay <= kLightingMaxMinuteOfDay;
}

constexpr bool validLightingDuty(uint16_t duty) {
  return duty <= kLightingMaxDuty;
}

constexpr bool validLightingIntensityPercent(uint16_t intensityPercent) {
  return intensityPercent <= kLightingMaxIntensityPercent;
}

inline uint8_t lightingChannelIndex(LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return 0;
    case LightingChannel::kBlue:
      return 1;
    case LightingChannel::kRoyalBlue:
      return 2;
    case LightingChannel::kUv:
      return 3;
    case LightingChannel::kUnknown:
      return kLightingFunctionalChannelCount;
  }

  return kLightingFunctionalChannelCount;
}

inline LightingChannelMask lightingChannelMask(LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return kLightingChannelMaskWhite;
    case LightingChannel::kBlue:
      return kLightingChannelMaskBlue;
    case LightingChannel::kRoyalBlue:
      return kLightingChannelMaskRoyalBlue;
    case LightingChannel::kUv:
      return kLightingChannelMaskUv;
    case LightingChannel::kUnknown:
      return kLightingChannelMaskNone;
  }

  return kLightingChannelMaskNone;
}

inline const char* lightingModeName(LightingMode mode) {
  switch (mode) {
    case LightingMode::kManual:
      return "MANUAL";
    case LightingMode::kAutomatic:
      return "AUTOMATIC";
    case LightingMode::kAcclimation:
      return "ACCLIMATION";
  }

  return "UNKNOWN_LIGHTING_MODE";
}

inline const char* lightingChannelName(LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return "WHITE";
    case LightingChannel::kBlue:
      return "BLUE";
    case LightingChannel::kRoyalBlue:
      return "ROYAL_BLUE";
    case LightingChannel::kUv:
      return "UV";
    case LightingChannel::kUnknown:
      return "UNKNOWN_LIGHTING_CHANNEL";
  }

  return "UNKNOWN_LIGHTING_CHANNEL";
}

inline const char* lightingChannelSystemStateFieldName(
    LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return "system.lighting.white";
    case LightingChannel::kBlue:
      return "system.lighting.blue";
    case LightingChannel::kRoyalBlue:
      return "system.lighting.royalBlue";
    case LightingChannel::kUv:
      return "system.lighting.uv";
    case LightingChannel::kUnknown:
      return "";
  }

  return "";
}

inline const core::state::LightingChannelState* lightingChannelState(
    const core::state::LightingState& state, LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return &state.white;
    case LightingChannel::kBlue:
      return &state.blue;
    case LightingChannel::kRoyalBlue:
      return &state.royalBlue;
    case LightingChannel::kUv:
      return &state.uv;
    case LightingChannel::kUnknown:
      return nullptr;
  }

  return nullptr;
}

inline core::state::LightingChannelState* lightingChannelState(
    core::state::LightingState& state, LightingChannel channel) {
  return const_cast<core::state::LightingChannelState*>(
      lightingChannelState(static_cast<const core::state::LightingState&>(state),
                           channel));
}

inline LightingCommand makeLocalLightingModeCommand(
    LightingMode mode, uint32_t requestedAtMillis,
    LightingCommandReason reason = LightingCommandReason::kModeSelection) {
  return {mode,
          LightingCommandTarget::kMode,
          LightingChannel::kUnknown,
          0,
          0,
          {},
          LightingCommandSource::kLocal,
          requestedAtMillis,
          reason};
}

inline LightingCommand makeLocalLightingDutyCommand(
    LightingMode mode, LightingChannel channel, uint16_t duty,
    uint32_t requestedAtMillis,
    LightingCommandReason reason = LightingCommandReason::kUserRequest) {
  return {mode,
          LightingCommandTarget::kChannelDuty,
          channel,
          duty,
          0,
          {},
          LightingCommandSource::kLocal,
          requestedAtMillis,
          reason};
}

inline LightingCommand makeLocalLightingIntensityCommand(
    LightingMode mode, LightingChannel channel, uint8_t intensityPercent,
    uint32_t requestedAtMillis,
    LightingCommandReason reason = LightingCommandReason::kUserRequest) {
  return {mode,
          LightingCommandTarget::kChannelIntensity,
          channel,
          0,
          intensityPercent,
          {},
          LightingCommandSource::kLocal,
          requestedAtMillis,
          reason};
}

}  // namespace reeflow::modules::lighting
