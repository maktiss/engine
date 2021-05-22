#pragma once

#include "GlobalStateManagerBase.hpp"

#include "engine/states/States.hpp"


namespace Engine {
class GlobalStateManager : public GlobalStateManagerBase<DebugState, ImGuiState, TerrainState> {};
} // namespace Engine