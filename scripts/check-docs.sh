#!/usr/bin/env bash
# Check documentation builds correctly
# Usage: ./scripts/check-docs.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Check dependencies
command -v python3 >/dev/null 2>&1 || { echo "Error: python3 is required"; exit 1; }
command -v doxygen >/dev/null 2>&1 || { echo "Error: doxygen is required"; exit 1; }
command -v mkdocs >/dev/null 2>&1 || { echo "Error: mkdocs is required"; exit 1; }

# Build documentation
./scripts/build-docs.sh --clean
