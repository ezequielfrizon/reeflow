#pragma once

#include "alerts/alert_mqtt_publisher.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_command_handler.h"
#include "mqtt/mqtt_events.h"
#include "mqtt/mqtt_outbox.h"
#include "mqtt/mqtt_telemetry_publisher.h"

namespace reeflow::mqtt {

struct MqttServiceSnapshot {
  MqttConnectionState state;
  MqttConnectResult lastConnectResult;
  MqttSubscribeResult lastSubscribeResult;
  MqttDisconnectReason lastDisconnectReason;
  uint32_t nextReconnectAtMillis;
  uint32_t currentBackoffMillis;
  uint32_t lastTelemetryPublishMillis;
  uint32_t lastHeartbeatPublishMillis;
  MqttPublishResult lastTelemetryPublishResult;
  MqttPublishResult lastHeartbeatPublishResult;
  MqttPublishResult lastEventPublishResult;
  MqttCommandResult lastCommandResult;
  MqttPublishResult lastCommandResponsePublishResult;
  MqttOutboxStatus outboxStatus;
  bool commandTopicSubscribed;
};

class MqttService {
 public:
  MqttService(MqttClient& client, const config::ConfigManager& configManager,
              core::events::EventBus& eventBus);

  bool begin(uint32_t nowMillis);
  void setLogger(core::logging::Logger* logger);
  void configureCommandCallbacks(const MqttCommandCallbacks& callbacks);
  void tick(uint32_t nowMillis);
  MqttServiceSnapshot snapshot() const;

 private:
  static bool handleImportantEvent(const core::events::Event& event,
                                   void* context);
  static void handleClientMessage(const MqttMessage& message, void* context);

  bool mqttConfigured() const;
  MqttConfig currentMqttConfig() const;
  MqttPublishContext publishContext(uint32_t nowMillis) const;
  bool buildCommandTopic(char* output, size_t outputSize) const;
  MqttOutboxPolicy outboxPolicy() const;
  void configureOutbox();
  void drainOutbox(uint32_t nowMillis);
  MqttPublishResult enqueueMessage(const MqttMessage& message,
                                   uint32_t nowMillis);
  bool buildEventMessage(const core::events::Event& event,
                         uint32_t nowMillis, char* topic,
                         size_t topicSize, char* payload,
                         size_t payloadSize) const;
  bool buildCommandResponseMessage(const MqttCommandResponse& response,
                                   char* topic, size_t topicSize) const;
  MqttPublishResult publishCommandResponse(
      const MqttCommandResponse& response, uint32_t nowMillis);
  void handleCommandMessage(const MqttMessage& message);
  void attemptConnection(uint32_t nowMillis);
  void handleConnected(uint32_t nowMillis);
  void handleConnectionFailure(uint32_t nowMillis, MqttConnectResult result);
  void handleDisconnected(uint32_t nowMillis, MqttDisconnectReason reason);
  void publishDueTelemetry(uint32_t nowMillis);
  void publishImportantEvent(const core::events::Event& event,
                             uint32_t nowMillis);
  void publishAlertEvent(const core::events::Event& event,
                         uint32_t nowMillis);
  void registerImportantEventSubscriptions();
  void scheduleReconnect(uint32_t nowMillis);
  void resetBackoff();
  void updateNetworkMqttConnected(bool connected);
  void publish(MqttEventType type, uint32_t nowMillis,
               MqttDisconnectReason reason = MqttDisconnectReason::kUnknown);
  void logInfo(const char* message);
  void logWarning(const char* message);

  MqttClient& client_;
  const config::ConfigManager& configManager_;
  core::events::EventBus& eventBus_;
  core::logging::Logger* logger_ = nullptr;
  MqttCommandHandler commandHandler_;
  MqttOutbox outbox_;
  MqttTelemetryPublisher telemetryPublisher_;
  alerts::AlertMqttPublisher alertPublisher_;
  MqttServiceSnapshot snapshot_ = {};
  uint32_t lastTickMillis_ = 0;
  bool everConnected_ = false;
  bool eventSubscriptionsRegistered_ = false;
};

}  // namespace reeflow::mqtt
