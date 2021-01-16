#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"


namespace Engine::Graphics {
class StagingBuffer {
private:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	vk::Buffer vkBuffer {};
	VmaAllocation vmaAllocation {};

	vk::DeviceSize vkBufferSize {};


public:
	[[nodiscard]] int init(vk::Device device, VmaAllocator allocator, vk::DeviceSize size);
	[[nodiscard]] int update(void* pData, uint64_t size);

	[[nodiscard]] int transfer(vk::Queue transferQueue, vk::CommandPool commandPool, vk::Buffer dstBuffer,
							   vk::DeviceSize size);

	[[nodiscard]] int transfer(vk::Queue transferQueue, vk::CommandPool commandPool, vk::Image dstImage,
							   vk::Extent3D size);

	void dispose();


private:
	[[nodiscard]] int beginCommandBuffer(vk::CommandPool commandPool, vk::CommandBuffer& commandBuffer);
	[[nodiscard]] int endCommandBuffer(vk::Queue transferQueue, vk::CommandPool commandPool,
									   vk::CommandBuffer& commandBuffer);
};
} // namespace Engine::Graphics