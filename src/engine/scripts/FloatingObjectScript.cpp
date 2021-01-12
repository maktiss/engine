#include "FloatingObjectScript.hpp"


namespace Engine::Scripts {
int FloatingObjectScript::onUpdate(Engine::Managers::EntityManager::Handle handle, double dt) {
	auto& transform = handle.getComponent<Engine::Components::Transform>();

	transform.rotation.y += 0.2f * static_cast<float>(dt);
	transform.position.y = -0.5f + 0.2f * std::sin(transform.rotation.y * 3.2f);
	transform.scale = { 0.1f, 0.1f, 0.1f };

	return 0;
}
} // namespace Engine::Scripts