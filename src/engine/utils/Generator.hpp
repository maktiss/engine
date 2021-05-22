#pragma once

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include <glm/glm.hpp>

#include <cmath>


namespace Engine {
class Generator {
public:
	static void skyBoxMesh(MeshManager::Handle& meshHandle);
	static void skySphereMesh(MeshManager::Handle& meshHandle, uint verticalVertexCount, uint horisontalVertexCount);

	static void quad(MeshManager::Handle& meshHandle);

	static void screenTriangle(MeshManager::Handle& meshHandle);

	static void cubeViewMatrices(glm::mat4 matrices[6], float nearZ, float farZ);

	static void fibonacciSphere(glm::vec4* pData, uint sampleCount, uint fraction = 1);

	static void normalMapFromHeight(TextureManager::Handle& textureHandle, TextureManager::Handle& normalTextureHandle,
									uint maxHeight);
};
}; // namespace Engine