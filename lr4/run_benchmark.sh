#!/usr/bin/env bash
set -euo pipefail

# Determine repository root as the directory containing this script.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Build the benchmark binary.
make code/benchmark

# Default options can be overridden by CLI arguments.
# Pass through any user-provided flags to the benchmark executable.
./code/benchmark "$@"
