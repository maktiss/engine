#include "CameraScript.hpp"


namespace Engine::Scripts {
int CameraScript::onUpdate(Engine::Managers::EntityManager::Handle handle, double dt) {
	glm::vec3 movementDir = glm::vec3(0.0f);

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

	transform.position += movementDir * static_cast<float>(dt) * 5.0f;

	return 0;
}
} // namespace Engine::Scripts