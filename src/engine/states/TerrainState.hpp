#pragma once

#include "engine/managers/GraphicsShaderManager.hpp"
#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include <array>


namespace Engine {
class TerrainState {
public:
	int size {};
	uint maxHeight {};

	TextureManager::Handle heightMapHandle {};
	TextureManager::Handle normalMapHandle {};

	std::array<MaterialManager::Handle, 4> materialHandles {};
	std::array<GraphicsShaderManager::Handle, 4> shaderHandles {};
};
} // namespace Engine