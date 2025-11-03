#!/bin/bash
# Copilot Environment Setup Script
# This script is automatically executed by GitHub Copilot to prepare the development environment
# Documentation: https://docs.github.com/en/enterprise-cloud@latest/copilot/how-tos/use-copilot-agents/coding-agent/customize-the-agent-environment

set -e  # Exit on error

echo "=================================================="
echo "Setting up Citrus Engine development environment"
echo "=================================================="

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     MACHINE=Linux;;
    Darwin*)    MACHINE=Mac;;
    CYGWIN*)    MACHINE=Cygwin;;
    MINGW*)     MACHINE=MinGw;;
    *)          MACHINE="UNKNOWN:${OS}"
esac

echo "Detected OS: ${MACHINE}"

# ======================
# Step 1: Install System Dependencies
# ======================
if [ "${MACHINE}" == "Linux" ]; then
    echo ""
    echo "Step 1: Installing system dependencies (Linux)..."
    
    # Check if apt-get is available
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            pkg-config \
            libx11-dev \
            libxrandr-dev \
            libxinerama-dev \
            libxcursor-dev \
            libxi-dev \
            libgl1-mesa-dev \
            libglu1-mesa-dev \
            xvfb
        
        echo "✓ System dependencies installed"
    else
        echo "⚠ apt-get not found, skipping system dependencies installation"
    fi
elif [ "${MACHINE}" == "Mac" ]; then
    echo ""
    echo "Step 1: Checking system dependencies (macOS)..."
    
    # Check if Homebrew is available
    if command -v brew &> /dev/null; then
        echo "Homebrew detected. Installing dependencies..."
        brew install cmake ninja pkg-config
        echo "✓ System dependencies installed"
    else
        echo "⚠ Homebrew not found. Please install it from https://brew.sh/"
        echo "  Then run: brew install cmake ninja pkg-config"
    fi
else
    echo "⚠ Unsupported OS for automatic dependency installation: ${MACHINE}"
fi

# ======================
# Step 2: Setup Compiler (Clang-18+)
# ======================
echo ""
echo "Step 2: Setting up compiler..."

# Use aminya/setup-cpp for compiler setup if available (for Copilot environments)
# This is primarily for Linux environments where we need Clang-18+
if [ "${MACHINE}" == "Linux" ]; then
    # Check if clang-18 is already installed
    if command -v clang-18 &> /dev/null; then
        echo "✓ Clang-18 already installed"
        export CC=clang-18
        export CXX=clang++-18
    else
        # Try to install via apt (Ubuntu/Debian)
        if command -v apt-get &> /dev/null; then
            echo "Installing Clang-18..."
            # Add LLVM repository for Clang-18
            wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add - 2>/dev/null || true
            sudo add-apt-repository -y "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-18 main" 2>/dev/null || true
            sudo apt-get update
            sudo apt-get install -y clang-18 || {
                echo "⚠ Failed to install Clang-18. Falling back to system default compiler."
            }
            
            if command -v clang-18 &> /dev/null; then
                export CC=clang-18
                export CXX=clang++-18
                echo "✓ Clang-18 installed and configured"
            fi
        fi
    fi
elif [ "${MACHINE}" == "Mac" ]; then
    # macOS typically uses Apple Clang by default
    if command -v clang &> /dev/null; then
        export CC=clang
        export CXX=clang++
        echo "✓ Using system Clang compiler"
    fi
fi

# Display compiler information
if [ -n "${CC}" ]; then
    echo "Compiler: ${CC}"
    ${CC} --version 2>&1 | head -n 1 || echo "Unable to get compiler version"
fi

# ======================
# Step 3: Setup vcpkg
# ======================
echo ""
echo "Step 3: Setting up vcpkg..."

# Determine vcpkg location (parent directory of citrus-engine)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
VCPKG_ROOT="${REPO_ROOT}/../vcpkg"

# Check if vcpkg exists
if [ -d "${VCPKG_ROOT}" ]; then
    echo "✓ vcpkg found at: ${VCPKG_ROOT}"
else
    echo "vcpkg not found. Cloning vcpkg..."
    PARENT_DIR="$(dirname "${REPO_ROOT}")"
    
    if [ -w "${PARENT_DIR}" ]; then
        cd "${PARENT_DIR}"
        git clone https://github.com/microsoft/vcpkg.git
        echo "✓ vcpkg cloned to: ${VCPKG_ROOT}"
    else
        echo "⚠ Cannot write to parent directory: ${PARENT_DIR}"
        echo "  Please manually clone vcpkg to: ${VCPKG_ROOT}"
        echo "  Command: git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT}"
    fi
fi

# Bootstrap vcpkg if not already done
if [ -d "${VCPKG_ROOT}" ]; then
    if [ -f "${VCPKG_ROOT}/vcpkg" ] || [ -f "${VCPKG_ROOT}/vcpkg.exe" ]; then
        echo "✓ vcpkg already bootstrapped"
    else
        echo "Bootstrapping vcpkg..."
        cd "${VCPKG_ROOT}"
        
        if [ "${MACHINE}" == "Linux" ] || [ "${MACHINE}" == "Mac" ]; then
            ./bootstrap-vcpkg.sh
        else
            ./bootstrap-vcpkg.bat
        fi
        
        echo "✓ vcpkg bootstrapped"
    fi
    
    # Export VCPKG_ROOT
    export VCPKG_ROOT="${VCPKG_ROOT}"
    echo "✓ VCPKG_ROOT set to: ${VCPKG_ROOT}"
else
    echo "⚠ vcpkg directory not found at: ${VCPKG_ROOT}"
fi

# ======================
# Step 4: Verify Setup
# ======================
echo ""
echo "Step 4: Verifying setup..."

SETUP_SUCCESS=true

# Check compiler
if [ -n "${CC}" ] && command -v ${CC} &> /dev/null; then
    echo "✓ Compiler: ${CC}"
else
    echo "✗ Compiler not found"
    SETUP_SUCCESS=false
fi

# Check CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n 1)
    echo "✓ CMake: ${CMAKE_VERSION}"
else
    echo "✗ CMake not found"
    SETUP_SUCCESS=false
fi

# Check Ninja
if command -v ninja &> /dev/null; then
    NINJA_VERSION=$(ninja --version)
    echo "✓ Ninja: ${NINJA_VERSION}"
else
    echo "✗ Ninja not found"
    SETUP_SUCCESS=false
fi

# Check vcpkg
if [ -n "${VCPKG_ROOT}" ] && [ -d "${VCPKG_ROOT}" ]; then
    echo "✓ vcpkg: ${VCPKG_ROOT}"
else
    echo "✗ vcpkg not properly configured"
    SETUP_SUCCESS=false
fi

# ======================
# Summary
# ======================
echo ""
echo "=================================================="
if [ "${SETUP_SUCCESS}" = true ]; then
    echo "✓ Environment setup complete!"
    echo ""
    echo "Environment variables set:"
    echo "  VCPKG_ROOT=${VCPKG_ROOT}"
    [ -n "${CC}" ] && echo "  CC=${CC}"
    [ -n "${CXX}" ] && echo "  CXX=${CXX}"
    echo ""
    echo "Next steps:"
    echo "  1. Configure: cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=<triplet>"
    echo "  2. Build:     cmake --build --preset cli-native-debug"
    echo ""
    echo "Available triplets:"
    echo "  - x64-linux (Linux native with Clang-18+)"
    echo "  - x64-windows (Windows native)"
    echo "  - x64-osx (macOS native)"
    echo "  - wasm32-emscripten (Web/Emscripten)"
else
    echo "⚠ Environment setup completed with warnings"
    echo "  Some dependencies may need manual installation."
    echo "  See AGENTS.md for detailed setup instructions."
fi
echo "=================================================="
