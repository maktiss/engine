#pragma once

#include "engine/managers/MeshManager.hpp"

#include <glm/glm.hpp>

#include <cmath>


namespace Engine {
class Generator {
public:
	static void skyBoxMesh(MeshManager::Handle& meshHandle);
	static void skySphereMesh(MeshManager::Handle& meshHandle, uint verticalVertexCount, uint horisontalVertexCount);

	static void screenTriangle(MeshManager::Handle& meshHandle);

	static void cubeViewMatrices(glm::mat4 matrices[6], float nearZ, float farZ);

	static void fibonacciSphere(glm::vec4* pData, uint sampleCount, uint fraction = 1);
};
}; // namespace Engine