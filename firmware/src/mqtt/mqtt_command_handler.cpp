#include "mqtt/mqtt_command_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace reeflow::mqtt {
namespace {

constexpr size_t kMqttCommandPayloadMaxLength = 256;
constexpr size_t kMqttCommandValueMaxLength = 32;

bool textEquals(const char* left, const char* right) {
  return left != nullptr && right != nullptr && strcmp(left, right) == 0;
}

bool topicAllowed(const MqttCommandRequest& request) {
  return request.topic != nullptr && request.expectedTopic != nullptr &&
         strcmp(request.topic, request.expectedTopic) == 0;
}

void copyLimited(char* destination, size_t capacity, const char* source,
                 size_t sourceLength) {
  if (destination == nullptr || capacity == 0) {
    return;
  }

  const size_t copied =
      sourceLength < capacity - 1 ? sourceLength : capacity - 1;
  if (source != nullptr && copied > 0) {
    memcpy(destination, source, copied);
  }
  destination[copied] = '\0';
}

bool copyPayload(const MqttCommandRequest& request, char* output,
                 size_t outputSize) {
  if (request.payload == nullptr || request.payloadLength == 0 ||
      request.payloadLength >= outputSize ||
      request.payloadLength > kMqttCommandPayloadMaxLength) {
    return false;
  }

  memcpy(output, request.payload, request.payloadLength);
  output[request.payloadLength] = '\0';
  return true;
}

bool findField(const char* payload, const char* key, char* output,
               size_t outputSize) {
  if (payload == nullptr || key == nullptr || output == nullptr ||
      outputSize == 0) {
    return false;
  }

  const size_t keyLength = strlen(key);
  const char* cursor = payload;
  while (*cursor != '\0') {
    if ((cursor == payload || cursor[-1] == ';') &&
        strncmp(cursor, key, keyLength) == 0 && cursor[keyLength] == '=') {
      const char* value = cursor + keyLength + 1;
      const char* end = strchr(value, ';');
      const size_t valueLength =
          end == nullptr ? strlen(value) : static_cast<size_t>(end - value);
      copyLimited(output, outputSize, value, valueLength);
      return output[0] != '\0';
    }

    const char* next = strchr(cursor, ';');
    if (next == nullptr) {
      break;
    }
    cursor = next + 1;
  }

  return false;
}

bool parseUnsigned(const char* value, uint32_t maxValue, uint32_t* output) {
  if (value == nullptr || value[0] == '\0' || output == nullptr) {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(value, &end, 10);
  if (end == value || *end != '\0' || parsed > maxValue) {
    return false;
  }

  *output = static_cast<uint32_t>(parsed);
  return true;
}

bool parseMode(const char* value, modules::modes::OperationalMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  if (textEquals(value, "NORMAL")) {
    *mode = modules::modes::NORMAL;
    return true;
  }
  if (textEquals(value, "FEEDING")) {
    *mode = modules::modes::FEEDING;
    return true;
  }
  if (textEquals(value, "TPA")) {
    *mode = modules::modes::TPA;
    return true;
  }
  if (textEquals(value, "MAINTENANCE")) {
    *mode = modules::modes::MAINTENANCE;
    return true;
  }
  return false;
}

bool parseRelay(const char* value, modules::relays::RelayId* relay) {
  if (relay == nullptr) {
    return false;
  }
  if (textEquals(value, "RECALQUE")) {
    *relay = modules::relays::RECALQUE;
    return true;
  }
  if (textEquals(value, "HEATER")) {
    *relay = modules::relays::HEATER;
    return true;
  }
  if (textEquals(value, "ATO")) {
    *relay = modules::relays::ATO_PUMP;
    return true;
  }
  if (textEquals(value, "RESERVE")) {
    *relay = modules::relays::RESERVE;
    return true;
  }
  return false;
}

bool parseRelayState(const char* value,
                     modules::relays::RelayDesiredState* state) {
  if (state == nullptr) {
    return false;
  }
  if (textEquals(value, "ON")) {
    *state = modules::relays::RelayDesiredState::kOn;
    return true;
  }
  if (textEquals(value, "OFF")) {
    *state = modules::relays::RelayDesiredState::kOff;
    return true;
  }
  return false;
}

bool parseLightingChannel(const char* value,
                          modules::lighting::LightingChannel* channel) {
  if (channel == nullptr) {
    return false;
  }
  if (textEquals(value, "WHITE")) {
    *channel = modules::lighting::WHITE;
    return true;
  }
  if (textEquals(value, "BLUE")) {
    *channel = modules::lighting::BLUE;
    return true;
  }
  if (textEquals(value, "ROYAL_BLUE")) {
    *channel = modules::lighting::ROYAL_BLUE;
    return true;
  }
  if (textEquals(value, "UV")) {
    *channel = modules::lighting::UV;
    return true;
  }
  return false;
}

const char* resultText(MqttCommandResult result) {
  switch (result) {
    case MqttCommandResult::kAccepted:
      return "accepted";
    case MqttCommandResult::kRejected:
      return "rejected";
    case MqttCommandResult::kInvalidPayload:
      return "invalid";
    case MqttCommandResult::kUnsupported:
      return "unsupported";
    case MqttCommandResult::kLocalConflict:
      return "conflict";
    case MqttCommandResult::kInternalError:
      return "error";
  }
  return "error";
}

MqttCommandResponse makeResponse(MqttCommandResult result,
                                 const char* correlation) {
  MqttCommandResponse response = {};
  response.result = result;
  if (correlation != nullptr) {
    copyLimited(response.correlation, sizeof(response.correlation),
                correlation, strlen(correlation));
  }
  snprintf(response.payload, sizeof(response.payload),
           "{\"result\":\"%s\",\"correlation\":\"%s\"}",
           resultText(result), response.correlation);
  return response;
}

MqttCommandResult executeMode(const MqttCommandCallbacks& callbacks,
                              const char* payload) {
  char value[kMqttCommandValueMaxLength] = {};
  modules::modes::OperationalMode mode = modules::modes::NORMAL;
  if (!findField(payload, "mode", value, sizeof(value)) ||
      !parseMode(value, &mode)) {
    return MqttCommandResult::kInvalidPayload;
  }
  if (callbacks.mode == nullptr) {
    return MqttCommandResult::kUnsupported;
  }
  return callbacks.mode(mode, callbacks.context);
}

MqttCommandResult executeRelay(const MqttCommandCallbacks& callbacks,
                               const char* payload) {
  char relayValue[kMqttCommandValueMaxLength] = {};
  char stateValue[kMqttCommandValueMaxLength] = {};
  modules::relays::RelayId relay = modules::relays::RelayId::kUnknown;
  modules::relays::RelayDesiredState state =
      modules::relays::RelayDesiredState::kOff;
  if (!findField(payload, "relay", relayValue, sizeof(relayValue)) ||
      !findField(payload, "state", stateValue, sizeof(stateValue)) ||
      !parseRelay(relayValue, &relay) ||
      !parseRelayState(stateValue, &state)) {
    return MqttCommandResult::kInvalidPayload;
  }
  if (callbacks.relay == nullptr) {
    return MqttCommandResult::kUnsupported;
  }
  return callbacks.relay(relay, state, callbacks.context);
}

MqttCommandResult executeLighting(const MqttCommandCallbacks& callbacks,
                                  const char* payload) {
  char channelValue[kMqttCommandValueMaxLength] = {};
  char dutyValue[kMqttCommandValueMaxLength] = {};
  modules::lighting::LightingChannel channel =
      modules::lighting::LightingChannel::kUnknown;
  uint32_t duty = 0;
  if (!findField(payload, "channel", channelValue, sizeof(channelValue)) ||
      !findField(payload, "duty", dutyValue, sizeof(dutyValue)) ||
      !parseLightingChannel(channelValue, &channel) ||
      !parseUnsigned(dutyValue, modules::lighting::kLightingMaxDuty, &duty)) {
    return MqttCommandResult::kInvalidPayload;
  }
  if (callbacks.lighting == nullptr) {
    return MqttCommandResult::kUnsupported;
  }
  return callbacks.lighting(channel, static_cast<uint16_t>(duty),
                            callbacks.context);
}

MqttCommandResult executeConfig(const MqttCommandCallbacks& callbacks,
                                const char* payload) {
  char domain[kMqttCommandValueMaxLength] = {};
  if (!findField(payload, "domain", domain, sizeof(domain))) {
    return MqttCommandResult::kInvalidPayload;
  }
  if (!textEquals(domain, "mqtt")) {
    return MqttCommandResult::kUnsupported;
  }

  MqttCommandConfigRequest request = {};
  char value[kMqttCommandValueMaxLength] = {};
  uint32_t parsed = 0;
  if (findField(payload, "heartbeat", value, sizeof(value))) {
    if (!parseUnsigned(value, 86400000UL, &parsed) || parsed == 0) {
      return MqttCommandResult::kInvalidPayload;
    }
    request.updateMqttHeartbeat = true;
    request.mqttHeartbeatIntervalMillis = parsed;
  }
  if (findField(payload, "maxPayload", value, sizeof(value))) {
    if (!parseUnsigned(value, 4096, &parsed) || parsed == 0) {
      return MqttCommandResult::kInvalidPayload;
    }
    request.updateMqttMaxPayload = true;
    request.mqttMaxPayloadBytes = static_cast<uint16_t>(parsed);
  }
  if (!request.updateMqttHeartbeat && !request.updateMqttMaxPayload) {
    return MqttCommandResult::kInvalidPayload;
  }
  if (callbacks.config == nullptr) {
    return MqttCommandResult::kUnsupported;
  }
  return callbacks.config(request, callbacks.context);
}

}  // namespace

void MqttCommandHandler::configure(const MqttCommandCallbacks& callbacks) {
  callbacks_ = callbacks;
}

MqttCommandResponse MqttCommandHandler::handle(
    const MqttCommandRequest& request) const {
  char payload[kMqttCommandPayloadMaxLength + 1] = {};
  char type[kMqttCommandValueMaxLength] = {};
  char correlation[kMqttCommandCorrelationMaxLength + 1] = {};

  if (!request.mqttConnected) {
    return makeResponse(MqttCommandResult::kRejected, "");
  }
  if (!topicAllowed(request)) {
    return makeResponse(MqttCommandResult::kRejected, "");
  }
  if (!copyPayload(request, payload, sizeof(payload))) {
    return makeResponse(MqttCommandResult::kInvalidPayload, "");
  }

  findField(payload, "correlation", correlation, sizeof(correlation));
  if (!findField(payload, "type", type, sizeof(type))) {
    return makeResponse(MqttCommandResult::kInvalidPayload, correlation);
  }

  MqttCommandResult result = MqttCommandResult::kUnsupported;
  if (textEquals(type, "mode")) {
    result = executeMode(callbacks_, payload);
  } else if (textEquals(type, "relay")) {
    result = executeRelay(callbacks_, payload);
  } else if (textEquals(type, "lighting")) {
    result = executeLighting(callbacks_, payload);
  } else if (textEquals(type, "config")) {
    result = executeConfig(callbacks_, payload);
  }

  return makeResponse(result, correlation);
}

}  // namespace reeflow::mqtt
