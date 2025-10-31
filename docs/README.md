# Documentation

This directory contains the Citrus Engine documentation built with MkDocs and Doxygen.

## Quick Start

Use the automated scripts or Makefile:

```bash
# Build documentation (Doxygen + MkDocs)
make docs
# or
./scripts/build-docs.sh

# Build and serve locally with auto-reload
make docs-serve
# or
./scripts/build-docs.sh --serve

# Clean build artifacts
make docs-clean
# or
./scripts/build-docs.sh --clean
```

## Building Documentation Manually

If you prefer to build manually:

```bash
# Install dependencies
pip install -r requirements.txt

# Generate Doxygen API documentation
doxygen Doxyfile

# Build MkDocs site
mkdocs build

# Serve locally (with auto-reload)
mkdocs serve
```

## Build Methods

Choose the method that works best for your workflow:

### Method 1: Makefile (Recommended for quick tasks)

```bash
make docs              # Build documentation
make docs-serve        # Build and serve locally
make docs-clean        # Clean build artifacts
```

### Method 2: CMake Targets (Recommended for C++ developers)

```bash
# Configure with documentation enabled
cmake --preset cli-native -DBUILD_DOCS=ON

# Build documentation
cmake --build build --target docs

# Or individual targets:
cmake --build build --target docs-doxygen  # Doxygen only
cmake --build build --target docs-mkdocs   # MkDocs only
cmake --build build --target docs-serve    # Serve locally
cmake --build build --target docs-clean    # Clean artifacts
```

### Method 3: Build Script (Recommended for CI/automation)

```bash
./scripts/build-docs.sh         # Build documentation
./scripts/build-docs.sh --serve # Build and serve
./scripts/build-docs.sh --clean # Clean artifacts
```

### Method 4: Manual Commands

```bash
# Install dependencies
pip install -r requirements.txt

# Generate Doxygen
doxygen Doxyfile

# Build MkDocs
mkdocs build

# Serve locally
mkdocs serve
```

## CI/CD and Deployment

GitHub Actions automatically:
- Builds documentation on every PR to validate changes
- Deploys to GitHub Pages on main branch pushes

Documentation is published at: `https://adam4813.github.io/citrus-engine/`

See `.github/workflows/docs.yml` for details.
