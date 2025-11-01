#!/usr/bin/env bash
# Build documentation locally
# Usage: ./scripts/build-docs.sh [--clean] [--serve]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Check dependencies
command -v python3 >/dev/null 2>&1 || { echo "Error: python3 is required"; exit 1; }
command -v doxygen >/dev/null 2>&1 || { echo "Error: doxygen is required"; exit 1; }
command -v mkdocs >/dev/null 2>&1 || { echo "Error: mkdocs is required (pip install -r docs/requirements.txt)"; exit 1; }

# Parse arguments
CLEAN=false
SERVE=false
for arg in "$@"; do
    case $arg in
        --clean) CLEAN=true ;;
        --serve) SERVE=true ;;
    esac
done

# Clean if requested
[ "$CLEAN" = true ] && rm -rf docs/_doxygen site

# Build documentation
doxygen Doxyfile
mkdocs build

# Serve if requested
[ "$SERVE" = true ] && exec mkdocs serve
