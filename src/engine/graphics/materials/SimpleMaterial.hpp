#pragma once

#include "MaterialBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Materials {
class SimpleMaterial : public MaterialBase<SimpleMaterial, Engine::Graphics::Shaders::SimpleShader> {
public:
	glm::vec3 color;

public:
	uint32_t getFlags() const {
		uint32_t flags = 0;
		if (color != glm::vec3()) {
			flags |= Flags::USE_COLOR_DIFFUSE;
		}

		return flags;
	}
};
} // namespace Engine::Graphics::Materials
