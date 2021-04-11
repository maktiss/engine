#pragma once

#include <array>

#include "engine/managers/TextureManager.hpp"


namespace Engine::Graphics::Materials {
template <typename DerivedMaterialType, typename ShaderType>
class MaterialBase {
public:
	using Flags				   = typename ShaderType::Flags;
	using MaterialUniformBlock = typename ShaderType::MaterialUniformBlock;


public:
	void writeBuffer(void* buffer) {
		static_cast<DerivedMaterialType*>(this)->writeMaterialUniformBlock(static_cast<MaterialUniformBlock*>(buffer));
	}


	static constexpr uint32_t getMaterialUniformBlockSize() {
		return sizeof(MaterialUniformBlock);
	}

	// TODO: rename ComparableShaderType to something more meaningful?
	template <typename ComparableShaderType>
	static constexpr bool isShaderCompatible() {
		return std::is_same<ShaderType, ComparableShaderType>::value;
	}
};
} // namespace Engine::Graphics::Materials
