#include "FloatingObjectScript.hpp"


namespace Engine {
int FloatingObjectScript::onUpdate(EntityManager::Handle handle, double dt) {
	auto& transform = handle.getComponent<TransformComponent>();

	float halfHeight = 1.0f;

	transform.rotation.y += 0.2f * static_cast<float>(dt);
	transform.position.y = halfHeight + halfHeight * std::sin(transform.rotation.y * 3.2f);

	return 0;
}
} // namespace Engine