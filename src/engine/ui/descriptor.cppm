// ============================================================================
// UI Descriptor System
// ============================================================================
//
// This module re-exports the descriptor sub-modules for convenient importing.
//
// ## Adding New UI Element Types
//
// 1. Create a new file in `descriptors/` (e.g., `my_widget.cppm`)
// 2. Define the descriptor struct with all properties
// 3. Add `to_json` and `from_json` functions for JSON serialization
// 4. Add the descriptor to `UIDescriptorVariant` and `CompleteUIDescriptor`
//    in `descriptors/container.cppm`
// 5. Add the import to this file
// 6. Add a `Create()` method in `factory.cppm`
// 7. Register the module in `CMakeLists.txt`
//
// See individual descriptor files in `descriptors/` for examples.
//
// ============================================================================

export module engine.ui:descriptor;

// Re-export all descriptor types from the descriptors submodule
export import :descriptors.common;
export import :descriptors.button;
export import :descriptors.panel;
export import :descriptors.label;
export import :descriptors.slider;
export import :descriptors.checkbox;
export import :descriptors.divider;
export import :descriptors.progress_bar;
export import :descriptors.image;
export import :descriptors.container;
