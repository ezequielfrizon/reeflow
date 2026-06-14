#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

check_present() {
  local pattern="$1"
  local message="$2"
  shift 2

  if ! grep -RInE "$pattern" "$@" >/dev/null; then
    echo "$message" >&2
    exit 1
  fi
}

check_absent() {
  local pattern="$1"
  local message="$2"
  shift 2

  if grep -RInE "$pattern" "$@" >/dev/null; then
    echo "$message" >&2
    grep -RInE "$pattern" "$@" >&2
    exit 1
  fi
}

check_present \
  "MqttService|MqttTelemetryPublisher|MqttCommandHandler|MqttOutbox" \
  "Phase 11 MQTT services must exist behind firmware boundaries." \
  "$FIRMWARE_DIR/include/mqtt" \
  "$FIRMWARE_DIR/src/mqtt"

check_present \
  "ArduinoMqttClient|MqttService mqttService" \
  "Phase 11 default app must wire the real MQTT client and service." \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_present \
  "restoreMqtt|mqttService_->begin|runMqttTask|kMqttTaskName|setLogger" \
  "Phase 11 runtime must restore MQTT config, begin MQTT, register the scheduler task, and attach logger." \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_present \
  "mqtt::kDefaultMqttInitialBackoffMillis" \
  "Phase 11 MQTT scheduler interval must use MQTT runtime configuration defaults." \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_present \
  "wifiConnected|mqttConfigured\\(\\)|client_\\.connected\\(\\)" \
  "Phase 11 MQTT tick must gate connection on Wi-Fi, valid config, and client state." \
  "$FIRMWARE_DIR/src/mqtt/mqtt_service.cpp"

check_present \
  "logger_->(info|warning)\\(\"mqtt\"" \
  "Phase 11 MQTT runtime must consolidate functional MQTT logs." \
  "$FIRMWARE_DIR/src/mqtt/mqtt_service.cpp"

check_absent \
  "test/fakes|fake_mqtt_client" \
  "Real firmware code must not include MQTT test fakes." \
  "$FIRMWARE_DIR/include" \
  "$FIRMWARE_DIR/src"

if find "$FIRMWARE_DIR/test/unit" -type f \( -iname '*mqtt*' -o -iname '*phase11*' \) -print | grep -q .; then
  echo "Phase 11 must not create MQTT or phase11 unit tests." >&2
  find "$FIRMWARE_DIR/test/unit" -type f \( -iname '*mqtt*' -o -iname '*phase11*' \) -print >&2
  exit 1
fi
