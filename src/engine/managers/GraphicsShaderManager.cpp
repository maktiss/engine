#include "GraphicsShaderManager.hpp"

#include <spdlog/spdlog.h>

#include <array>


namespace Engine::Managers {
int GraphicsShaderManager::init() {
	spdlog::info("Initializing GraphicsShaderManager...");
	return GraphicsShaderManagerBase::init();
}
} // namespace Engine::Managers