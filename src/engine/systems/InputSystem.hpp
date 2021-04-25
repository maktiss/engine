#pragma once

#include "SystemBase.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <map>
#include <set>


namespace Engine {
class InputSystem : public SystemBase {
private:
	GLFWwindow* glfwWindow {};

	std::map<uint, InputManager::KeyAction> keyMap {};

	std::map<uint, std::set<InputManager::KeyAction>> mouseButtonMap {};

	// x and y mouse axes
	std::array<std::set<InputManager::AxisAction>, 2> mouseAxisMap {};

	double lastMousePosX {};
	double lastMousePosY {};


public:
	inline void setWindow(GLFWwindow* pGLFWWindow) {
		glfwWindow = pGLFWWindow;
	}

	int init() override;
	int run(double dt) override;
};
} // namespace Engine