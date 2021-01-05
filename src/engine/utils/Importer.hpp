#pragma once

#include "engine/managers/MeshManager.hpp"

#include <assimp/Importer.hpp>

#include <string>
#include <vector>


namespace Engine::Utils {
class Importer {
private:
	static Assimp::Importer assimpImporter;

public:
	static int importMesh(std::string filename, std::vector<Engine::Managers::MeshManager::Handle>& meshHandles);
};
} // namespace Engine::Utils