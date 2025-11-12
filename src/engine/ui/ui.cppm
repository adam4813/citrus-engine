export module engine.ui;

// Import and re-export all UI module partitions
export import :uitypes;
export import :uirenderer;
export import :text_renderer;
export import :mouse_event;
export import :ui_element;
export import :mouse_event_manager;

// Re-export batch_renderer as a separate submodule
export import engine.ui.batch_renderer;
