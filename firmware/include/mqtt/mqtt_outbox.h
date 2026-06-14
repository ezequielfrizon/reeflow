#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_types.h"

namespace reeflow::mqtt {

constexpr uint8_t kMqttOutboxMaxStoredMessages = 8;
constexpr size_t kMqttOutboxTopicMaxLength = 128;
constexpr size_t kMqttOutboxPayloadMaxLength = 768;

enum class MqttOutboxResult {
  kQueued,
  kQueueFull,
  kPayloadTooLarge,
  kTopicInvalid,
  kExpired,
};

struct MqttOutboxPolicy {
  uint8_t maxMessages;
  uint32_t maxAgeMillis;
  uint8_t maxAttempts;
};

struct MqttOutboxStatus {
  MqttOutboxState state;
  uint8_t queuedMessages;
  uint8_t capacity;
  uint8_t droppedMessages;
  uint8_t expiredMessages;
  MqttOutboxResult lastEnqueueResult;
  MqttPublishResult lastDrainResult;
};

class MqttOutbox {
 public:
  void configure(const MqttOutboxPolicy& policy);
  MqttOutboxResult enqueue(const MqttMessage& message, uint32_t nowMillis);
  MqttPublishResult drainOne(MqttClient& client, uint32_t nowMillis);
  MqttOutboxStatus status() const;
  void clear();

 private:
  struct Entry {
    bool active;
    char topic[kMqttOutboxTopicMaxLength];
    char payload[kMqttOutboxPayloadMaxLength];
    size_t payloadLength;
    MqttQos qos;
    bool retain;
    uint32_t queuedAtMillis;
    uint8_t attempts;
  };

  uint8_t capacity() const;
  uint8_t activeCount() const;
  int firstActiveIndex() const;
  int firstFreeIndex() const;
  void dropIndex(size_t index, bool expired);
  bool expired(const Entry& entry, uint32_t nowMillis) const;

  MqttOutboxPolicy policy_ = {};
  Entry entries_[kMqttOutboxMaxStoredMessages] = {};
  MqttOutboxStatus status_ = {};
};

}  // namespace reeflow::mqtt
