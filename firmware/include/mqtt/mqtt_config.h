#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::mqtt {

constexpr const char* kDefaultMqttTopicPrefix = "reeflow";
constexpr uint16_t kDefaultMqttPort = 1883;
constexpr uint16_t kDefaultMqttKeepAliveSeconds = 30;
constexpr uint32_t kDefaultMqttConnectTimeoutMillis = 10000;
constexpr uint32_t kDefaultMqttHeartbeatIntervalMillis = 30000;
constexpr uint16_t kDefaultMqttMaxPayloadBytes = 512;
constexpr uint8_t kDefaultMqttMaxOutboxMessages = 8;
constexpr uint32_t kDefaultMqttOutboxMaxAgeMillis = 120000;
constexpr uint8_t kDefaultMqttOutboxMaxAttempts = 3;
constexpr uint32_t kDefaultMqttInitialBackoffMillis = 1000;
constexpr uint32_t kDefaultMqttMaxBackoffMillis = 60000;

struct MqttConfig {
  bool enabled;
  const char* brokerHost;
  uint16_t brokerPort;
  const char* clientId;
  const char* username;
  const char* password;
  const char* topicPrefix;
  bool credentialsRequired;
  bool cleanSession;
  uint16_t keepAliveSeconds;
  uint32_t connectTimeoutMillis;
  uint32_t heartbeatIntervalMillis;
  uint16_t maxPayloadBytes;
  uint8_t maxOutboxMessages;
  uint32_t outboxMaxAgeMillis;
  uint8_t outboxMaxAttempts;
  uint32_t initialBackoffMillis;
  uint32_t maxBackoffMillis;
};

inline MqttConfig makeDefaultMqttConfig() {
  return {false,
          "",
          kDefaultMqttPort,
          "",
          "",
          "",
          kDefaultMqttTopicPrefix,
          false,
          true,
          kDefaultMqttKeepAliveSeconds,
          kDefaultMqttConnectTimeoutMillis,
          kDefaultMqttHeartbeatIntervalMillis,
          kDefaultMqttMaxPayloadBytes,
          kDefaultMqttMaxOutboxMessages,
          kDefaultMqttOutboxMaxAgeMillis,
          kDefaultMqttOutboxMaxAttempts,
          kDefaultMqttInitialBackoffMillis,
          kDefaultMqttMaxBackoffMillis};
}

inline bool mqttTextPresent(const char* value) {
  return value != nullptr && value[0] != '\0';
}

inline bool validMqttConfig(const MqttConfig& config) {
  const bool hasCredentials =
      mqttTextPresent(config.username) && mqttTextPresent(config.password);
  return config.brokerPort > 0 && config.keepAliveSeconds > 0 &&
         config.connectTimeoutMillis > 0 &&
         config.heartbeatIntervalMillis > 0 &&
         config.maxPayloadBytes > 0 && config.maxOutboxMessages > 0 &&
         config.outboxMaxAgeMillis > 0 && config.outboxMaxAttempts > 0 &&
         config.initialBackoffMillis > 0 &&
         config.maxBackoffMillis >= config.initialBackoffMillis &&
         mqttTextPresent(config.topicPrefix) &&
         (!config.credentialsRequired || hasCredentials) &&
         (!config.enabled ||
          (mqttTextPresent(config.brokerHost) &&
           mqttTextPresent(config.clientId)));
}

}  // namespace reeflow::mqtt
