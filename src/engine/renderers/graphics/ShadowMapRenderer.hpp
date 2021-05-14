#pragma once

#include "GraphicsRendererBase.hpp"
#include "ObjectRendererBase.hpp"

#include <map>


namespace Engine {
class ShadowMapRenderer : public ObjectRendererBase {
private:
	PROPERTY(uint, "Graphics", directionalLightCascadeCount, 3);

	PROPERTY(float, "Graphics", directionalLightCascadeBase, 2.0f);
	PROPERTY(float, "Graphics", directionalLightCascadeOffset, 0.75f);


private:
	std::vector<Frustum> excludeFrustums {};


public:
	ShadowMapRenderer() : ObjectRendererBase(0, 2) {
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
		return { "ShadowMap", "DepthBuffer" };
	}


	inline uint getLayerCount() const override {
		return directionalLightCascadeCount;
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const override {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(2);

		outputDescriptions[0].format = vk::Format::eR32G32Sfloat;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		outputDescriptions[1].format = vk::Format::eD24UnormS8Uint;
		outputDescriptions[1].usage	 = vk::ImageUsageFlagBits::eDepthStencilAttachment;

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
		outputInitialLayouts.resize(2);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;
		outputInitialLayouts[1] = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return outputInitialLayouts;
	}


	std::vector<vk::ClearValue> getVkClearValues() const override {
		std::vector<vk::ClearValue> clearValues {};
		clearValues.resize(2);

		clearValues[0].color			  = std::array { 1.0f, 1.0f, 1.0f, 1.0f };
		clearValues[1].depthStencil.depth = 1.0f;

		return clearValues;
	}


	std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() const override {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(2);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eClear;
		attachmentDescriptions[0].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[0].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		attachmentDescriptions[1].format		 = outputDescriptions[1].format;
		attachmentDescriptions[1].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[1].loadOp		 = vk::AttachmentLoadOp::eClear;
		attachmentDescriptions[1].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[1].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

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