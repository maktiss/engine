#pragma once

#include "GlobalStateManagerBase.hpp"

#include "engine/states/DebugState.hpp"


namespace Engine {
class GlobalStateManager : public GlobalStateManagerBase<DebugState> {};
} // namespace Engine