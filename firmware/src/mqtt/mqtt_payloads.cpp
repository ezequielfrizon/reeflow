#include "mqtt/mqtt_payloads.h"

#include <stdio.h>

namespace reeflow::mqtt {
namespace {

const char* boolText(bool value) {
  return value ? "true" : "false";
}

uint32_t timestampValue(const MqttPayloadTimestamp& timestamp) {
  return timestamp.synchronized ? timestamp.epochSeconds
                                : timestamp.localMillis;
}

const char* timestampKind(const MqttPayloadTimestamp& timestamp) {
  return timestamp.synchronized ? "epoch" : "local";
}

}  // namespace

bool serializeTelemetryPayload(const core::state::SystemState& system,
                               const MqttPayloadTimestamp& timestamp,
                               char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) {
    return false;
  }

  const int written = snprintf(
      output, outputSize,
      "{\"timestamp\":{\"kind\":\"%s\",\"value\":%lu},"
      "\"temperature\":{\"current\":%.2f,\"status\":%u},"
      "\"waterLevel\":{\"current\":%u,\"status\":%u},"
      "\"relays\":{\"recalque\":%s,\"heater\":%s,\"atoPump\":%s,"
      "\"reserve\":%s},"
      "\"mode\":%u,"
      "\"lighting\":{\"white\":%u,\"blue\":%u,\"royalBlue\":%u,\"uv\":%u},"
      "\"network\":{\"wifiConnected\":%s,\"mqttConnected\":%s,"
      "\"internetAvailable\":%s,\"rssi\":%ld},"
      "\"systemHealth\":{\"uptime\":%lu,\"freeHeap\":%lu,"
      "\"watchdogTriggered\":%s}}",
      timestampKind(timestamp),
      static_cast<unsigned long>(timestampValue(timestamp)),
      static_cast<double>(system.temperature.currentTemperature),
      static_cast<unsigned>(system.temperature.status),
      static_cast<unsigned>(system.waterLevel.currentLevel),
      static_cast<unsigned>(system.waterLevel.status),
      boolText(system.relays.recalque.enabled),
      boolText(system.relays.heater.enabled),
      boolText(system.relays.atoPump.enabled),
      boolText(system.relays.reserve.enabled),
      static_cast<unsigned>(system.modes.currentMode),
      static_cast<unsigned>(system.lighting.white.currentPWM),
      static_cast<unsigned>(system.lighting.blue.currentPWM),
      static_cast<unsigned>(system.lighting.royalBlue.currentPWM),
      static_cast<unsigned>(system.lighting.uv.currentPWM),
      boolText(system.network.wifiConnected),
      boolText(system.network.mqttConnected),
      boolText(system.network.internetAvailable),
      static_cast<long>(system.network.rssi),
      static_cast<unsigned long>(system.systemHealth.uptime),
      static_cast<unsigned long>(system.systemHealth.freeHeap),
      boolText(system.systemHealth.watchdogTriggered));

  return written >= 0 && static_cast<size_t>(written) < outputSize;
}

bool serializeHeartbeatPayload(bool mqttConnected,
                               const MqttPayloadTimestamp& timestamp,
                               char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) {
    return false;
  }

  const int written = snprintf(
      output, outputSize,
      "{\"timestamp\":{\"kind\":\"%s\",\"value\":%lu},"
      "\"mqttConnected\":%s}",
      timestampKind(timestamp),
      static_cast<unsigned long>(timestampValue(timestamp)),
      boolText(mqttConnected));

  return written >= 0 && static_cast<size_t>(written) < outputSize;
}

bool serializeEventPayload(const core::events::Event& event,
                           const MqttPayloadTimestamp& timestamp,
                           char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) {
    return false;
  }

  const int written = snprintf(
      output, outputSize,
      "{\"timestamp\":{\"kind\":\"%s\",\"value\":%lu},"
      "\"event\":{\"type\":%u,\"stateArea\":%u,\"configDomain\":%u,"
      "\"schedulerTaskId\":%u,\"sequence\":%lu}}",
      timestampKind(timestamp),
      static_cast<unsigned long>(timestampValue(timestamp)),
      static_cast<unsigned>(event.type),
      static_cast<unsigned>(event.stateArea),
      static_cast<unsigned>(event.configDomain),
      static_cast<unsigned>(event.schedulerTaskId),
      static_cast<unsigned long>(event.sequence));

  return written >= 0 && static_cast<size_t>(written) < outputSize;
}

}  // namespace reeflow::mqtt
