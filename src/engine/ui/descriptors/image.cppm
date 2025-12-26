module;

#include <cstdint>
#include <optional>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.image;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Image elements
 *
 * @code
 * auto desc = ImageDescriptor{
 *     .bounds = {10, 10, 64, 64},
 *     .texture_id = myTexture.GetId(),
 *     .tint = Colors::WHITE
 * };
 * @endcode
 */
struct ImageDescriptor {
	/// Bounds (position and size)
	Bounds bounds;

	/// Texture ID to display (0 = no texture)
	uint32_t texture_id{0};

	/// Tint color
	batch_renderer::Color tint{batch_renderer::Colors::WHITE};

	/// UV coordinates (optional, for atlas textures)
	std::optional<UVCoords> uv_coords;

	/// Whether image is initially visible
	bool visible{true};
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const ImageDescriptor& d) {
	j = nlohmann::json{
		{"type", "image"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"texture_id", d.texture_id},
		{"tint", detail::color_to_json(d.tint)},
		{"visible", d.visible}
	};
	if (d.uv_coords) {
		j["uv_coords"] = detail::uv_to_json(*d.uv_coords);
	}
}

inline void from_json(const nlohmann::json& j, ImageDescriptor& d) {
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	d.texture_id = j.value("texture_id", 0u);
	if (j.contains("tint")) {
		d.tint = detail::color_from_json(j["tint"]);
	}
	if (j.contains("uv_coords")) {
		d.uv_coords = detail::uv_from_json(j["uv_coords"]);
	}
	d.visible = j.value("visible", true);
}

} // namespace engine::ui::descriptor
