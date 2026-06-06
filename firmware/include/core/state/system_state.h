#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/events/event_bus.h"

namespace reeflow::core::state {

constexpr size_t kProfileNameMaxLength = 32;
constexpr size_t kIpAddressMaxLength = 16;
constexpr size_t kAlertNameMaxLength = 40;
constexpr size_t kRebootReasonMaxLength = 40;
constexpr size_t kMaxActiveAlerts = 8;

constexpr float kDefaultMinTemperatureC = 25.0F;
constexpr float kDefaultMaxTemperatureC = 28.0F;

enum class TemperatureStatus {
  kNormal,
  kHigh,
  kLow,
  kSensorOffline,
};

enum class WaterLevelStatus {
  kNormal,
  kLow,
  kHigh,
  kSensorOffline,
};

enum class LightingMode {
  kManual,
  kAutomatic,
  kAcclimation,
};

enum class RelaySource {
  kLocal,
  kMqtt,
  kApp,
  kAutomation,
  kFailsafe,
};

enum class OperationalMode {
  kNormal,
  kFeeding,
  kTpa,
  kMaintenance,
};

enum class AtoStatus {
  kNormal,
  kRefilling,
  kTimeout,
  kSensorOffline,
  kDisabled,
};

enum class AlertPriority {
  kInfo,
  kWarning,
  kCritical,
};

struct SystemBlock {
  uint32_t version;
};

struct TemperatureState {
  float currentTemperature;
  float minTemperature;
  float maxTemperature;
  TemperatureStatus status;
  uint32_t lastUpdate;
};

struct WaterLevelState {
  uint16_t currentLevel;
  uint16_t minimumLevel;
  uint16_t maximumLevel;
  WaterLevelStatus status;
  uint32_t lastUpdate;
};

struct LightingChannelState {
  uint16_t currentPWM;
  uint16_t maxPWM;
  bool enabled;
};

struct LightingState {
  LightingMode mode;
  bool sunriseEnabled;
  bool sunsetEnabled;
  bool acclimationEnabled;
  char currentProfile[kProfileNameMaxLength];
  LightingChannelState white;
  LightingChannelState blue;
  LightingChannelState royalBlue;
  LightingChannelState uv;
};

struct RelayEntryState {
  bool enabled;
  uint32_t lastChanged;
  RelaySource source;
};

struct RelaysState {
  RelayEntryState recalque;
  RelayEntryState heater;
  RelayEntryState atoPump;
  RelayEntryState reserve;
};

struct ModesState {
  OperationalMode currentMode;
  uint32_t startedAt;
  uint32_t remainingTime;
};

struct AtoState {
  bool enabled;
  AtoStatus status;
  bool pumpRunning;
  uint32_t lastActivation;
  uint32_t lastCompletion;
  uint32_t timeoutCounter;
};

struct NetworkState {
  bool wifiConnected;
  bool mqttConnected;
  bool internetAvailable;
  char ipAddress[kIpAddressMaxLength];
  int32_t rssi;
  uint32_t lastHeartbeat;
};

struct ActiveAlertState {
  bool active;
  char name[kAlertNameMaxLength];
  AlertPriority priority;
  uint32_t raisedAt;
};

struct AlertsState {
  ActiveAlertState activeAlerts[kMaxActiveAlerts];
  uint8_t activeAlertCount;
  char lastAlert[kAlertNameMaxLength];
  char lastRecovery[kAlertNameMaxLength];
  AlertPriority priority;
};

struct SystemHealthState {
  uint32_t uptime;
  uint32_t freeHeap;
  uint8_t cpuUsage;
  uint32_t lastReboot;
  char rebootReason[kRebootReasonMaxLength];
  bool watchdogTriggered;
};

struct SystemState {
  SystemBlock system;
  TemperatureState temperature;
  WaterLevelState waterLevel;
  LightingState lighting;
  RelaysState relays;
  ModesState modes;
  AtoState ato;
  NetworkState network;
  AlertsState alerts;
  SystemHealthState systemHealth;
};

SystemState makeDefaultSystemState();
void resetSystemState();
const SystemState& currentSystemState();
void setSystemStateEventBus(events::EventBus& eventBus);

void updateSystemBlock(const SystemBlock& system);
void updateTemperatureState(const TemperatureState& temperature);
void updateWaterLevelState(const WaterLevelState& waterLevel);
void updateLightingState(const LightingState& lighting);
void updateRelaysState(const RelaysState& relays);
void updateModesState(const ModesState& modes);
void updateAtoState(const AtoState& ato);
void updateNetworkState(const NetworkState& network);
void updateAlertsState(const AlertsState& alerts);
void updateSystemHealthState(const SystemHealthState& systemHealth);

}  // namespace reeflow::core::state
