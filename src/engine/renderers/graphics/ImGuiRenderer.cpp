#include "ImGuiRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "thirdparty/imgui/imgui_impl_vulkan.h"

#include <glm/glm.hpp>


namespace Engine {
int ImGuiRenderer::init() {
	spdlog::info("Initializing ImGuiRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());

	assert(vkInstance != vk::Instance());
	assert(vkPhysicalDevice != vk::PhysicalDevice());
	assert(vkGraphicsQueue != vk::Queue());

	{
		auto result = GraphicsRendererBase::init();
		if (result) {
			return result;
		}
	}

	vk::DescriptorPoolSize poolSizes[] = {
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 },
	};

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo {};
	descriptorPoolCreateInfo.flags		   = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	descriptorPoolCreateInfo.maxSets	   = 1000 * 11;
	descriptorPoolCreateInfo.poolSizeCount = 11;
	descriptorPoolCreateInfo.pPoolSizes	   = poolSizes;

	auto result = vkDevice.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ImGuiRenderer] Failed to create descriptor pool. Error code: {} ({})", result,
					  vk::to_string(result));
		return 1;
	}


	ImGui_ImplVulkan_InitInfo initInfo {};
	initInfo.Instance		 = vkInstance;
	initInfo.PhysicalDevice	 = vkPhysicalDevice;
	initInfo.Device			 = vkDevice;
	initInfo.Queue			 = vkGraphicsQueue;
	initInfo.DescriptorPool	 = vkDescriptorPool;
	initInfo.MinImageCount	 = framesInFlightCount;
	initInfo.ImageCount		 = framesInFlightCount;
	initInfo.CheckVkResultFn = imGuiErrorCallback;

	ImGui_ImplVulkan_Init(&initInfo, vkRenderPass);


	if (oneTimeCommandBuffer.init(vkDevice, vkGraphicsQueue, vkQueueFamily)) {
		return 1;
	}

	if (oneTimeCommandBuffer.begin()) {
		return 1;
	}
	ImGui_ImplVulkan_CreateFontsTexture(oneTimeCommandBuffer.get());
	if (oneTimeCommandBuffer.end()) {
		return 1;
	}

	return 0;
}


void ImGuiRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
												  uint descriptorSetIndex, double dt) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}


void ImGuiRenderer::imGuiErrorCallback(VkResult result) {
	if (result != VK_SUCCESS) {
		spdlog::error("[ImGuiRenderer] Error code: {} ()", result, vk::to_string(vk::Result(result)));
	}
}
} // namespace Engine