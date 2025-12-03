module;

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

export module engine.os;

export namespace engine::os {
// OS/Platform abstraction interface
class OS {
public:
	[[nodiscard]] static bool Initialize();
	[[nodiscard]] static GLFWwindow* CreateWindow(int window_width, int window_height, const char* title);
	static void Shutdown(GLFWwindow* window = nullptr);

	static void SetKeyCallback(GLFWwindow* window, GLFWkeyfun callback);
	static void SetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback);
	static void SetScrollCallback(GLFWwindow* window, GLFWscrollfun callback);
	static void SetFrameBufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun callback);
	static void MakeContextCurrent(GLFWwindow* window);
};
} // namespace engine::os
