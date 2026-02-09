module;

#include <iostream>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import :framebuffer;

namespace engine::rendering {

Framebuffer::Framebuffer() = default;

Framebuffer::~Framebuffer() { Destroy(); }

Framebuffer::Framebuffer(Framebuffer&& other) noexcept :
		fbo_id_(other.fbo_id_), color_texture_id_(other.color_texture_id_), depth_rbo_id_(other.depth_rbo_id_),
		width_(other.width_), height_(other.height_) {
	other.fbo_id_ = 0;
	other.color_texture_id_ = 0;
	other.depth_rbo_id_ = 0;
	other.width_ = 0;
	other.height_ = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
	if (this != &other) {
		Destroy();
		fbo_id_ = other.fbo_id_;
		color_texture_id_ = other.color_texture_id_;
		depth_rbo_id_ = other.depth_rbo_id_;
		width_ = other.width_;
		height_ = other.height_;
		other.fbo_id_ = 0;
		other.color_texture_id_ = 0;
		other.depth_rbo_id_ = 0;
		other.width_ = 0;
		other.height_ = 0;
	}
	return *this;
}

bool Framebuffer::Create(const uint32_t width, const uint32_t height) {
	if (width == 0 || height == 0) {
		std::cerr << "Framebuffer::Create: Invalid dimensions " << width << "x" << height << std::endl;
		return false;
	}

	// Clean up existing resources if any
	Destroy();

	width_ = width;
	height_ = height;

	// Create framebuffer object
	glGenFramebuffers(1, &fbo_id_);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);

	// Create color texture attachment
	glGenTextures(1, &color_texture_id_);
	glBindTexture(GL_TEXTURE_2D, color_texture_id_);
	glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			static_cast<GLsizei>(width),
			static_cast<GLsizei>(height),
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_id_, 0);

	// Create depth/stencil renderbuffer attachment (GL_DEPTH24_STENCIL8 is WebGL2/GLES3 compatible)
	glGenRenderbuffers(1, &depth_rbo_id_);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_id_);
	glRenderbufferStorage(
			GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rbo_id_);

	// Check framebuffer completeness
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer::Create: Framebuffer incomplete, status: " << status << std::endl;
		Destroy();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	// Unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void Framebuffer::Destroy() {
	if (color_texture_id_ != 0) {
		glDeleteTextures(1, &color_texture_id_);
		color_texture_id_ = 0;
	}
	if (depth_rbo_id_ != 0) {
		glDeleteRenderbuffers(1, &depth_rbo_id_);
		depth_rbo_id_ = 0;
	}
	if (fbo_id_ != 0) {
		glDeleteFramebuffers(1, &fbo_id_);
		fbo_id_ = 0;
	}
	width_ = 0;
	height_ = 0;
}

void Framebuffer::Resize(const uint32_t width, const uint32_t height) {
	if (width == width_ && height == height_) {
		return;
	}
	// Recreate with new dimensions
	Create(width, height);
}

void Framebuffer::Bind() const {
	if (fbo_id_ != 0) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
		glViewport(0, 0, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_));
	}
}

void Framebuffer::Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

uint32_t Framebuffer::GetColorTextureId() const { return color_texture_id_; }

uint32_t Framebuffer::GetWidth() const { return width_; }

uint32_t Framebuffer::GetHeight() const { return height_; }

bool Framebuffer::IsValid() const { return fbo_id_ != 0 && color_texture_id_ != 0; }

} // namespace engine::rendering
