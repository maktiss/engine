#pragma once

namespace Engine::Graphics::Materials {
template <typename DerivedMaterialType, typename ShaderType>
class MaterialBase {
public:
	using Flags = typename ShaderType::Flags;

public:
	// TODO: rename ComparableShaderType to something more meaningful?
	template <typename ComparableShaderType>
	static constexpr bool isShaderCompatible() {
		return std::is_same<ShaderType, ComparableShaderType>::value;
	}
};
} // namespace Engine::Graphics
