#pragma once

#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"

namespace reeflow::config {

constexpr uint8_t kMaxWifiSsidLength = 32;
constexpr uint8_t kMaxWifiPasswordLength = 64;
constexpr uint8_t kMaxMqttHostLength = 64;
constexpr uint8_t kMaxMqttClientIdLength = 32;
constexpr uint8_t kMaxMqttUsernameLength = 32;
constexpr uint8_t kMaxMqttPasswordLength = 64;
constexpr uint8_t kMaxMqttTopicPrefixLength = 32;

struct TemperatureConfig {
  float targetTemperature;
  float minTemperature;
  float maxTemperature;
  uint32_t readIntervalMillis;
  uint32_t sensorOfflineTimeoutMillis;
};

struct AtoConfig {
  bool enabled;
  uint16_t minimumLevel;
  uint16_t maximumLevel;
  uint32_t timeoutMillis;
  uint32_t cooldownMillis;
};

struct LightingChannelConfig {
  bool enabled;
  uint16_t maxPWM;
};

struct LightingConfig {
  core::state::LightingMode mode;
  bool sunriseEnabled;
  bool sunsetEnabled;
  bool acclimationEnabled;
  uint16_t startMinuteOfDay;
  uint16_t endMinuteOfDay;
  uint8_t maxIntensityPercent;
  uint16_t acclimationDays;
  LightingChannelConfig white;
  LightingChannelConfig blue;
  LightingChannelConfig royalBlue;
  LightingChannelConfig uv;
};

struct TimersConfig {
  uint32_t feedingDurationSeconds;
  uint32_t tpaDurationSeconds;
  uint32_t maintenanceDurationSeconds;
};

struct ModeConfig {
  core::state::OperationalMode currentMode;
};

struct WifiConfig {
  bool enabled;
  char ssid[kMaxWifiSsidLength + 1];
  char password[kMaxWifiPasswordLength + 1];
};

struct MqttConfig {
  bool enabled;
  char host[kMaxMqttHostLength + 1];
  uint16_t port;
  char clientId[kMaxMqttClientIdLength + 1];
  char username[kMaxMqttUsernameLength + 1];
  char password[kMaxMqttPasswordLength + 1];
  char topicPrefix[kMaxMqttTopicPrefixLength + 1];
  bool credentialsRequired;
  bool cleanSession;
  uint16_t keepAliveSeconds;
  uint32_t connectTimeoutMillis;
  uint32_t heartbeatIntervalMillis;
  uint16_t maxPayloadBytes;
  uint8_t maxOutboxMessages;
  uint32_t initialBackoffMillis;
  uint32_t maxBackoffMillis;
};

struct CalibrationsConfig {
  float temperatureOffset;
  int16_t waterLevelOffset;
};

struct ReeflowConfig {
  TemperatureConfig temperature;
  AtoConfig ato;
  LightingConfig lighting;
  TimersConfig timers;
  ModeConfig mode;
  WifiConfig wifi;
  MqttConfig mqtt;
  CalibrationsConfig calibrations;
};

ReeflowConfig makeDefaultReeflowConfig();

class ConfigManager {
 public:
  explicit ConfigManager(core::events::EventBus& eventBus);

  void loadDefaults();
  const ReeflowConfig& currentConfig() const;

  const TemperatureConfig& temperature() const;
  const AtoConfig& ato() const;
  const LightingConfig& lighting() const;
  const TimersConfig& timers() const;
  const ModeConfig& mode() const;
  const WifiConfig& wifi() const;
  const MqttConfig& mqtt() const;
  const CalibrationsConfig& calibrations() const;

  bool updateTemperature(const TemperatureConfig& temperature);
  bool updateAto(const AtoConfig& ato);
  bool updateLighting(const LightingConfig& lighting);
  bool updateTimers(const TimersConfig& timers);
  bool updateMode(const ModeConfig& mode);
  bool updateWifi(const WifiConfig& wifi);
  bool updateMqtt(const MqttConfig& mqtt);
  bool updateCalibrations(const CalibrationsConfig& calibrations);

 private:
  void publishChanged(core::events::ConfigDomain domain);

  ReeflowConfig config_;
  core::events::EventBus& eventBus_;
};

ConfigManager& defaultConfigManager();

}  // namespace reeflow::config
