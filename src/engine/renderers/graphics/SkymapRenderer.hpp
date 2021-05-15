#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class SkymapRenderer : public GraphicsRendererBase {
private:
	struct CameraBlock {
		glm::mat4 viewProjectionMatrices[6];
	};


private:
	MeshManager::Handle skySphereMesh {};
	GraphicsShaderManager::Handle shaderHandle {};


public:
	SkymapRenderer() : GraphicsRendererBase(0, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_SKYMAP";
	}


	virtual std::vector<std::string> getInputNames() const {
		return {};
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "SkyMap" };
	}


	inline uint getMultiviewLayerCount() const override {
		return 6;
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		return outputDescriptions;
	}


	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;

		return outputInitialLayouts;
	}


	std::vector<DescriptorSetDescription> getDescriptorSetDescriptions() const {
		std::vector<DescriptorSetDescription> descriptorSetDescriptions {};

		descriptorSetDescriptions.push_back({ 0, 0, vk::DescriptorType::eUniformBuffer, sizeof(CameraBlock) });
		descriptorSetDescriptions.push_back({ 1, 0, vk::DescriptorType::eUniformBuffer, 16 });

		return descriptorSetDescriptions;
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