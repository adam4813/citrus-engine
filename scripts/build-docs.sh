#!/usr/bin/env bash
# Build documentation locally
# Usage: ./scripts/build-docs.sh [--clean] [--serve]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# Parse arguments
CLEAN=false
SERVE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --serve)
            SERVE=true
            shift
            ;;
        *)
            error "Unknown option: $1"
            ;;
    esac
done

# Check dependencies
if ! command -v python3 &> /dev/null; then
    error "python3 is required but not installed"
fi

if ! command -v doxygen &> /dev/null; then
    error "doxygen is required but not installed. Install with: apt-get install doxygen (or brew install doxygen on macOS)"
fi

# Clean build directories if requested
if [ "$CLEAN" = true ]; then
    info "Cleaning build directories..."
    rm -rf docs/_doxygen site
fi

# Install Python dependencies
info "Installing Python dependencies..."
if ! pip3 show mkdocs &> /dev/null; then
    info "Installing from requirements.txt..."
    pip3 install -q -r docs/requirements.txt
else
    info "Dependencies already installed (use --clean to reinstall)"
fi

# Generate Doxygen documentation
info "Generating Doxygen documentation..."
doxygen Doxyfile

if [ ! -d "docs/_doxygen/html" ]; then
    error "Doxygen build failed - docs/_doxygen/html not found"
fi

info "✓ Doxygen documentation generated at docs/_doxygen/html/"

# Build MkDocs site
info "Building MkDocs site..."
mkdocs build

if [ ! -d "site" ]; then
    error "MkDocs build failed - site directory not found"
fi

info "✓ MkDocs site built at site/"

# Serve if requested
if [ "$SERVE" = true ]; then
    info "Starting development server at http://127.0.0.1:8000"
    info "Press Ctrl+C to stop"
    mkdocs serve
else
    info ""
    info "Documentation built successfully!"
    info "  - Doxygen API docs: docs/_doxygen/html/index.html"
    info "  - MkDocs site: site/index.html"
    info ""
    info "To serve locally with auto-reload:"
    info "  ./scripts/build-docs.sh --serve"
fi
