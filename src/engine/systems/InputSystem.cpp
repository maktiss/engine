#include "InputSystem.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Systems {
int InputSystem::init() {
	spdlog::info("Initializing InputSystem...");

	assert(glfwWindow != nullptr);
	
	glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE);


	InputManager::init();

	keyMap[GLFW_KEY_W] = InputManager::KeyAction::CAMERA_MOVE_FORWARD;
	keyMap[GLFW_KEY_A] = InputManager::KeyAction::CAMERA_MOVE_LEFT;
	keyMap[GLFW_KEY_S] = InputManager::KeyAction::CAMERA_MOVE_BACKWARD;
	keyMap[GLFW_KEY_D] = InputManager::KeyAction::CAMERA_MOVE_RIGHT;
	keyMap[GLFW_KEY_Q] = InputManager::KeyAction::CAMERA_MOVE_DOWN;
	keyMap[GLFW_KEY_E] = InputManager::KeyAction::CAMERA_MOVE_UP;


	return 0;
}

int InputSystem::run(double dt) {
	glfwPollEvents();

	for (auto const& [key, action] : keyMap) {
		auto state = glfwGetKey(glfwWindow, key);

		switch (state) {
		case GLFW_PRESS:
			InputManager::setKeyActionPressed(action);
			break;

		case GLFW_RELEASE:
			InputManager::setKeyActionReleased(action);
			break;
		}
	}

	return 0;
}
} // namespace Engine::Systems