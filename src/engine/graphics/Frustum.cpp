#include "Frustum.hpp"

#include <glm/gtx/transform.hpp>


namespace Engine {
void Frustum::buildFromViewProjectionMatrix(glm::mat4 viewProjectionMatrix) {
	for (uint i = 0; i < 6; i++) {
		planes[i].normal = { viewProjectionMatrix[0][3] + (i % 2 ? -1 : 1) * viewProjectionMatrix[0][i / 2],
							 viewProjectionMatrix[1][3] + (i % 2 ? -1 : 1) * viewProjectionMatrix[1][i / 2],
							 viewProjectionMatrix[2][3] + (i % 2 ? -1 : 1) * viewProjectionMatrix[2][i / 2] };
		planes[i].offset = viewProjectionMatrix[3][3] + (i % 2 ? -1 : 1) * viewProjectionMatrix[3][i / 2];

		float magnitude = glm::length(planes[i].normal);
		planes[i].normal /= magnitude;
		planes[i].offset /= -magnitude;
	}
}
} // namespace Engine