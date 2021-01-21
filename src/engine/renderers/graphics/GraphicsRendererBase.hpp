#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine::Renderers::Graphics {
class GraphicsRendererBase : public Engine::Renderers::RendererBase {
protected:
	vk::RenderPass vkRenderPass {};
	vk::Framebuffer vkFramebuffer {};


public:
	GraphicsRendererBase(uint inputCount, uint outputCount) :
		Engine::Renderers::RendererBase(inputCount, outputCount) {
	}


	virtual int init();

	// Records command buffer within renderpass
	virtual void recordCommandBuffer(double dt, vk::CommandBuffer& commandBuffer) = 0;

	virtual const char* getRenderPassName() const = 0;


	int render(vk::CommandBuffer commandBuffer, double dt) override;


	int createRenderPass();
	int createFramebuffer();
	int createGraphicsPipelines();


	virtual std::vector<vk::ClearValue> getVkClearValues() {
		return std::vector<vk::ClearValue>();
	}


	virtual std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() {
		return std::vector<vk::AttachmentDescription>();
	}

	virtual std::vector<vk::AttachmentReference> getVkAttachmentReferences() {
		return std::vector<vk::AttachmentReference>();
	}


	virtual vk::PipelineInputAssemblyStateCreateInfo getVkPipelineInputAssemblyStateCreateInfo() {
		return vk::PipelineInputAssemblyStateCreateInfo();
	}

	virtual vk::Viewport getVkViewport() {
		return vk::Viewport();
	}

	virtual vk::Rect2D getVkScissor() {
		return vk::Rect2D();
	}

	virtual vk::PipelineRasterizationStateCreateInfo getVkPipelineRasterizationStateCreateInfo() {
		return vk::PipelineRasterizationStateCreateInfo();
	}

	virtual vk::PipelineMultisampleStateCreateInfo getVkPipelineMultisampleStateCreateInfo() {
		return vk::PipelineMultisampleStateCreateInfo();
	}

	virtual vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() {
		return vk::PipelineDepthStencilStateCreateInfo();
	}

	virtual vk::PipelineColorBlendAttachmentState getVkPipelineColorBlendAttachmentState() {
		return vk::PipelineColorBlendAttachmentState();
	}
};
} // namespace Engine::Renderers::Graphics