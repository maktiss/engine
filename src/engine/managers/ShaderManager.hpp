#pragma once

#include "ShaderManagerBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"


namespace Engine::Managers {
class ShaderManager : public ShaderManagerBase<ShaderManager, Engine::Graphics::Shaders::SimpleShader> {
public:
	static int init();

	static constexpr auto getRenderPassStrings() {
		return std::array { "RENDER_PASS_FORWARD", "RED", "RENDER_PASS_DEPTH_NORMAL", "GREEN", "BLUE" };
	}


private:
	ShaderManager() {
	}
};
} // namespace Engine::Managers