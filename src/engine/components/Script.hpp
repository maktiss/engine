#pragma once

#include "ComponentBase.hpp"

#include "engine/managers/ScriptManager.hpp"


namespace Engine::Components {
class Script : public ComponentBase {
public:
	Engine::Managers::ScriptManager::Handle handle { 0 };
};
} // namespace Engine::Components