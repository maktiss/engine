#pragma once

#include "engine/managers/EntityManager.hpp"
#include "engine/managers/InputManager.hpp"
#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/MeshManager.hpp"
#include "engine/managers/ScriptManager.hpp"
#include "engine/managers/GraphicsShaderManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include <string>


namespace Engine::Scripts {
class ScriptBase {
public:
	// Called once per frame
	virtual int onUpdate(Engine::Managers::EntityManager::Handle handle, double dt) {
		return 0;
	}

	virtual const char* getScriptName() const {
		return "script_unnamed";
	}
};
} // namespace Engine::Scripts