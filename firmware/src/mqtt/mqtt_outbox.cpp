#include "mqtt/mqtt_outbox.h"

#include <string.h>

#include "mqtt/mqtt_config.h"

namespace reeflow::mqtt {
namespace {

bool validTopic(const char* topic) {
  return topic != nullptr && topic[0] != '\0' &&
         strlen(topic) < kMqttOutboxTopicMaxLength;
}

}  // namespace

void MqttOutbox::configure(const MqttOutboxPolicy& policy) {
  policy_ = policy;
  if (policy_.maxMessages == 0 ||
      policy_.maxMessages > kMqttOutboxMaxStoredMessages) {
    policy_.maxMessages = kMqttOutboxMaxStoredMessages;
  }
  if (policy_.maxAgeMillis == 0) {
    policy_.maxAgeMillis = kDefaultMqttOutboxMaxAgeMillis;
  }
  if (policy_.maxAttempts == 0) {
    policy_.maxAttempts = kDefaultMqttOutboxMaxAttempts;
  }

  status_.capacity = capacity();
  status_.queuedMessages = activeCount();
  status_.state = status_.queuedMessages == 0 ? MqttOutboxState::kEmpty
                                               : MqttOutboxState::kHasMessages;
}

MqttOutboxResult MqttOutbox::enqueue(const MqttMessage& message,
                                     uint32_t nowMillis) {
  status_.capacity = capacity();

  if (!validTopic(message.topic)) {
    status_.lastEnqueueResult = MqttOutboxResult::kTopicInvalid;
    return status_.lastEnqueueResult;
  }
  if (message.payload == nullptr ||
      message.payloadLength >= kMqttOutboxPayloadMaxLength) {
    status_.lastEnqueueResult = MqttOutboxResult::kPayloadTooLarge;
    return status_.lastEnqueueResult;
  }
  if (message.qos != MqttQos::kAtLeastOnce) {
    status_.lastEnqueueResult = MqttOutboxResult::kTopicInvalid;
    return status_.lastEnqueueResult;
  }

  if (activeCount() >= capacity()) {
    status_.droppedMessages += 1;
    status_.state = MqttOutboxState::kFull;
    status_.lastEnqueueResult = MqttOutboxResult::kQueueFull;
    return status_.lastEnqueueResult;
  }

  const int freeIndex = firstFreeIndex();
  if (freeIndex < 0) {
    status_.droppedMessages += 1;
    status_.state = MqttOutboxState::kFull;
    status_.lastEnqueueResult = MqttOutboxResult::kQueueFull;
    return status_.lastEnqueueResult;
  }

  Entry& entry = entries_[freeIndex];
  entry = {};
  entry.active = true;
  strncpy(entry.topic, message.topic, sizeof(entry.topic) - 1);
  memcpy(entry.payload, message.payload, message.payloadLength);
  entry.payload[message.payloadLength] = '\0';
  entry.payloadLength = message.payloadLength;
  entry.qos = message.qos;
  entry.retain = message.retain;
  entry.queuedAtMillis = nowMillis;
  entry.attempts = 0;

  status_.queuedMessages = activeCount();
  status_.state = MqttOutboxState::kHasMessages;
  status_.lastEnqueueResult = MqttOutboxResult::kQueued;
  return status_.lastEnqueueResult;
}

MqttPublishResult MqttOutbox::drainOne(MqttClient& client,
                                       uint32_t nowMillis) {
  status_.capacity = capacity();
  const int index = firstActiveIndex();
  if (index < 0) {
    status_.queuedMessages = 0;
    status_.state = MqttOutboxState::kEmpty;
    status_.lastDrainResult = MqttPublishResult::kAccepted;
    return status_.lastDrainResult;
  }

  Entry& entry = entries_[index];
  if (expired(entry, nowMillis) || entry.attempts >= policy_.maxAttempts) {
    dropIndex(index, true);
    status_.lastDrainResult = MqttPublishResult::kClientFailure;
    return status_.lastDrainResult;
  }

  if (!client.connected()) {
    status_.lastDrainResult = MqttPublishResult::kNotConnected;
    return status_.lastDrainResult;
  }

  const MqttMessage message = {entry.topic, entry.payload,
                               entry.payloadLength, entry.qos, entry.retain};
  entry.attempts += 1;
  status_.lastDrainResult = client.publish(message);
  if (status_.lastDrainResult == MqttPublishResult::kAccepted) {
    dropIndex(index, false);
  }

  return status_.lastDrainResult;
}

MqttOutboxStatus MqttOutbox::status() const {
  return status_;
}

void MqttOutbox::clear() {
  for (size_t index = 0; index < kMqttOutboxMaxStoredMessages; ++index) {
    entries_[index] = {};
  }
  status_.queuedMessages = 0;
  status_.state = MqttOutboxState::kEmpty;
}

uint8_t MqttOutbox::capacity() const {
  if (policy_.maxMessages == 0 ||
      policy_.maxMessages > kMqttOutboxMaxStoredMessages) {
    return kMqttOutboxMaxStoredMessages;
  }
  return policy_.maxMessages;
}

uint8_t MqttOutbox::activeCount() const {
  uint8_t count = 0;
  for (size_t index = 0; index < kMqttOutboxMaxStoredMessages; ++index) {
    if (entries_[index].active) {
      count += 1;
    }
  }
  return count;
}

int MqttOutbox::firstActiveIndex() const {
  for (size_t index = 0; index < kMqttOutboxMaxStoredMessages; ++index) {
    if (entries_[index].active) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

int MqttOutbox::firstFreeIndex() const {
  const uint8_t effectiveCapacity = capacity();
  for (size_t index = 0; index < effectiveCapacity; ++index) {
    if (!entries_[index].active) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

void MqttOutbox::dropIndex(size_t index, bool expiredMessage) {
  if (index >= kMqttOutboxMaxStoredMessages) {
    return;
  }
  entries_[index] = {};
  status_.queuedMessages = activeCount();
  if (expiredMessage) {
    status_.expiredMessages += 1;
    status_.state = MqttOutboxState::kExpiredMessageDropped;
  } else {
    status_.state = status_.queuedMessages == 0
                        ? MqttOutboxState::kEmpty
                        : MqttOutboxState::kHasMessages;
  }
}

bool MqttOutbox::expired(const Entry& entry, uint32_t nowMillis) const {
  return static_cast<uint32_t>(nowMillis - entry.queuedAtMillis) >=
         policy_.maxAgeMillis;
}

}  // namespace reeflow::mqtt
