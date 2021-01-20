#include "ShaderManager.hpp"

#include <spdlog/spdlog.h>

#include <array>


namespace Engine::Managers {
int ShaderManager::init() {
	spdlog::info("Initializing ShaderManager...");
	return ShaderManagerBase::init();
}
} // namespace Engine::Managers