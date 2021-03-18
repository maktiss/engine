#pragma once

#include "engine/managers/ConfigManager.hpp"
#include "engine/managers/EntityManager.hpp"
#include "engine/managers/InputManager.hpp"
#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/MeshManager.hpp"
#include "engine/managers/ScriptManager.hpp"
#include "engine/managers/ShaderManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include <memory>


namespace Engine::Systems {
class SystemBase {
public:
	virtual int init()		   = 0;
	virtual int run(double dt) = 0;
};
} // namespace Engine::Systems