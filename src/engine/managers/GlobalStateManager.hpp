#pragma once

#include "GlobalStateManagerBase.hpp"

#include "engine/states/DebugState.hpp"


namespace Engine::Managers {
class GlobalStateManager : public GlobalStateManagerBase<Engine::States::DebugState> {

};
}