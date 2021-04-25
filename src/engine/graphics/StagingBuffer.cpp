#include "StagingBuffer.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int StagingBuffer::init(vk::Device device, VmaAllocator allocator, vk::DeviceSize size) {
	vkDevice	 = device;
	vmaAllocator = allocator;
	vkBufferSize = size;

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);
	assert(vkBufferSize > 0);

	vk::BufferCreateInfo bufferCreateInfo {};
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	bufferCreateInfo.size  = vkBufferSize;

	VkBufferCreateInfo cBufferCreateInfo(bufferCreateInfo);

	VmaAllocationCreateInfo allocationCreateInfo {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VkBuffer buffer;
	auto result = vk::Result(
		vmaCreateBuffer(vmaAllocator, &cBufferCreateInfo, &allocationCreateInfo, &buffer, &vmaAllocation, nullptr));
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to allocate buffer memory. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	vkBuffer = vk::Buffer(buffer);

	return 0;
}

int StagingBuffer::update(void* pData, uint64_t size) {
	assert(size <= vkBufferSize);

	void* pBufferData;
	auto result = vk::Result(vmaMapMemory(vmaAllocator, vmaAllocation, &pBufferData));
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to write buffer memory. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	memcpy(pBufferData, pData, size);
	vmaUnmapMemory(vmaAllocator, vmaAllocation);
	return 0;
}


int StagingBuffer::transfer(vk::Queue transferQueue, vk::CommandPool commandPool, vk::Buffer dstBuffer,
							vk::DeviceSize size) {
	vk::CommandBuffer commandBuffer;
	if (beginCommandBuffer(commandPool, commandBuffer)) {
		return 1;
	}

	vk::BufferCopy bufferCopy {};
	bufferCopy.size = vkBufferSize;
	commandBuffer.copyBuffer(vkBuffer, dstBuffer, 1, &bufferCopy);

	if (endCommandBuffer(transferQueue, commandPool, commandBuffer)) {
		return 1;
	}

	return 0;
}

int StagingBuffer::transfer(vk::Queue transferQueue, vk::CommandPool commandPool, vk::Image dstImage,
							vk::Extent3D size) {
	vk::CommandBuffer commandBuffer;
	if (beginCommandBuffer(commandPool, commandBuffer)) {
		return 1;
	}


	vk::ImageMemoryBarrier imageMemoryBarrierBefore {};
	imageMemoryBarrierBefore.oldLayout					   = vk::ImageLayout::eUndefined;
	imageMemoryBarrierBefore.newLayout					   = vk::ImageLayout::eTransferDstOptimal;
	imageMemoryBarrierBefore.srcAccessMask				   = {};
	imageMemoryBarrierBefore.dstAccessMask				   = vk::AccessFlagBits::eTransferWrite;
	imageMemoryBarrierBefore.image						   = dstImage;
	imageMemoryBarrierBefore.subresourceRange.aspectMask   = vk::ImageAspectFlagBits::eColor;
	imageMemoryBarrierBefore.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrierBefore.subresourceRange.levelCount   = 1;
	imageMemoryBarrierBefore.subresourceRange.layerCount   = 1;

	vk::ImageMemoryBarrier imageMemoryBarrierAfter {};
	imageMemoryBarrierAfter.oldLayout					  = vk::ImageLayout::eTransferDstOptimal;
	imageMemoryBarrierAfter.newLayout					  = vk::ImageLayout::eTransferSrcOptimal;
	imageMemoryBarrierAfter.srcAccessMask				  = vk::AccessFlagBits::eTransferWrite;
	imageMemoryBarrierAfter.dstAccessMask				  = vk::AccessFlagBits::eTransferRead;
	imageMemoryBarrierAfter.image						  = dstImage;
	imageMemoryBarrierAfter.subresourceRange.aspectMask	  = vk::ImageAspectFlagBits::eColor;
	imageMemoryBarrierAfter.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrierAfter.subresourceRange.levelCount	  = 1;
	imageMemoryBarrierAfter.subresourceRange.layerCount	  = 1;


	vk::BufferImageCopy region {};
	region.bufferOffset		 = 0;
	region.bufferRowLength	 = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask	   = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel	   = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount	   = 1;

	region.imageOffset = vk::Offset3D(0, 0, 0);
	region.imageExtent = size;


	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0,
								  nullptr, 0, nullptr, 1, &imageMemoryBarrierBefore);

	commandBuffer.copyBufferToImage(vkBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &region);

	// commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
	// 							  0, nullptr, 0, nullptr, 1, &imageMemoryBarrierAfter);


	if (endCommandBuffer(transferQueue, commandPool, commandBuffer)) {
		return 1;
	}

	return 0;
}


void StagingBuffer::dispose() {
	if (vmaAllocation != nullptr) {
		vmaDestroyBuffer(vmaAllocator, vkBuffer, vmaAllocation);
		vmaAllocation = nullptr;
	}
}


int StagingBuffer::beginCommandBuffer(vk::CommandPool commandPool, vk::CommandBuffer& commandBuffer) {
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.level				 = vk::CommandBufferLevel::ePrimary;
	commandBufferAllocateInfo.commandPool		 = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	auto result = vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to allocate staging command buffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	vk::CommandBufferBeginInfo commandBufferBeginInfo {};
	commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	result = commandBuffer.begin(&commandBufferBeginInfo);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to record staging command buffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	return 0;
}

int StagingBuffer::endCommandBuffer(vk::Queue transferQueue, vk::CommandPool commandPool,
									vk::CommandBuffer& commandBuffer) {
	auto result = commandBuffer.end();
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to record staging command buffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	vk::SubmitInfo submitInfo {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &commandBuffer;

	result = transferQueue.submit(1, &submitInfo, nullptr);
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to submit staging buffer. Error code: {} ({})", result, vk::to_string(result));
		return 1;
	}

	result = transferQueue.waitIdle();
	if (result != vk::Result::eSuccess) {
		spdlog::error("Failed to wait for staging command buffer to complete. Error code: {} ({})", result,
					  vk::to_string(result));
		return 1;
	}

	vkDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
	return 0;
}
} // namespace Engine