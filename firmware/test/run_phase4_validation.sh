#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$FIRMWARE_DIR/test"

"$TEST_DIR/unit/run_phase4_unit_tests.sh"
"$TEST_DIR/integration/run_phase4_integration_tests.sh"

"$TEST_DIR/unit/run_phase2_event_bus_tests.sh"
"$TEST_DIR/unit/run_phase2_system_state_tests.sh"
"$TEST_DIR/unit/run_phase2_config_manager_tests.sh"
"$TEST_DIR/unit/run_phase2_task_scheduler_tests.sh"
"$TEST_DIR/integration/run_phase2_core_app_integration_tests.sh"

"$TEST_DIR/unit/run_phase3_temperature_sensor_boundary_tests.sh"
"$TEST_DIR/unit/run_phase3_ds18b20_adapter_tests.sh"
"$TEST_DIR/unit/run_phase3_temperature_state_evaluator_tests.sh"
"$TEST_DIR/unit/run_phase3_temperature_service_tests.sh"
"$TEST_DIR/integration/run_phase3_temperature_scheduler_integration_tests.sh"

"$TEST_DIR/integration/run_phase4_scope_review.sh"

pio run -d "$FIRMWARE_DIR"
