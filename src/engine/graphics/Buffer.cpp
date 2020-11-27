#include "Buffer.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Graphics {
vk::Result Buffer::allocate(VmaAllocator allocator, vk::DeviceSize bufferSize) {
	vmaAllocator = allocator;
	vkBufferSize = bufferSize;

	VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size  = VkDeviceSize(vkBufferSize);
	bufferCreateInfo.usage = VkBufferUsageFlags(vkBufferUsageFlags);

	VmaAllocationCreateInfo allocationCreateInfo {};
	allocationCreateInfo.usage = vmaMemoryUsage;

	VkBuffer buffer;
	auto result =
		vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &vmaAllocation, nullptr);
	if (result != VK_SUCCESS) {
		return vk::Result(result);
	}
	vkBuffer = vk::Buffer(buffer);

	return vk::Result::eSuccess;
}

vk::Result Buffer::write(void* data) {
	void* pBufferData;
	auto result = vmaMapMemory(vmaAllocator, vmaAllocation, &pBufferData);
	if (result != VK_SUCCESS) {
		return vk::Result(result);
	}

	memcpy(pBufferData, data, vkBufferSize);
	vmaUnmapMemory(vmaAllocator, vmaAllocation);

	return vk::Result::eSuccess;
}

void Buffer::destroy() {
	if (vmaAllocator != nullptr) {
		vmaDestroyBuffer(vmaAllocator, vkBuffer, vmaAllocation);
	}
}
} // namespace Engine::Graphics