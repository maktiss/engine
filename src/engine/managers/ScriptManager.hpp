#pragma once

#include "ScriptManagerBase.hpp"


namespace Engine::Scripts {
class ScriptBase;
} // namespace Engine::Scripts

namespace Engine::Managers {
class ScriptManager : public ScriptManagerBase<ScriptManager, Engine::Scripts::ScriptBase> {
public:
	static int init() {
		if (ScriptManagerBase::init()) {
			return 1;
		}

		return 0;
	}


private:
	ScriptManager() {};
};
} // namespace Engine::Managers