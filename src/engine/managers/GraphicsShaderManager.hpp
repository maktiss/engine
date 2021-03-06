#pragma once

#include "GraphicsShaderManagerBase.hpp"

#include "engine/graphics/shaders/Shaders.hpp"


namespace Engine {
class GraphicsShaderManager
	: public GraphicsShaderManagerBase<GraphicsShaderManager, SimpleShader, SkyboxShader, SkymapShader, PostFxShader,
									   ReflectionShader, IrradianceShader, BoxBlurShader, VolumetricLightShader> {
public:
	static int init();

	// TODO: generate dynamically
	static constexpr auto getRenderPassStrings() {
		return std::array { "RENDER_PASS_FORWARD",	"RENDER_PASS_DEPTH_NORMAL",	   "RENDER_PASS_SHADOW_MAP",
							"RENDER_PASS_SKYBOX",	"RENDER_PASS_SKYMAP",		   "RENDER_PASS_IMGUI",
							"RENDER_PASS_POSTFX",	"RENDER_PASS_REFLECTION",	   "RENDER_PASS_IRRADIANCE_MAP",
							"RENDER_PASS_BOX_BLUR", "RENDER_PASS_VOLUMETRIC_LIGHT" };
	}


private:
	GraphicsShaderManager() {
	}
};
} // namespace Engine