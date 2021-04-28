#pragma once

#include "MaterialBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"

#include <glm/glm.hpp>


namespace Engine {
class SimpleMaterial : public MaterialBase<SimpleMaterial, SimpleShader> {
public:
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };

	float metallic = 0.0f;
	float roughness = 0.5f;
	float ambientOcclusion = 1.0f;

	TextureManager::Handle textureAlbedo {};
	TextureManager::Handle textureNormal {};
	TextureManager::Handle textureMRA {};


public:
	uint32_t getSignature() const {
		uint32_t flags = 0;

		if (textureAlbedo.getIndex()) {
			flags |= Flags::USE_TEXTURE_ALBEDO;
		}

		if (textureNormal.getIndex()) {
			flags |= Flags::USE_TEXTURE_NORMAL;
		}

		if (textureMRA.getIndex()) {
			flags |= Flags::USE_TEXTURE_MRA;
		}

		return flags;
	}

	void writeMaterialUniformBlock(MaterialUniformBlock* buffer) {
		buffer->color = glm::vec4(color, 1.0f);

		buffer->metallic = metallic;
		buffer->roughness = roughness;
		buffer->ambientOcclusion = ambientOcclusion;

		buffer->textureAlbedo = textureAlbedo.getIndex();
		buffer->textureNormal = textureNormal.getIndex();
		buffer->textureMRA = textureMRA.getIndex();
	}
};
} // namespace Engine