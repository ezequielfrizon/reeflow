#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase13_resilience_integration_tests"
mkdir -p "$BUILD_DIR"

COMMON_SOURCES=(
  "$FIRMWARE_DIR/src/alerts/alert_cooldown.cpp"
  "$FIRMWARE_DIR/src/alerts/alert_detector.cpp"
  "$FIRMWARE_DIR/src/alerts/alert_events.cpp"
  "$FIRMWARE_DIR/src/alerts/alert_history.cpp"
  "$FIRMWARE_DIR/src/alerts/alert_manager.cpp"
  "$FIRMWARE_DIR/src/alerts/alert_mqtt_publisher.cpp"
  "$FIRMWARE_DIR/src/alerts/alert_registry.cpp"
  "$FIRMWARE_DIR/src/app/core_app.cpp"
  "$FIRMWARE_DIR/src/config/config_manager.cpp"
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp"
  "$FIRMWARE_DIR/src/core/logging/logger.cpp"
  "$FIRMWARE_DIR/src/core/platform/core_platform.cpp"
  "$FIRMWARE_DIR/src/core/scheduler/task_scheduler.cpp"
  "$FIRMWARE_DIR/src/core/state/system_state.cpp"
  "$FIRMWARE_DIR/src/core/watchdog/watchdog_service.cpp"
  "$FIRMWARE_DIR/src/modules/ato/ato_events.cpp"
  "$FIRMWARE_DIR/src/modules/ato/ato_policy.cpp"
  "$FIRMWARE_DIR/src/modules/ato/ato_service.cpp"
  "$FIRMWARE_DIR/src/modules/lighting/lighting_events.cpp"
  "$FIRMWARE_DIR/src/modules/lighting/lighting_policy.cpp"
  "$FIRMWARE_DIR/src/modules/lighting/lighting_service.cpp"
  "$FIRMWARE_DIR/src/modules/modes/mode_automation_gate.cpp"
  "$FIRMWARE_DIR/src/modules/modes/mode_effects.cpp"
  "$FIRMWARE_DIR/src/modules/modes/mode_events.cpp"
  "$FIRMWARE_DIR/src/modules/modes/mode_policy.cpp"
  "$FIRMWARE_DIR/src/modules/modes/mode_service.cpp"
  "$FIRMWARE_DIR/src/modules/relays/relay_events.cpp"
  "$FIRMWARE_DIR/src/modules/relays/relay_service.cpp"
  "$FIRMWARE_DIR/src/modules/temperature/temperature_events.cpp"
  "$FIRMWARE_DIR/src/modules/temperature/temperature_service.cpp"
  "$FIRMWARE_DIR/src/modules/temperature/temperature_state_evaluator.cpp"
  "$FIRMWARE_DIR/src/modules/water_level/water_level_calibration.cpp"
  "$FIRMWARE_DIR/src/modules/water_level/water_level_events.cpp"
  "$FIRMWARE_DIR/src/modules/water_level/water_level_service.cpp"
  "$FIRMWARE_DIR/src/modules/water_level/water_level_state_evaluator.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_command_handler.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_events.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_outbox.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_payloads.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_service.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_telemetry_publisher.cpp"
  "$FIRMWARE_DIR/src/mqtt/mqtt_topics.cpp"
  "$FIRMWARE_DIR/src/network/network_events.cpp"
  "$FIRMWARE_DIR/src/network/network_heartbeat.cpp"
  "$FIRMWARE_DIR/src/network/network_status_service.cpp"
  "$FIRMWARE_DIR/src/network/ntp_service.cpp"
  "$FIRMWARE_DIR/src/network/wifi_service.cpp"
  "$FIRMWARE_DIR/src/resilience/resilience_report.cpp"
  "$FIRMWARE_DIR/src/resilience/resilience_scenarios.cpp"
  "$FIRMWARE_DIR/src/storage/storage_events.cpp"
  "$FIRMWARE_DIR/src/storage/storage_service.cpp"
  "$FIRMWARE_DIR/src/storage/storage_write_policy.cpp"
)

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/integration/test_resilience_end_to_end_smoke.cpp" \
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
  "$FIRMWARE_DIR/src/core/logging/logger.cpp" \
  "$FIRMWARE_DIR/src/core/scheduler/task_scheduler.cpp" \
  "$FIRMWARE_DIR/src/core/state/system_state.cpp" \
  "$FIRMWARE_DIR/src/resilience/resilience_report.cpp" \
  "$FIRMWARE_DIR/src/resilience/resilience_scenarios.cpp" \
  -o "$BUILD_DIR/test_resilience_end_to_end_smoke"

"$BUILD_DIR/test_resilience_end_to_end_smoke"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/integration/test_resilience_boot_power_integration.cpp" \
  "${COMMON_SOURCES[@]}" \
  -o "$BUILD_DIR/test_resilience_boot_power_integration"

"$BUILD_DIR/test_resilience_boot_power_integration"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/integration/test_resilience_network_mqtt_integration.cpp" \
  "${COMMON_SOURCES[@]}" \
  -o "$BUILD_DIR/test_resilience_network_mqtt_integration"

"$BUILD_DIR/test_resilience_network_mqtt_integration"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/integration/test_resilience_sensor_offline_integration.cpp" \
  "${COMMON_SOURCES[@]}" \
  -o "$BUILD_DIR/test_resilience_sensor_offline_integration"

"$BUILD_DIR/test_resilience_sensor_offline_integration"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/integration/test_resilience_ato_stuck_integration.cpp" \
  "${COMMON_SOURCES[@]}" \
  -o "$BUILD_DIR/test_resilience_ato_stuck_integration"

"$BUILD_DIR/test_resilience_ato_stuck_integration"
