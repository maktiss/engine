#pragma once

#include "SystemBase.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace Engine {
class ImGuiSystem : public SystemBase {
private:
	GLFWwindow* glfwWindow {};


public:
	inline void setWindow(GLFWwindow*& pGLFWWindow) {
		glfwWindow = pGLFWWindow;
	}

	int init() override;
	int run(double dt) override;

	void showLogWindow();
	void showStatisticsWindow();
};
} // namespace Engine