#include "MeshManager.hpp"


namespace Engine::Managers {

std::vector<MeshManager::MeshInfo> MeshManager::meshInfos;

vk::Device MeshManager::vkDevice;
VmaAllocator MeshManager::vmaAllocator;

vk::Queue MeshManager::vkTransferQueue;
vk::CommandPool MeshManager::vkCommandPool;


void MeshManager::postCreate(Handle handle) {
	meshInfos.push_back({});
	postLoad(handle);
}

void MeshManager::postLoad(Handle handle) {
	auto& meshInfo = getMeshInfo(handle);

	vk::DeviceSize vertexBufferSize;
	void* vertexBufferData;
	uint32_t vertexCount = 0;

	apply(handle, [&](auto& mesh) {
		vertexBufferSize = mesh.getVertexBufferSize();
		vertexBufferData = mesh.getVertexBuffer().data();
		vertexCount		 = mesh.getVertexBuffer().size();
	});

	meshInfo.vertexCount = vertexCount;

	meshInfo.vertexBuffer.allocate(vmaAllocator, vertexBufferSize);
	// RETURN_IF_VK_ERROR(vertexBuffer.allocate(), "Failed to allocate buffer");

	Engine::Graphics::Buffer stagingVertexBuffer = { vk::BufferUsageFlagBits::eTransferSrc,
													 VMA_MEMORY_USAGE_CPU_TO_GPU };

	stagingVertexBuffer.allocate(vmaAllocator, vertexBufferSize);
	// RETURN_IF_VK_ERROR(stagingVertexBuffer.allocate(), "Failed to allocate buffer");
	stagingVertexBuffer.write(vertexBufferData);
	// RETURN_IF_VK_ERROR(stagingVertexBuffer.write(mesh.getVertexBuffer().data()), "Failed to write buffer");

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.level				 = vk::CommandBufferLevel::ePrimary;
	commandBufferAllocateInfo.commandPool		 = vkCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	vk::CommandBuffer stagingCommandBuffer;
	vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &stagingCommandBuffer);
	// RETURN_IF_VK_ERROR(vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &stagingCommandBuffer),
	// 				"Failed to allocate staging command buffer");

	vk::CommandBufferBeginInfo commandBufferBeginInfo {};
	commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	stagingCommandBuffer.begin(&commandBufferBeginInfo);
	// RETURN_IF_VK_ERROR(stagingCommandBuffer.begin(&commandBufferBeginInfo), "Failed to record command
	// buffer");

	vk::BufferCopy bufferCopy {};
	bufferCopy.size = vertexBufferSize;
	stagingCommandBuffer.copyBuffer(
		stagingVertexBuffer.getVkBuffer(), meshInfo.vertexBuffer.getVkBuffer(), 1, &bufferCopy);

	stagingCommandBuffer.end();

	vk::SubmitInfo submitInfo {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &stagingCommandBuffer;
	vkTransferQueue.submit(1, &submitInfo, nullptr);
	// RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, nullptr), "Failed to submit staging command
	// buffer");
	vkTransferQueue.waitIdle();
	// RETURN_IF_VK_ERROR(vkGraphicsQueue.waitIdle(), "Failed to wait for staging command buffer to complete");

	stagingVertexBuffer.destroy();
}

} // namespace Engine::Managers