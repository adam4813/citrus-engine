module;

#include <cstdint>

module engine.graph;

import :types;

namespace engine::graph {

bool AreTypesCompatible(PinType from, PinType to) {
	// Exact match
	if (from == to) {
		return true;
	}

	// Flow pins can only connect to Flow (even Any can't accept Flow)
	if (from == PinType::Flow || to == PinType::Flow) {
		return false;
	}

	// Any accepts/produces anything (except Flow, checked above)
	if (from == PinType::Any || to == PinType::Any) {
		return true;
	}

	// Color <-> Vec4 (colors are stored as Vec4)
	if ((from == PinType::Color && to == PinType::Vec4) || (from == PinType::Vec4 && to == PinType::Color)) {
		return true;
	}

	// Float can broadcast to Vec2/Vec3/Vec4
	if (from == PinType::Float) {
		return to == PinType::Vec2 || to == PinType::Vec3 || to == PinType::Vec4 || to == PinType::Color;
	}

	// Vec2 can extend to Vec3/Vec4 (z, w = 0)
	if (from == PinType::Vec2) {
		return to == PinType::Vec3 || to == PinType::Vec4;
	}

	// Vec3 can extend to Vec4 (w = 1 for positions, 0 for vectors)
	if (from == PinType::Vec3) {
		return to == PinType::Vec4;
	}

	// Int can promote to Float
	if (from == PinType::Int && to == PinType::Float) {
		return true;
	}

	// Bool can convert to Int (0/1)
	if (from == PinType::Bool && to == PinType::Int) {
		return true;
	}

	return false;
}

} // namespace engine::graph
