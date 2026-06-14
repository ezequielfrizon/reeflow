#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::mqtt {

constexpr uint8_t kMqttTelemetryQos = 0;
constexpr uint8_t kMqttImportantQos = 1;

enum class MqttQos : uint8_t {
  kAtMostOnce = 0,
  kAtLeastOnce = 1,
};

enum class MqttConnectionState {
  kDisabled,
  kDisconnected,
  kConnecting,
  kConnected,
  kReconnectPending,
};

enum class MqttDisconnectReason {
  kUnknown,
  kManualDisconnect,
  kBrokerUnavailable,
  kCredentialMissing,
  kAuthenticationFailed,
  kConnectionTimeout,
  kNetworkDisconnected,
  kRuntimeDisconnect,
};

enum class MqttConnectResult {
  kConnected,
  kAlreadyConnected,
  kDisabled,
  kBrokerUnavailable,
  kCredentialMissing,
  kAuthenticationFailed,
  kTimeout,
  kNetworkUnavailable,
  kClientFailure,
};

enum class MqttPublishResult {
  kAccepted,
  kNotConnected,
  kPayloadTooLarge,
  kBrokerUnavailable,
  kClientFailure,
};

enum class MqttSubscribeResult {
  kSubscribed,
  kNotConnected,
  kTopicInvalid,
  kBrokerUnavailable,
  kClientFailure,
};

enum class MqttCommandResult {
  kAccepted,
  kRejected,
  kInvalidPayload,
  kUnsupported,
  kLocalConflict,
  kInternalError,
};

enum class MqttOutboxState {
  kEmpty,
  kHasMessages,
  kFull,
  kExpiredMessageDropped,
};

struct MqttMessage {
  const char* topic;
  const char* payload;
  size_t payloadLength;
  MqttQos qos;
  bool retain;
};

using MqttMessageHandler = void (*)(const MqttMessage& message,
                                    void* context);

}  // namespace reeflow::mqtt
