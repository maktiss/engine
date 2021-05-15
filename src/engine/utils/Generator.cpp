#include "Generator.hpp"

#include <glm/gtx/transform.hpp>


namespace Engine {
void Generator::skyBoxMesh(MeshManager::Handle& meshHandle) {
	meshHandle.apply([](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(8);

		std::get<0>(vertexBuffer[0]) = glm::vec3(-1.0f, -1.0f, -1.0f);
		std::get<0>(vertexBuffer[1]) = glm::vec3(-1.0f, -1.0f, 1.0f);
		std::get<0>(vertexBuffer[2]) = glm::vec3(1.0f, -1.0f, 1.0f);
		std::get<0>(vertexBuffer[3]) = glm::vec3(1.0f, -1.0f, -1.0f);

		std::get<0>(vertexBuffer[4]) = glm::vec3(-1.0f, 1.0f, -1.0f);
		std::get<0>(vertexBuffer[5]) = glm::vec3(-1.0f, 1.0f, 1.0f);
		std::get<0>(vertexBuffer[6]) = glm::vec3(1.0f, 1.0f, 1.0f);
		std::get<0>(vertexBuffer[7]) = glm::vec3(1.0f, 1.0f, -1.0f);

		auto& indexBuffer = mesh.getIndexBuffer();

		indexBuffer = {
			0, 2, 1, 0, 3, 2, 5, 6, 7, 5, 7, 4, 0, 4, 7, 0, 7, 3, 3, 7, 6, 3, 6, 2, 2, 6, 5, 2, 5, 1, 1, 5, 4, 1, 4, 0,
		};
	});
	meshHandle.update();
}

void Generator::skySphereMesh(MeshManager::Handle& meshHandle, uint verticalVertexCount, uint horisontalVertexCount) {
	const float yStep		= 1.0f / (verticalVertexCount + 1);
	const float angularStep = (2.0f * M_PI) / horisontalVertexCount;

	meshHandle.apply([&](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(2 + verticalVertexCount * horisontalVertexCount);

		auto& indexBuffer = mesh.getIndexBuffer();
		indexBuffer.resize(verticalVertexCount * horisontalVertexCount * 3 * 2, 0);

		// 1-\left(\left(\frac{1+\sqrt[5]{0.5}}{2x+1}-1\right)^{5}+0.5\right)\cdot\left(1-\left(x\right)^{3}\right)
		auto translate = [](float value) {
			const float left  = 3.0f;
			const float right = 3.0f;
			return 1.0f - (std::pow((1.0f + std::pow(0.5f, 1.0f / left)) / (2.0f * value + 1.0f) - 1.0f, left) + 0.5f) *
							  (1.0f - std::pow(value, right));
		};


		// Generate vertices

		std::get<0>(vertexBuffer[0])					   = glm::vec3(0.0f, -1.0f, 0.0f);
		std::get<0>(vertexBuffer[vertexBuffer.size() - 1]) = glm::vec3(0.0f, 1.0f, 0.0f);

		for (uint i = 1; i <= verticalVertexCount; i++) {
			float height = translate(yStep * i) * 2.0f - 1.0f;

			for (uint j = 0; j < horisontalVertexCount; j++) {
				uint index = 1 + (i - 1) * horisontalVertexCount + j;

				float angle = angularStep * j;

				std::get<0>(vertexBuffer[index]).x = std::cos(angle) * std::sin(std::acos(height));
				std::get<0>(vertexBuffer[index]).y = height;
				std::get<0>(vertexBuffer[index]).z = std::sin(angle) * std::sin(std::acos(height));
			}
		}


		// Generate bottom strip

		for (uint i = 0; i < horisontalVertexCount; i++) {
			uint index = i * 3;

			indexBuffer[index + 0] = 0;
			indexBuffer[index + 1] = i + 1;
			indexBuffer[index + 2] = i + 2;
		}

		// Fix last triangle in bottom strip
		indexBuffer[horisontalVertexCount * 3 - 1] = 1;


		// Generate top strip

		uint topVertexIndex = vertexBuffer.size() - 1;

		for (uint i = 0; i < horisontalVertexCount; i++) {
			uint index = indexBuffer.size() - 3 - i * 3;

			indexBuffer[index + 0] = topVertexIndex - 0;
			indexBuffer[index + 1] = topVertexIndex - i - 1;
			indexBuffer[index + 2] = topVertexIndex - i - 2;
		}

		// Fix last triangle in top strip
		indexBuffer[indexBuffer.size() - horisontalVertexCount * 3 + 2] = topVertexIndex - 1;


		// Generate all other strips

		uint offset = horisontalVertexCount * 3;
		for (uint i = 1; i < verticalVertexCount; i++) {
			for (uint j = 0; j < horisontalVertexCount; j++) {
				uint vertexIndex = 1 + (i - 1) * horisontalVertexCount + j;
				uint index		 = offset + (i - 1) * horisontalVertexCount * 6 + j * 6;

				indexBuffer[index + 0] = vertexIndex;
				indexBuffer[index + 1] = vertexIndex + horisontalVertexCount;
				indexBuffer[index + 2] = vertexIndex + horisontalVertexCount + 1;

				indexBuffer[index + 3] = vertexIndex;
				indexBuffer[index + 4] = vertexIndex + horisontalVertexCount + 1;
				indexBuffer[index + 5] = vertexIndex + 1;
			}

			// Fix last triangle in a strip
			uint vertexIndex = 1 + (i - 1) * horisontalVertexCount + horisontalVertexCount - 1;
			uint index		 = offset + (i - 1) * horisontalVertexCount * 6 + (horisontalVertexCount - 1) * 6;

			indexBuffer[index + 2] = vertexIndex + 1;

			indexBuffer[index + 4] = vertexIndex + 1;
			indexBuffer[index + 5] = vertexIndex + 1 - horisontalVertexCount;
		}
	});
	meshHandle.update();
}


void Generator::screenTriangle(MeshManager::Handle& meshHandle) {
	meshHandle.apply([](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(8);

		std::get<0>(vertexBuffer[0]) = glm::vec3(-1.0f, 1.0f, 0.5f);
		std::get<0>(vertexBuffer[1]) = glm::vec3(-1.0f, -3.0f, 0.5f);
		std::get<0>(vertexBuffer[2]) = glm::vec3(3.0f, 1.0f, 0.5f);

		std::get<1>(vertexBuffer[0]) = glm::vec2(0.0f, 0.0f);
		std::get<1>(vertexBuffer[1]) = glm::vec2(0.0f, 2.0f);
		std::get<1>(vertexBuffer[2]) = glm::vec2(2.0f, 0.0f);

		auto& indexBuffer = mesh.getIndexBuffer();

		indexBuffer = {
			0,
			1,
			2,
		};
	});
	meshHandle.update();
}


void Generator::cubeViewMatrices(glm::mat4 matrices[6], float nearZ, float farZ) {
	glm::vec3 viewVectors[6] = {
		{ 1.0f, 0.0f, 0.0f },  { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },  { 0.0f, 0.0f, -1.0f },
	};

	auto projectionMatrix = glm::perspectiveLH(glm::radians(90.0f), 1.0f, nearZ, farZ);

	for (uint i = 0; i < 6; i++) {
		auto up = glm::vec3(0.0f, 1.0f, 0.0f);

		if (i == 2) {
			up = glm::vec3(0.0f, 0.0f, -1.0f);
		} else if (i == 3) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		auto viewMatrix = glm::lookAtLH(glm::vec3(0.0f), viewVectors[i], up);

		matrices[i] = projectionMatrix * viewMatrix;
	}
}


void Generator::fibonacciSphere(glm::vec4* pData, uint sampleCount, uint fraction) {
	float phi = M_PI * (3.0 - std::sqrt(5.0));

	for (uint i = 0; i < sampleCount; i++) {
		float y		 = 1.0 - (i / static_cast<float>(sampleCount * fraction - 1)) * 2.0;
		float radius = std::sqrt(1.0 - y * y);

		float theta = phi * i;

		float x = std::cos(theta) * radius;
		float z = std::sin(theta) * radius;

		pData[i] = { x, y, z, 0.0 };
	}
}
} // namespace Engine