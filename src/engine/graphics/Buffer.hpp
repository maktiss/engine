#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"


namespace Engine::Graphics {
class Buffer {
private:
	VmaAllocator vmaAllocator = nullptr;

	vk::Buffer vkBuffer;
	VmaAllocation vmaAllocation = nullptr;

	vk::DeviceSize vkBufferSize = 0;

	vk::BufferUsageFlags vkBufferUsageFlags;
	VmaMemoryUsage vmaMemoryUsage;

public:
	Buffer(vk::BufferUsageFlags bufferUsageFlags,
		   VmaMemoryUsage memoryUsage) : vkBufferUsageFlags(bufferUsageFlags), vmaMemoryUsage(memoryUsage) {
	}

	[[nodiscard]] vk::Result allocate(VmaAllocator allocator, vk::DeviceSize bufferSize);

	[[nodiscard]] vk::Result write(void* data);

	void destroy();

	inline vk::Buffer getVkBuffer() const {
		return vkBuffer;
	}
};
} // namespace Engine::Graphics