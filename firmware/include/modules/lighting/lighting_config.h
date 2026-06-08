#pragma once

#include <stdint.h>

#include "modules/lighting/lighting_types.h"

namespace reeflow::modules::lighting {

constexpr uint32_t kLightingDefaultEvaluationIntervalMillis = 1000;
constexpr uint32_t kLightingDefaultPwmFrequencyHz = 1000;
constexpr uint16_t kLightingDefaultMaxFadeStepPerEvaluation = 5;
constexpr uint16_t kLightingDefaultSunriseDurationMinutes = 60;
constexpr uint16_t kLightingDefaultSunsetDurationMinutes = 60;
constexpr uint8_t kLightingDefaultMoonlightMaxIntensityPercent = 5;

struct LightingPwmConfig {
  uint32_t frequencyHz;
  uint8_t resolutionBits;
  uint16_t minDuty;
  uint16_t maxDuty;
};

struct LightingModuleConfig {
  uint32_t evaluationIntervalMillis;
  LightingPwmConfig pwm;
  uint16_t maxFadeStepPerEvaluation;
  uint16_t sunriseDurationMinutes;
  uint16_t sunsetDurationMinutes;
  uint8_t moonlightMaxIntensityPercent;
};

constexpr LightingModuleConfig makeDefaultLightingModuleConfig() {
  return {kLightingDefaultEvaluationIntervalMillis,
          {kLightingDefaultPwmFrequencyHz, kLightingPwmResolutionBits,
           kLightingMinDuty, kLightingMaxDuty},
          kLightingDefaultMaxFadeStepPerEvaluation,
          kLightingDefaultSunriseDurationMinutes,
          kLightingDefaultSunsetDurationMinutes,
          kLightingDefaultMoonlightMaxIntensityPercent};
}

constexpr bool validLightingPwmConfig(const LightingPwmConfig& config) {
  return config.frequencyHz > 0 &&
         config.resolutionBits == kLightingPwmResolutionBits &&
         config.minDuty == kLightingMinDuty &&
         config.maxDuty == kLightingMaxDuty;
}

constexpr bool validLightingModuleConfig(
    const LightingModuleConfig& config) {
  return config.evaluationIntervalMillis > 0 &&
         validLightingPwmConfig(config.pwm) &&
         config.maxFadeStepPerEvaluation > 0 &&
         config.maxFadeStepPerEvaluation <= config.pwm.maxDuty &&
         config.sunriseDurationMinutes < kLightingMinutesPerDay &&
         config.sunsetDurationMinutes < kLightingMinutesPerDay &&
         validLightingIntensityPercent(config.moonlightMaxIntensityPercent);
}

}  // namespace reeflow::modules::lighting
