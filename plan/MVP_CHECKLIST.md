# MVP Implementation Checklist

## Core Systems
- [x] engine.platform (cross-platform abstraction)
- [x] engine.ecs (entity component system)
- [x] engine.rendering (OpenGL/WebGL pipeline)
- [ ] engine.scene (transform hierarchies)  <!-- TODO: Minimal or missing -->

## Additional MVP Components
- [x] Simplified Input (basic keyboard input, arrows + WASD)
- [x] Basic Asset Loading (texture loading with stb_image)
- [x] Hardcoded Geometry (cube mesh data in code)

## Demo Specification
- [x] 10 textured cubes in 3D space
- [x] Independent cube rotation via input
- [x] Perspective camera system
- [x] Camera movement (inverse world transform)
- [x] Textured cube rendering
- [x] Perspective projection matrix
- [x] Basic texture mapping
- [x] Z-buffer depth testing
- [ ] 2D HUD overlay (sprites, normalized coordinates)  <!-- TODO: Not implemented -->
- [ ] Separate shader pipeline for HUD  <!-- TODO: Not implemented -->
- [ ] Orthographic projection for HUD  <!-- TODO: Not implemented -->
- [ ] Alpha blending for HUD  <!-- TODO: Not implemented -->

## Input System
- [x] Polling-based input system (keyboard only, GLFW-based, thread-safe, custom keycode enum)
- [ ] Input module registered in engine core  <!-- TODO: No engine core, add when available -->
- [x] Demo logs key press events in main.cpp
- [x] Polling/events API: up/down/repeat, just-pressed state, frame-coherent

## Platform Requirements
- [x] Windows native build (OpenGL 3.3+, GLFW)
- [x] WebAssembly build (WebGL 2.0, Emscripten)

## Error Handling
- [x] Graceful error handling (texture, shader, input failures)
- [x] Critical assertions (context, shader, memory, platform init)

## Success Criteria
- [x] 10 cubes visible and rotating
- [ ] HUD overlays correctly  <!-- TODO: Not implemented -->
- [x] Identical output on Windows/WASM
- [x] Smooth real-time input
- [ ] 60 FPS performance  <!-- TODO: Render stats/FPS measurement -->
- [x] Error-free compilation
- [x] Clean shutdown, resource cleanup

## Development Workflow
- [ ] Simple build process (CMake)  <!-- TODO: Documentation could be improved -->
- [ ] WASM build  <!-- TODO: Documentation could be improved -->
- [x] Asset changes reflected after rebuild
- [x] Clear error messages