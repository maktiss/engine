#pragma once

#include "ShaderBase.hpp"


namespace Engine::Graphics::Shaders {
class SimpleShader : public ShaderBase<SimpleShader> {
public:
	enum Flags {
		USE_COLOR_DIFFUSE = 1 << 0,
		USE_COLOR_SPECULAR = 1 << 1,
	};

public:
	static inline const char* getFlagName(Flags flag) {
		switch (flag) {
			case Flags::USE_COLOR_DIFFUSE: return "USE_COLOR_DIFFUSE";
			case Flags::USE_COLOR_SPECULAR: return "USE_COLOR_SPECULAR";
		}

		return "";
	}

	static constexpr auto getFlagNames() {
		return std::array { "USE_COLOR_DIFFUSE", "USE_COLOR_SPECULAR" };
	}

	static constexpr uint32_t getFlagCount() {
		return getFlagNames().size();
	}
};
} // namespace Engine::Graphics::Shaders
