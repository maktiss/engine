#pragma once

#include "GraphicsShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine {
class SimpleShader : public GraphicsShaderBase<SimpleShader> {
public:
	enum Flags {
		USE_TEXTURE_ALBEDO = 1 << 0,
		USE_TEXTURE_NORMAL = 1 << 1,
		USE_TEXTURE_MRA = 1 << 2,
	};

	struct MaterialUniformBlock {
		glm::vec4 color {};

		float metallic {};
		float roughness {};
		float ambientOcclusion {};

		uint textureAlbedo {};
		uint textureNormal {};
		uint textureMRA {};
	};


public:
	static constexpr const char* getFlagName(Flags flag) {
		switch (flag) {
		case Flags::USE_TEXTURE_ALBEDO:
			return "USE_TEXTURE_ALBEDO";
		case Flags::USE_TEXTURE_NORMAL:
			return "USE_TEXTURE_NORMAL";
		case Flags::USE_TEXTURE_MRA:
			return "USE_TEXTURE_MRA";
		}
		return "";
	}
};
} // namespace Engine