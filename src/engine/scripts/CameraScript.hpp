#pragma once

#include "ScriptBase.hpp"

#include <glm/glm.hpp>


namespace Engine {
class CameraScript : public ScriptBase {
private:
	PROPERTY(float, "Camera", baseSpeed, 5.0f);
	PROPERTY(float, "Camera", speedMultiplier, 15.0f);


public:
	int onUpdate(EntityManager::Handle handle, double dt) override;

	const char* getScriptName() const override {
		return "script_camera";
	}
};
} // namespace Engine