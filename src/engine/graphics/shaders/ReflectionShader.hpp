#pragma once

#include "GraphicsShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine {
class ReflectionShader : public GraphicsShaderBase<ReflectionShader> {
public:
	enum Flags {};


public:
	static constexpr const char* getFlagName(Flags flag) {
		return "";
	}
};
} // namespace Engine