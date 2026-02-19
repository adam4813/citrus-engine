#include "texture_editor_panel.h"

#include <algorithm>
#include <cmath>
#include <variant>

#include <stb_image.h>

namespace editor {

// ============================================================================
// Texture Generation — Full Node Graph Evaluation
// ============================================================================

// Simple hash-based Perlin noise implementation
namespace {

float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

float Grad(int hash, float x, float y) {
	const int h = hash & 3;
	const float u = h < 2 ? x : y;
	const float v = h < 2 ? y : x;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Permutation table
const int kPerm[512] = {
		151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
		69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,
		94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
		171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
		60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161,
		1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
		164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
		255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
		119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
		19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193,
		238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
		181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
		222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180,
		// repeat
		151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
		69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,
		94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
		171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
		60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161,
		1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
		164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
		255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
		119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
		19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193,
		238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
		181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
		222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180};

float PerlinNoise2D(float x, float y) {
	const int xi = static_cast<int>(std::floor(x)) & 255;
	const int yi = static_cast<int>(std::floor(y)) & 255;
	const float xf = x - std::floor(x);
	const float yf = y - std::floor(y);

	const float u = Fade(xf);
	const float v = Fade(yf);

	const int aa = kPerm[kPerm[xi] + yi];
	const int ab = kPerm[kPerm[xi] + yi + 1];
	const int ba = kPerm[kPerm[xi + 1] + yi];
	const int bb = kPerm[kPerm[xi + 1] + yi + 1];

	const float x1 = std::lerp(Grad(aa, xf, yf), Grad(ba, xf - 1.0f, yf), u);
	const float x2 = std::lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);
	return (std::lerp(x1, x2, v) + 1.0f) * 0.5f; // Map to [0, 1]
}

float FractionalBrownianMotion(float x, float y, int octaves) {
	float value = 0.0f;
	float amplitude = 0.5f;
	float frequency = 1.0f;
	for (int i = 0; i < octaves; ++i) {
		value += amplitude * PerlinNoise2D(x * frequency, y * frequency);
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}
	return value;
}

// Simple hash for Voronoi cell points
glm::vec2 VoronoiRandomPoint(int ix, int iy, float randomness) {
	const int n = ix * 374761393 + iy * 668265263;
	const int hash = (n ^ (n >> 13)) * 1274126177;
	const float fx = static_cast<float>((hash & 0xFFFF)) / 65535.0f;
	const float fy = static_cast<float>(((hash >> 16) & 0xFFFF)) / 65535.0f;
	return glm::vec2(
			static_cast<float>(ix) + 0.5f + (fx - 0.5f) * randomness,
			static_cast<float>(iy) + 0.5f + (fy - 0.5f) * randomness);
}

float VoronoiNoise(float x, float y, float randomness) {
	const int cell_x = static_cast<int>(std::floor(x));
	const int cell_y = static_cast<int>(std::floor(y));

	float min_dist = 1e10f;
	for (int dy = -1; dy <= 1; ++dy) {
		for (int dx = -1; dx <= 1; ++dx) {
			const glm::vec2 pt = VoronoiRandomPoint(cell_x + dx, cell_y + dy, randomness);
			const float dist = glm::length(glm::vec2(x, y) - pt);
			min_dist = std::min(min_dist, dist);
		}
	}
	return std::clamp(min_dist, 0.0f, 1.0f);
}

// HSV <-> RGB conversion helpers
glm::vec3 RgbToHsv(glm::vec3 rgb) {
	const float cmax = std::max({rgb.r, rgb.g, rgb.b});
	const float cmin = std::min({rgb.r, rgb.g, rgb.b});
	const float delta = cmax - cmin;
	float h = 0.0f;
	if (delta > 0.0001f) {
		if (cmax == rgb.r) {
			h = std::fmod((rgb.g - rgb.b) / delta, 6.0f);
		}
		else if (cmax == rgb.g) {
			h = (rgb.b - rgb.r) / delta + 2.0f;
		}
		else {
			h = (rgb.r - rgb.g) / delta + 4.0f;
		}
		h /= 6.0f;
		if (h < 0.0f) {
			h += 1.0f;
		}
	}
	const float s = cmax > 0.0001f ? delta / cmax : 0.0f;
	return {h, s, cmax};
}

glm::vec3 HsvToRgb(glm::vec3 hsv) {
	const float h = hsv.x * 6.0f;
	const float s = hsv.y;
	const float v = hsv.z;
	const float c = v * s;
	const float x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
	const float m = v - c;
	glm::vec3 rgb;
	if (h < 1.0f) {
		rgb = {c, x, 0.0f};
	}
	else if (h < 2.0f) {
		rgb = {x, c, 0.0f};
	}
	else if (h < 3.0f) {
		rgb = {0.0f, c, x};
	}
	else if (h < 4.0f) {
		rgb = {0.0f, x, c};
	}
	else if (h < 5.0f) {
		rgb = {x, 0.0f, c};
	}
	else {
		rgb = {c, 0.0f, x};
	}
	return rgb + glm::vec3(m);
}

float OverlayBlendChannel(float a, float b) { return a < 0.5f ? 2.0f * a * b : 1.0f - 2.0f * (1.0f - a) * (1.0f - b); }

} // anonymous namespace

// Helper: extract a float from a PinValue
static float PinValueToFloat(const engine::graph::PinValue& val) {
	if (auto* f = std::get_if<float>(&val)) {
		return *f;
	}
	if (auto* i = std::get_if<int>(&val)) {
		return static_cast<float>(*i);
	}
	if (auto* v4 = std::get_if<glm::vec4>(&val)) {
		return v4->r;
	}
	return 0.0f;
}

static glm::vec4 PinValueToColor(const engine::graph::PinValue& val) {
	if (auto* v4 = std::get_if<glm::vec4>(&val)) {
		return *v4;
	}
	if (auto* f = std::get_if<float>(&val)) {
		return glm::vec4(*f, *f, *f, 1.0f);
	}
	return glm::vec4(1.0f);
}

static glm::vec2 PinValueToVec2(const engine::graph::PinValue& val) {
	if (auto* v2 = std::get_if<glm::vec2>(&val)) {
		return *v2;
	}
	if (auto* f = std::get_if<float>(&val)) {
		return glm::vec2(*f);
	}
	return glm::vec2(0.0f);
}

// Find which node/pin feeds into a given input pin
static const engine::graph::Link* FindInputLink(
		const engine::graph::NodeGraph& graph, int node_id, int pin_index) {
	for (const auto& link : graph.GetLinks()) {
		if (link.to_node_id == node_id && link.to_pin_index == pin_index) {
			return &link;
		}
	}
	return nullptr;
}

float TextureEditorPanel::GetInputFloat(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const {
	if (const auto* link = FindInputLink(*texture_graph_, node.id, pin_index)) {
		const glm::vec4 result = EvaluateNodeOutput(link->from_node_id, link->from_pin_index, uv);
		return result.r;
	}
	if (pin_index < static_cast<int>(node.inputs.size())) {
		return PinValueToFloat(node.inputs[pin_index].default_value);
	}
	return 0.0f;
}

glm::vec4 TextureEditorPanel::GetInputColor(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const {
	if (const auto* link = FindInputLink(*texture_graph_, node.id, pin_index)) {
		return EvaluateNodeOutput(link->from_node_id, link->from_pin_index, uv);
	}
	if (pin_index < static_cast<int>(node.inputs.size())) {
		return PinValueToColor(node.inputs[pin_index].default_value);
	}
	return glm::vec4(1.0f);
}

glm::vec2 TextureEditorPanel::GetInputVec2(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const {
	if (const auto* link = FindInputLink(*texture_graph_, node.id, pin_index)) {
		const glm::vec4 result = EvaluateNodeOutput(link->from_node_id, link->from_pin_index, uv);
		return glm::vec2(result.x, result.y);
	}
	if (pin_index < static_cast<int>(node.inputs.size())) {
		return PinValueToVec2(node.inputs[pin_index].default_value);
	}
	return uv; // Default UV passthrough
}

glm::vec4 TextureEditorPanel::EvaluateNodeOutput(int node_id, int pin_index, glm::vec2 uv) const {
	const auto* node = texture_graph_->GetNode(node_id);
	if (!node) {
		return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta = error
	}

	const auto& type = node->type_name;

	// --- Generators ---
	if (type == "Perlin Noise") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		// Default UV if nothing connected and default is zero
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		float scale = GetInputFloat(*node, 1, uv);
		if (scale <= 0.0f) {
			scale = 4.0f;
		}
		float octaves_f = GetInputFloat(*node, 2, uv);
		int octaves = octaves_f > 0.0f ? static_cast<int>(octaves_f) : 4;
		octaves = std::clamp(octaves, 1, 8);
		const float val = FractionalBrownianMotion(sample_uv.x * scale, sample_uv.y * scale, octaves);
		return glm::vec4(val, val, val, 1.0f);
	}

	if (type == "Checkerboard") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		float scale = GetInputFloat(*node, 1, uv);
		if (scale <= 0.0f) {
			scale = 8.0f;
		}
		const int cx = static_cast<int>(std::floor(sample_uv.x * scale));
		const int cy = static_cast<int>(std::floor(sample_uv.y * scale));
		const float val = ((cx + cy) % 2 == 0) ? 1.0f : 0.0f;
		return glm::vec4(val, val, val, 1.0f);
	}

	if (type == "Gradient") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		const glm::vec4 color_a = GetInputColor(*node, 1, uv);
		const glm::vec4 color_b = GetInputColor(*node, 2, uv);
		const float t = std::clamp(sample_uv.x, 0.0f, 1.0f);
		return glm::mix(color_a, color_b, t);
	}

	if (type == "Solid Color") {
		return GetInputColor(*node, 0, uv);
	}

	if (type == "Voronoi") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		float scale = GetInputFloat(*node, 1, uv);
		if (scale <= 0.0f) {
			scale = 4.0f;
		}
		float randomness = GetInputFloat(*node, 2, uv);
		if (randomness <= 0.0f) {
			randomness = 1.0f;
		}
		const float val = VoronoiNoise(sample_uv.x * scale, sample_uv.y * scale, randomness);
		return glm::vec4(val, val, val, 1.0f);
	}

	if (type == "Texture Sample") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}

		// Get the Path pin value
		std::string path;
		for (const auto& pin : node->inputs) {
			if (pin.name == "Path") {
				if (const auto* p = std::get_if<std::string>(&pin.default_value)) {
					path = *p;
				}
				break;
			}
		}
		if (path.empty()) {
			return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Grey = no path set
		}

		// Load into cache on first access
		auto it = sampler_cache_.find(path);
		if (it == sampler_cache_.end()) {
			int w = 0, h = 0, ch = 0;
			stbi_set_flip_vertically_on_load(false);
			unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
			SamplerEntry entry;
			if (data) {
				entry.width = w;
				entry.height = h;
				entry.pixels.assign(data, data + w * h * 4);
				stbi_image_free(data);
			}
			it = sampler_cache_.emplace(path, std::move(entry)).first;
		}

		const auto& entry = it->second;
		if (entry.width <= 0 || entry.height <= 0) {
			return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta = load failed
		}

		// Nearest-neighbour sample with wrapping
		const float u_w = sample_uv.x - std::floor(sample_uv.x);
		const float v_w = sample_uv.y - std::floor(sample_uv.y);
		const int px = std::clamp(static_cast<int>(u_w * static_cast<float>(entry.width)), 0, entry.width - 1);
		const int py = std::clamp(static_cast<int>(v_w * static_cast<float>(entry.height)), 0, entry.height - 1);
		const int idx = (py * entry.width + px) * 4;
		return glm::vec4(
				entry.pixels[idx + 0] / 255.0f,
				entry.pixels[idx + 1] / 255.0f,
				entry.pixels[idx + 2] / 255.0f,
				entry.pixels[idx + 3] / 255.0f);
	}

	// --- Math ---
	if (type == "Add") {
		const float a = GetInputFloat(*node, 0, uv);
		const float b = GetInputFloat(*node, 1, uv);
		const float r = a + b;
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Multiply") {
		const float a = GetInputFloat(*node, 0, uv);
		const float b = GetInputFloat(*node, 1, uv);
		const float r = a * b;
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Lerp") {
		const float a = GetInputFloat(*node, 0, uv);
		const float b = GetInputFloat(*node, 1, uv);
		const float t = std::clamp(GetInputFloat(*node, 2, uv), 0.0f, 1.0f);
		const float r = std::lerp(a, b, t);
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Clamp") {
		const float val = GetInputFloat(*node, 0, uv);
		const float lo = GetInputFloat(*node, 1, uv);
		float hi = GetInputFloat(*node, 2, uv);
		if (hi <= lo) {
			hi = 1.0f;
		}
		const float r = std::clamp(val, lo, hi);
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Remap") {
		const float val = GetInputFloat(*node, 0, uv);
		const float in_min = GetInputFloat(*node, 1, uv);
		float in_max = GetInputFloat(*node, 2, uv);
		const float out_min = GetInputFloat(*node, 3, uv);
		const float out_max = GetInputFloat(*node, 4, uv);
		if (std::abs(in_max - in_min) < 0.0001f) {
			in_max = in_min + 1.0f;
		}
		const float t = (val - in_min) / (in_max - in_min);
		const float r = out_min + t * (out_max - out_min);
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Power") {
		const float base = std::max(0.0f, GetInputFloat(*node, 0, uv));
		float exponent = GetInputFloat(*node, 1, uv);
		if (exponent == 0.0f) {
			exponent = 1.0f;
		}
		const float r = std::pow(base, exponent);
		return glm::vec4(r, r, r, 1.0f);
	}

	// --- Filters ---
	if (type == "Invert") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		return glm::vec4(1.0f - input.r, 1.0f - input.g, 1.0f - input.b, input.a);
	}

	if (type == "Levels") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		float lo = GetInputFloat(*node, 1, uv);
		float hi = GetInputFloat(*node, 2, uv);
		float gamma = GetInputFloat(*node, 3, uv);
		if (hi <= lo) {
			lo = 0.0f;
			hi = 1.0f;
		}
		if (gamma <= 0.0f) {
			gamma = 1.0f;
		}
		auto apply = [&](float v) {
			v = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
			return std::pow(v, 1.0f / gamma);
		};
		return glm::vec4(apply(input.r), apply(input.g), apply(input.b), input.a);
	}

	if (type == "Blur") {
		// Approximate blur by sampling neighbors
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		float radius = GetInputFloat(*node, 1, uv);
		if (radius <= 0.0f) {
			return input;
		}
		const float step = radius / static_cast<float>(preview_resolution_);
		glm::vec4 accum(0.0f);
		int count = 0;
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const glm::vec2 offset_uv = uv + glm::vec2(static_cast<float>(dx), static_cast<float>(dy)) * step;
				accum += GetInputColor(*node, 0, offset_uv);
				++count;
			}
		}
		return accum / static_cast<float>(count);
	}

	// --- Color ---
	if (type == "HSV Adjust") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		const float h_offset = GetInputFloat(*node, 1, uv);
		const float s_offset = GetInputFloat(*node, 2, uv);
		const float v_offset = GetInputFloat(*node, 3, uv);
		glm::vec3 hsv = RgbToHsv(glm::vec3(input));
		hsv.x = std::fmod(hsv.x + h_offset, 1.0f);
		if (hsv.x < 0.0f) {
			hsv.x += 1.0f;
		}
		hsv.y = std::clamp(hsv.y + s_offset, 0.0f, 1.0f);
		hsv.z = std::clamp(hsv.z + v_offset, 0.0f, 1.0f);
		const glm::vec3 rgb = HsvToRgb(hsv);
		return glm::vec4(rgb, input.a);
	}

	if (type == "Channel Split") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		// Return the requested channel based on pin_index
		if (pin_index == 0) {
			return glm::vec4(input.r, input.r, input.r, 1.0f);
		}
		if (pin_index == 1) {
			return glm::vec4(input.g, input.g, input.g, 1.0f);
		}
		if (pin_index == 2) {
			return glm::vec4(input.b, input.b, input.b, 1.0f);
		}
		return glm::vec4(input.a, input.a, input.a, 1.0f);
	}

	if (type == "Channel Merge") {
		const float r = GetInputFloat(*node, 0, uv);
		const float g = GetInputFloat(*node, 1, uv);
		const float b = GetInputFloat(*node, 2, uv);
		const float a = GetInputFloat(*node, 3, uv);
		return glm::vec4(r, g, b, a > 0.0f ? a : 1.0f);
	}

	if (type == "Colorize") {
		const float val = std::clamp(GetInputFloat(*node, 0, uv), 0.0f, 1.0f);
		const glm::vec4 color = GetInputColor(*node, 1, uv);
		return color * val;
	}

	// --- Blend ---
	if (type == "Blend Multiply") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::vec4(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
	}

	if (type == "Blend Screen") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::vec4(
				1.0f - (1.0f - a.r) * (1.0f - b.r),
				1.0f - (1.0f - a.g) * (1.0f - b.g),
				1.0f - (1.0f - a.b) * (1.0f - b.b),
				a.a);
	}

	if (type == "Blend Overlay") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::vec4(
				OverlayBlendChannel(a.r, b.r),
				OverlayBlendChannel(a.g, b.g),
				OverlayBlendChannel(a.b, b.b),
				a.a);
	}

	if (type == "Blend Add") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::clamp(a + b, glm::vec4(0.0f), glm::vec4(1.0f));
	}

	// --- Output ---
	if (type == "Texture Output") {
		return GetInputColor(*node, 0, uv);
	}

	// Unknown node type
	return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
}

void TextureEditorPanel::GenerateTextureData() {
	const int res = preview_resolution_;
	preview_pixels_.resize(res * res * 4);

	// Find the output node
	int output_node_id = -1;
	for (const auto& node : texture_graph_->GetNodes()) {
		if (node.type_name == "Texture Output") {
			output_node_id = node.id;
			break;
		}
	}

	for (int y = 0; y < res; ++y) {
		for (int x = 0; x < res; ++x) {
			const glm::vec2 uv(
					static_cast<float>(x) / static_cast<float>(res - 1),
					static_cast<float>(y) / static_cast<float>(res - 1));

			glm::vec4 color;
			if (output_node_id >= 0) {
				color = EvaluateNodeOutput(output_node_id, 0, uv);
			}
			else {
				color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
			}

			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			const int idx = (y * res + x) * 4;
			preview_pixels_[idx + 0] = static_cast<unsigned char>(color.r * 255.0f);
			preview_pixels_[idx + 1] = static_cast<unsigned char>(color.g * 255.0f);
			preview_pixels_[idx + 2] = static_cast<unsigned char>(color.b * 255.0f);
			preview_pixels_[idx + 3] = static_cast<unsigned char>(color.a * 255.0f);
		}
	}

	// Set preview_color_ to the center pixel for the status display
	const glm::vec2 center_uv(0.5f, 0.5f);
	if (output_node_id >= 0) {
		preview_color_ = EvaluateNodeOutput(output_node_id, 0, center_uv);
	}
	else {
		preview_color_ = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
	}
}

void TextureEditorPanel::UploadPreviewTexture() {
	if (preview_pixels_.empty()) {
		return;
	}

	const int res = preview_resolution_;

	if (preview_texture_id_ == 0) {
		glGenTextures(1, &preview_texture_id_);
	}

	glBindTexture(GL_TEXTURE_2D, preview_texture_id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res, res, 0, GL_RGBA, GL_UNSIGNED_BYTE, preview_pixels_.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace editor
