# Documentation Build Approaches - Trade-offs

This document compares the three approaches implemented for building documentation in Citrus Engine.

## Overview

We provide three complementary approaches:

1. **Shell Scripts** (`scripts/*.sh`)
2. **CMake Custom Targets** (`cmake/Documentation.cmake`)
3. **GitHub Actions** (`.github/workflows/docs.yml`)
4. **Makefile** (convenience wrapper)

## Approach Comparison

### 1. Shell Scripts

**Location:** `scripts/build-docs.sh`, `scripts/update-deps.sh`, `scripts/check-docs.sh`

**Pros:**
- ✅ Simple and explicit - easy to understand what's happening
- ✅ Works anywhere (no CMake configure required)
- ✅ Great for CI/CD pipelines
- ✅ Easy to debug and modify
- ✅ Can be called from any directory
- ✅ Rich output with colors and progress messages

**Cons:**
- ❌ Not integrated with main build system
- ❌ Platform-specific (bash may not be available on all systems)
- ❌ No dependency tracking (always rebuilds)

**Best for:**
- Quick manual builds
- CI/CD automation
- One-off documentation updates
- When you don't want to configure CMake

**Usage:**
```bash
./scripts/build-docs.sh --serve
./scripts/check-docs.sh
```

---

### 2. CMake Custom Targets

**Location:** `cmake/Documentation.cmake` included in `CMakeLists.txt`

**Pros:**
- ✅ Integrated with C++ build system
- ✅ Familiar to C++ developers
- ✅ Can add documentation to overall build target
- ✅ Respects CMake conventions
- ✅ Cross-platform (CMake handles platform differences)
- ✅ Can be part of "make all" workflow
- ✅ Proper dependency tracking possible

**Cons:**
- ❌ Requires CMake configure step first
- ❌ Opt-in with `-DBUILD_DOCS=ON` flag
- ❌ Adds dependencies (Doxygen, Python) to build requirements
- ❌ More complex to debug than shell scripts
- ❌ Overhead for documentation-only changes

**Best for:**
- C++ developers who work primarily in CMake
- Integrated build workflows
- When documentation is part of release process
- Projects where CMake is always configured

**Usage:**
```bash
cmake --preset cli-native -DBUILD_DOCS=ON
cmake --build build --target docs
```

---

### 3. GitHub Actions

**Location:** `.github/workflows/docs.yml`

**Pros:**
- ✅ Automatic validation on every PR
- ✅ Catches documentation issues before merge
- ✅ Produces artifacts for download
- ✅ Can deploy to GitHub Pages automatically
- ✅ No local setup required for contributors
- ✅ Consistent environment (Ubuntu 22.04)

**Cons:**
- ❌ Requires GitHub (not portable to other platforms)
- ❌ Slower feedback than local builds
- ❌ Uses CI minutes
- ❌ Can't test locally without act or similar

**Best for:**
- Automated quality checks
- Ensuring PRs don't break documentation
- Publishing documentation automatically
- Team workflows with multiple contributors

**Usage:**
Automatic on push/PR to main/develop branches

---

### 4. Makefile (Bonus)

**Location:** `Makefile` in project root

**Pros:**
- ✅ Familiar to most developers
- ✅ Short, memorable commands
- ✅ Works as wrapper around other methods
- ✅ Good discoverability with `make help`

**Cons:**
- ❌ Another tool to remember
- ❌ Just calls shell scripts (thin wrapper)

**Best for:**
- Quick access to common commands
- Muscle memory for developers who use make

**Usage:**
```bash
make docs
make docs-serve
```

---

## Which Approach to Use?

### For Daily Development

**Use shell scripts or Makefile:**
```bash
make docs-serve  # Quick and easy
# or
./scripts/build-docs.sh --serve
```

### For C++ Development Workflow

**Use CMake targets:**
```bash
cmake --preset cli-native -DBUILD_DOCS=ON
cmake --build build --target docs
```

### For CI/CD

**GitHub Actions runs automatically**, but you can also use scripts:
```bash
./scripts/check-docs.sh  # Same check as CI
```

### For Documentation-Only Contributors

**Use shell scripts** (no C++ build required):
```bash
./scripts/build-docs.sh --serve
# Edit markdown files
# Refresh browser to see changes
```

---

## Recommendation

**All three approaches are maintained** because they serve different purposes:

1. **Shell Scripts** - Primary method for manual documentation work
2. **CMake Targets** - Secondary method for C++ developers
3. **GitHub Actions** - Automated validation (always enabled)
4. **Makefile** - Convenience wrapper for shell scripts

Choose based on your context:
- **Quick edit?** → Shell scripts or Makefile
- **C++ development?** → CMake targets
- **PR validation?** → GitHub Actions (automatic)

---

## Similar Projects

This multi-approach pattern is used by many C++ projects:

- **LLVM** - CMake targets + shell scripts
- **Boost** - Documentation separate from build
- **Catch2** - CMake targets + ReadTheDocs
- **fmt** - CMake targets + GitHub Actions

The key is that **scripts are always available** even when CMake isn't configured, while **CMake targets integrate** with the main build for those who want it.
