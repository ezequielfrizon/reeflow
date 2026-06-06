#include "config/config_manager.h"

#include <string.h>

namespace reeflow::config {
namespace {

constexpr float kMinAllowedTemperatureC = 0.0F;
constexpr float kMaxAllowedTemperatureC = 40.0F;
constexpr uint32_t kDefaultTemperatureReadIntervalMillis = 5000;
constexpr uint32_t kDefaultSensorOfflineTimeoutMillis = 30000;
constexpr uint32_t kDefaultAtoTimeoutMillis = 60000;
constexpr uint32_t kDefaultAtoCooldownMillis = 300000;
constexpr uint16_t kDefaultAtoMinimumLevel = 20;
constexpr uint16_t kDefaultAtoMaximumLevel = 80;
constexpr uint16_t kMinutesPerDay = 1440;
constexpr uint16_t kMaxPwmValue = 255;
constexpr uint32_t kMaxTimerDurationSeconds = 86400;
constexpr uint32_t kDefaultFeedingDurationSeconds = 600;
constexpr uint16_t kDefaultMqttPort = 1883;
constexpr uint32_t kDefaultMqttHeartbeatIntervalMillis = 30000;
constexpr float kMaxTemperatureCalibrationOffset = 10.0F;
constexpr int16_t kMaxWaterLevelCalibrationOffset = 1000;

bool textIsTerminated(const char* value, size_t length) {
  return memchr(value, '\0', length) != nullptr;
}

bool textIsPresent(const char* value, size_t length) {
  if (!textIsTerminated(value, length)) {
    return false;
  }

  return value[0] != '\0';
}

bool sameText(const char* left, const char* right, size_t length) {
  return strncmp(left, right, length) == 0;
}

bool validTemperature(const TemperatureConfig& config) {
  return config.minTemperature >= kMinAllowedTemperatureC &&
         config.maxTemperature <= kMaxAllowedTemperatureC &&
         config.minTemperature < config.maxTemperature &&
         config.targetTemperature >= config.minTemperature &&
         config.targetTemperature <= config.maxTemperature &&
         config.readIntervalMillis > 0 &&
         config.sensorOfflineTimeoutMillis >= config.readIntervalMillis;
}

bool sameTemperature(const TemperatureConfig& left,
                     const TemperatureConfig& right) {
  return left.targetTemperature == right.targetTemperature &&
         left.minTemperature == right.minTemperature &&
         left.maxTemperature == right.maxTemperature &&
         left.readIntervalMillis == right.readIntervalMillis &&
         left.sensorOfflineTimeoutMillis == right.sensorOfflineTimeoutMillis;
}

bool validAto(const AtoConfig& config) {
  return config.minimumLevel < config.maximumLevel &&
         config.timeoutMillis > 0 && config.cooldownMillis > 0;
}

bool sameAto(const AtoConfig& left, const AtoConfig& right) {
  return left.enabled == right.enabled &&
         left.minimumLevel == right.minimumLevel &&
         left.maximumLevel == right.maximumLevel &&
         left.timeoutMillis == right.timeoutMillis &&
         left.cooldownMillis == right.cooldownMillis;
}

bool validLightingChannel(const LightingChannelConfig& config) {
  return config.maxPWM <= kMaxPwmValue;
}

bool validLightingMode(core::state::LightingMode mode) {
  return mode == core::state::LightingMode::kManual ||
         mode == core::state::LightingMode::kAutomatic ||
         mode == core::state::LightingMode::kAcclimation;
}

bool sameLightingChannel(const LightingChannelConfig& left,
                         const LightingChannelConfig& right) {
  return left.enabled == right.enabled && left.maxPWM == right.maxPWM;
}

bool validLighting(const LightingConfig& config) {
  return validLightingMode(config.mode) &&
         config.startMinuteOfDay < kMinutesPerDay &&
         config.endMinuteOfDay < kMinutesPerDay &&
         config.maxIntensityPercent <= 100 &&
         validLightingChannel(config.white) &&
         validLightingChannel(config.blue) &&
         validLightingChannel(config.royalBlue) &&
         validLightingChannel(config.uv);
}

bool sameLighting(const LightingConfig& left, const LightingConfig& right) {
  return left.mode == right.mode &&
         left.sunriseEnabled == right.sunriseEnabled &&
         left.sunsetEnabled == right.sunsetEnabled &&
         left.acclimationEnabled == right.acclimationEnabled &&
         left.startMinuteOfDay == right.startMinuteOfDay &&
         left.endMinuteOfDay == right.endMinuteOfDay &&
         left.maxIntensityPercent == right.maxIntensityPercent &&
         left.acclimationDays == right.acclimationDays &&
         sameLightingChannel(left.white, right.white) &&
         sameLightingChannel(left.blue, right.blue) &&
         sameLightingChannel(left.royalBlue, right.royalBlue) &&
         sameLightingChannel(left.uv, right.uv);
}

bool validTimers(const TimersConfig& config) {
  return config.feedingDurationSeconds <= kMaxTimerDurationSeconds &&
         config.tpaDurationSeconds <= kMaxTimerDurationSeconds &&
         config.maintenanceDurationSeconds <= kMaxTimerDurationSeconds;
}

bool sameTimers(const TimersConfig& left, const TimersConfig& right) {
  return left.feedingDurationSeconds == right.feedingDurationSeconds &&
         left.tpaDurationSeconds == right.tpaDurationSeconds &&
         left.maintenanceDurationSeconds == right.maintenanceDurationSeconds;
}

bool sameMode(const ModeConfig& left, const ModeConfig& right) {
  return left.currentMode == right.currentMode;
}

bool validMode(const ModeConfig& config) {
  return config.currentMode == core::state::OperationalMode::kNormal ||
         config.currentMode == core::state::OperationalMode::kFeeding ||
         config.currentMode == core::state::OperationalMode::kTpa ||
         config.currentMode == core::state::OperationalMode::kMaintenance;
}

bool validWifi(const WifiConfig& config) {
  if (!textIsTerminated(config.ssid, sizeof(config.ssid)) ||
      !textIsTerminated(config.password, sizeof(config.password))) {
    return false;
  }

  return !config.enabled || textIsPresent(config.ssid, sizeof(config.ssid));
}

bool sameWifi(const WifiConfig& left, const WifiConfig& right) {
  return left.enabled == right.enabled &&
         sameText(left.ssid, right.ssid, sizeof(left.ssid)) &&
         sameText(left.password, right.password, sizeof(left.password));
}

bool validMqtt(const MqttConfig& config) {
  if (!textIsTerminated(config.host, sizeof(config.host)) ||
      !textIsTerminated(config.clientId, sizeof(config.clientId))) {
    return false;
  }

  return config.port > 0 && config.heartbeatIntervalMillis > 0 &&
         (!config.enabled ||
          (textIsPresent(config.host, sizeof(config.host)) &&
           textIsPresent(config.clientId, sizeof(config.clientId))));
}

bool sameMqtt(const MqttConfig& left, const MqttConfig& right) {
  return left.enabled == right.enabled &&
         sameText(left.host, right.host, sizeof(left.host)) &&
         left.port == right.port &&
         sameText(left.clientId, right.clientId, sizeof(left.clientId)) &&
         left.heartbeatIntervalMillis == right.heartbeatIntervalMillis;
}

bool validCalibrations(const CalibrationsConfig& config) {
  return config.temperatureOffset >= -kMaxTemperatureCalibrationOffset &&
         config.temperatureOffset <= kMaxTemperatureCalibrationOffset &&
         config.waterLevelOffset >= -kMaxWaterLevelCalibrationOffset &&
         config.waterLevelOffset <= kMaxWaterLevelCalibrationOffset;
}

bool sameCalibrations(const CalibrationsConfig& left,
                      const CalibrationsConfig& right) {
  return left.temperatureOffset == right.temperatureOffset &&
         left.waterLevelOffset == right.waterLevelOffset;
}

}  // namespace

ReeflowConfig makeDefaultReeflowConfig() {
  ReeflowConfig config = {};

  config.temperature.targetTemperature = 26.5F;
  config.temperature.minTemperature = 25.0F;
  config.temperature.maxTemperature = 28.0F;
  config.temperature.readIntervalMillis =
      kDefaultTemperatureReadIntervalMillis;
  config.temperature.sensorOfflineTimeoutMillis =
      kDefaultSensorOfflineTimeoutMillis;

  config.ato.enabled = false;
  config.ato.minimumLevel = kDefaultAtoMinimumLevel;
  config.ato.maximumLevel = kDefaultAtoMaximumLevel;
  config.ato.timeoutMillis = kDefaultAtoTimeoutMillis;
  config.ato.cooldownMillis = kDefaultAtoCooldownMillis;

  config.lighting.mode = core::state::LightingMode::kManual;
  config.lighting.sunriseEnabled = false;
  config.lighting.sunsetEnabled = false;
  config.lighting.acclimationEnabled = false;
  config.lighting.startMinuteOfDay = 0;
  config.lighting.endMinuteOfDay = 0;
  config.lighting.maxIntensityPercent = 100;
  config.lighting.acclimationDays = 0;

  config.timers.feedingDurationSeconds = kDefaultFeedingDurationSeconds;
  config.timers.tpaDurationSeconds = 0;
  config.timers.maintenanceDurationSeconds = 0;

  config.mode.currentMode = core::state::OperationalMode::kNormal;

  config.wifi.enabled = false;

  config.mqtt.enabled = false;
  config.mqtt.port = kDefaultMqttPort;
  config.mqtt.heartbeatIntervalMillis =
      kDefaultMqttHeartbeatIntervalMillis;

  config.calibrations.temperatureOffset = 0.0F;
  config.calibrations.waterLevelOffset = 0;

  return config;
}

ConfigManager::ConfigManager(core::events::EventBus& eventBus)
    : config_(makeDefaultReeflowConfig()), eventBus_(eventBus) {}

void ConfigManager::loadDefaults() {
  config_ = makeDefaultReeflowConfig();
}

const ReeflowConfig& ConfigManager::currentConfig() const {
  return config_;
}

const TemperatureConfig& ConfigManager::temperature() const {
  return config_.temperature;
}

const AtoConfig& ConfigManager::ato() const {
  return config_.ato;
}

const LightingConfig& ConfigManager::lighting() const {
  return config_.lighting;
}

const TimersConfig& ConfigManager::timers() const {
  return config_.timers;
}

const ModeConfig& ConfigManager::mode() const {
  return config_.mode;
}

const WifiConfig& ConfigManager::wifi() const {
  return config_.wifi;
}

const MqttConfig& ConfigManager::mqtt() const {
  return config_.mqtt;
}

const CalibrationsConfig& ConfigManager::calibrations() const {
  return config_.calibrations;
}

bool ConfigManager::updateTemperature(const TemperatureConfig& temperature) {
  if (!validTemperature(temperature)) {
    return false;
  }

  if (sameTemperature(config_.temperature, temperature)) {
    return true;
  }

  config_.temperature = temperature;
  publishChanged(core::events::ConfigDomain::kTemperature);
  return true;
}

bool ConfigManager::updateAto(const AtoConfig& ato) {
  if (!validAto(ato)) {
    return false;
  }

  if (sameAto(config_.ato, ato)) {
    return true;
  }

  config_.ato = ato;
  publishChanged(core::events::ConfigDomain::kAto);
  return true;
}

bool ConfigManager::updateLighting(const LightingConfig& lighting) {
  if (!validLighting(lighting)) {
    return false;
  }

  if (sameLighting(config_.lighting, lighting)) {
    return true;
  }

  config_.lighting = lighting;
  publishChanged(core::events::ConfigDomain::kLighting);
  return true;
}

bool ConfigManager::updateTimers(const TimersConfig& timers) {
  if (!validTimers(timers)) {
    return false;
  }

  if (sameTimers(config_.timers, timers)) {
    return true;
  }

  config_.timers = timers;
  publishChanged(core::events::ConfigDomain::kTimers);
  return true;
}

bool ConfigManager::updateMode(const ModeConfig& mode) {
  if (!validMode(mode)) {
    return false;
  }

  if (sameMode(config_.mode, mode)) {
    return true;
  }

  config_.mode = mode;
  publishChanged(core::events::ConfigDomain::kMode);
  return true;
}

bool ConfigManager::updateWifi(const WifiConfig& wifi) {
  if (!validWifi(wifi)) {
    return false;
  }

  if (sameWifi(config_.wifi, wifi)) {
    return true;
  }

  config_.wifi = wifi;
  publishChanged(core::events::ConfigDomain::kWifi);
  return true;
}

bool ConfigManager::updateMqtt(const MqttConfig& mqtt) {
  if (!validMqtt(mqtt)) {
    return false;
  }

  if (sameMqtt(config_.mqtt, mqtt)) {
    return true;
  }

  config_.mqtt = mqtt;
  publishChanged(core::events::ConfigDomain::kMqtt);
  return true;
}

bool ConfigManager::updateCalibrations(
    const CalibrationsConfig& calibrations) {
  if (!validCalibrations(calibrations)) {
    return false;
  }

  if (sameCalibrations(config_.calibrations, calibrations)) {
    return true;
  }

  config_.calibrations = calibrations;
  publishChanged(core::events::ConfigDomain::kCalibrations);
  return true;
}

void ConfigManager::publishChanged(core::events::ConfigDomain domain) {
  core::events::Event event = {};
  event.type = core::events::EventType::kConfigChanged;
  event.configDomain = domain;
  eventBus_.publish(event);
}

ConfigManager& defaultConfigManager() {
  static ConfigManager configManager(core::events::defaultEventBus());
  return configManager;
}

}  // namespace reeflow::config
