#pragma once

#include <array>
#include <string>
#include <vector>


namespace Engine {
template <typename DerivedShader>
class GraphicsShaderBase {
private:
	// SPIR-V sources
	std::array<std::vector<uint8_t>, 6> shaderSources;


public:
	// sets SPIR-V shader source for specified stage
	inline void setShaderSource(uint shaderStage, const void* data, uint32_t size) {
		shaderSources[shaderStage] =
			std::vector<uint8_t>(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
	}

	// returns SPIR-V shader source for specified stage
	inline std::vector<uint8_t>& getShaderSource(uint shaderStage) {
		return shaderSources[shaderStage];
	}


	static constexpr auto getFlagNames() {
		std::array<const char*, getFlagCount()> flagNames {};

		for (uint i = 0; i < getFlagCount(); i++) {
			flagNames[i] = DerivedShader::getFlagName((typename DerivedShader::Flags)(1 << i));
		}

		return flagNames;
	}

	static constexpr uint32_t getFlagCount() {
		uint flagCount = 0;
		for (uint i = 0; i < 16; i++) {
			if (strlen(DerivedShader::getFlagName((typename DerivedShader::Flags)(1 << i)))) {
				flagCount++;
			} else {
				break;
			}
		}

		return flagCount;
	}

	static constexpr uint32_t getSignatureCount() {
		return pow(2, getFlagCount());
	}


	static constexpr uint32_t getMaterialUniformBlockSize() {
		return sizeof(typename DerivedShader::MaterialUniformBlock);
	}
};
} // namespace Engine