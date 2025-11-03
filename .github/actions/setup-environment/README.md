# Setup Citrus Engine Environment Action

This composite action installs all dependencies needed to build Citrus Engine, including:
- System dependencies (Linux only: X11, OpenGL libraries, build tools)
- C++ compiler (Clang, GCC, MSVC, or Emscripten)
- vcpkg package manager

## Usage

```yaml
- name: Setup Citrus Engine Environment
  uses: ./.github/actions/setup-environment
  with:
    compiler: 'clang'
    clang-version: '18'  # Optional, only for clang
```

## Inputs

| Input | Description | Required | Default |
|-------|-------------|----------|---------|
| `compiler` | Compiler to use (clang, gcc, msvc, emscripten) | Yes | - |
| `clang-version` | Clang version number (if using clang) | No | `18` |
| `vcpkg-commit` | vcpkg Git commit ID to use | No | `dd3097e305afa53f7b4312371f62058d2e665320` |

## Examples

### Linux with Clang-18
```yaml
- uses: ./.github/actions/setup-environment
  with:
    compiler: clang
    clang-version: 18
```

### Windows with MSVC
```yaml
- uses: ./.github/actions/setup-environment
  with:
    compiler: msvc
```

### Emscripten for WebAssembly
```yaml
- uses: ./.github/actions/setup-environment
  with:
    compiler: emscripten
```

## What it does

1. **Linux System Dependencies**: Installs X11 development libraries, OpenGL, and build tools via apt-get
2. **Compiler Setup**: Configures the specified compiler using `aminya/setup-cpp` action
3. **Emscripten**: Sets up Emscripten SDK if specified
4. **vcpkg**: Installs and caches vcpkg for dependency management

## Related Files

- `.github/copilot-setup.sh` - Shell script for Copilot environment setup (runs automatically)
- `.github/workflows/build.yml` - Main build workflow using this action
- `.github/workflows/build-examples.yml` - Examples build workflow using this action
