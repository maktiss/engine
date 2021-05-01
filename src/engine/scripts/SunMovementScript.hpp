#pragma once

#include "ScriptBase.hpp"

#include <glm/glm.hpp>


namespace Engine {
class SunMovementScript : public ScriptBase {
public:
	int onUpdate(EntityManager::Handle handle, double dt) override;

	const char* getScriptName() const override {
		return "script_sun_movement";
	}
};
} // namespace Engine