#pragma once

#include "GraphicsShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine::Graphics::Shaders {
class SkyboxShader : public GraphicsShaderBase<SkyboxShader> {
public:
	enum Flags {
	};
	

public:
	static constexpr const char* getFlagName(Flags flag) {
		return "";
	}
};
} // namespace Engine::Graphics::Shaders
