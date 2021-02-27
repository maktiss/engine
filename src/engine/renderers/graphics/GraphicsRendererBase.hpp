#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine::Renderers::Graphics {
class GraphicsRendererBase : public Engine::Renderers::RendererBase {
protected:
	vk::RenderPass vkRenderPass {};
	std::vector<vk::Framebuffer> vkFramebuffers {};


public:
	GraphicsRendererBase(uint inputCount, uint outputCount, uint layerCount = 1) :
		Engine::Renderers::RendererBase(inputCount, outputCount, layerCount) {
	}


	virtual int init();

	// Records secondary command buffers within renderpass
	virtual void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
											   double dt) = 0;

	virtual const char* getRenderPassName() const = 0;


	virtual int render(const vk::CommandBuffer* pPrimaryCommandBuffers,
					   const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) override;


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