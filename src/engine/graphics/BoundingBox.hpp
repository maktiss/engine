#pragma once

#include <glm/glm.hpp>

#include <array>


namespace Engine {
class BoundingBox {
public:
	std::array<glm::vec3, 8> points {};


public:
	inline BoundingBox transform(glm::mat4 matrix) const {
		BoundingBox transformed = *this;
		for (auto& point : transformed.points) {
			point = glm::vec3(matrix * glm::vec4(point, 1.0));
		}

		return transformed;
	}
};
} // namespace Engine