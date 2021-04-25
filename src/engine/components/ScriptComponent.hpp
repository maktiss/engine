#pragma once

#include "ComponentBase.hpp"

#include "engine/managers/ScriptManager.hpp"


namespace Engine {
class ScriptComponent : public ComponentBase {
public:
	ScriptManager::Handle handle { 0 };
};
} // namespace Engine