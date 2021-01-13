#include "CameraScript.hpp"


namespace Engine::Scripts {
int CameraScript::onUpdate(Engine::Managers::EntityManager::Handle handle, double dt) {
	auto movementDir = glm::vec4(0.0f);

	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_MOVE_LEFT)) {
		movementDir.x = -1.0f;
	}
	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_MOVE_RIGHT)) {
		movementDir.x = 1.0f;
	}

	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_MOVE_DOWN)) {
		movementDir.y = -1.0f;
	}
	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_MOVE_UP)) {
		movementDir.y = 1.0f;
	}

	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_MOVE_BACKWARD)) {
		movementDir.z = -1.0f;
	}
	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_MOVE_FORWARD)) {
		movementDir.z = 1.0f;
	}

	if (glm::length(movementDir) >= 1.0f) {
		movementDir = glm::normalize(movementDir);
	}


	auto& transform = handle.getComponent<Engine::Components::Transform>();


	double sensitivity = 0.002;

	if (Engine::Managers::InputManager::isKeyActionStatePressed(
			Engine::Managers::InputManager::KeyAction::CAMERA_ACTIVE)) {
		transform.rotation.y +=
			Engine::Managers::InputManager::getActionAxis(Engine::Managers::InputManager::AxisAction::CAMERA_ROTATE_X) * sensitivity;
		transform.rotation.x +=
			Engine::Managers::InputManager::getActionAxis(Engine::Managers::InputManager::AxisAction::CAMERA_ROTATE_Y) * sensitivity;
	}

	transform.rotation.x = std::max(-1.5707f, transform.rotation.x);
	transform.rotation.x = std::min(1.5707f, transform.rotation.x);

	movementDir = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * movementDir;
	movementDir = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * movementDir;
	
	transform.position += glm::vec3(movementDir.x, movementDir.y, movementDir.z) * static_cast<float>(dt) * 5.0f;

	return 0;
}
} // namespace Engine::Scripts