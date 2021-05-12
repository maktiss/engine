#pragma once

#include "ComponentBase.hpp"

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


namespace Engine {
class CameraComponent : public ComponentBase {
public:
	glm::vec2 viewport = { 0, 0 };

	bool isPerspective = true;

	float zNear = 0.01f;
	float zFar	= 4000.0f;

	float fov = 70.0f;

	// targetTextureHandle?

public:
	inline glm::mat4 getProjectionMatrix() const {
		if (isPerspective) {
			return glm::perspectiveFovLH(glm::radians(fov), viewport.x, viewport.y, zNear, zFar);
		} else {
			float halfWidth	 = viewport.x * 0.5f;
			float halfHeight = viewport.y * 0.5f;
			return glm::orthoLH_ZO(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
		}
	}
};
} // namespace Engine
