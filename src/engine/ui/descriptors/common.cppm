module;

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.common;

import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

// ============================================================================
// Common Types for UI Descriptors
// ============================================================================

/**
 * @brief Position descriptor for UI elements
 */
struct Position {
	float x{0.0f};
	float y{0.0f};
};

/**
 * @brief Size descriptor for UI elements
 */
struct Size {
	float width{0.0f};
	float height{0.0f};
};

/**
 * @brief Bounds descriptor combining position and size
 */
struct Bounds {
	float x{0.0f};
	float y{0.0f};
	float width{100.0f};
	float height{100.0f};

	static Bounds From(const Position& pos, const Size& size) {
		return {pos.x, pos.y, size.width, size.height};
	}

	static Bounds WithSize(const float w, const float h) { return {0.0f, 0.0f, w, h}; }
};

/**
 * @brief Border styling descriptor
 */
struct BorderStyle {
	float width{0.0f};
	batch_renderer::Color color{batch_renderer::Colors::LIGHT_GRAY};
};

/**
 * @brief Text styling descriptor
 */
struct TextStyle {
	float font_size{16.0f};
	batch_renderer::Color color{batch_renderer::Colors::WHITE};
};

/**
 * @brief UV coordinates for texture sampling
 */
struct UVCoords {
	float u0{0.0f};
	float v0{0.0f};
	float u1{1.0f};
	float v1{1.0f};
};

// ============================================================================
// JSON Serialization Helpers
// ============================================================================

namespace detail {

inline nlohmann::json color_to_json(const batch_renderer::Color& c) {
	return nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
}

inline batch_renderer::Color color_from_json(const nlohmann::json& j) {
	return batch_renderer::Color{
		j.value("r", 1.0f),
		j.value("g", 1.0f),
		j.value("b", 1.0f),
		j.value("a", 1.0f)
	};
}

inline nlohmann::json bounds_to_json(const Bounds& b) {
	return nlohmann::json{{"x", b.x}, {"y", b.y}, {"width", b.width}, {"height", b.height}};
}

inline Bounds bounds_from_json(const nlohmann::json& j) {
	return Bounds{
		j.value("x", 0.0f),
		j.value("y", 0.0f),
		j.value("width", 100.0f),
		j.value("height", 100.0f)
	};
}

inline nlohmann::json border_to_json(const BorderStyle& b) {
	return nlohmann::json{{"width", b.width}, {"color", color_to_json(b.color)}};
}

inline BorderStyle border_from_json(const nlohmann::json& j) {
	BorderStyle b;
	b.width = j.value("width", 0.0f);
	if (j.contains("color")) {
		b.color = color_from_json(j["color"]);
	}
	return b;
}

inline nlohmann::json textstyle_to_json(const TextStyle& t) {
	return nlohmann::json{{"font_size", t.font_size}, {"color", color_to_json(t.color)}};
}

inline TextStyle textstyle_from_json(const nlohmann::json& j) {
	TextStyle t;
	t.font_size = j.value("font_size", 16.0f);
	if (j.contains("color")) {
		t.color = color_from_json(j["color"]);
	}
	return t;
}

inline nlohmann::json uv_to_json(const UVCoords& uv) {
	return nlohmann::json{{"u0", uv.u0}, {"v0", uv.v0}, {"u1", uv.u1}, {"v1", uv.v1}};
}

inline UVCoords uv_from_json(const nlohmann::json& j) {
	return UVCoords{
		j.value("u0", 0.0f),
		j.value("v0", 0.0f),
		j.value("u1", 1.0f),
		j.value("v1", 1.0f)
	};
}

} // namespace detail

// --- ADL functions for common types ---

inline void to_json(nlohmann::json& j, const Bounds& b) {
	j = detail::bounds_to_json(b);
}

inline void from_json(const nlohmann::json& j, Bounds& b) {
	b = detail::bounds_from_json(j);
}

inline void to_json(nlohmann::json& j, const BorderStyle& b) {
	j = detail::border_to_json(b);
}

inline void from_json(const nlohmann::json& j, BorderStyle& b) {
	b = detail::border_from_json(j);
}

inline void to_json(nlohmann::json& j, const TextStyle& t) {
	j = detail::textstyle_to_json(t);
}

inline void from_json(const nlohmann::json& j, TextStyle& t) {
	t = detail::textstyle_from_json(j);
}

inline void to_json(nlohmann::json& j, const UVCoords& uv) {
	j = detail::uv_to_json(uv);
}

inline void from_json(const nlohmann::json& j, UVCoords& uv) {
	uv = detail::uv_from_json(j);
}

} // namespace engine::ui::descriptor
