#pragma once

#include "ScriptManagerBase.hpp"


namespace Engine {
class ScriptBase;
} // namespace Engine

namespace Engine {
class ScriptManager : public ScriptManagerBase<ScriptManager, ScriptBase> {
public:
	static int init();


private:
	ScriptManager() {};
};
} // namespace Engine