#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine::Renderers::Graphics {
class ShadowMapRenderer : public GraphicsRendererBase {
private:
	PROPERTY(float, "Graphics", directionalLightCascadeBase, 2.0f);
	PROPERTY(float, "Graphics", directionalLightCascadeOffset, 0.75f);


public:
	ShadowMapRenderer() : GraphicsRendererBase(0, 1, 3) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_SHADOW_MAP";
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eD24UnormS8Uint;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eDepthStencilAttachment;

		return outputDescriptions;
	}

	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> descriptions {};
		return descriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		return std::vector<vk::ImageLayout>();
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return outputInitialLayouts;
	}


	inline std::vector<vk::ClearValue> getVkClearValues() override {
		std::vector<vk::ClearValue> clearValues {};
		clearValues.resize(1);

		clearValues[0].depthStencil.depth = 1.0f;

		return clearValues;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() override {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(1);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eClear;
		attachmentDescriptions[0].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[0].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		return attachmentDescriptions;
	}

	inline std::vector<vk::AttachmentReference> getVkAttachmentReferences() override {
		std::vector<vk::AttachmentReference> attachmentReferences {};
		attachmentReferences.resize(1);

		attachmentReferences[0].attachment = 0;
		attachmentReferences[0].layout	   = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return attachmentReferences;
	}


	inline vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() override {
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = true;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = true;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = vk::CompareOp::eLess;

		return pipelineDepthStencilStateCreateInfo;
	}
};
} // namespace Engine::Renderers::Graphics