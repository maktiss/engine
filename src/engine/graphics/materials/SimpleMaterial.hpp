#pragma once

#include "MaterialBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"

#include <glm/glm.hpp>


namespace Engine {
class SimpleMaterial : public MaterialBase<SimpleMaterial, SimpleShader> {
public:
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };

	TextureManager::Handle textureAlbedo {};
	TextureManager::Handle textureNormal {};

public:
	uint32_t getSignature() const {
		uint32_t flags = 0;

		if (textureAlbedo.getIndex()) {
			flags |= Flags::USE_TEXTURE_ALBEDO;
		}

		if (textureNormal.getIndex()) {
			flags |= Flags::USE_TEXTURE_NORMAL;
		}

		return flags;
	}

	void writeMaterialUniformBlock(MaterialUniformBlock* buffer) {
		buffer->color = glm::vec4(color, 1.0f);

		buffer->textureAlbedo = textureAlbedo.getIndex();
		buffer->textureNormal = textureNormal.getIndex();
	}
};
} // namespace Engine