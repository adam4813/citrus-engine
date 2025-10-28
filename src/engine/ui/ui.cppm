export module engine.ui;

// Import and re-export all UI module partitions
export import :uitypes;
export import :uirenderer;

// Re-export batch_renderer as a separate submodule
export import engine.ui.batch_renderer;
