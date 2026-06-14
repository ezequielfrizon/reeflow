#include "app/core_app.h"

#include "core/state/system_state.h"
#include "modules/ato/ato_config.h"
#include "modules/ato/ato_service.h"
#include "modules/lighting/lighting_service.h"
#include "modules/temperature/temperature_service.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_service.h"

namespace reeflow::app {
namespace {

constexpr const char* kTemperatureTaskName = "temperature";
constexpr const char* kWaterLevelTaskName = "water-level";
constexpr const char* kAtoTaskName = "ato";
constexpr const char* kModeTaskName = "modes";
constexpr const char* kLightingTaskName = "lighting";
constexpr const char* kStorageFlushTaskName = "storage-flush";

config::LightingChannelConfig lightingChannelConfig(
    const modules::lighting::LightingChannelProfile& profileChannel) {
  return {profileChannel.enabled, profileChannel.maxDuty};
}

config::LightingConfig lightingConfigFromProfile(
    const modules::lighting::LightingProfile& profile,
    core::state::LightingMode mode) {
  config::LightingConfig config = {};
  config.mode = mode;
  config.sunriseEnabled = profile.sunriseEnabled;
  config.sunsetEnabled = profile.sunsetEnabled;
  config.acclimationEnabled = mode == modules::lighting::ACCLIMATION &&
                              profile.acclimationEnabled;
  config.startMinuteOfDay = profile.startMinuteOfDay;
  config.endMinuteOfDay = profile.endMinuteOfDay;
  config.maxIntensityPercent = profile.maxGlobalIntensityPercent;
  config.acclimationDays = profile.acclimationDurationDays;
  config.white = lightingChannelConfig(profile.white);
  config.blue = lightingChannelConfig(profile.blue);
  config.royalBlue = lightingChannelConfig(profile.royalBlue);
  config.uv = lightingChannelConfig(profile.uv);
  return config;
}

}  // namespace

CoreApp::CoreApp(core::platform::CorePlatform& platform,
                 core::events::EventBus& eventBus,
                 config::ConfigManager& config,
                 modules::temperature::TemperatureSensor& temperatureSensor,
                 modules::water_level::WaterLevelSensor& waterLevelSensor,
                 modules::relays::RelayController& relayController,
                 modules::lighting::LightingPwmController& lightingController,
                 modules::modes::ModeStore& modeStore,
                 storage::StorageService* storageService)
    : platform_(platform),
      eventBus_(eventBus),
      config_(config),
      storageService_(storageService),
      logger_(platform_.logSink()),
      scheduler_(platform_.timeSource(), eventBus_, logger_),
      watchdog_(platform_.watchdogBackend(), platform_.timeSource(), logger_),
      temperatureService_(temperatureSensor, config_, platform_.timeSource(),
                          eventBus_),
      waterLevelService_(waterLevelSensor, config_, platform_.timeSource(),
                         eventBus_),
      relayService_(relayController, platform_.timeSource(), eventBus_),
      modeEffects_(relayService_, modeAutomationGate_),
      modeService_(config_, platform_.timeSource(), eventBus_, modeEffects_,
                   modeStore),
      atoService_(relayService_, config_, platform_.timeSource(), eventBus_,
                  modules::ato::makeDefaultAtoModuleConfig(),
                  &modeAutomationGate_),
      lightingPersistedState_({}),
      lightingProfile_(modules::lighting::makeDefaultLightingProfile()),
      lightingModuleConfig_(modules::lighting::makeDefaultLightingModuleConfig()),
      lightingService_(lightingController, config_, platform_.timeSource(),
                       eventBus_, lightingProfile_, lightingModuleConfig_) {}

bool CoreApp::setup() {
  core::state::setSystemStateEventBus(eventBus_);
  core::state::resetSystemState();
  config_.loadDefaults();
  lightingProfile_ = modules::lighting::makeDefaultLightingProfile();
  syncLightingPersistedState();
  lightingService_.setActiveProfile(lightingProfile_);
  if (!config_.updateLighting(lightingConfigFromProfile(
          lightingProfile_, core::state::LightingMode::kManual))) {
    logger_.error("core", "lighting config failed");
    return false;
  }
  scheduler_.clear();

  if (!watchdog_.initialize(kCoreWatchdogTimeoutMillis)) {
    logger_.error("core", "watchdog init failed");
    return false;
  }

  const uint8_t watchdogTaskId =
      scheduler_.registerTask("watchdog", kCoreWatchdogFeedIntervalMillis,
                              core::watchdog::feedWatchdogTask, &watchdog_);
  if (watchdogTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "watchdog task failed");
    return false;
  }

  const modules::relays::RelayCommandResult relaySafeStateResult =
      relayService_.allOff(core::state::RelaySource::kFailsafe);
  if (relaySafeStateResult !=
      modules::relays::RelayCommandResult::kSuccess) {
    logger_.error("core", "relay safe state failed");
    return false;
  }

  restorePersistedConfiguration();

  const modules::modes::ModeServiceResult modeRestoreResult =
      modeService_.restoreModeFromStore();
  if (modeRestoreResult != modules::modes::ModeServiceResult::kSuccess) {
    logger_.error("core", "mode restore failed");
  }

  temperatureService_.resetMonitor();
  const uint8_t temperatureTaskId = scheduler_.registerTask(
      kTemperatureTaskName, config_.temperature().readIntervalMillis,
      modules::temperature::runTemperatureServiceTask,
      &temperatureService_);
  if (temperatureTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "temperature task failed");
    return false;
  }

  waterLevelService_.resetMonitor();
  const modules::water_level::WaterLevelModuleConfig waterLevelConfig =
      modules::water_level::makeDefaultWaterLevelModuleConfig();
  const uint8_t waterLevelTaskId = scheduler_.registerTask(
      kWaterLevelTaskName, waterLevelConfig.readIntervalMillis,
      modules::water_level::runWaterLevelServiceTask,
      &waterLevelService_);
  if (waterLevelTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "water level task failed");
    return false;
  }

  const modules::ato::AtoModuleConfig atoConfig =
      modules::ato::makeDefaultAtoModuleConfig();
  const uint8_t atoTaskId = scheduler_.registerTask(
      kAtoTaskName, atoConfig.evaluationIntervalMillis,
      modules::ato::runAtoServiceTask, &atoService_);
  if (atoTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "ato task failed");
    return false;
  }

  const uint8_t modeTaskId = scheduler_.registerTask(
      kModeTaskName, modules::modes::kModeServiceTaskIntervalMillis,
      modules::modes::runModeServiceTask, &modeService_);
  if (modeTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "mode task failed");
    return false;
  }

  if (!registerStorageFlushTask()) {
    logger_.error("core", "storage flush task failed");
    return false;
  }

  const modules::lighting::LightingServiceEvaluationResult lightingInit =
      lightingService_.initializeSafeState();
  if (lightingInit.result != modules::lighting::LightingServiceResult::kSuccess) {
    logger_.error("core", "lighting safe state failed");
    return false;
  }

  const uint8_t lightingTaskId = scheduler_.registerTask(
      kLightingTaskName, lightingModuleConfig_.evaluationIntervalMillis,
      modules::lighting::runLightingServiceTask, &lightingService_);
  if (lightingTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "lighting task failed");
    return false;
  }

  initialized_ = true;
  logger_.info("core", "initialized");
  return true;
}

core::scheduler::SchedulerRunResult CoreApp::loopOnce() {
  core::scheduler::SchedulerRunResult result = {};
  if (!initialized_) {
    return result;
  }

  return scheduler_.runDueTasks();
}

modules::relays::RelayCommandResult CoreApp::setLocalRelay(
    modules::relays::RelayId relay,
    modules::relays::RelayDesiredState desiredState) {
  if (!initialized_) {
    return modules::relays::RelayCommandResult::kControllerFailure;
  }

  return relayService_.setLocalRelay(relay, desiredState);
}

modules::modes::ModeServiceResult CoreApp::requestMode(
    modules::modes::OperationalMode mode) {
  if (!initialized_) {
    return modules::modes::ModeServiceResult::kInvalidCommand;
  }

  return modeService_.requestMode(modules::modes::makeLocalModeCommand(
      mode, platform_.timeSource().uptimeMillis()));
}

modules::lighting::LightingServiceResult CoreApp::requestLightingManualMode() {
  if (!initialized_) {
    return modules::lighting::LightingServiceResult::kInvalidCommand;
  }

  if (!config_.updateLighting(lightingConfigFromProfile(
          lightingProfile_, core::state::LightingMode::kManual))) {
    return modules::lighting::LightingServiceResult::kInvalidConfig;
  }

  return lightingService_.requestMode(
      modules::lighting::MANUAL, platform_.timeSource().uptimeMillis());
}

modules::lighting::LightingServiceResult
CoreApp::requestLightingAutomaticMode() {
  if (!initialized_) {
    return modules::lighting::LightingServiceResult::kInvalidCommand;
  }

  if (!config_.updateLighting(lightingConfigFromProfile(
          lightingProfile_, core::state::LightingMode::kAutomatic))) {
    return modules::lighting::LightingServiceResult::kInvalidConfig;
  }

  return lightingService_.requestMode(
      modules::lighting::AUTOMATIC, platform_.timeSource().uptimeMillis());
}

modules::lighting::LightingServiceResult
CoreApp::requestLightingAcclimationMode() {
  if (!initialized_) {
    return modules::lighting::LightingServiceResult::kInvalidCommand;
  }

  if (!config_.updateLighting(lightingConfigFromProfile(
          lightingProfile_, core::state::LightingMode::kAcclimation))) {
    return modules::lighting::LightingServiceResult::kInvalidConfig;
  }

  lightingService_.setAcclimationElapsedDays(
      config_.lighting().acclimationDays > 0 ? 1 : 0);
  return lightingService_.requestMode(
      modules::lighting::ACCLIMATION, platform_.timeSource().uptimeMillis());
}

modules::lighting::LightingServiceResult CoreApp::setLightingProfile(
    const modules::lighting::LightingProfile& profile) {
  if (!initialized_) {
    return modules::lighting::LightingServiceResult::kInvalidCommand;
  }

  if (modules::lighting::validateLightingProfile(profile) !=
      modules::lighting::LightingProfileValidationResult::kValid) {
    return modules::lighting::LightingServiceResult::kInvalidProfile;
  }

  lightingProfile_ = profile;
  lightingService_.setActiveProfile(lightingProfile_);
  syncLightingPersistedState();
  if (!config_.updateLighting(
          lightingConfigFromProfile(lightingProfile_, config_.lighting().mode))) {
    return modules::lighting::LightingServiceResult::kInvalidConfig;
  }

  return lightingService_.requestProfile(platform_.timeSource().uptimeMillis());
}

modules::lighting::LightingServiceResult CoreApp::setLightingManualDuty(
    modules::lighting::LightingChannel channel, uint16_t duty) {
  if (!initialized_) {
    return modules::lighting::LightingServiceResult::kInvalidCommand;
  }

  if (!config_.updateLighting(lightingConfigFromProfile(
          lightingProfile_, core::state::LightingMode::kManual))) {
    return modules::lighting::LightingServiceResult::kInvalidConfig;
  }

  return lightingService_.requestManualDuty(
      channel, duty, platform_.timeSource().uptimeMillis());
}

config::ConfigManager& CoreApp::configManager() {
  return config_;
}

core::events::EventBus& CoreApp::eventBus() {
  return eventBus_;
}

core::scheduler::TaskScheduler& CoreApp::scheduler() {
  return scheduler_;
}

core::watchdog::WatchdogService& CoreApp::watchdog() {
  return watchdog_;
}

bool CoreApp::handleConfigChanged(const core::events::Event& event,
                                  void* context) {
  if (context == nullptr ||
      event.type != core::events::EventType::kConfigChanged) {
    return false;
  }

  static_cast<CoreApp*>(context)->trackConfigChange(event.configDomain);
  return true;
}

bool CoreApp::registerStorageFlushTask() {
  if (storageService_ == nullptr) {
    return true;
  }

  eventBus_.subscribe(core::events::EventType::kConfigChanged,
                      CoreApp::handleConfigChanged, this);

  const uint8_t storageTaskId = scheduler_.registerTask(
      kStorageFlushTaskName, kStorageFlushIntervalMillis,
      storage::runStorageFlushTask, storageService_);
  return storageTaskId != core::scheduler::kInvalidTaskId;
}

void CoreApp::restorePersistedConfiguration() {
  if (storageService_ == nullptr) {
    return;
  }

  const uint32_t nowMillis = platform_.timeSource().uptimeMillis();
  storageService_->restoreWifi(config_, nowMillis);
  storageService_->restoreMqtt(config_, nowMillis);
  storageService_->restoreTemperature(config_, nowMillis);
  storageService_->restoreAto(config_, nowMillis);
  storageService_->restoreTimers(config_, nowMillis);
  storageService_->restoreCalibrations(config_, nowMillis);

  if (storageService_->restoreLighting(config_, lightingPersistedState_,
                                       nowMillis) ==
      storage::StorageResult::kSuccess) {
    lightingProfile_ =
        lightingPersistedState_
            .profiles[lightingPersistedState_.currentProfileIndex];
    lightingService_.setActiveProfile(lightingProfile_);
  } else {
    syncLightingPersistedState();
  }
}

void CoreApp::trackConfigChange(core::events::ConfigDomain domain) {
  if (storageService_ == nullptr) {
    return;
  }

  if (domain == core::events::ConfigDomain::kLighting) {
    syncLightingPersistedState();
    storageService_->markLightingChanged(lightingPersistedState_);
    return;
  }

  storageService_->markConfigChanged(config_, domain);
}

void CoreApp::syncLightingPersistedState() {
  lightingPersistedState_ = {};
  lightingPersistedState_.config = config_.lighting();
  lightingPersistedState_.profiles[0] = lightingProfile_;
  lightingPersistedState_.profileCount = 1;
  lightingPersistedState_.currentProfileIndex = 0;
}

}  // namespace reeflow::app
