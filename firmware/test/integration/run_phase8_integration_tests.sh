#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$TEST_DIR/run_phase8_lighting_scheduler_integration_tests.sh"
