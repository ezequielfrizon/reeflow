#include "modules/lighting/lighting_service.h"

#include <stddef.h>
#include <string.h>

namespace reeflow::modules::lighting {
namespace {

constexpr LightingChannel kChannels[] = {WHITE, BLUE, ROYAL_BLUE, UV};

core::state::LightingChannelState& mutableStateChannel(
    core::state::LightingState& state, LightingChannel channel) {
  return *lightingChannelState(state, channel);
}

const config::LightingChannelConfig& configChannel(
    const config::LightingConfig& config, LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return config.white;
    case LightingChannel::kBlue:
      return config.blue;
    case LightingChannel::kRoyalBlue:
      return config.royalBlue;
    case LightingChannel::kUv:
      return config.uv;
    case LightingChannel::kUnknown:
      return config.white;
  }

  return config.white;
}

const LightingChannelProfile& profileChannel(const LightingProfile& profile,
                                             LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return profile.white;
    case LightingChannel::kBlue:
      return profile.blue;
    case LightingChannel::kRoyalBlue:
      return profile.royalBlue;
    case LightingChannel::kUv:
      return profile.uv;
    case LightingChannel::kUnknown:
      return profile.white;
  }

  return profile.white;
}

uint16_t effectiveMaxPwm(const config::LightingConfig& config,
                         const LightingProfile& profile,
                         LightingChannel channel) {
  const uint16_t configMax = configChannel(config, channel).maxPWM;
  const uint16_t profileMax = profileChannel(profile, channel).maxDuty;
  return configMax < profileMax ? configMax : profileMax;
}

bool effectiveEnabled(const config::LightingConfig& config,
                      const LightingProfile& profile,
                      LightingChannel channel) {
  return configChannel(config, channel).enabled &&
         profileChannel(profile, channel).enabled;
}

bool anyChannelActive(const core::state::LightingState& state) {
  return state.white.currentPWM > 0 || state.blue.currentPWM > 0 ||
         state.royalBlue.currentPWM > 0 || state.uv.currentPWM > 0;
}

LightingChannelMask activeChannelMask(const core::state::LightingState& state) {
  LightingChannelMask mask = kLightingChannelMaskNone;
  if (state.white.currentPWM > 0) {
    mask |= kLightingChannelMaskWhite;
  }
  if (state.blue.currentPWM > 0) {
    mask |= kLightingChannelMaskBlue;
  }
  if (state.royalBlue.currentPWM > 0) {
    mask |= kLightingChannelMaskRoyalBlue;
  }
  if (state.uv.currentPWM > 0) {
    mask |= kLightingChannelMaskUv;
  }
  return mask;
}

void copyProfileName(char* destination, const char* source) {
  strncpy(destination, source, core::state::kProfileNameMaxLength - 1);
  destination[core::state::kProfileNameMaxLength - 1] = '\0';
}

bool textEquals(const char* left, const char* right) {
  return strncmp(left, right, core::state::kProfileNameMaxLength) == 0;
}

bool commandUsesProfile(const LightingCommand* command) {
  return command != nullptr &&
         command->target == LightingCommandTarget::kProfile;
}

bool evaluationUsesProfile(LightingMode mode, const LightingCommand* command) {
  return mode == AUTOMATIC || mode == ACCLIMATION || commandUsesProfile(command);
}

uint32_t hashStep(uint32_t hash, uint32_t value) {
  return (hash ^ value) * 16777619UL;
}

uint32_t hashText(uint32_t hash, const char* text) {
  for (size_t index = 0; index < core::state::kProfileNameMaxLength &&
                         text[index] != '\0';
       ++index) {
    hash = hashStep(hash, static_cast<uint8_t>(text[index]));
  }

  return hash;
}

uint32_t hashCurve(uint32_t hash, const LightingCurve& curve) {
  hash = hashStep(hash, curve.pointCount);
  for (uint8_t index = 0; index < curve.pointCount; ++index) {
    hash = hashStep(hash, curve.points[index].minuteOfDay);
    hash = hashStep(hash, static_cast<uint32_t>(curve.points[index].targetType));
    hash = hashStep(hash, curve.points[index].value);
  }

  return hash;
}

uint32_t profileSignature(const LightingProfile& profile) {
  uint32_t hash = 2166136261UL;
  hash = hashText(hash, profile.name);
  hash = hashStep(hash, profile.startMinuteOfDay);
  hash = hashStep(hash, profile.endMinuteOfDay);
  hash = hashStep(hash, profile.sunriseEnabled ? 1U : 0U);
  hash = hashStep(hash, profile.sunsetEnabled ? 1U : 0U);
  hash = hashStep(hash, profile.acclimationEnabled ? 1U : 0U);
  hash = hashStep(hash, profile.maxGlobalIntensityPercent);
  hash = hashStep(hash, profile.acclimationDurationDays);

  const LightingChannelProfile channels[] = {profile.white, profile.blue,
                                             profile.royalBlue, profile.uv};
  for (const LightingChannelProfile& channel : channels) {
    hash = hashStep(hash, channel.enabled ? 1U : 0U);
    hash = hashStep(hash, channel.maxDuty);
    hash = hashStep(hash, channel.maxIntensityPercent);
    hash = hashCurve(hash, channel.curve);
  }

  return hash;
}

uint16_t applyPercent(uint16_t value, uint16_t percent) {
  return static_cast<uint16_t>((static_cast<uint32_t>(value) * percent) / 100U);
}

uint8_t effectiveGlobalIntensityPercent(const config::LightingConfig& config,
                                        const LightingProfile& profile) {
  return config.maxIntensityPercent < profile.maxGlobalIntensityPercent
             ? config.maxIntensityPercent
             : profile.maxGlobalIntensityPercent;
}

uint8_t effectiveChannelIntensityPercent(const LightingProfile& profile,
                                         LightingChannel channel) {
  return profileChannel(profile, channel).maxIntensityPercent;
}

uint16_t effectiveManualDuty(const config::LightingConfig& config,
                             const LightingProfile& profile,
                             const LightingCommand& command) {
  if (!effectiveEnabled(config, profile, command.channel)) {
    return 0;
  }

  uint16_t duty = command.requestedDuty;
  const uint16_t maxPwm = effectiveMaxPwm(config, profile, command.channel);
  if (duty > maxPwm) {
    duty = maxPwm;
  }

  duty = applyPercent(duty,
                      effectiveChannelIntensityPercent(profile, command.channel));
  return applyPercent(duty, effectiveGlobalIntensityPercent(config, profile));
}

}  // namespace

LightingService::LightingService(LightingPwmController& controller,
                                 const config::ConfigManager& configManager,
                                 core::events::EventBus& eventBus,
                                 const LightingProfile& activeProfile,
                                 LightingModuleConfig moduleConfig)
    : controller_(controller),
      configManager_(configManager),
      timeSource_(nullptr),
      eventBus_(eventBus),
      activeProfile_(activeProfile),
      moduleConfig_(moduleConfig),
      lastAppliedProfileSignature_(0),
      hasLastAppliedProfileSignature_(false),
      pendingCommand_{},
      hasPendingCommand_(false),
      acclimationElapsedDays_(0) {}

LightingService::LightingService(
    LightingPwmController& controller,
    const config::ConfigManager& configManager,
    const core::platform::TimeSource& timeSource,
    core::events::EventBus& eventBus, const LightingProfile& activeProfile,
    LightingModuleConfig moduleConfig)
    : controller_(controller),
      configManager_(configManager),
      timeSource_(&timeSource),
      eventBus_(eventBus),
      activeProfile_(activeProfile),
      moduleConfig_(moduleConfig),
      lastAppliedProfileSignature_(0),
      hasLastAppliedProfileSignature_(false),
      pendingCommand_{},
      hasPendingCommand_(false),
      acclimationElapsedDays_(0) {}

void LightingService::setActiveProfile(const LightingProfile& profile) {
  activeProfile_ = profile;
}

const LightingProfile& LightingService::activeProfile() const {
  return activeProfile_;
}

LightingServiceEvaluationResult LightingService::initializeSafeState() {
  LightingServiceEvaluationResult result = {};
  result.result = LightingServiceResult::kSuccess;
  result.pwmResult = LightingPwmResult::kSuccess;

  for (const LightingChannel channel : kChannels) {
    result.pwmResult = controller_.configureChannel(channel, moduleConfig_.pwm);
    if (result.pwmResult != LightingPwmResult::kSuccess) {
      result.result = LightingServiceResult::kControllerFailure;
      return result;
    }
  }

  result.pwmResult = controller_.allOff();
  if (result.pwmResult != LightingPwmResult::kSuccess) {
    result.result = LightingServiceResult::kControllerFailure;
    return result;
  }

  core::state::LightingState lighting =
      core::state::currentSystemState().lighting;
  const config::LightingConfig& config = configManager_.lighting();
  lighting.mode = config.mode;
  lighting.sunriseEnabled = config.sunriseEnabled;
  lighting.sunsetEnabled = config.sunsetEnabled;
  lighting.acclimationEnabled = config.acclimationEnabled;
  for (const LightingChannel channel : kChannels) {
    core::state::LightingChannelState& state =
        mutableStateChannel(lighting, channel);
    state.currentPWM = 0;
    state.maxPWM = effectiveMaxPwm(config, activeProfile_, channel);
    state.enabled = effectiveEnabled(config, activeProfile_, channel);
  }

  core::state::updateLightingState(lighting);
  result.stateUpdated = true;
  return result;
}

LightingServiceResult LightingService::requestMode(
    LightingMode mode, uint32_t requestedAtMillis) {
  if (!isCanonicalLightingMode(mode)) {
    return LightingServiceResult::kInvalidCommand;
  }

  queueCommand(makeLocalLightingModeCommand(mode, requestedAtMillis));
  return LightingServiceResult::kSuccess;
}

LightingServiceResult LightingService::requestProfile(
    uint32_t requestedAtMillis) {
  LightingCommand command = {};
  command.requestedMode = configManager_.lighting().mode;
  command.target = LightingCommandTarget::kProfile;
  command.channel = LightingChannel::kUnknown;
  command.source = LightingCommandSource::kLocal;
  command.requestedAtMillis = requestedAtMillis;
  command.reason = LightingCommandReason::kProfileSelection;
  queueCommand(command);
  return LightingServiceResult::kSuccess;
}

LightingServiceResult LightingService::requestManualDuty(
    LightingChannel channel, uint16_t duty, uint32_t requestedAtMillis) {
  if (!isCanonicalLightingChannel(channel) || !validLightingDuty(duty)) {
    return LightingServiceResult::kInvalidCommand;
  }

  queueCommand(
      makeLocalLightingDutyCommand(MANUAL, channel, duty, requestedAtMillis));
  return LightingServiceResult::kSuccess;
}

void LightingService::setAcclimationElapsedDays(uint16_t elapsedDays) {
  acclimationElapsedDays_ = elapsedDays;
}

LightingServiceEvaluationResult LightingService::evaluateOnce() {
  const LightingCommand* command =
      hasPendingCommand_ ? &pendingCommand_ : nullptr;
  const LightingServiceEvaluationResult result = evaluateOnce(
      currentMinuteOfDay(), command, acclimationElapsedDays_);

  if (result.result != LightingServiceResult::kControllerFailure &&
      pendingCommandConverged()) {
    hasPendingCommand_ = false;
  }

  return result;
}

LightingServiceEvaluationResult LightingService::evaluateOnce(
    uint16_t currentMinuteOfDay, const LightingCommand* command,
    uint16_t acclimationElapsedDays) {
  const LightingPolicyInput input = {
      core::state::currentSystemState().lighting,
      configManager_.lighting(),
      activeProfile_,
      currentMinuteOfDay,
      acclimationElapsedDays,
      command,
      moduleConfig_,
  };

  const LightingPolicyDecision decision = evaluateLightingPolicy(input);
  return applyDecision(decision, command);
}

uint16_t LightingService::currentMinuteOfDay() const {
  if (timeSource_ == nullptr) {
    return 0;
  }

  return static_cast<uint16_t>((timeSource_->uptimeMillis() / 60000UL) %
                               kLightingMinutesPerDay);
}

void LightingService::queueCommand(const LightingCommand& command) {
  pendingCommand_ = command;
  hasPendingCommand_ = true;
}

bool LightingService::pendingCommandConverged() const {
  if (!hasPendingCommand_) {
    return true;
  }

  const core::state::LightingState& lighting =
      core::state::currentSystemState().lighting;
  switch (pendingCommand_.target) {
    case LightingCommandTarget::kChannelDuty: {
      const core::state::LightingChannelState* channel =
          lightingChannelState(lighting, pendingCommand_.channel);
      if (channel == nullptr) {
        return false;
      }

      const uint16_t expectedDuty = effectiveManualDuty(
          configManager_.lighting(), activeProfile_, pendingCommand_);
      return channel->currentPWM == expectedDuty;
    }
    case LightingCommandTarget::kAllChannelsOff:
      return lighting.white.currentPWM == 0 && lighting.blue.currentPWM == 0 &&
             lighting.royalBlue.currentPWM == 0 && lighting.uv.currentPWM == 0;
    case LightingCommandTarget::kMode:
    case LightingCommandTarget::kProfile:
    case LightingCommandTarget::kChannelIntensity:
      return true;
  }

  return true;
}

LightingServiceEvaluationResult LightingService::applyDecision(
    const LightingPolicyDecision& decision, const LightingCommand* command) {
  LightingServiceEvaluationResult result = {};
  result.result = serviceResultForDecision(decision);
  result.pwmResult = LightingPwmResult::kSuccess;

  if (result.result == LightingServiceResult::kInvalidCommand) {
    return result;
  }

  if (!applyPwmWrites(decision, result.pwmResult)) {
    result.result = LightingServiceResult::kControllerFailure;
    return result;
  }

  const core::state::LightingState before =
      core::state::currentSystemState().lighting;
  const core::state::LightingState after =
      makeNextLightingState(decision, command);
  const bool profileChanged = runtimeProfileChanged(before, after, command);

  core::state::updateLightingState(after);
  result.stateUpdated = true;
  publishTransitionEvents(before, after, profileChanged, command, result);
  return result;
}

LightingServiceResult LightingService::serviceResultForDecision(
    const LightingPolicyDecision& decision) const {
  if (decision.accepted) {
    return LightingServiceResult::kSuccess;
  }

  switch (decision.reason) {
    case LightingPolicyReason::kInvalidCommand:
      return LightingServiceResult::kInvalidCommand;
    case LightingPolicyReason::kInvalidConfig:
      return LightingServiceResult::kInvalidConfig;
    case LightingPolicyReason::kInvalidProfile:
      return LightingServiceResult::kInvalidProfile;
    case LightingPolicyReason::kNone:
    case LightingPolicyReason::kManualCommand:
    case LightingPolicyReason::kManualProfile:
    case LightingPolicyReason::kAutomaticCurve:
    case LightingPolicyReason::kSunrise:
    case LightingPolicyReason::kSunset:
    case LightingPolicyReason::kMoonlight:
    case LightingPolicyReason::kAcclimation:
    case LightingPolicyReason::kChannelDisabled:
    case LightingPolicyReason::kFade:
    case LightingPolicyReason::kIdempotent:
      break;
  }

  return LightingServiceResult::kInvalidConfig;
}

bool LightingService::applyPwmWrites(const LightingPolicyDecision& decision,
                                     LightingPwmResult& pwmResult) {
  for (size_t index = 0; index < kLightingFunctionalChannelCount; ++index) {
    const LightingPolicyChannelDecision& channel = decision.channels[index];
    if (!channel.shouldWrite) {
      continue;
    }

    pwmResult = controller_.writeDuty(channel.channel, channel.nextDuty);
    if (pwmResult != LightingPwmResult::kSuccess) {
      return false;
    }
  }

  pwmResult = LightingPwmResult::kSuccess;
  return true;
}

core::state::LightingState LightingService::makeNextLightingState(
    const LightingPolicyDecision& decision, const LightingCommand* command)
    const {
  core::state::LightingState next = core::state::currentSystemState().lighting;
  const config::LightingConfig& config = configManager_.lighting();

  if (decision.accepted) {
    next.mode = decision.mode;
    next.sunriseEnabled = config.sunriseEnabled;
    next.sunsetEnabled = config.sunsetEnabled;
    next.acclimationEnabled = config.acclimationEnabled;

    if (evaluationUsesProfile(decision.mode, command)) {
      copyProfileName(next.currentProfile, activeProfile_.name);
    }
  }

  for (size_t index = 0; index < kLightingFunctionalChannelCount; ++index) {
    const LightingPolicyChannelDecision& channelDecision =
        decision.channels[index];
    core::state::LightingChannelState& channel =
        mutableStateChannel(next, channelDecision.channel);
    channel.currentPWM = channelDecision.nextDuty;
    if (decision.accepted) {
      channel.maxPWM = effectiveMaxPwm(config, activeProfile_,
                                       channelDecision.channel);
      channel.enabled = effectiveEnabled(config, activeProfile_,
                                         channelDecision.channel);
    }
  }

  return next;
}

void LightingService::publishTransitionEvents(
    const core::state::LightingState& before,
    const core::state::LightingState& after, bool profileChanged,
    const LightingCommand* command,
    LightingServiceEvaluationResult& result) {
  if (profileChanged) {
    publishLightingEvent(LightingEventType::kLightingProfileChanged, after,
                         kLightingChannelMaskAll, command);
    result.emittedProfileChanged = true;
  }

  const bool wasActive = anyChannelActive(before);
  const bool isActive = anyChannelActive(after);
  if (!wasActive && isActive) {
    publishLightingEvent(LightingEventType::kLightingStarted, after,
                         activeChannelMask(after), command);
    result.emittedStarted = true;
  } else if (wasActive && !isActive) {
    publishLightingEvent(LightingEventType::kLightingStopped, after,
                         activeChannelMask(before), command);
    result.emittedStopped = true;
  }
}

bool LightingService::runtimeProfileChanged(
    const core::state::LightingState& before,
    const core::state::LightingState& after, const LightingCommand* command) {
  const bool usesProfile = evaluationUsesProfile(after.mode, command);
  const bool modeOrProfileChanged =
      before.mode != after.mode ||
      !textEquals(before.currentProfile, after.currentProfile);
  const bool profileRuntimeFlagsChanged =
      usesProfile &&
      (before.sunriseEnabled != after.sunriseEnabled ||
       before.sunsetEnabled != after.sunsetEnabled ||
       before.acclimationEnabled != after.acclimationEnabled);

  if (modeOrProfileChanged || profileRuntimeFlagsChanged) {
    if (usesProfile) {
      lastAppliedProfileSignature_ = profileSignature(activeProfile_);
      hasLastAppliedProfileSignature_ = true;
    }
    return true;
  }

  if (!usesProfile) {
    return false;
  }

  const uint32_t signature = profileSignature(activeProfile_);
  const bool changed =
      hasLastAppliedProfileSignature_ &&
      signature != lastAppliedProfileSignature_;
  lastAppliedProfileSignature_ = signature;
  hasLastAppliedProfileSignature_ = true;
  return changed;
}

void LightingService::publishLightingEvent(
    LightingEventType eventType, const core::state::LightingState& state,
    LightingChannelMask affectedChannels, const LightingCommand* command) {
  LightingEvent lightingEvent = {};
  lightingEvent.type = eventType;
  lightingEvent.mode = state.mode;
  lightingEvent.affectedChannels = affectedChannels;
  copyProfileName(lightingEvent.profile, state.currentProfile);
  lightingEvent.occurredAtMillis =
      timeSource_ != nullptr
          ? timeSource_->uptimeMillis()
          : (command != nullptr ? command->requestedAtMillis : 0);

  core::events::Event event = {};
  event.type = lightingCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kLighting;
  event.payload = &lightingEvent;
  eventBus_.publish(event);
}

bool runLightingServiceTask(void* context) {
  if (context == nullptr) {
    return false;
  }

  LightingService* service = static_cast<LightingService*>(context);
  return service->evaluateOnce().result == LightingServiceResult::kSuccess;
}

}  // namespace reeflow::modules::lighting
