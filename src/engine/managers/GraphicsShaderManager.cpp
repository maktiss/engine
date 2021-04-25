#include "GraphicsShaderManager.hpp"

#include <spdlog/spdlog.h>

#include <array>


namespace Engine {
int GraphicsShaderManager::init() {
	spdlog::info("Initializing GraphicsShaderManager...");
	return GraphicsShaderManagerBase::init();
}
} // namespace Engine