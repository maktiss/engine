#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine {
class GraphicsRendererBase : public RendererBase {
protected:
	vk::RenderPass vkRenderPass {};
	std::vector<vk::Framebuffer> vkFramebuffers {};


public:
	GraphicsRendererBase(uint inputCount, uint outputCount) : RendererBase(inputCount, outputCount) {
	}


	virtual int init();

	// Records secondary command buffers within renderpass
	virtual void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
											   double dt) = 0;

	virtual const char* getRenderPassName() const = 0;


	virtual int render(const vk::CommandBuffer* pPrimaryCommandBuffers,
					   const vk::CommandBuffer* pSecondaryCommandBuffers, const vk::QueryPool& timestampQueryPool,
					   double dt) override;


	virtual std::vector<vk::DescriptorSetLayout> getVkDescriptorSetLayouts() override {
		auto layouts = RendererBase::getVkDescriptorSetLayouts();
		layouts.push_back(MaterialManager::getVkDescriptorSetLayout());
		layouts.push_back(TextureManager::getVkDescriptorSetLayout());
		return layouts;
	}


	uint getColorAttachmentCount() const;

	std::vector<vk::AttachmentReference> getVkAttachmentReferences() const;


	int createRenderPass();
	int createFramebuffer();
	int createGraphicsPipelines();


	virtual std::vector<vk::ClearValue> getVkClearValues() const {
		return std::vector<vk::ClearValue>();
	}


	virtual std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() const {
		return std::vector<vk::AttachmentDescription>();
	}


	virtual vk::PipelineInputAssemblyStateCreateInfo getVkPipelineInputAssemblyStateCreateInfo() const {
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
		pipelineInputAssemblyStateCreateInfo.topology				= vk::PrimitiveTopology::eTriangleList;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = false;

		return pipelineInputAssemblyStateCreateInfo;
	}

	virtual vk::Viewport getVkViewport() const {
		vk::Viewport viewport {};
		viewport.x		  = 0.0f;
		viewport.y		  = outputSize.height;
		viewport.width	  = outputSize.width;
		viewport.height	  = -(static_cast<int>(outputSize.height));
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return viewport;
	}

	virtual vk::Rect2D getVkScissor() const {
		vk::Rect2D scissor {};
		scissor.offset = vk::Offset2D(0, 0);
		scissor.extent = outputSize;

		return scissor;
	}

	virtual vk::PipelineRasterizationStateCreateInfo getVkPipelineRasterizationStateCreateInfo() const {
		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
		pipelineRasterizationStateCreateInfo.depthClampEnable		 = false;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = false;
		pipelineRasterizationStateCreateInfo.polygonMode			 = vk::PolygonMode::eFill;
		pipelineRasterizationStateCreateInfo.cullMode				 = vk::CullModeFlagBits::eBack;
		pipelineRasterizationStateCreateInfo.frontFace				 = vk::FrontFace::eCounterClockwise;
		pipelineRasterizationStateCreateInfo.depthBiasEnable		 = false;
		pipelineRasterizationStateCreateInfo.lineWidth				 = 1.0f;

		return pipelineRasterizationStateCreateInfo;
	}

	virtual vk::PipelineMultisampleStateCreateInfo getVkPipelineMultisampleStateCreateInfo() const {
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
		pipelineMultisampleStateCreateInfo.sampleShadingEnable	= false;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

		return pipelineMultisampleStateCreateInfo;
	}

	virtual vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() const {
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = false;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = false;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = vk::CompareOp::eAlways;

		return pipelineDepthStencilStateCreateInfo;
	}

	virtual std::vector<vk::PipelineColorBlendAttachmentState> getVkPipelineColorBlendAttachmentStates() const {
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
		pipelineColorBlendAttachmentState.colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		pipelineColorBlendAttachmentState.blendEnable = false;

		std::vector<vk::PipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates {};
		pipelineColorBlendAttachmentStates.resize(getColorAttachmentCount(), pipelineColorBlendAttachmentState);

		return pipelineColorBlendAttachmentStates;
	}
};
} // namespace Engine