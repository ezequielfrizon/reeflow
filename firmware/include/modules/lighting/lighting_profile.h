#pragma once

#include <stdint.h>
#include <string.h>

#include "modules/lighting/lighting_types.h"

namespace reeflow::modules::lighting {

enum class LightingCurveTargetType {
  kDuty,
  kIntensityPercent,
};

enum class LightingCurveValidationResult {
  kValid,
  kEmptyCurve,
  kTooManyPoints,
  kMinuteOutOfRange,
  kPointsNotIncreasing,
  kTargetOutOfRange,
};

enum class LightingProfileValidationResult {
  kValid,
  kNameNotTerminated,
  kTimeWindowOutOfRange,
  kGlobalIntensityOutOfRange,
  kChannelLimitOutOfRange,
  kCurveInvalid,
};

struct LightingCurvePoint {
  uint16_t minuteOfDay;
  LightingCurveTargetType targetType;
  uint16_t value;
};

struct LightingCurve {
  LightingCurvePoint points[kLightingMaxCurvePoints];
  uint8_t pointCount;
};

struct LightingChannelProfile {
  bool enabled;
  uint16_t maxDuty;
  uint8_t maxIntensityPercent;
  LightingCurve curve;
};

struct LightingProfile {
  char name[kLightingProfileNameStorageLength];
  uint16_t startMinuteOfDay;
  uint16_t endMinuteOfDay;
  bool sunriseEnabled;
  bool sunsetEnabled;
  bool acclimationEnabled;
  uint8_t maxGlobalIntensityPercent;
  uint16_t acclimationDurationDays;
  LightingChannelProfile white;
  LightingChannelProfile blue;
  LightingChannelProfile royalBlue;
  LightingChannelProfile uv;
};

constexpr bool lightingCurveEmptyMeansDutyZero() {
  return true;
}

inline LightingCurveValidationResult validateLightingCurve(
    const LightingCurve& curve) {
  if (curve.pointCount == 0) {
    return LightingCurveValidationResult::kEmptyCurve;
  }

  if (curve.pointCount > kLightingMaxCurvePoints) {
    return LightingCurveValidationResult::kTooManyPoints;
  }

  uint16_t previousMinute = 0;
  for (uint8_t index = 0; index < curve.pointCount; ++index) {
    const LightingCurvePoint point = curve.points[index];
    if (!validLightingMinuteOfDay(point.minuteOfDay)) {
      return LightingCurveValidationResult::kMinuteOutOfRange;
    }

    if (index > 0 && point.minuteOfDay <= previousMinute) {
      return LightingCurveValidationResult::kPointsNotIncreasing;
    }

    if ((point.targetType == LightingCurveTargetType::kDuty &&
         !validLightingDuty(point.value)) ||
        (point.targetType == LightingCurveTargetType::kIntensityPercent &&
         !validLightingIntensityPercent(point.value))) {
      return LightingCurveValidationResult::kTargetOutOfRange;
    }

    previousMinute = point.minuteOfDay;
  }

  return LightingCurveValidationResult::kValid;
}

inline bool lightingProfileNameIsTerminated(const LightingProfile& profile) {
  return memchr(profile.name, '\0', sizeof(profile.name)) != nullptr;
}

inline bool validLightingProfileTimeWindow(
    const LightingProfile& profile) {
  return validLightingMinuteOfDay(profile.startMinuteOfDay) &&
         validLightingMinuteOfDay(profile.endMinuteOfDay);
}

inline bool validLightingChannelProfileLimit(
    const LightingChannelProfile& channel) {
  return validLightingDuty(channel.maxDuty) &&
         validLightingIntensityPercent(channel.maxIntensityPercent);
}

inline bool lightingCurveResultAllowedForProfile(
    LightingCurveValidationResult result) {
  return result == LightingCurveValidationResult::kValid ||
         result == LightingCurveValidationResult::kEmptyCurve;
}

inline LightingProfileValidationResult validateLightingProfile(
    const LightingProfile& profile) {
  if (!lightingProfileNameIsTerminated(profile)) {
    return LightingProfileValidationResult::kNameNotTerminated;
  }

  if (!validLightingProfileTimeWindow(profile)) {
    return LightingProfileValidationResult::kTimeWindowOutOfRange;
  }

  if (!validLightingIntensityPercent(profile.maxGlobalIntensityPercent)) {
    return LightingProfileValidationResult::kGlobalIntensityOutOfRange;
  }

  const LightingChannelProfile channels[] = {profile.white, profile.blue,
                                             profile.royalBlue, profile.uv};
  for (const LightingChannelProfile& channel : channels) {
    if (!validLightingChannelProfileLimit(channel)) {
      return LightingProfileValidationResult::kChannelLimitOutOfRange;
    }

    if (!lightingCurveResultAllowedForProfile(
            validateLightingCurve(channel.curve))) {
      return LightingProfileValidationResult::kCurveInvalid;
    }
  }

  return LightingProfileValidationResult::kValid;
}

inline LightingCurve makeDefaultLightingCurve(uint16_t peakDuty) {
  LightingCurve curve = {};
  curve.pointCount = 3;
  curve.points[0] = {0, LightingCurveTargetType::kDuty, 0};
  curve.points[1] = {720, LightingCurveTargetType::kDuty, peakDuty};
  curve.points[2] = {1439, LightingCurveTargetType::kDuty, 0};
  return curve;
}

inline LightingProfile makeDefaultLightingProfile() {
  LightingProfile profile = {};
  strncpy(profile.name, "default", sizeof(profile.name) - 1);
  profile.name[sizeof(profile.name) - 1] = '\0';
  profile.startMinuteOfDay = 480;
  profile.endMinuteOfDay = 1200;
  profile.sunriseEnabled = true;
  profile.sunsetEnabled = true;
  profile.acclimationEnabled = true;
  profile.maxGlobalIntensityPercent = kLightingMaxIntensityPercent;
  profile.acclimationDurationDays = 14;
  profile.white = {true, kLightingMaxDuty, kLightingMaxIntensityPercent,
                   makeDefaultLightingCurve(kLightingMaxDuty)};
  profile.blue = {true, kLightingMaxDuty, kLightingMaxIntensityPercent,
                  makeDefaultLightingCurve(kLightingMaxDuty)};
  profile.royalBlue = {true, kLightingMaxDuty, kLightingMaxIntensityPercent,
                       makeDefaultLightingCurve(kLightingMaxDuty)};
  profile.uv = {true, kLightingMaxDuty, kLightingMaxIntensityPercent,
                makeDefaultLightingCurve(kLightingMaxDuty)};
  return profile;
}

}  // namespace reeflow::modules::lighting
