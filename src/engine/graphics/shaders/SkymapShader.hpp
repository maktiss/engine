#pragma once

#include "GraphicsShaderBase.hpp"

#include <glm/glm.hpp>


namespace Engine {
class SkymapShader : public GraphicsShaderBase<SkymapShader> {
public:
	enum Flags {};


public:
	static constexpr const char* getFlagName(Flags flag) {
		return "";
	}
};
} // namespace Engine