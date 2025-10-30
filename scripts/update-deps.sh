#!/usr/bin/env bash
# Update Python dependencies using pip-tools
# Usage: ./scripts/update-deps.sh [--upgrade]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
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
UPGRADE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --upgrade)
            UPGRADE=true
            shift
            ;;
        *)
            error "Unknown option: $1"
            ;;
    esac
done

# Check if pip-tools is installed
if ! pip3 show pip-tools &> /dev/null; then
    info "Installing pip-tools..."
    pip3 install pip-tools
fi

# Compile requirements
if [ "$UPGRADE" = true ]; then
    info "Upgrading all dependencies to latest compatible versions..."
    pip-compile --upgrade --strip-extras docs/requirements.in
else
    info "Compiling requirements with existing versions..."
    pip-compile --strip-extras docs/requirements.in
fi

info "✓ requirements.txt updated"
info ""
info "Next steps:"
info "  1. Review the changes in docs/requirements.txt"
info "  2. Test the build: ./scripts/build-docs.sh"
info "  3. Commit both requirements.in and requirements.txt"
