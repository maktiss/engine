#include "SunMovementScript.hpp"


namespace Engine {
int SunMovementScript::onUpdate(EntityManager::Handle handle, double dt) {
	auto& transform = handle.getComponent<TransformComponent>();

	float speed = 0.0f;

	// TODO: specify axis to rotate around
	transform.rotation.x += speed * static_cast<float>(dt);
	transform.rotation.z += 0.2f * speed * static_cast<float>(dt);

	return 0;
}
} // namespace Engine