#pragma once

#include "BoundingBox.hpp"

#include <glm/glm.hpp>


namespace Engine {
class BoundingSphere {
public:
	glm::vec3 center {};
	float radius {};


public:
	BoundingSphere() = default;

	BoundingSphere(const BoundingBox& boundingBox) {
		buildFromBoundingBox(boundingBox);
	}


	inline BoundingSphere transform(glm::mat4 matrix) const {
		BoundingSphere transformed = *this;

		transformed.center = transformed.center + glm::vec3(matrix[3]);
		transformed.radius *= std::max(std::max(matrix[0][0], matrix[1][1]), matrix[2][2]);

		return transformed;
	}

	void buildFromBoundingBox(const BoundingBox& boundingBox) {
		center = {};

		for (const auto& point : boundingBox.points) {
			center += point;
		}
		center /= 8.0f;

		radius = {};
		float radius2 {};

		for (const auto& point : boundingBox.points) {
			auto vec = point - center;
			radius2	 = std::max(radius2, vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
		}
		radius = std::sqrt(radius2);
	}
};
} // namespace Engine