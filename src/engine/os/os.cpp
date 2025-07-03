module;

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#endif
#include <spdlog/spdlog.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module engine.os;

namespace {
	void ErrorCallback(int error_no, const char *description) {
		spdlog::error("[OS] GLFW Error {} : {}", error_no, description);
	}
} // namespace

namespace engine::os {
	bool OS::Initialize() {
		spdlog::info("Initializing OS...");
		glfwSetErrorCallback(ErrorCallback);

		if (!glfwInit()) {
			spdlog::critical("Failed to initialize GLFW");
			return false;
		}

		return true;
	}

	GLFWwindow *OS::CreateWindow(const int window_width, const int window_height, const char *title) {
		GLFWwindow *window = glfwCreateWindow(window_width, window_height, title, nullptr, nullptr);
		if (window == nullptr) {
			spdlog::critical("Failed to create GLFW window");
			glfwTerminate();
			return nullptr;
		}
		glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__
	// NOLINTNEXTLINE
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		spdlog::critical("Failed to initialize GLAD");
		return nullptr;
	}
#endif

#ifndef __EMSCRIPTEN__
	glfwSwapInterval(1);
#endif
		return window;
	}

	void OS::Shutdown(GLFWwindow *window) {
		spdlog::info("Shutting down OS...");
		if (window != nullptr) {
			spdlog::info("Destroying GLFW window...");
			glfwDestroyWindow(window);
		}
		glfwTerminate();
	}

	void OS::SetKeyCallback(GLFWwindow *window, const GLFWkeyfun callback) { glfwSetKeyCallback(window, callback); }

	void OS::SetMouseButtonCallback(GLFWwindow *window, const GLFWmousebuttonfun callback) {
		glfwSetMouseButtonCallback(window, callback);
	}

	void OS::SetFrameBufferSizeCallback(GLFWwindow *window, GLFWframebuffersizefun callback) {
		glfwSetFramebufferSizeCallback(window, callback);
	}

	void OS::MakeContextCurrent(GLFWwindow *window) { glfwMakeContextCurrent(window); }
} // namespace engine::os
