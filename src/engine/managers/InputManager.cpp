#include "InputManager.hpp"


namespace Engine::Managers {
std::vector<uint> InputManager::actionStates {};
std::vector<double> InputManager::actionAxes {};


int InputManager::init() {
	// TODO: better solution
	actionStates.resize(8);
	actionAxes.resize(3);

	return 0;
}
} // namespace Engine::Managers