#include "ScriptingSystem.hpp"

#include "engine/scripts/ScriptBase.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int ScriptingSystem::init() {
	spdlog::info("Initializing ScriptingSystem...");

	return 0;
}

int ScriptingSystem::run(double dt) {

	EntityManager::forEachIndexed<ScriptComponent>([dt](auto handle, auto& script) {
		ScriptManager::getScript<ScriptBase>(script.handle)->onUpdate(handle, dt);
	});

	return 0;
}
} // namespace Engine