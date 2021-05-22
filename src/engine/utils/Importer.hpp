#pragma once

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include <assimp/Importer.hpp>

#include <string>
#include <vector>


namespace Engine {
class Importer {
private:
	static Assimp::Importer assimpImporter;

public:
	[[nodiscard]] static int importMesh(std::string filename, std::vector<MeshManager::Handle>& meshHandles);

	[[nodiscard]] static int importTexture(std::string filename, TextureManager::Handle& textureHandle, uint channels,
										   uint channelWidth, vk::Format format);
};
} // namespace Engine