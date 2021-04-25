#pragma once

#include <array>
#include <cstdint>
#include <vector>


namespace Engine {
class Shader {
public:
	enum Type {
		VERTEX		 = 0,
		GEOMETRY	 = 1,
		TESS_CONTROL = 2,
		TESS_EVAL	 = 3,
		FRAGMENT	 = 4,
		COMPUTE		 = 5,
	};

private:
	// SPIR-V sources
	std::array<std::vector<uint8_t>, 6> shaderSources;

public:
	// sets SPIR-V shader source for specified stage
	inline void setShaderSource(Type shaderStage, const void* data, uint32_t size) {
		shaderSources[shaderStage] =
			std::vector<uint8_t>(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
	}

	// returns SPIR-V shader source for specified stage
	inline std::vector<uint8_t>& getShaderSource(Type shaderStage) {
		return shaderSources[shaderStage];
	}
};
} // namespace Engine