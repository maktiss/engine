#pragma once

#include "engine/managers/Managers.hpp"

#include <string>


namespace Engine {
class ScriptBase {
public:
	// Called once per frame
	virtual int onUpdate(EntityManager::Handle handle, double dt) {
		return 0;
	}

	virtual const char* getScriptName() const {
		return "script_unnamed";
	}
};
} // namespace Engine