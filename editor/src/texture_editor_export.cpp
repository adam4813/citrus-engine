#include "texture_editor_panel.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace editor {

void TextureEditorPanel::ExportPng(const std::string& path) {
	// Ensure data is up-to-date
	GenerateTextureData();

	const int res = preview_resolution_;
	stbi_write_png(path.c_str(), res, res, 4, preview_pixels_.data(), res * 4);
}

} // namespace editor
