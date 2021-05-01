#include "ScriptManager.hpp"

#include "engine/scripts/Scripts.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int ScriptManager::init() {
	spdlog::info("Initializing ScriptManager...");

	if (ScriptManagerBase::init()) {
		return 1;
	}

	registerScript<CameraScript>();
	registerScript<FloatingObjectScript>();
	registerScript<SunMovementScript>();

	return 0;
}
} // namespace Engine