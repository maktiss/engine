#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine::Renderers::Graphics {
class DepthNormalRenderer : public GraphicsRendererBase {
public:
	DepthNormalRenderer() : GraphicsRendererBase(0, 1) {
	}


	int init() override;

	void recordCommandBuffer(double dt, vk::CommandBuffer& commandBuffer) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_DEPTH_NORMAL";
	}
};
} // namespace Engine::Renderers::Graphics