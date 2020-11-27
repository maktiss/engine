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

	vk::DeviceSize indexBufferSize;
	void* indexBufferData;
	uint32_t indexCount = 0;

	apply(handle, [&](auto& mesh) {
		vertexBufferSize = mesh.getVertexBufferSize();
		vertexBufferData = mesh.getVertexBuffer().data();

		indexBufferSize = mesh.getIndexBufferSize();
		indexBufferData = mesh.getIndexBuffer().data();
		indexCount		 = mesh.getIndexBuffer().size();
	});

	meshInfo.vertexBuffer.allocate(vmaAllocator, vertexBufferSize);
	meshInfo.vertexBuffer.writeStaged(vkDevice, vkTransferQueue, vkCommandPool, vertexBufferData);

	meshInfo.indexCount = indexCount;
	meshInfo.indexBuffer.allocate(vmaAllocator, indexBufferSize);
	meshInfo.indexBuffer.writeStaged(vkDevice, vkTransferQueue, vkCommandPool, indexBufferData);
}

} // namespace Engine::Managers