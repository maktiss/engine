#include "ScriptingSystem.hpp"

#include "engine/scripts/ScriptBase.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Systems {
int ScriptingSystem::init() {
	spdlog::info("Initializing ScriptingSystem...");

	return 0;
}

int ScriptingSystem::run(double dt) {

	Engine::Managers::EntityManager::forEachIndexed<Engine::Components::Script>([dt](auto handle, auto& script){
		Engine::Managers::ScriptManager::getScript<Engine::Scripts::ScriptBase>(script.handle)->onUpdate(handle, dt);
	});

	return 0;
}
} // namespace Engine::Systems