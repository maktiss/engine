#pragma once

#include "GraphicsShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine {
class PostFxShader : public GraphicsShaderBase<PostFxShader> {
public:
	enum Flags {};


public:
	static constexpr const char* getFlagName(Flags flag) {
		return "";
	}
};
} // namespace Engine