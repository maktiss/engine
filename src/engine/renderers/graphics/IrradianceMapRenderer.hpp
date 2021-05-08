#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class IrradianceMapRenderer : public GraphicsRendererBase {
private:
	struct CameraBlock {
		glm::mat4 viewProjectionMatrices[6] {};
	};


private:
	MeshManager::Handle boxMesh {};
	GraphicsShaderManager::Handle shaderHandle {};

	// TODO: destroy
	vk::Sampler vkSampler {};
	vk::ImageView vkImageView {};

	PROPERTY(uint, "Graphics", irradianceMapSampleCount, 64);


public:
	IrradianceMapRenderer() : GraphicsRendererBase(1, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_IRRADIANCE_MAP";
	}


	virtual std::vector<std::string> getInputNames() const {
		return { "EnvironmentMap" };
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "IrradianceMap" };
	}


	inline uint getMultiviewLayerCount() const override {
		return 6;
	}


	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> inputDescriptions {};
		inputDescriptions.resize(1);

		inputDescriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[0].usage = vk::ImageUsageFlagBits::eSampled;
		inputDescriptions[0].flags = vk::ImageCreateFlagBits::eCubeCompatible;

		return inputDescriptions;
	}

	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		return outputDescriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		std::vector<vk::ImageLayout> inputInitialLayouts {};
		inputInitialLayouts.resize(1);

		inputInitialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;

		return inputInitialLayouts;
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