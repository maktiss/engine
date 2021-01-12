#pragma once

#include "ScriptBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Scripts {
class FloatingObjectScript : public ScriptBase {
public:
	int onUpdate(Engine::Managers::EntityManager::Handle handle, double dt) override;

	const char* getScriptName() const override {
		return "script_floating_object";
	}
};
} // namespace Engine::Scripts