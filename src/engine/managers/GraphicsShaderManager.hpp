#pragma once

#include "GraphicsShaderManagerBase.hpp"

#include "engine/graphics/shaders/SimpleShader.hpp"
#include "engine/graphics/shaders/SkyboxShader.hpp"
#include "engine/graphics/shaders/SkymapShader.hpp"


namespace Engine::Managers {
class GraphicsShaderManager
	: public GraphicsShaderManagerBase<GraphicsShaderManager, Engine::Graphics::Shaders::SimpleShader,
									   Engine::Graphics::Shaders::SkyboxShader, Engine::Graphics::Shaders::SkymapShader> {
public:
	static int init();

	// TODO: generate dynamically
	static constexpr auto getRenderPassStrings() {
		return std::array { "RENDER_PASS_FORWARD", "RENDER_PASS_DEPTH_NORMAL", "RENDER_PASS_SHADOW_MAP",
							"RENDER_PASS_SKYBOX", "RENDER_PASS_SKYMAP", "RENDER_PASS_IMGUI" };
	}


private:
	GraphicsShaderManager() {
	}
};
} // namespace Engine::Managers