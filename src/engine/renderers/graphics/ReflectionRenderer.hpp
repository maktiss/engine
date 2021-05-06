#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class ReflectionRenderer : public GraphicsRendererBase {
private:
	struct ParamBlock {
		glm::mat4 invViewMatrix {};
		glm::mat4 invProjectionMatrix {};
	};

private:
	MeshManager::Handle mesh {};
	GraphicsShaderManager::Handle shaderHandle {};

	// TODO: destroy
	vk::Sampler vkSampler {};

	vk::ImageView vkDepthImageView {};
	vk::ImageView vkNormalImageView {};
	vk::ImageView vkEnvironmentImageView {};


public:
	ReflectionRenderer() : GraphicsRendererBase(3, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_REFLECTION";
	}


	virtual std::vector<std::string> getInputNames() const {
		return { "DepthBuffer", "NormalBuffer", "EnvironmentMap" };
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "ReflectionBuffer" };
	}


	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> inputDescriptions {};
		inputDescriptions.resize(3);

		inputDescriptions[0].format = vk::Format::eD24UnormS8Uint;
		inputDescriptions[0].usage	= vk::ImageUsageFlagBits::eSampled;

		inputDescriptions[1].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[1].usage	= vk::ImageUsageFlagBits::eSampled;

		inputDescriptions[2].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[2].usage	= vk::ImageUsageFlagBits::eSampled;
		inputDescriptions[2].flags	= vk::ImageCreateFlagBits::eCubeCompatible;

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
		std::vector<vk::ImageLayout> initialLayouts {};
		initialLayouts.resize(3);

		initialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[1] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[2] = vk::ImageLayout::eShaderReadOnlyOptimal;

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