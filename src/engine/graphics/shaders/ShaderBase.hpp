#pragma once

#include <array>
#include <string>
#include <vector>


namespace Engine::Graphics::Shaders {
template <typename DerivedShader>
class ShaderBase {
private:
	// SPIR-V sources
	std::array<std::vector<uint8_t>, 6> shaderSources;

public:
	// sets SPIR-V shader source for specified stage
	inline void setShaderSource(uint shaderStageIndex, const void* data, uint32_t size) {
		shaderSources[shaderStageIndex] =
			std::vector<uint8_t>(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
	}

	// returns SPIR-V shader source for specified stage
	inline std::vector<uint8_t>& getShaderSource(uint shaderStageIndex) {
		return shaderSources[shaderStageIndex];
	}
};
} // namespace Engine::Graphics::Shaders
