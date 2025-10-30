# Citrus Engine Documentation

This directory contains the user-facing documentation for Citrus Engine.

## Building Documentation Locally

### Prerequisites

Install Python dependencies:

```bash
pip install -r requirements.txt
```

Install Doxygen:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows
# Download from https://www.doxygen.nl/download.html
```

### Build HTML Documentation

```bash
# From the docs directory
cd docs

# Build with Sphinx
sphinx-build -b html . _build/html

# Or use make (if available)
make html
```

### View Documentation

Open `_build/html/index.html` in your browser.

### Build PDF

```bash
sphinx-build -b latex . _build/latex
cd _build/latex
make
```

## Documentation Structure

```
docs/
├── index.rst                  # Main documentation index
├── getting-started/           # Installation and first project
├── guides/                    # Feature guides and tutorials
├── api/                       # API reference (Doxygen-generated)
├── advanced/                  # Advanced topics
├── conf.py                    # Sphinx configuration
└── requirements.txt           # Python dependencies
```

## ReadTheDocs

This documentation is automatically built and published to ReadTheDocs when changes are pushed to the repository.

Configuration: `.readthedocs.yml` in the repository root.

## Contributing

When adding documentation:

1. Follow the existing structure
2. Use reStructuredText (.rst) format
3. Include code examples
4. Test builds locally before committing
5. Update this README if adding new sections

## Documentation Guidelines

- **Getting Started**: Installation, setup, first project
- **Guides**: How to use specific features
- **API Reference**: Auto-generated from code (Doxygen)
- **Advanced**: Performance, platform-specific, extending

For more details, see `AGENTS.md` in the repository root.
