#pragma once

#include "ComponentBase.hpp"

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/MaterialManager.hpp"
#include "engine/managers/GraphicsShaderManager.hpp"

#include <array>
#include <vector>


namespace Engine::Components {
class Model : public ComponentBase {
public:
	std::array<Engine::Managers::MeshManager::Handle, 4> meshHandles;
	std::array<Engine::Managers::MaterialManager::Handle, 4> materialHandles;

	// Cached shader handles, have to be updated on mesh and/or material handle change
	std::array<Engine::Managers::GraphicsShaderManager::Handle, 4> shaderHandles;
};
} // namespace Engine::Components
