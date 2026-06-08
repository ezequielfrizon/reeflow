#include <assert.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "modules/ato/ato_config.h"
#include "modules/ato/ato_policy.h"
#include "modules/ato/ato_types.h"

namespace {

using reeflow::config::AtoConfig;
using reeflow::config::ConfigManager;
using reeflow::core::events::EventBus;
using reeflow::core::state::AtoState;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::RelayEntryState;
using reeflow::core::state::RelaySource;
using reeflow::core::state::WaterLevelState;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::ato::AtoDecision;
using reeflow::modules::ato::AtoDecisionReason;
using reeflow::modules::ato::AtoPolicyInput;
using reeflow::modules::ato::AtoPumpCommand;
using reeflow::modules::ato::atoConfigIsValid;
using reeflow::modules::ato::evaluateAtoConfiguration;
using reeflow::modules::ato::evaluateAtoPolicy;
using reeflow::modules::ato::makeDefaultAtoModuleConfig;

AtoConfig defaultAtoConfig() {
  return reeflow::config::makeDefaultReeflowConfig().ato;
}

AtoConfig enabledAtoConfig() {
  AtoConfig config = defaultAtoConfig();
  config.enabled = true;
  return config;
}

RelayEntryState relayState(bool enabled) {
  RelayEntryState state = {};
  state.enabled = enabled;
  state.lastChanged = 0;
  state.source = RelaySource::kLocal;
  return state;
}

AtoState atoState(bool pumpRunning, AtoStatus status) {
  AtoState state = {};
  state.enabled = true;
  state.status = status;
  state.pumpRunning = pumpRunning;
  state.lastActivation = 10;
  state.lastCompletion = 20;
  state.timeoutCounter = 1;
  return state;
}

WaterLevelState waterLevelState() {
  WaterLevelState state = {};
  state.currentLevel = 50;
  state.minimumLevel = 20;
  state.maximumLevel = 80;
  state.status = WaterLevelStatus::kNormal;
  state.lastUpdate = 30;
  return state;
}

AtoPolicyInput inputWithConfig(const AtoConfig& config) {
  AtoPolicyInput input = {};
  input.waterLevel = waterLevelState();
  input.ato = atoState(false, AtoStatus::kNormal);
  input.atoPumpRelay = relayState(false);
  input.config = config;
  input.nowMillis = 100;
  return input;
}

void assertInvalidConfigBlocksSafely(const AtoConfig& config) {
  const AtoPolicyInput input = inputWithConfig(config);
  const auto evaluation = evaluateAtoConfiguration(input);

  assert(!atoConfigIsValid(config));
  assert(evaluation.decision == AtoDecision::kBlockInvalidConfig);
  assert(evaluation.reason == AtoDecisionReason::kInvalidConfig);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
  assert(!evaluation.metadata.requiresRelayCommand);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kNone);
  assert(!evaluation.metadata.emitsEvent);
}

void testDefaultAtoModuleConfigUsesOneSecondEvaluationInterval() {
  const auto config = makeDefaultAtoModuleConfig();

  assert(config.evaluationIntervalMillis == 1000);
  assert(config.canonicalMinimumLevel == 0);
  assert(config.canonicalMaximumLevel == 100);
}

void testValidConfigFromConfigManagerIsAccepted() {
  EventBus eventBus;
  ConfigManager configManager(eventBus);
  AtoConfig config = configManager.ato();
  config.enabled = true;
  config.minimumLevel = 25;
  config.maximumLevel = 75;
  config.timeoutMillis = 120000;
  config.cooldownMillis = 30000;
  assert(configManager.updateAto(config));

  const AtoPolicyInput input = inputWithConfig(configManager.ato());
  const auto evaluation = evaluateAtoConfiguration(input);

  assert(atoConfigIsValid(configManager.ato()));
  assert(evaluation.decision == AtoDecision::kNoAction);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
  assert(!evaluation.metadata.requiresRelayCommand);
}

void testMinimumEqualMaximumIsBlocked() {
  AtoConfig config = defaultAtoConfig();
  config.minimumLevel = 50;
  config.maximumLevel = 50;

  assertInvalidConfigBlocksSafely(config);
}

void testMinimumGreaterThanMaximumIsBlocked() {
  AtoConfig config = defaultAtoConfig();
  config.minimumLevel = 80;
  config.maximumLevel = 20;

  assertInvalidConfigBlocksSafely(config);
}

void testMinimumAboveCanonicalMaximumIsBlocked() {
  AtoConfig config = defaultAtoConfig();
  config.minimumLevel = 101;
  config.maximumLevel = 120;

  assertInvalidConfigBlocksSafely(config);
}

void testMaximumAboveCanonicalMaximumIsBlocked() {
  AtoConfig config = defaultAtoConfig();
  config.maximumLevel = 101;

  assertInvalidConfigBlocksSafely(config);
}

void testZeroTimeoutIsBlocked() {
  AtoConfig config = defaultAtoConfig();
  config.timeoutMillis = 0;

  assertInvalidConfigBlocksSafely(config);
}

void testZeroCooldownIsBlocked() {
  AtoConfig config = defaultAtoConfig();
  config.cooldownMillis = 0;

  assertInvalidConfigBlocksSafely(config);
}

void testInvalidConfigRequiresFailsafeOffWhenAtoPumpIsRunning() {
  AtoConfig config = defaultAtoConfig();
  config.maximumLevel = 101;
  AtoPolicyInput input = inputWithConfig(config);
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);

  const auto evaluation = evaluateAtoConfiguration(input);

  assert(evaluation.decision == AtoDecision::kBlockInvalidConfig);
  assert(evaluation.nextStatus == AtoStatus::kRefilling);
  assert(evaluation.metadata.requiresRelayCommand);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
  assert(evaluation.metadata.updatesAtoState);
}

void testInvalidConfigDoesNotMutateWaterLevel() {
  AtoConfig config = defaultAtoConfig();
  config.timeoutMillis = 0;
  AtoPolicyInput input = inputWithConfig(config);
  const WaterLevelState before = input.waterLevel;

  const auto evaluation = evaluateAtoConfiguration(input);

  assert(evaluation.decision == AtoDecision::kBlockInvalidConfig);
  assert(input.waterLevel.currentLevel == before.currentLevel);
  assert(input.waterLevel.minimumLevel == before.minimumLevel);
  assert(input.waterLevel.maximumLevel == before.maximumLevel);
  assert(input.waterLevel.status == before.status);
  assert(input.waterLevel.lastUpdate == before.lastUpdate);
}

void testSafetyPriorityDisablesBeforeInvalidConfig() {
  AtoConfig config = defaultAtoConfig();
  config.enabled = false;
  config.maximumLevel = 101;
  AtoPolicyInput input = inputWithConfig(config);
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kDisable);
  assert(evaluation.reason == AtoDecisionReason::kConfigDisabled);
  assert(evaluation.nextStatus == AtoStatus::kDisabled);
  assert(!evaluation.nextAto.enabled);
  assert(!evaluation.nextAto.pumpRunning);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
}

void testDisabledWithPumpOffReturnsDisabledWithoutRelayCommand() {
  AtoConfig config = enabledAtoConfig();
  config.enabled = false;
  AtoPolicyInput input = inputWithConfig(config);
  input.ato = atoState(false, AtoStatus::kNormal);
  input.atoPumpRelay = relayState(false);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kDisable);
  assert(evaluation.nextStatus == AtoStatus::kDisabled);
  assert(!evaluation.metadata.requiresRelayCommand);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kNone);
}

void testDisabledWithAtoPumpRunningRequiresFailsafeOff() {
  AtoConfig config = enabledAtoConfig();
  config.enabled = false;
  AtoPolicyInput input = inputWithConfig(config);
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(false);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kDisable);
  assert(evaluation.metadata.requiresRelayCommand);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
  assert(!evaluation.nextAto.pumpRunning);
}

void testDisabledWithRelayPumpRunningRequiresFailsafeOff() {
  AtoConfig config = enabledAtoConfig();
  config.enabled = false;
  AtoPolicyInput input = inputWithConfig(config);
  input.ato = atoState(false, AtoStatus::kNormal);
  input.atoPumpRelay = relayState(true);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kDisable);
  assert(evaluation.metadata.requiresRelayCommand);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
}

void testInvalidConfigWithEnabledAtoBlocksBeforeSensorOffline() {
  AtoConfig config = enabledAtoConfig();
  config.maximumLevel = 101;
  AtoPolicyInput input = inputWithConfig(config);
  input.waterLevel.status = WaterLevelStatus::kSensorOffline;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kBlockInvalidConfig);
  assert(evaluation.reason == AtoDecisionReason::kInvalidConfig);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
}

void testPumpRunningDivergenceIsReconciledByFailsafe() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(false);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kReconcilePumpDivergence);
  assert(evaluation.reason == AtoDecisionReason::kPumpStateDiverged);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
  assert(!evaluation.nextAto.pumpRunning);
}

void testRelayRunningDivergenceIsReconciledByFailsafe() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(false, AtoStatus::kNormal);
  input.atoPumpRelay = relayState(true);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kReconcilePumpDivergence);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
}

void testSensorOfflineBlocksWithoutRelayWhenPumpOff() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.status = WaterLevelStatus::kSensorOffline;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kBlockSensorOffline);
  assert(evaluation.reason == AtoDecisionReason::kSensorOffline);
  assert(evaluation.nextStatus == AtoStatus::kSensorOffline);
  assert(!evaluation.metadata.requiresRelayCommand);
}

void testSensorOfflineBlocksWithFailsafeWhenPumpOn() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.status = WaterLevelStatus::kSensorOffline;
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kBlockSensorOffline);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
  assert(!evaluation.nextAto.pumpRunning);
}

void testImpossibleLevelAboveCanonicalMaximumBlocksAsSensorOffline() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.currentLevel = 101;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kBlockSensorOffline);
  assert(evaluation.reason == AtoDecisionReason::kInvalidSensorReading);
  assert(evaluation.nextStatus == AtoStatus::kSensorOffline);
}

void testLowLevelStartsRefillWhenCooldownHasExpired() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.currentLevel = input.config.minimumLevel;
  input.ato.lastCompletion = 1000;
  input.nowMillis = input.ato.lastCompletion + input.config.cooldownMillis;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kStartRefill);
  assert(evaluation.reason == AtoDecisionReason::kLevelAtOrBelowMinimum);
  assert(evaluation.nextStatus == AtoStatus::kRefilling);
  assert(evaluation.nextAto.pumpRunning);
  assert(evaluation.nextAto.lastActivation == input.nowMillis);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOn);
  assert(evaluation.metadata.emitsEvent);
}

void testLowLevelBelowMinimumStartsRefill() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.currentLevel = input.config.minimumLevel - 1;
  input.ato.lastCompletion = 0;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kStartRefill);
  assert(evaluation.nextAto.pumpRunning);
}

void testLowLevelDuringCooldownDoesNotStartRefill() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.currentLevel = input.config.minimumLevel;
  input.ato.lastCompletion = 1000;
  input.nowMillis = input.ato.lastCompletion + input.config.cooldownMillis - 1;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kBlockCooldown);
  assert(evaluation.reason == AtoDecisionReason::kCooldownActive);
  assert(!evaluation.nextAto.pumpRunning);
  assert(!evaluation.metadata.requiresRelayCommand);
}

void testNormalLevelDoesNotStartRefill() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.currentLevel = 50;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kNoAction);
  assert(!evaluation.nextAto.pumpRunning);
}

void testHighLevelDoesNotStartRefillWhenPumpIsOff() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.waterLevel.currentLevel = input.config.maximumLevel;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kNoAction);
  assert(!evaluation.nextAto.pumpRunning);
}

void testRefillingContinuesBeforeMaximumAndTimeout() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);
  input.ato.lastActivation = 1000;
  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis - 1;
  input.waterLevel.currentLevel = input.config.maximumLevel - 1;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kNoAction);
  assert(evaluation.nextAto.pumpRunning);
  assert(evaluation.nextStatus == AtoStatus::kRefilling);
}

void testRefillingStopsAtMaximumLevel() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);
  input.ato.lastActivation = 1000;
  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis - 1;
  input.waterLevel.currentLevel = input.config.maximumLevel;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kStopAtMaximumLevel);
  assert(evaluation.reason == AtoDecisionReason::kLevelAtOrAboveMaximum);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
  assert(!evaluation.nextAto.pumpRunning);
  assert(evaluation.nextAto.lastCompletion == input.nowMillis);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffAutomation);
}

void testRefillingDoesNotTimeoutBeforeConfiguredDuration() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);
  input.ato.lastActivation = 1000;
  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis - 1;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision != AtoDecision::kStopTimeout);
}

void testRefillingTimeoutAtConfiguredDuration() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);
  input.ato.lastActivation = 1000;
  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis;
  input.waterLevel.currentLevel = input.config.maximumLevel - 1;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kStopTimeout);
  assert(evaluation.reason == AtoDecisionReason::kTimeoutElapsed);
  assert(evaluation.nextStatus == AtoStatus::kTimeout);
  assert(!evaluation.nextAto.pumpRunning);
  assert(evaluation.nextAto.lastCompletion == input.nowMillis);
  assert(evaluation.nextAto.timeoutCounter == input.ato.timeoutCounter + 1);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kTurnOffFailsafe);
}

void testRefillingTimeoutAfterConfiguredDuration() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);
  input.ato.lastActivation = 1000;
  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis + 1;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kStopTimeout);
}

void testLevelThatDoesNotRiseEventuallyTimesOut() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(true, AtoStatus::kRefilling);
  input.atoPumpRelay = relayState(true);
  input.ato.lastActivation = 1000;
  input.waterLevel.currentLevel = input.config.minimumLevel;

  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis - 1;
  assert(evaluateAtoPolicy(input).decision != AtoDecision::kStopTimeout);

  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis;
  assert(evaluateAtoPolicy(input).decision == AtoDecision::kStopTimeout);
}

void testTimeoutIsNotDuplicatedWhenAlreadyTimedOut() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(false, AtoStatus::kTimeout);
  input.atoPumpRelay = relayState(false);
  input.waterLevel.currentLevel = input.config.minimumLevel;
  input.nowMillis = input.ato.lastActivation + input.config.timeoutMillis + 10;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision != AtoDecision::kStopTimeout);
  assert(evaluation.nextAto.timeoutCounter == input.ato.timeoutCounter);
}

void testRecoveryFromTimeoutAtAcceptableLevel() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(false, AtoStatus::kTimeout);
  input.atoPumpRelay = relayState(false);
  input.waterLevel.currentLevel = 50;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kRecover);
  assert(evaluation.reason == AtoDecisionReason::kRecoveredFromTimeout);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
  assert(!evaluation.nextAto.pumpRunning);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kNone);
}

void testRecoveryFromSensorOfflineAtAcceptableLevel() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(false, AtoStatus::kSensorOffline);
  input.atoPumpRelay = relayState(false);
  input.waterLevel.status = WaterLevelStatus::kNormal;
  input.waterLevel.currentLevel = 50;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kRecover);
  assert(evaluation.reason == AtoDecisionReason::kRecoveredFromSensorOffline);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
  assert(!evaluation.nextAto.pumpRunning);
}

void testRecoveryDoesNotStartPumpInSameEvaluation() {
  AtoPolicyInput input = inputWithConfig(enabledAtoConfig());
  input.ato = atoState(false, AtoStatus::kSensorOffline);
  input.atoPumpRelay = relayState(false);
  input.waterLevel.status = WaterLevelStatus::kNormal;
  input.waterLevel.currentLevel = 50;

  const auto evaluation = evaluateAtoPolicy(input);

  assert(evaluation.decision == AtoDecision::kRecover);
  assert(!evaluation.nextAto.pumpRunning);
  assert(!evaluation.metadata.requiresRelayCommand);
}

}  // namespace

int main() {
  testDefaultAtoModuleConfigUsesOneSecondEvaluationInterval();
  testValidConfigFromConfigManagerIsAccepted();
  testMinimumEqualMaximumIsBlocked();
  testMinimumGreaterThanMaximumIsBlocked();
  testMinimumAboveCanonicalMaximumIsBlocked();
  testMaximumAboveCanonicalMaximumIsBlocked();
  testZeroTimeoutIsBlocked();
  testZeroCooldownIsBlocked();
  testInvalidConfigRequiresFailsafeOffWhenAtoPumpIsRunning();
  testInvalidConfigDoesNotMutateWaterLevel();
  testSafetyPriorityDisablesBeforeInvalidConfig();
  testDisabledWithPumpOffReturnsDisabledWithoutRelayCommand();
  testDisabledWithAtoPumpRunningRequiresFailsafeOff();
  testDisabledWithRelayPumpRunningRequiresFailsafeOff();
  testInvalidConfigWithEnabledAtoBlocksBeforeSensorOffline();
  testPumpRunningDivergenceIsReconciledByFailsafe();
  testRelayRunningDivergenceIsReconciledByFailsafe();
  testSensorOfflineBlocksWithoutRelayWhenPumpOff();
  testSensorOfflineBlocksWithFailsafeWhenPumpOn();
  testImpossibleLevelAboveCanonicalMaximumBlocksAsSensorOffline();
  testLowLevelStartsRefillWhenCooldownHasExpired();
  testLowLevelBelowMinimumStartsRefill();
  testLowLevelDuringCooldownDoesNotStartRefill();
  testNormalLevelDoesNotStartRefill();
  testHighLevelDoesNotStartRefillWhenPumpIsOff();
  testRefillingContinuesBeforeMaximumAndTimeout();
  testRefillingStopsAtMaximumLevel();
  testRefillingDoesNotTimeoutBeforeConfiguredDuration();
  testRefillingTimeoutAtConfiguredDuration();
  testRefillingTimeoutAfterConfiguredDuration();
  testLevelThatDoesNotRiseEventuallyTimesOut();
  testTimeoutIsNotDuplicatedWhenAlreadyTimedOut();
  testRecoveryFromTimeoutAtAcceptableLevel();
  testRecoveryFromSensorOfflineAtAcceptableLevel();
  testRecoveryDoesNotStartPumpInSameEvaluation();
  return 0;
}
