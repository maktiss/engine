#include "Buffer.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
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

vk::Result Buffer::writeStaged(vk::Device device, vk::Queue transferQueue, vk::CommandPool commandPool, void* data) {
	Buffer stagingBuffer = { vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU };

	stagingBuffer.allocate(vmaAllocator, vkBufferSize);
	// RETURN_IF_VK_ERROR(stagingBuffer.allocate(), "Failed to allocate buffer");
	stagingBuffer.write(data);
	// RETURN_IF_VK_ERROR(stagingBuffer.write(mesh.getVertexBuffer().data()), "Failed to write buffer");

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.level				 = vk::CommandBufferLevel::ePrimary;
	commandBufferAllocateInfo.commandPool		 = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	vk::CommandBuffer stagingCommandBuffer;
	device.allocateCommandBuffers(&commandBufferAllocateInfo, &stagingCommandBuffer);
	// RETURN_IF_VK_ERROR(vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &stagingCommandBuffer),
	// 				"Failed to allocate staging command buffer");

	vk::CommandBufferBeginInfo commandBufferBeginInfo {};
	commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	stagingCommandBuffer.begin(&commandBufferBeginInfo);
	// RETURN_IF_VK_ERROR(stagingCommandBuffer.begin(&commandBufferBeginInfo), "Failed to record command
	// buffer");

	vk::BufferCopy bufferCopy {};
	bufferCopy.size = vkBufferSize;
	stagingCommandBuffer.copyBuffer(stagingBuffer.getVkBuffer(), vkBuffer, 1, &bufferCopy);

	stagingCommandBuffer.end();

	vk::SubmitInfo submitInfo {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &stagingCommandBuffer;
	transferQueue.submit(1, &submitInfo, nullptr);
	// RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, nullptr), "Failed to submit staging command
	// buffer");
	transferQueue.waitIdle();
	// RETURN_IF_VK_ERROR(vkGraphicsQueue.waitIdle(), "Failed to wait for staging command buffer to complete");

	stagingBuffer.destroy();

	return vk::Result::eSuccess;
}


void Buffer::destroy() {
	if (vmaAllocator != nullptr) {
		vmaDestroyBuffer(vmaAllocator, vkBuffer, vmaAllocation);
	}
}
} // namespace Engine