#pragma once

#include "GraphicsRendererBase.hpp"

#include "engine/graphics/OneTimeCommandBuffer.hpp"


namespace Engine {
class ImGuiRenderer : public GraphicsRendererBase {
private:
	vk::Instance vkInstance {};
	vk::PhysicalDevice vkPhysicalDevice {};
	uint32_t vkQueueFamily {};
	vk::Queue vkGraphicsQueue {};

	// TODO: destroy
	vk::DescriptorPool vkDescriptorPool {};

	OneTimeCommandBuffer oneTimeCommandBuffer {};


public:
	ImGuiRenderer() : GraphicsRendererBase(0, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_IMGUI";
	}


	void setVkInstance(const vk::Instance& instance) {
		vkInstance = instance;
	}

	void setVkPhysicalDevice(const vk::PhysicalDevice& physicalDevice) {
		vkPhysicalDevice = physicalDevice;
	}

	void setVkQueueFamily(const uint32_t& queueFamily) {
		vkQueueFamily = queueFamily;
	}

	void setVkGraphicsQueue(const vk::Queue& queue) {
		vkGraphicsQueue = queue;
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eR8G8B8A8Srgb;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		return outputDescriptions;
	}


	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;

		return outputInitialLayouts;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() override {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(1);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eLoad;
		attachmentDescriptions[0].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[0].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		return attachmentDescriptions;
	}

	inline std::vector<vk::AttachmentReference> getVkAttachmentReferences() override {
		std::vector<vk::AttachmentReference> attachmentReferences {};
		attachmentReferences.resize(1);

		attachmentReferences[0].attachment = 0;
		attachmentReferences[0].layout	   = vk::ImageLayout::eColorAttachmentOptimal;

		return attachmentReferences;
	}


	static void imGuiErrorCallback(VkResult result);
};
} // namespace Engine