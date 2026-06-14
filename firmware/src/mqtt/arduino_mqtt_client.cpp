#include "mqtt/arduino_mqtt_client.h"

#include <string.h>

namespace reeflow::mqtt {
namespace {

constexpr uint8_t kMqttPacketTypeConnect = 0x10;
constexpr uint8_t kMqttPacketTypeConnack = 0x20;
constexpr uint8_t kMqttPacketTypePublish = 0x30;
constexpr uint8_t kMqttPacketTypePuback = 0x40;
constexpr uint8_t kMqttPacketTypeSubscribe = 0x82;
constexpr uint8_t kMqttPacketTypeSuback = 0x90;
constexpr uint8_t kMqttPacketTypeDisconnect = 0xE0;
constexpr uint8_t kMqttProtocolLevel311 = 0x04;
constexpr size_t kTopicBufferLength = 128;
constexpr size_t kPayloadBufferLength = 512;

uint16_t nextNonZeroPacketId(uint16_t value) {
  return value == 0 ? 1 : value;
}

size_t mqttStringLength(const char* value) {
  return value == nullptr ? 0 : strlen(value);
}

bool validQos(MqttQos qos) {
  return qos == MqttQos::kAtMostOnce || qos == MqttQos::kAtLeastOnce;
}

MqttConnectResult connackResult(uint8_t returnCode) {
  switch (returnCode) {
    case 0:
      return MqttConnectResult::kConnected;
    case 4:
      return MqttConnectResult::kAuthenticationFailed;
    case 5:
      return MqttConnectResult::kAuthenticationFailed;
    default:
      return MqttConnectResult::kBrokerUnavailable;
  }
}

}  // namespace

void ArduinoMqttClient::configure(const MqttConfig& config) {
  config_ = config;
  const uint32_t timeoutMillis =
      config_.connectTimeoutMillis == 0 ? kDefaultMqttConnectTimeoutMillis
                                        : config_.connectTimeoutMillis;
  client_.setTimeout(timeoutMillis / 1000U);
}

MqttConnectResult ArduinoMqttClient::connect() {
  if (!config_.enabled) {
    return MqttConnectResult::kDisabled;
  }
  if (!mqttTextPresent(config_.brokerHost) ||
      !mqttTextPresent(config_.clientId)) {
    return MqttConnectResult::kCredentialMissing;
  }
  if (config_.credentialsRequired &&
      (!mqttTextPresent(config_.username) ||
       !mqttTextPresent(config_.password))) {
    return MqttConnectResult::kCredentialMissing;
  }
  if (client_.connected()) {
    return MqttConnectResult::kAlreadyConnected;
  }

  const uint32_t timeoutMillis =
      config_.connectTimeoutMillis == 0 ? kDefaultMqttConnectTimeoutMillis
                                        : config_.connectTimeoutMillis;
  if (!client_.connect(config_.brokerHost, config_.brokerPort,
                       timeoutMillis)) {
    client_.stop();
    return MqttConnectResult::kBrokerUnavailable;
  }

  const bool hasUsername = mqttTextPresent(config_.username);
  const bool hasPassword = mqttTextPresent(config_.password);
  const size_t clientIdLength = mqttStringLength(config_.clientId);
  const size_t usernameLength = mqttStringLength(config_.username);
  const size_t passwordLength = mqttStringLength(config_.password);
  const uint32_t remainingLength =
      10U + 2U + clientIdLength + (hasUsername ? 2U + usernameLength : 0U) +
      (hasPassword ? 2U + passwordLength : 0U);
  uint8_t connectFlags = 0;
  if (config_.cleanSession) {
    connectFlags |= 0x02;
  }
  if (hasPassword) {
    connectFlags |= 0x40;
  }
  if (hasUsername) {
    connectFlags |= 0x80;
  }

  if (client_.write(kMqttPacketTypeConnect) != 1 ||
      !writeRemainingLength(remainingLength) || !writeString("MQTT") ||
      client_.write(kMqttProtocolLevel311) != 1 ||
      client_.write(connectFlags) != 1 ||
      client_.write(static_cast<uint8_t>(config_.keepAliveSeconds >> 8)) != 1 ||
      client_.write(static_cast<uint8_t>(config_.keepAliveSeconds & 0xFF)) !=
          1 ||
      !writeString(config_.clientId) ||
      (hasUsername && !writeString(config_.username)) ||
      (hasPassword && !writeString(config_.password))) {
    client_.stop();
    return MqttConnectResult::kClientFailure;
  }

  uint32_t connackLength = 0;
  uint8_t connack[2] = {};
  if (!readPacketHeader(kMqttPacketTypeConnack, &connackLength) ||
      connackLength != sizeof(connack) || !readExact(connack, sizeof(connack))) {
    client_.stop();
    return MqttConnectResult::kTimeout;
  }

  const MqttConnectResult result = connackResult(connack[1]);
  if (result != MqttConnectResult::kConnected) {
    client_.stop();
  }
  return result;
}

void ArduinoMqttClient::disconnect(MqttDisconnectReason reason) {
  (void)reason;
  if (client_.connected()) {
    client_.write(kMqttPacketTypeDisconnect);
    client_.write(static_cast<uint8_t>(0));
  }
  client_.stop();
}

bool ArduinoMqttClient::connected() const {
  return client_.connected();
}

MqttPublishResult ArduinoMqttClient::publish(const MqttMessage& message) {
  if (!client_.connected()) {
    return MqttPublishResult::kNotConnected;
  }
  if (message.topic == nullptr || message.topic[0] == '\0' ||
      message.payload == nullptr || !validQos(message.qos)) {
    return MqttPublishResult::kClientFailure;
  }
  if (config_.maxPayloadBytes > 0 &&
      message.payloadLength > config_.maxPayloadBytes) {
    return MqttPublishResult::kPayloadTooLarge;
  }
  return publishPacket(message);
}

MqttSubscribeResult ArduinoMqttClient::subscribe(const char* topic,
                                                 MqttQos qos) {
  if (!client_.connected()) {
    return MqttSubscribeResult::kNotConnected;
  }
  if (topic == nullptr || topic[0] == '\0' || !validQos(qos)) {
    return MqttSubscribeResult::kTopicInvalid;
  }

  const uint16_t packetId = nextNonZeroPacketId(nextPacketId_++);
  const uint32_t remainingLength = 2U + 2U + mqttStringLength(topic) + 1U;
  if (client_.write(kMqttPacketTypeSubscribe) != 1 ||
      !writeRemainingLength(remainingLength) ||
      client_.write(static_cast<uint8_t>(packetId >> 8)) != 1 ||
      client_.write(static_cast<uint8_t>(packetId & 0xFF)) != 1 ||
      !writeString(topic) ||
      client_.write(static_cast<uint8_t>(qos)) != 1) {
    return MqttSubscribeResult::kClientFailure;
  }

  uint32_t subackLength = 0;
  uint8_t suback[3] = {};
  if (!readPacketHeader(kMqttPacketTypeSuback, &subackLength) ||
      subackLength != sizeof(suback) || !readExact(suback, sizeof(suback))) {
    return MqttSubscribeResult::kClientFailure;
  }
  const uint16_t acknowledgedPacketId =
      (static_cast<uint16_t>(suback[0]) << 8) | suback[1];
  if (acknowledgedPacketId != packetId || suback[2] == 0x80) {
    return MqttSubscribeResult::kClientFailure;
  }

  return MqttSubscribeResult::kSubscribed;
}

void ArduinoMqttClient::setMessageHandler(MqttMessageHandler handler,
                                          void* context) {
  messageHandler_ = handler;
  messageHandlerContext_ = context;
}

void ArduinoMqttClient::loop(uint32_t nowMillis) {
  (void)nowMillis;
  while (client_.connected() && client_.available() > 0) {
    uint8_t packetType = 0;
    uint32_t remainingLength = 0;
    if (!readExact(&packetType, 1)) {
      return;
    }

    uint32_t multiplier = 1;
    uint8_t encodedByte = 0;
    do {
      if (!readExact(&encodedByte, 1)) {
        return;
      }
      remainingLength += (encodedByte & 127U) * multiplier;
      multiplier *= 128U;
      if (multiplier > 128U * 128U * 128U * 128U) {
        return;
      }
    } while ((encodedByte & 128U) != 0);

    if ((packetType & 0xF0) == kMqttPacketTypePublish) {
      handleIncomingPublish(packetType, remainingLength);
      continue;
    }

    while (remainingLength > 0 && client_.connected()) {
      if (client_.read() < 0) {
        return;
      }
      remainingLength -= 1;
    }
  }
}

bool ArduinoMqttClient::writeString(const char* value) {
  const size_t length = mqttStringLength(value);
  if (length > 0xFFFFU) {
    return false;
  }

  return client_.write(static_cast<uint8_t>(length >> 8)) == 1 &&
         client_.write(static_cast<uint8_t>(length & 0xFF)) == 1 &&
         (length == 0 ||
          client_.write(reinterpret_cast<const uint8_t*>(value), length) ==
              length);
}

bool ArduinoMqttClient::writeRemainingLength(uint32_t length) {
  do {
    uint8_t encodedByte = length % 128U;
    length /= 128U;
    if (length > 0) {
      encodedByte |= 128U;
    }
    if (client_.write(encodedByte) != 1) {
      return false;
    }
  } while (length > 0);
  return true;
}

bool ArduinoMqttClient::readPacketHeader(uint8_t expectedType,
                                         uint32_t* remainingLength) {
  uint8_t packetType = 0;
  if (!readExact(&packetType, 1) || packetType != expectedType) {
    return false;
  }

  uint32_t multiplier = 1;
  uint32_t value = 0;
  uint8_t encodedByte = 0;
  do {
    if (!readExact(&encodedByte, 1)) {
      return false;
    }
    value += (encodedByte & 127U) * multiplier;
    multiplier *= 128U;
    if (multiplier > 128U * 128U * 128U * 128U) {
      return false;
    }
  } while ((encodedByte & 128U) != 0);

  if (remainingLength != nullptr) {
    *remainingLength = value;
  }
  return true;
}

bool ArduinoMqttClient::readExact(uint8_t* buffer, size_t length) {
  size_t offset = 0;
  while (offset < length && client_.connected()) {
    const int value = client_.read();
    if (value < 0) {
      return false;
    }
    buffer[offset++] = static_cast<uint8_t>(value);
  }
  return offset == length;
}

MqttPublishResult ArduinoMqttClient::publishPacket(
    const MqttMessage& message) {
  const uint8_t packetType =
      kMqttPacketTypePublish |
      (message.retain ? 0x01 : 0x00) |
      (static_cast<uint8_t>(message.qos) << 1);
  const uint32_t remainingLength =
      2U + mqttStringLength(message.topic) + message.payloadLength +
      (message.qos == MqttQos::kAtLeastOnce ? 2U : 0U);
  if (client_.write(packetType) != 1 ||
      !writeRemainingLength(remainingLength) || !writeString(message.topic)) {
    return MqttPublishResult::kClientFailure;
  }

  if (message.qos == MqttQos::kAtLeastOnce) {
    const uint16_t packetId = nextNonZeroPacketId(nextPacketId_++);
    if (client_.write(static_cast<uint8_t>(packetId >> 8)) != 1 ||
        client_.write(static_cast<uint8_t>(packetId & 0xFF)) != 1) {
      return MqttPublishResult::kClientFailure;
    }
  }

  if (message.payloadLength > 0 &&
      client_.write(reinterpret_cast<const uint8_t*>(message.payload),
                    message.payloadLength) != message.payloadLength) {
    return MqttPublishResult::kClientFailure;
  }

  return MqttPublishResult::kAccepted;
}

void ArduinoMqttClient::handleIncomingPublish(uint8_t packetType,
                                              uint32_t remainingLength) {
  if (remainingLength < 2) {
    return;
  }

  const uint8_t qos = (packetType & 0x06) >> 1;

  uint8_t topicLengthBuffer[2] = {};
  if (!readExact(topicLengthBuffer, sizeof(topicLengthBuffer))) {
    return;
  }
  const uint16_t topicLength =
      (static_cast<uint16_t>(topicLengthBuffer[0]) << 8) |
      topicLengthBuffer[1];
  if (topicLength >= kTopicBufferLength ||
      remainingLength < static_cast<uint32_t>(2U + topicLength)) {
    return;
  }

  char topic[kTopicBufferLength] = {};
  if (!readExact(reinterpret_cast<uint8_t*>(topic), topicLength)) {
    return;
  }
  topic[topicLength] = '\0';

  uint16_t packetId = 0;
  uint32_t headerLength = 2U + topicLength;
  if (qos > 0) {
    if (remainingLength < headerLength + 2U) {
      return;
    }
    uint8_t packetIdBuffer[2] = {};
    if (!readExact(packetIdBuffer, sizeof(packetIdBuffer))) {
      return;
    }
    packetId = (static_cast<uint16_t>(packetIdBuffer[0]) << 8) |
               packetIdBuffer[1];
    headerLength += 2U;
  }

  const uint32_t payloadLength = remainingLength - headerLength;
  if (payloadLength >= kPayloadBufferLength) {
    return;
  }

  char payload[kPayloadBufferLength] = {};
  if (!readExact(reinterpret_cast<uint8_t*>(payload), payloadLength)) {
    return;
  }
  payload[payloadLength] = '\0';

  if (messageHandler_ != nullptr) {
    const MqttMessage message = {topic, payload, payloadLength,
                                 qos == 0 ? MqttQos::kAtMostOnce
                                          : MqttQos::kAtLeastOnce,
                                 false};
    messageHandler_(message, messageHandlerContext_);
  }

  if (qos == 1 && packetId != 0) {
    client_.write(kMqttPacketTypePuback);
    client_.write(static_cast<uint8_t>(2));
    client_.write(static_cast<uint8_t>(packetId >> 8));
    client_.write(static_cast<uint8_t>(packetId & 0xFF));
  }
}

}  // namespace reeflow::mqtt
