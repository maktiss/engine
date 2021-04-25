#pragma once

#include "engine/managers/Managers.hpp"


namespace Engine {
class SystemBase {
public:
	virtual int init()		   = 0;
	virtual int run(double dt) = 0;
};
} // namespace Engine