#include "InputSystem.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int InputSystem::init() {
	spdlog::info("Initializing InputSystem...");

	assert(glfwWindow != nullptr);

	glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);


	InputManager::init();

	keyMap[GLFW_KEY_W] = InputManager::KeyAction::CAMERA_MOVE_FORWARD;
	keyMap[GLFW_KEY_A] = InputManager::KeyAction::CAMERA_MOVE_LEFT;
	keyMap[GLFW_KEY_S] = InputManager::KeyAction::CAMERA_MOVE_BACKWARD;
	keyMap[GLFW_KEY_D] = InputManager::KeyAction::CAMERA_MOVE_RIGHT;
	keyMap[GLFW_KEY_Q] = InputManager::KeyAction::CAMERA_MOVE_DOWN;
	keyMap[GLFW_KEY_E] = InputManager::KeyAction::CAMERA_MOVE_UP;

	keyMap[GLFW_KEY_F1] = InputManager::KeyAction::IMGUI_TOGGLE;

	mouseButtonMap[GLFW_MOUSE_BUTTON_RIGHT].insert(InputManager::KeyAction::CAMERA_ACTIVE);

	mouseAxisMap[0].insert(InputManager::AxisAction::CAMERA_ROTATE_X);
	mouseAxisMap[1].insert(InputManager::AxisAction::CAMERA_ROTATE_Y);


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


	for (auto const& [button, actions] : mouseButtonMap) {
		auto state = glfwGetMouseButton(glfwWindow, button);

		switch (state) {
		case GLFW_PRESS:
			for (auto const& action : actions) {
				InputManager::setKeyActionPressed(action);
			}
			break;

		case GLFW_RELEASE:
			for (auto const& action : actions) {
				InputManager::setKeyActionReleased(action);
			}
			break;
		}
	}


	double mousePosX;
	double mousePosY;
	glfwGetCursorPos(glfwWindow, &mousePosX, &mousePosY);

	double dX = mousePosX - lastMousePosX;
	double dY = mousePosY - lastMousePosY;

	for (auto axis : mouseAxisMap[0]) {
		InputManager::setAxisActionValue(axis, dX);
	}

	for (auto axis : mouseAxisMap[1]) {
		InputManager::setAxisActionValue(axis, dY);
	}


	lastMousePosX = mousePosX;
	lastMousePosY = mousePosY;


	if (InputManager::isKeyActionStatePressed(InputManager::KeyAction::CAMERA_ACTIVE)) {
		glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}


	return 0;
}
} // namespace Engine