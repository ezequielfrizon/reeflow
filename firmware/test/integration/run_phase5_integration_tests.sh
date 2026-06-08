#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$TEST_DIR/run_phase2_core_app_integration_tests.sh"
