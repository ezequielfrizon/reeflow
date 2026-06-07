#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$TEST_DIR/run_phase4_water_level_sensor_boundary_tests.sh"
"$TEST_DIR/run_phase4_vl6180x_level_sensor_tests.sh"
"$TEST_DIR/run_phase4_water_level_state_evaluator_tests.sh"
"$TEST_DIR/run_phase4_water_level_service_tests.sh"
