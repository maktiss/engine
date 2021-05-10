#pragma once

#include "ComponentBase.hpp"

#include "engine/managers/GraphicsShaderManager.hpp"
#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/MeshManager.hpp"

#include <array>
#include <vector>


namespace Engine {
class ModelComponent : public ComponentBase {
public:
	std::array<MeshManager::Handle, 4> meshHandles {};
	std::array<MaterialManager::Handle, 4> materialHandles {};

	// Cached shader handles, have to be updated on mesh and/or material handle change
	std::array<GraphicsShaderManager::Handle, 4> shaderHandles {};
};
} // namespace Engine
