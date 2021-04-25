#pragma once

#include "SystemBase.hpp"


namespace Engine {
class ScriptingSystem : public SystemBase {
public:
	int init() override;
	int run(double dt) override;
};
} // namespace Engine