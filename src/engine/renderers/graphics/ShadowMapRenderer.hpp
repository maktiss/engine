#pragma once

#include "GraphicsRendererBase.hpp"
#include "ObjectRendererBase.hpp"

#include <map>


namespace Engine {
class ShadowMapRenderer : public ObjectRendererBase {
private:
	PROPERTY(float, "Graphics", directionalLightCascadeBase, 2.0f);
	PROPERTY(float, "Graphics", directionalLightCascadeOffset, 0.75f);

	PROPERTY(uint, "Graphics", directionalLightCascadeCount, 3);


public:
	ShadowMapRenderer() : ObjectRendererBase(0, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_SHADOW_MAP";
	}


	std::vector<std::string> getInputNames() const override {
		return {};
	}

	std::vector<std::string> getOutputNames() const override {
		return { "ShadowMap" };
	}


	inline uint getLayerCount() const override {
		return directionalLightCascadeCount;
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const override {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eD24UnormS8Uint;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eDepthStencilAttachment;

		return outputDescriptions;
	}

	std::vector<AttachmentDescription> getInputDescriptions() const override {
		std::vector<AttachmentDescription> descriptions {};
		return descriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const override {
		return std::vector<vk::ImageLayout>();
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const override {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return outputInitialLayouts;
	}


	std::vector<vk::ClearValue> getVkClearValues() const override {
		std::vector<vk::ClearValue> clearValues {};
		clearValues.resize(1);

		clearValues[0].depthStencil.depth = 1.0f;

		return clearValues;
	}


	std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() const override {
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


	vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() const override {
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = true;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = true;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = vk::CompareOp::eLess;

		return pipelineDepthStencilStateCreateInfo;
	}
};
} // namespace Engine