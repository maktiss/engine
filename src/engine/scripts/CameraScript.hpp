#pragma once

#include "ScriptBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Scripts {
class CameraScript : public ScriptBase {
public:
	int onUpdate(Engine::Managers::EntityManager::Handle handle, double dt) override;

	const char* getScriptName() const override {
		return "script_camera";
	}
};
} // namespace Engine::Scripts