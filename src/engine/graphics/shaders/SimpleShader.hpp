#pragma once

#include "ShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Shaders {
class SimpleShader : public ShaderBase<SimpleShader> {
public:
	enum Flags {
		USE_TEXTURE = 1 << 0,
	};

	struct MaterialUniformBlock {
		glm::vec4 color;
	};

public:
	static constexpr uint32_t getTextureCount() {
		return 1;
	}

	static constexpr const char* getFlagName(Flags flag) {
		switch (flag) {
		case Flags::USE_TEXTURE:
			return "USE_TEXTURE";
		}
		return "";
	}
};
} // namespace Engine::Graphics::Shaders
