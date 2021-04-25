#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include <spdlog/spdlog.h>


namespace Engine {
class OneTimeCommandBuffer {
private:
	vk::Device vkDevice {};
	vk::Queue vkQueue {};

	vk::CommandPool vkCommandPool {};
	vk::CommandBuffer vkCommandBuffer {};


public:
	[[nodiscard]] inline int init(vk::Device device, vk::Queue queue, uint32_t queueFamilyIndex) {
		vkDevice = device;
		vkQueue	 = queue;

		vk::CommandPoolCreateInfo commandPoolCreateInfo {};
		commandPoolCreateInfo.flags			   = vk::CommandPoolCreateFlagBits::eTransient;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

		auto result = vkDevice.createCommandPool(&commandPoolCreateInfo, nullptr, &vkCommandPool);

		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to create command pool for one-time command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return 1;
		}

		return 0;
	}

	inline auto get() {
		return vkCommandBuffer;
	}


	[[nodiscard]] inline int begin() {
		assert(vkDevice != vk::Device());
		assert(vkCommandPool != vk::CommandPool());

		vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
		commandBufferAllocateInfo.level				 = vk::CommandBufferLevel::ePrimary;
		commandBufferAllocateInfo.commandPool		 = vkCommandPool;
		commandBufferAllocateInfo.commandBufferCount = 1;

		auto result = vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &vkCommandBuffer);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to allocate one-time command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return 1;
		}

		vk::CommandBufferBeginInfo commandBufferBeginInfo {};
		commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		result = vkCommandBuffer.begin(&commandBufferBeginInfo);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record one-time command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return 1;
		}
	}

	[[nodiscard]] inline int end() {
		auto result = vkCommandBuffer.end();
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record one-time command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return 1;
		}

		vk::SubmitInfo submitInfo {};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers	  = &vkCommandBuffer;

		result = vkQueue.submit(1, &submitInfo, nullptr);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to submit one-time buffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}

		result = vkQueue.waitIdle();
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to wait for one-time command buffer to complete. Error code: {} ({})", result,
						  vk::to_string(result));
			return 1;
		}

		vkDevice.freeCommandBuffers(vkCommandPool, 1, &vkCommandBuffer);

		return 0;
	}


	void dispose() {
		if (vkCommandPool != vk::CommandPool()) {
			vkDevice.destroyCommandPool(vkCommandPool);
		}
	}
};
} // namespace Engine