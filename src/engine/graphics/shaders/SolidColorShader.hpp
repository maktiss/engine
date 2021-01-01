#pragma once

#include "ShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Shaders {
class SolidColorShader : public ShaderBase<SolidColorShader> {
public:
	enum Flags {
	};

	struct MaterialUniformBlock {
		glm::vec4 color;
	};

public:
	static constexpr const char* getFlagName(Flags flag) {
		return "";
	}
};
} // namespace Engine::Graphics::Shaders
