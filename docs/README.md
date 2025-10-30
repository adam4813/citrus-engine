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

# Check documentation for issues
make docs-check
# or
./scripts/check-docs.sh
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

## Updating Dependencies

Dependencies are managed using `pip-tools` for reproducible builds:

```bash
# Update dependencies (preserving versions where possible)
make update-deps
# or
./scripts/update-deps.sh

# Upgrade all dependencies to latest compatible versions
make update-deps-upgrade
# or
./scripts/update-deps.sh --upgrade
```

The `requirements.in` file contains high-level dependencies, while `requirements.txt` is auto-generated with all transitive dependencies pinned to specific versions for reproducibility.

## Adding New Dependencies

1. Add the dependency to `requirements.in`
2. Run `make update-deps` or `./scripts/update-deps.sh`
3. Test the build: `make docs`
4. Commit both `requirements.in` and `requirements.txt`

## Build Methods

Choose the method that works best for your workflow:

### Method 1: Makefile (Recommended for quick tasks)

```bash
make docs              # Build documentation
make docs-serve        # Build and serve locally
make docs-check        # Check for issues
make update-deps       # Update dependencies
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
cmake --build build --target docs-check    # Check for issues
```

### Method 3: Scripts (Recommended for CI/automation)

```bash
./scripts/build-docs.sh         # Build documentation
./scripts/build-docs.sh --serve # Build and serve
./scripts/check-docs.sh         # Check for issues
./scripts/update-deps.sh        # Update dependencies
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

## Available Scripts

- **`scripts/build-docs.sh`** - Build documentation with Doxygen and MkDocs
  - `--clean` - Clean build directories first
  - `--serve` - Serve locally after building
  
- **`scripts/update-deps.sh`** - Update Python dependencies
  - `--upgrade` - Upgrade to latest compatible versions
  
- **`scripts/check-docs.sh`** - Check documentation for issues
  - Validates builds succeed
  - Checks for broken links
  - Verifies Doxygen output
  - Checks dependency freshness

## CI/CD

GitHub Actions automatically checks documentation on every PR:
- Validates documentation builds successfully
- Checks for broken links and issues
- Uploads built documentation as artifacts (on main branch)

See `.github/workflows/docs.yml` for details.
