#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$TEST_DIR/run_phase3_temperature_sensor_boundary_tests.sh"
"$TEST_DIR/run_phase3_ds18b20_adapter_tests.sh"
"$TEST_DIR/run_phase3_temperature_state_evaluator_tests.sh"
"$TEST_DIR/run_phase3_temperature_service_tests.sh"
