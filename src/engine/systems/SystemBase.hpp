#pragma once

#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/MeshManager.hpp"
#include "engine/managers/ShaderManager.hpp"

#include <memory>


namespace Engine::Systems {
class SystemBase {
public:
	virtual int init()		   = 0;
	virtual int run(double dt) = 0;
};
} // namespace Engine::Systems