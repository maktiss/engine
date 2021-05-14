#pragma once

#include "BoundingBox.hpp"
#include "BoundingSphere.hpp"

#include <glm/glm.hpp>

#include <array>


namespace Engine {
class Frustum {
public:
	struct Plane {
		glm::vec3 normal;
		float offset;
	};
	std::array<Plane, 6> planes {};


public:
	Frustum() = default;

	Frustum(glm::mat4 viewProjectionMatrix) {
		buildFromViewProjectionMatrix(viewProjectionMatrix);
	}


	inline bool intersects(const std::array<glm::vec3, 8>& boundingBox) const {
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

	inline bool intersects(const BoundingBox& boundingBox) const {
		return intersects(boundingBox.points);
	}

	// FIXME: bounding sphere presicion issues
	inline bool intersects(const BoundingSphere& boundingSphere) const {
		for (const auto& plane : planes) {
			const auto point = boundingSphere.center + plane.normal * boundingSphere.radius;

			if (glm::dot(plane.normal, point) < plane.offset) {
				return false;
			}
		}

		return true;
	}


	inline bool contains(const std::array<glm::vec3, 8>& boundingBox) const {
		for (const auto& plane : planes) {
			for (const auto& point : boundingBox) {
				if (glm::dot(plane.normal, point) < plane.offset) {
					return false;
				}
			}
		}

		return true;
	}

	inline bool contains(const BoundingBox& boundingBox) const {
		return contains(boundingBox.points);
	}


	inline bool contains(const BoundingSphere& boundingSphere) const {
		for (const auto& plane : planes) {
			const auto point = boundingSphere.center - plane.normal * boundingSphere.radius;

			if (glm::dot(plane.normal, point) > plane.offset) {
				return false;
			}
		}

		return true;
	}


	void buildFromViewProjectionMatrix(glm::mat4 viewProjectionMatrix);
};
} // namespace Engine