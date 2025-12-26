// ============================================================================
// UI Descriptor System
// ============================================================================
//
// This module re-exports the descriptor sub-modules for backwards compatibility.
// New code should import the specific descriptor modules directly.
//
// See descriptors/descriptors.cppm for documentation on adding new types.
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
