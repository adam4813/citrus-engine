# Documentation

This directory contains the Citrus Engine documentation built with MkDocs.

## Building Documentation Locally

```bash
# Install dependencies
pip install -r requirements.txt

# Build documentation
mkdocs build

# Serve locally (with auto-reload)
mkdocs serve
```

## Updating Dependencies

Dependencies are managed using `pip-tools` for reproducible builds:

```bash
# Install pip-tools
pip install pip-tools

# Update requirements.in with new dependencies or version constraints
# Then compile to generate requirements.txt with pinned versions
pip-compile docs/requirements.in

# Or to upgrade all dependencies to latest compatible versions
pip-compile --upgrade docs/requirements.in
```

The `requirements.in` file contains high-level dependencies, while `requirements.txt` is auto-generated with all transitive dependencies pinned to specific versions for reproducibility.

## Adding New Dependencies

1. Add the dependency to `requirements.in`
2. Run `pip-compile docs/requirements.in`
3. Commit both `requirements.in` and `requirements.txt`
