#pragma once

#include "SystemBase.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>


namespace Engine::Systems {
class InputSystem : public SystemBase {
private:
	GLFWwindow* glfwWindow {};

	std::map<uint, Engine::Managers::InputManager::KeyAction> keyMap;

	using InputManager = Engine::Managers::InputManager;


public:
	inline void setWindow(GLFWwindow* pGLFWWindow) {
		glfwWindow = pGLFWWindow;
	}

	int init() override;
	int run(double dt) override;
};
};