#pragma once

#include "GraphicsShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Shaders {
class SkymapShader : public GraphicsShaderBase<SkymapShader> {
public:
	enum Flags {
	};
	

public:
	static constexpr const char* getFlagName(Flags flag) {
		return "";
	}
};
} // namespace Engine::Graphics::Shaders
