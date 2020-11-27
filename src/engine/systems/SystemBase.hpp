#pragma once

#include "engine/managers/MeshManager.hpp"

#include <memory>


namespace Engine::Systems {
class SystemBase {
public:
	virtual int init()		   = 0;
	virtual int run(double dt) = 0;
};
} // namespace Engine::Systems