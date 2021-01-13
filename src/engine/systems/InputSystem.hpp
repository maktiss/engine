#pragma once

#include "SystemBase.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <map>
#include <set>


namespace Engine::Systems {
class InputSystem : public SystemBase {
private:
	GLFWwindow* glfwWindow {};

	std::map<uint, Engine::Managers::InputManager::KeyAction> keyMap {};

	std::map<uint, std::set<Engine::Managers::InputManager::KeyAction>> mouseButtonMap {};

	// x and y mouse axes
	std::array<std::set<Engine::Managers::InputManager::AxisAction>, 2> mouseAxisMap {};

	double lastMousePosX {};
	double lastMousePosY {};

	using InputManager = Engine::Managers::InputManager;


public:
	inline void setWindow(GLFWwindow* pGLFWWindow) {
		glfwWindow = pGLFWWindow;
	}

	int init() override;
	int run(double dt) override;
};
};