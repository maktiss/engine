#pragma once

#include "ScriptManagerBase.hpp"


namespace Engine::Scripts {
class ScriptBase;
} // namespace Engine::Scripts

namespace Engine::Managers {
class ScriptManager : public ScriptManagerBase<ScriptManager, Engine::Scripts::ScriptBase> {
public:
	static int init();


private:
	ScriptManager() {};
};
} // namespace Engine::Managers