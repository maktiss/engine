#include "ScriptManager.hpp"

#include "engine/scripts/CameraScript.hpp"
#include "engine/scripts/FloatingObjectScript.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Managers {
int ScriptManager::init() {
	spdlog::info("Initializing ScriptManager...");

	if (ScriptManagerBase::init()) {
		return 1;
	}

	registerScript<Engine::Scripts::CameraScript>();
	registerScript<Engine::Scripts::FloatingObjectScript>();

	return 0;
}
} // namespace Engine::Managers