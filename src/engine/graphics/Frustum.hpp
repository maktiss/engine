#pragma once

#include "BoundingBox.hpp"

#include <glm/glm.hpp>

#include <array>


namespace Engine {
class Frustum {
public:
	struct Plane {
		glm::vec3 normal;
		float offset;
	};
	std::array<Plane, 6> planes;


public:
	Frustum() {
	}

	Frustum(glm::mat4 viewProjectionMatrix);


	inline bool contains(const std::array<glm::vec3, 8>& boundingBox) const {
		for (const auto& plane : planes) {
			bool isInside = false;

			for (const auto& point : boundingBox) {
				if (glm::dot(plane.normal, point) >= plane.offset) {
					isInside = true;
					break;
				}
			}

			if (!isInside) {
				return false;
			}
		}
		return true;
	}

	inline bool contains(const BoundingBox& boundingBox) const {
		return contains(boundingBox.points);
	}

	void buildFromViewProjectionMatrix(glm::mat4 viewProjectionMatrix);
};
} // namespace Engine