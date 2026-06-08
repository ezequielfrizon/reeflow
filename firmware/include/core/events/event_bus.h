#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::core::events {

constexpr size_t kMaxEventSubscriptions = 12;
constexpr uint8_t kInvalidSubscriptionId = 0;

enum class EventType {
  kSystemStateChanged,
  kConfigChanged,
  kSchedulerTaskFailed,
  kTemperatureUpdated,
  kTemperatureHigh,
  kTemperatureLow,
  kTemperatureSensorOffline,
  kTemperatureSensorRecovered,
  kWaterLevelUpdated,
  kWaterLevelLow,
  kWaterLevelHigh,
  kWaterLevelSensorOffline,
  kWaterLevelSensorRecovered,
  kRelayOn,
  kRelayOff,
  kAtoStart,
  kAtoStop,
  kAtoTimeout,
  kAtoSensorOffline,
  kAtoRecovered,
  kModeChanged,
  kModeStarted,
  kModeFinished,
};

enum class StateArea {
  kSystem,
  kTemperature,
  kWaterLevel,
  kLighting,
  kRelays,
  kModes,
  kAto,
  kNetwork,
  kAlerts,
  kSystemHealth,
};

enum class ConfigDomain {
  kNone,
  kTemperature,
  kAto,
  kLighting,
  kTimers,
  kMode,
  kWifi,
  kMqtt,
  kCalibrations,
};

struct Event {
  EventType type;
  StateArea stateArea;
  ConfigDomain configDomain;
  uint8_t schedulerTaskId;
  uint32_t sequence;
};

struct PublishResult {
  uint8_t deliveredCount;
  uint8_t failedCount;
};

using EventCallback = bool (*)(const Event& event, void* context);

class EventBus {
 public:
  uint8_t subscribe(EventType type, EventCallback callback, void* context);
  bool unsubscribe(uint8_t subscriptionId);
  PublishResult publish(Event event);
  void reset();

 private:
  struct Subscription {
    bool active;
    uint8_t id;
    EventType type;
    EventCallback callback;
    void* context;
  };

  uint8_t nextSubscriptionId_ = 1;
  uint32_t nextEventSequence_ = 1;
  Subscription subscriptions_[kMaxEventSubscriptions] = {};
};

EventBus& defaultEventBus();

}  // namespace reeflow::core::events
