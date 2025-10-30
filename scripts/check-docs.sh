#!/usr/bin/env bash
# Check documentation for issues (broken links, missing Doxygen comments, etc.)
# Usage: ./scripts/check-docs.sh

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
}

ISSUES_FOUND=0

info "Checking documentation..."
echo ""

# Check if documentation builds without errors
info "1. Building documentation..."
if ./scripts/build-docs.sh --clean > /tmp/build-docs.log 2>&1; then
    info "✓ Documentation builds successfully"
else
    error "✗ Documentation build failed. Check /tmp/build-docs.log"
    ((ISSUES_FOUND++))
fi

# Check for broken links in MkDocs
info "2. Checking for broken internal links..."
if grep -r "WARNING.*link" /tmp/build-docs.log > /dev/null 2>&1; then
    warn "⚠ Found broken links in documentation:"
    grep "WARNING.*link" /tmp/build-docs.log || true
    ((ISSUES_FOUND++))
else
    info "✓ No broken links found"
fi

# Check if Doxygen generated output
info "3. Checking Doxygen output..."
if [ -d "docs/_doxygen/html" ] && [ -f "docs/_doxygen/html/index.html" ]; then
    info "✓ Doxygen documentation generated"
else
    error "✗ Doxygen documentation not found"
    ((ISSUES_FOUND++))
fi

# Count documented vs undocumented classes
info "4. Checking API documentation coverage..."
if [ -f "docs/_doxygen/xml/index.xml" ]; then
    TOTAL_CLASSES=$(grep -c "<compounddef.*kind=\"class\"" docs/_doxygen/xml/*.xml 2>/dev/null || echo "0")
    info "  Found $TOTAL_CLASSES classes in codebase"
    
    if [ "$TOTAL_CLASSES" -eq 0 ]; then
        warn "⚠ No classes found - documentation may be incomplete"
    fi
fi

# Check if requirements.txt is up to date
info "5. Checking if requirements.txt is up to date..."
if [ -f "docs/requirements.in" ]; then
    # Try to compile and check if there are differences
    pip-compile --quiet --dry-run --strip-extras docs/requirements.in > /tmp/requirements-check.txt 2>&1 || true
    if diff -q docs/requirements.txt /tmp/requirements-check.txt > /dev/null 2>&1; then
        info "✓ requirements.txt is up to date"
    else
        warn "⚠ requirements.txt may be out of date. Run: ./scripts/update-deps.sh"
        ((ISSUES_FOUND++))
    fi
fi

echo ""
echo "========================================"
if [ $ISSUES_FOUND -eq 0 ]; then
    info "✓ All checks passed!"
    exit 0
else
    error "✗ Found $ISSUES_FOUND issue(s)"
    exit 1
fi
