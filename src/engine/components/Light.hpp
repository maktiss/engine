#pragma once

#include "ComponentBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Components {
class Light : public ComponentBase {
public:
	enum class Type {
		POINT,
		SPOT,
		DIRECTIONAL,
	};

public:
	Type type {};

	glm::vec3 color = { 1.0f, 1.0f, 1.0f };

	float radius = 5.0f;

	bool castsShadows {};
	int shadowMapIndex = -1;
};
} // namespace Engine::Components