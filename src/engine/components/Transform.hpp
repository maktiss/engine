#pragma once

#include "ComponentBase.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


namespace Engine::Components {
class Transform : public ComponentBase {
public:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

public:
	inline glm::mat4 getTransformMatrix() const {
		auto matrix = glm::identity<glm::mat4>();

		// TODO: check the order

		matrix = glm::scale(matrix, scale);

		matrix = glm::rotate(matrix, rotation.x, { 1.0f, 0.0f, 0.0f });
		matrix = glm::rotate(matrix, rotation.y, { 0.0f, 1.0f, 0.0f });
		matrix = glm::rotate(matrix, rotation.z, { 0.0f, 0.0f, 1.0f });

		matrix = glm::translate(matrix, position);

		return matrix;
	}
};
} // namespace Engine::Components