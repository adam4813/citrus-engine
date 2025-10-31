# Example Assets

This directory contains minimal sample assets for the Citrus Engine examples.

## Directory Structure

```
assets/
├── textures/     # Sample textures and images
├── shaders/      # Sample GLSL shaders
├── models/       # Sample 3D models
└── fonts/        # Sample fonts
```

## Asset Conventions

- **Textures**: PNG format preferred for web compatibility
- **Shaders**: OpenGL ES 2.0 / WebGL compatible GLSL
- **Models**: Simple formats (OBJ) for ease of loading
- **Fonts**: TTF format

## Adding Assets

When adding new assets for examples:

1. Keep file sizes small (examples should load quickly)
2. Use web-compatible formats
3. Ensure assets work on both native and WASM builds
4. Document any special requirements in the example code

## Asset Loading

Assets are loaded from:
- **Native builds**: `./assets/` relative to executable
- **WASM builds**: `/assets/` in the Emscripten virtual filesystem (preloaded)
