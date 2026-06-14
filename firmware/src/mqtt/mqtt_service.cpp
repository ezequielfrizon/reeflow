#include "mqtt/mqtt_service.h"

#include <string.h>

#include "core/state/system_state.h"
#include "mqtt/mqtt_events.h"
#include "mqtt/mqtt_payloads.h"
#include "mqtt/mqtt_topics.h"

namespace reeflow::mqtt {
namespace {

MqttDisconnectReason disconnectReasonForConnectResult(
    MqttConnectResult result) {
  switch (result) {
    case MqttConnectResult::kCredentialMissing:
      return MqttDisconnectReason::kCredentialMissing;
    case MqttConnectResult::kAuthenticationFailed:
      return MqttDisconnectReason::kAuthenticationFailed;
    case MqttConnectResult::kBrokerUnavailable:
      return MqttDisconnectReason::kBrokerUnavailable;
    case MqttConnectResult::kTimeout:
      return MqttDisconnectReason::kConnectionTimeout;
    case MqttConnectResult::kNetworkUnavailable:
      return MqttDisconnectReason::kNetworkDisconnected;
    case MqttConnectResult::kClientFailure:
    case MqttConnectResult::kDisabled:
    case MqttConnectResult::kConnected:
    case MqttConnectResult::kAlreadyConnected:
      return MqttDisconnectReason::kUnknown;
  }

  return MqttDisconnectReason::kUnknown;
}

const char* configuredPrefix(const config::MqttConfig& config) {
  return config.topicPrefix[0] == '\0' ? kDefaultMqttTopicPrefix
                                       : config.topicPrefix;
}

MqttPublishResult mqttResultForAlertPublish(
    alerts::AlertPublishResult result) {
  switch (result) {
    case alerts::AlertPublishResult::kPublished:
      return MqttPublishResult::kAccepted;
    case alerts::AlertPublishResult::kNotConnected:
    case alerts::AlertPublishResult::kUnavailable:
      return MqttPublishResult::kNotConnected;
    case alerts::AlertPublishResult::kRejected:
      return MqttPublishResult::kClientFailure;
  }

  return MqttPublishResult::kClientFailure;
}

}  // namespace

MqttService::MqttService(MqttClient& client,
                         const config::ConfigManager& configManager,
                         core::events::EventBus& eventBus)
    : client_(client),
      configManager_(configManager),
      eventBus_(eventBus),
      telemetryPublisher_(client),
      alertPublisher_(client) {
  snapshot_.state = MqttConnectionState::kDisconnected;
  snapshot_.lastConnectResult = MqttConnectResult::kDisabled;
  snapshot_.lastSubscribeResult = MqttSubscribeResult::kNotConnected;
  snapshot_.lastDisconnectReason = MqttDisconnectReason::kUnknown;
  snapshot_.lastTelemetryPublishResult = MqttPublishResult::kNotConnected;
  snapshot_.lastHeartbeatPublishResult = MqttPublishResult::kNotConnected;
  snapshot_.lastEventPublishResult = MqttPublishResult::kNotConnected;
  snapshot_.lastCommandResult = MqttCommandResult::kUnsupported;
  snapshot_.lastCommandResponsePublishResult =
      MqttPublishResult::kNotConnected;
  configureOutbox();
  resetBackoff();
}

bool MqttService::begin(uint32_t nowMillis) {
  (void)nowMillis;
  client_.configure(currentMqttConfig());
  client_.setMessageHandler(MqttService::handleClientMessage, this);
  configureOutbox();
  registerImportantEventSubscriptions();
  updateNetworkMqttConnected(false);
  return true;
}

void MqttService::setLogger(core::logging::Logger* logger) {
  logger_ = logger;
}

void MqttService::configureCommandCallbacks(
    const MqttCommandCallbacks& callbacks) {
  commandHandler_.configure(callbacks);
}

void MqttService::tick(uint32_t nowMillis) {
  lastTickMillis_ = nowMillis;
  configureOutbox();
  const bool wifiConnected =
      core::state::currentSystemState().network.wifiConnected;

  if (!mqttConfigured()) {
    if (client_.connected()) {
      client_.disconnect(MqttDisconnectReason::kManualDisconnect);
    }
    snapshot_.state = MqttConnectionState::kDisabled;
    snapshot_.lastConnectResult = MqttConnectResult::kDisabled;
    snapshot_.commandTopicSubscribed = false;
    snapshot_.nextReconnectAtMillis = 0;
    outbox_.clear();
    snapshot_.outboxStatus = outbox_.status();
    resetBackoff();
    updateNetworkMqttConnected(false);
    return;
  }

  if (!wifiConnected) {
    if (client_.connected()) {
      client_.disconnect(MqttDisconnectReason::kNetworkDisconnected);
      handleDisconnected(nowMillis, MqttDisconnectReason::kNetworkDisconnected);
    } else {
      snapshot_.state = MqttConnectionState::kDisconnected;
      snapshot_.lastConnectResult = MqttConnectResult::kNetworkUnavailable;
      snapshot_.lastDisconnectReason =
          MqttDisconnectReason::kNetworkDisconnected;
      updateNetworkMqttConnected(false);
    }
    snapshot_.commandTopicSubscribed = false;
    return;
  }

  if (client_.connected()) {
    client_.loop(nowMillis);
    if (client_.connected()) {
      snapshot_.state = MqttConnectionState::kConnected;
      updateNetworkMqttConnected(true);
      drainOutbox(nowMillis);
      publishDueTelemetry(nowMillis);
      return;
    }

    handleDisconnected(nowMillis, MqttDisconnectReason::kRuntimeDisconnect);
    return;
  }

  if (snapshot_.state != MqttConnectionState::kReconnectPending ||
      snapshot_.nextReconnectAtMillis == 0 ||
      nowMillis >= snapshot_.nextReconnectAtMillis) {
    attemptConnection(nowMillis);
  }
}

MqttServiceSnapshot MqttService::snapshot() const {
  return snapshot_;
}

bool MqttService::handleImportantEvent(const core::events::Event& event,
                                       void* context) {
  if (context == nullptr) {
    return false;
  }

  MqttService* service = static_cast<MqttService*>(context);
  service->publishImportantEvent(event, service->lastTickMillis_);
  return true;
}

void MqttService::handleClientMessage(const MqttMessage& message,
                                      void* context) {
  if (context == nullptr) {
    return;
  }

  static_cast<MqttService*>(context)->handleCommandMessage(message);
}

bool MqttService::mqttConfigured() const {
  const config::MqttConfig& config = configManager_.mqtt();
  return config.enabled && config.host[0] != '\0' && config.clientId[0] != '\0';
}

MqttConfig MqttService::currentMqttConfig() const {
  const config::MqttConfig& config = configManager_.mqtt();
  MqttConfig mqttConfig = {};
  mqttConfig.enabled = config.enabled;
  mqttConfig.brokerHost = config.host;
  mqttConfig.brokerPort = config.port;
  mqttConfig.clientId = config.clientId;
  mqttConfig.username = config.username;
  mqttConfig.password = config.password;
  mqttConfig.topicPrefix = configuredPrefix(config);
  mqttConfig.credentialsRequired = config.credentialsRequired;
  mqttConfig.cleanSession = config.cleanSession;
  mqttConfig.keepAliveSeconds = config.keepAliveSeconds;
  mqttConfig.connectTimeoutMillis = config.connectTimeoutMillis;
  mqttConfig.heartbeatIntervalMillis = config.heartbeatIntervalMillis;
  mqttConfig.maxPayloadBytes = config.maxPayloadBytes;
  mqttConfig.maxOutboxMessages = config.maxOutboxMessages;
  mqttConfig.outboxMaxAgeMillis = kDefaultMqttOutboxMaxAgeMillis;
  mqttConfig.outboxMaxAttempts = kDefaultMqttOutboxMaxAttempts;
  mqttConfig.initialBackoffMillis = config.initialBackoffMillis;
  mqttConfig.maxBackoffMillis = config.maxBackoffMillis;
  return mqttConfig;
}

MqttPublishContext MqttService::publishContext(uint32_t nowMillis) const {
  const config::MqttConfig& config = configManager_.mqtt();
  MqttPublishContext context = {};
  context.topic.prefix = configuredPrefix(config);
  context.topic.deviceId = config.clientId;
  context.timestamp.synchronized = false;
  context.timestamp.localMillis = nowMillis;
  context.maxPayloadBytes = config.maxPayloadBytes;
  return context;
}

bool MqttService::buildCommandTopic(char* output, size_t outputSize) const {
  const config::MqttConfig& config = configManager_.mqtt();
  const MqttTopicContext topicContext = {configuredPrefix(config),
                                         config.clientId};
  return buildMqttTopic(topicContext, MqttTopicKind::kCommandRequest, output,
                        outputSize);
}

MqttOutboxPolicy MqttService::outboxPolicy() const {
  const config::MqttConfig& config = configManager_.mqtt();
  return {config.maxOutboxMessages,
          kDefaultMqttOutboxMaxAgeMillis,
          kDefaultMqttOutboxMaxAttempts};
}

void MqttService::configureOutbox() {
  outbox_.configure(outboxPolicy());
  snapshot_.outboxStatus = outbox_.status();
}

void MqttService::drainOutbox(uint32_t nowMillis) {
  snapshot_.outboxStatus = outbox_.status();
  if (snapshot_.outboxStatus.queuedMessages == 0) {
    return;
  }

  const uint8_t droppedBefore = snapshot_.outboxStatus.droppedMessages;
  const uint8_t expiredBefore = snapshot_.outboxStatus.expiredMessages;
  outbox_.drainOne(client_, nowMillis);
  snapshot_.outboxStatus = outbox_.status();
  if (snapshot_.outboxStatus.droppedMessages > droppedBefore ||
      snapshot_.outboxStatus.expiredMessages > expiredBefore) {
    logWarning("outbox message discarded");
  }
}

MqttPublishResult MqttService::enqueueMessage(const MqttMessage& message,
                                              uint32_t nowMillis) {
  const MqttOutboxResult result = outbox_.enqueue(message, nowMillis);
  snapshot_.outboxStatus = outbox_.status();
  if (result != MqttOutboxResult::kQueued) {
    logWarning("outbox message discarded");
  }
  return result == MqttOutboxResult::kQueued
             ? MqttPublishResult::kNotConnected
             : MqttPublishResult::kClientFailure;
}

bool MqttService::buildEventMessage(const core::events::Event& event,
                                    uint32_t nowMillis, char* topic,
                                    size_t topicSize, char* payload,
                                    size_t payloadSize) const {
  const config::MqttConfig& config = configManager_.mqtt();
  const MqttTopicContext topicContext = {configuredPrefix(config),
                                         config.clientId};
  MqttPayloadTimestamp timestamp = {};
  timestamp.localMillis = nowMillis;
  return buildMqttTopic(topicContext, MqttTopicKind::kEvents, topic,
                        topicSize) &&
         serializeEventPayload(event, timestamp, payload, payloadSize);
}

bool MqttService::buildCommandResponseMessage(
    const MqttCommandResponse& response, char* topic,
    size_t topicSize) const {
  const config::MqttConfig& config = configManager_.mqtt();
  const MqttTopicContext topicContext = {configuredPrefix(config),
                                         config.clientId};
  return buildMqttTopic(topicContext, MqttTopicKind::kCommandResponse, topic,
                        topicSize) &&
         response.payload[0] != '\0';
}

MqttPublishResult MqttService::publishCommandResponse(
    const MqttCommandResponse& response, uint32_t nowMillis) {
  char responseTopic[128] = {};
  if (!buildCommandResponseMessage(response, responseTopic,
                                   sizeof(responseTopic))) {
    return MqttPublishResult::kClientFailure;
  }

  const MqttMessage responseMessage = {
      responseTopic, response.payload, strlen(response.payload),
      MqttQos::kAtLeastOnce, false};

  if (!client_.connected()) {
    return enqueueMessage(responseMessage, nowMillis);
  }

  const MqttPublishResult result = client_.publish(responseMessage);
  if (result != MqttPublishResult::kAccepted) {
    logWarning("command response publish failed");
    enqueueMessage(responseMessage, nowMillis);
  }
  return result;
}

void MqttService::handleCommandMessage(const MqttMessage& message) {
  char commandTopic[128] = {};
  const bool commandTopicBuilt =
      buildCommandTopic(commandTopic, sizeof(commandTopic));
  const MqttCommandRequest request = {
      message.topic,
      commandTopicBuilt ? commandTopic : "",
      message.payload,
      message.payloadLength,
      core::state::currentSystemState().network.mqttConnected};
  const MqttCommandResponse response = commandHandler_.handle(request);
  snapshot_.lastCommandResult = response.result;
  if (response.result != MqttCommandResult::kAccepted) {
    logWarning("command rejected");
  }
  snapshot_.lastCommandResponsePublishResult =
      publishCommandResponse(response, lastTickMillis_);
}

void MqttService::attemptConnection(uint32_t nowMillis) {
  const MqttConfig config = currentMqttConfig();
  client_.configure(config);
  snapshot_.state = MqttConnectionState::kConnecting;
  snapshot_.lastConnectResult = client_.connect();

  if (snapshot_.lastConnectResult == MqttConnectResult::kConnected ||
      snapshot_.lastConnectResult == MqttConnectResult::kAlreadyConnected) {
    handleConnected(nowMillis);
    return;
  }

  handleConnectionFailure(nowMillis, snapshot_.lastConnectResult);
}

void MqttService::handleConnected(uint32_t nowMillis) {
  char commandTopic[128] = {};
  if (buildCommandTopic(commandTopic, sizeof(commandTopic))) {
    snapshot_.lastSubscribeResult =
        client_.subscribe(commandTopic, MqttQos::kAtLeastOnce);
    snapshot_.commandTopicSubscribed =
        snapshot_.lastSubscribeResult == MqttSubscribeResult::kSubscribed;
  } else {
    snapshot_.lastSubscribeResult = MqttSubscribeResult::kTopicInvalid;
    snapshot_.commandTopicSubscribed = false;
  }
  if (!snapshot_.commandTopicSubscribed) {
    logWarning("subscribe failed");
  }

  snapshot_.state = MqttConnectionState::kConnected;
  snapshot_.lastDisconnectReason = MqttDisconnectReason::kUnknown;
  snapshot_.nextReconnectAtMillis = 0;
  resetBackoff();
  updateNetworkMqttConnected(true);
  drainOutbox(nowMillis);
  publish(everConnected_ ? MqttEventType::kReconnected
                         : MqttEventType::kConnected,
          nowMillis);
  logInfo(everConnected_ ? "reconnected" : "connected");
  publishDueTelemetry(nowMillis);
  everConnected_ = true;
}

void MqttService::handleConnectionFailure(uint32_t nowMillis,
                                          MqttConnectResult result) {
  snapshot_.commandTopicSubscribed = false;
  snapshot_.lastDisconnectReason = disconnectReasonForConnectResult(result);
  updateNetworkMqttConnected(false);
  logWarning("connection failed");
  scheduleReconnect(nowMillis);
}

void MqttService::handleDisconnected(uint32_t nowMillis,
                                     MqttDisconnectReason reason) {
  snapshot_.state = MqttConnectionState::kDisconnected;
  snapshot_.lastDisconnectReason = reason;
  snapshot_.lastConnectResult = MqttConnectResult::kClientFailure;
  snapshot_.lastSubscribeResult = MqttSubscribeResult::kNotConnected;
  snapshot_.commandTopicSubscribed = false;
  updateNetworkMqttConnected(false);
  publish(MqttEventType::kDisconnected, nowMillis, reason);
  logWarning("disconnected");
  scheduleReconnect(nowMillis);
}

void MqttService::publishDueTelemetry(uint32_t nowMillis) {
  if (!client_.connected() ||
      !core::state::currentSystemState().network.mqttConnected) {
    return;
  }

  const config::MqttConfig& config = configManager_.mqtt();
  const uint32_t intervalMillis =
      config.heartbeatIntervalMillis == 0 ? kDefaultMqttHeartbeatIntervalMillis
                                          : config.heartbeatIntervalMillis;

  if (snapshot_.lastTelemetryPublishMillis == 0 ||
      nowMillis - snapshot_.lastTelemetryPublishMillis >= intervalMillis) {
    snapshot_.lastTelemetryPublishResult =
        telemetryPublisher_.publishTelemetry(core::state::currentSystemState(),
                                             publishContext(nowMillis));
    if (snapshot_.lastTelemetryPublishResult == MqttPublishResult::kAccepted) {
      snapshot_.lastTelemetryPublishMillis = nowMillis;
    } else {
      logWarning("telemetry publish failed");
    }
  }

  if (snapshot_.lastHeartbeatPublishMillis == 0 ||
      nowMillis - snapshot_.lastHeartbeatPublishMillis >= intervalMillis) {
    snapshot_.lastHeartbeatPublishResult =
        telemetryPublisher_.publishHeartbeat(core::state::currentSystemState(),
                                             publishContext(nowMillis));
    if (snapshot_.lastHeartbeatPublishResult == MqttPublishResult::kAccepted) {
      snapshot_.lastHeartbeatPublishMillis = nowMillis;
    } else {
      logWarning("heartbeat publish failed");
    }
  }
}

void MqttService::publishImportantEvent(const core::events::Event& event,
                                        uint32_t nowMillis) {
  if (event.type == core::events::EventType::kAlertRaised ||
      event.type == core::events::EventType::kAlertRecovered) {
    publishAlertEvent(event, nowMillis);
    return;
  }

  char topic[128] = {};
  char payload[768] = {};
  if (!buildEventMessage(event, nowMillis, topic, sizeof(topic), payload,
                         sizeof(payload))) {
    snapshot_.lastEventPublishResult = MqttPublishResult::kClientFailure;
    logWarning("event publish failed");
    return;
  }

  const MqttMessage message = {topic, payload, strlen(payload),
                               MqttQos::kAtLeastOnce, false};
  if (!client_.connected() ||
      !core::state::currentSystemState().network.mqttConnected) {
    snapshot_.lastEventPublishResult = enqueueMessage(message, nowMillis);
    return;
  }

  snapshot_.lastEventPublishResult = client_.publish(message);
  if (snapshot_.lastEventPublishResult != MqttPublishResult::kAccepted) {
    logWarning("event publish failed");
    enqueueMessage(message, nowMillis);
  }
}

void MqttService::publishAlertEvent(const core::events::Event& event,
                                    uint32_t nowMillis) {
  if (event.payload == nullptr) {
    snapshot_.lastEventPublishResult = MqttPublishResult::kClientFailure;
    logWarning("alert publish failed");
    return;
  }

  const alerts::AlertEvent& alertEvent =
      *static_cast<const alerts::AlertEvent*>(event.payload);
  if (alertEvent.historyEntry == nullptr) {
    snapshot_.lastEventPublishResult = MqttPublishResult::kClientFailure;
    logWarning("alert publish failed");
    return;
  }

  char topic[128] = {};
  char payload[768] = {};
  if (!alertPublisher_.buildMessage(*alertEvent.historyEntry,
                                    publishContext(nowMillis), topic,
                                    sizeof(topic), payload,
                                    sizeof(payload))) {
    snapshot_.lastEventPublishResult = MqttPublishResult::kPayloadTooLarge;
    logWarning("alert publish failed");
    return;
  }

  const MqttMessage message = {topic, payload, strlen(payload),
                               MqttQos::kAtLeastOnce, false};
  if (!client_.connected() ||
      !core::state::currentSystemState().network.mqttConnected) {
    snapshot_.lastEventPublishResult = enqueueMessage(message, nowMillis);
    return;
  }

  snapshot_.lastEventPublishResult = mqttResultForAlertPublish(
      alertPublisher_.publish(*alertEvent.historyEntry,
                              publishContext(nowMillis)));
  if (snapshot_.lastEventPublishResult != MqttPublishResult::kAccepted) {
    logWarning("alert publish failed");
    enqueueMessage(message, nowMillis);
  }
}

void MqttService::registerImportantEventSubscriptions() {
  if (eventSubscriptionsRegistered_) {
    return;
  }

  eventBus_.subscribe(core::events::EventType::kSystemStateChanged,
                      MqttService::handleImportantEvent, this);
  eventBus_.subscribe(core::events::EventType::kMqttConnected,
                      MqttService::handleImportantEvent, this);
  eventBus_.subscribe(core::events::EventType::kMqttDisconnected,
                      MqttService::handleImportantEvent, this);
  eventBus_.subscribe(core::events::EventType::kMqttReconnected,
                      MqttService::handleImportantEvent, this);
  eventBus_.subscribe(core::events::EventType::kAlertRaised,
                      MqttService::handleImportantEvent, this);
  eventBus_.subscribe(core::events::EventType::kAlertRecovered,
                      MqttService::handleImportantEvent, this);
  eventSubscriptionsRegistered_ = true;
}

void MqttService::scheduleReconnect(uint32_t nowMillis) {
  const config::MqttConfig& config = configManager_.mqtt();
  const uint32_t delayMillis =
      snapshot_.currentBackoffMillis == 0 ? config.initialBackoffMillis
                                          : snapshot_.currentBackoffMillis;
  snapshot_.state = MqttConnectionState::kReconnectPending;
  snapshot_.nextReconnectAtMillis = nowMillis + delayMillis;
  snapshot_.currentBackoffMillis =
      delayMillis >= config.maxBackoffMillis
          ? config.maxBackoffMillis
          : (delayMillis * 2U > config.maxBackoffMillis
                 ? config.maxBackoffMillis
                 : delayMillis * 2U);
}

void MqttService::resetBackoff() {
  snapshot_.currentBackoffMillis = 0;
}

void MqttService::updateNetworkMqttConnected(bool connected) {
  core::state::NetworkState network =
      core::state::currentSystemState().network;
  network.mqttConnected = connected;
  core::state::updateNetworkState(network);
}

void MqttService::publish(MqttEventType type, uint32_t nowMillis,
                          MqttDisconnectReason reason) {
  MqttEvent event = {};
  event.type = type;
  event.disconnectReason = reason;
  event.occurredAtMillis = nowMillis;
  publishMqttEvent(event, eventBus_);
}

void MqttService::logInfo(const char* message) {
  if (logger_ != nullptr) {
    logger_->info("mqtt", message);
  }
}

void MqttService::logWarning(const char* message) {
  if (logger_ != nullptr) {
    logger_->warning("mqtt", message);
  }
}

}  // namespace reeflow::mqtt
