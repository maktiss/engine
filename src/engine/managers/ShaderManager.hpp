#pragma once

#include "ShaderManagerBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"

#include <array>
#include <string>


namespace Engine::Managers {
class ShaderManager : public ShaderManagerBase<ShaderManager, Engine::Graphics::Shaders::SimpleShader> {
public:
	static int init() {
		spdlog::info("Initializing ShaderManager...");
		return ShaderManagerBase::init();
	}

	static constexpr auto getRenderPassStrings() {
		return std::array { "RENDER_PASS_GEOMETRY" };
	}


private:
	ShaderManager() {
	}
};
} // namespace Engine::Managers