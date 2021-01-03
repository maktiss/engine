#pragma once

#include "ComponentBase.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


namespace Engine::Components {
class Transform : public ComponentBase {
public:
	glm::vec3 position {};
	glm::vec3 rotation {};
	glm::vec3 scale { 1.0f, 1.0f, 1.0f };

public:
	inline glm::mat4 getTransformMatrix() const {
		auto matrix = glm::mat4(1.0f);

		matrix *= glm::translate(glm::mat4(1.0f), position);

		matrix *= glm::rotate(glm::mat4(1.0f), rotation.z, { 0.0f, 0.0f, 1.0f });
		matrix *= glm::rotate(glm::mat4(1.0f), rotation.y, { 0.0f, 1.0f, 0.0f });
		matrix *= glm::rotate(glm::mat4(1.0f), rotation.x, { 1.0f, 0.0f, 0.0f });

		matrix *= glm::scale(glm::mat4(1.0f), scale);

		return matrix;
	}
};
} // namespace Engine::Components