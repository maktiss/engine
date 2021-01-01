#pragma once

#include "ComponentBase.hpp"

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/MaterialManager.hpp"

#include <vector>


namespace Engine::Components {
class Model : public ComponentBase {
public:
	std::vector<Engine::Managers::MeshManager::Handle> meshHandles;
	std::vector<Engine::Managers::MaterialManager::Handle> materialHandles;

	// std::vector<Engine::Managers::ShaderManager::Handle> shaderHandles;
};
} // namespace Engine::Components
