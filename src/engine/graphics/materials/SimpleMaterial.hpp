#pragma once

#include "MaterialBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Materials {
class SimpleMaterial : public MaterialBase<SimpleMaterial, Engine::Graphics::Shaders::SimpleShader> {
public:
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };

public:
	uint32_t getSignature() const {
		uint32_t flags = 0;

		if (textureHandles[0].getIndex()) {
			flags |= Flags::USE_TEXTURE_ALBEDO;
		}

		if (textureHandles[1].getIndex()) {
			flags |= Flags::USE_TEXTURE_NORMAL;
		}

		return flags;
	}

	void writeMaterialUniformBlock(MaterialUniformBlock* buffer) {
		buffer->color = glm::vec4(color, 1.0f);
	}
};
} // namespace Engine::Graphics::Materials
