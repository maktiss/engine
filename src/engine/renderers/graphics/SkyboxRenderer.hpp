#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class SkyboxRenderer : public GraphicsRendererBase {
private:
	MeshManager::Handle boxMesh {};
	GraphicsShaderManager::Handle shaderHandle {};

	// TODO: destroy
	vk::Sampler vkSampler {};
	vk::ImageView vkImageView {};


public:
	SkyboxRenderer() : GraphicsRendererBase(1, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_SKYBOX";
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		return outputDescriptions;
	}

	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> descriptions {};
		descriptions.resize(1);

		descriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
		descriptions[0].usage  = vk::ImageUsageFlagBits::eSampled;
		descriptions[0].flags  = vk::ImageCreateFlagBits::eCubeCompatible;

		return descriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		std::vector<vk::ImageLayout> initialLayouts {};
		initialLayouts.resize(1);

		initialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;

		return initialLayouts;
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;

		return outputInitialLayouts;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() const override {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(1);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[0].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		return attachmentDescriptions;
	}
};
} // namespace Engine