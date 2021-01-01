#pragma once

#include "MaterialBase.hpp"

#include "engine/graphics/shaders/SolidColorShader.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Materials {
class SolidColorMaterial : public MaterialBase<SolidColorMaterial, Engine::Graphics::Shaders::SolidColorShader> {
public:
	glm::vec3 color;

public:
	uint32_t getFlags() const {
		uint32_t flags = 0;
		return flags;
	}

	void writeMaterialUniformBlock(MaterialUniformBlock* buffer) {
		buffer->color = glm::vec4(color, 1.0f);
	}
};
} // namespace Engine::Graphics::Materials
