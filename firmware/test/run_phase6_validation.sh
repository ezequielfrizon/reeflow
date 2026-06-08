#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$FIRMWARE_DIR/test"

bash "$TEST_DIR/unit/run_phase6_unit_tests.sh"
bash "$TEST_DIR/integration/run_phase6_integration_tests.sh"

bash "$TEST_DIR/unit/run_phase2_event_bus_tests.sh"
bash "$TEST_DIR/unit/run_phase2_system_state_tests.sh"
bash "$TEST_DIR/unit/run_phase2_config_manager_tests.sh"
bash "$TEST_DIR/unit/run_phase2_task_scheduler_tests.sh"
bash "$TEST_DIR/unit/run_phase2_watchdog_tests.sh"
bash "$TEST_DIR/integration/run_phase2_core_app_integration_tests.sh"

bash "$TEST_DIR/unit/run_phase4_water_level_state_evaluator_tests.sh"
bash "$TEST_DIR/unit/run_phase4_water_level_service_tests.sh"
bash "$TEST_DIR/integration/run_phase4_water_level_scheduler_integration_tests.sh"

bash "$TEST_DIR/unit/run_phase5_relay_contract_tests.sh"
bash "$TEST_DIR/unit/run_phase5_relay_service_tests.sh"
bash "$TEST_DIR/integration/run_phase5_integration_tests.sh"

bash "$TEST_DIR/integration/run_phase6_scope_review.sh"

pio run -d "$FIRMWARE_DIR"
