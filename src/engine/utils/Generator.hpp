#pragma once

#include <glm/glm.hpp>

#include <cmath>


namespace Engine::Generator {
void fibonacciSphere(glm::vec4* pData, uint sampleCount, uint fraction = 1) {
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
}; // namespace Engine::Generator