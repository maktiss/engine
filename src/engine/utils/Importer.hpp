#pragma once

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include <assimp/Importer.hpp>

#include <string>
#include <vector>


namespace Engine::Utils {
class Importer {
private:
	static Assimp::Importer assimpImporter;

public:
	[[nodiscard]] static int importMesh(std::string filename,
										std::vector<Engine::Managers::MeshManager::Handle>& meshHandles);

	[[nodiscard]] static int importTexture(std::string filename,
										   Engine::Managers::TextureManager::Handle& textureHandle, bool srgb);
};
} // namespace Engine::Utils