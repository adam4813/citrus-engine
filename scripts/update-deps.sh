#!/usr/bin/env bash
# Update Python dependencies using pip-tools
# Usage: ./scripts/update-deps.sh [--upgrade]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Check dependencies
command -v pip-compile >/dev/null 2>&1 || { echo "Error: pip-compile is required (pip install pip-tools)"; exit 1; }

# Compile requirements
if [[ "$*" == *"--upgrade"* ]]; then
    pip-compile --upgrade --strip-extras docs/requirements.in
else
    pip-compile --strip-extras docs/requirements.in
fi
