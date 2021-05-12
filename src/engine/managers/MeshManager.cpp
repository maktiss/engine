#include "MeshManager.hpp"


namespace Engine {
std::vector<MeshManager::MeshInfo> MeshManager::meshInfos {};

vk::Device MeshManager::vkDevice {};
VmaAllocator MeshManager::vmaAllocator {};

vk::Queue MeshManager::vkTransferQueue {};
vk::CommandPool MeshManager::vkCommandPool {};


int MeshManager::init() {
	spdlog::info("Initializing MeshManager...");

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);
	assert(vkTransferQueue != vk::Queue());
	assert(vkCommandPool != vk::CommandPool());

	return ResourceManagerBase::init();
};


void MeshManager::postCreate(Handle& handle) {
	meshInfos.push_back({});
}

void MeshManager::update(Handle& handle) {
	auto& meshInfo = getMeshInfo(handle);

	vk::DeviceSize vertexBufferSize;
	void* vertexBufferData;

	vk::DeviceSize indexBufferSize;
	void* indexBufferData;
	uint32_t indexCount = 0;

	apply(handle, [&](auto& mesh) {
		meshInfo.boundingSphere = mesh.boundingSphere;
		meshInfo.boundingBox = mesh.boundingBox;

		vertexBufferSize = mesh.getVertexBufferSize();
		vertexBufferData = mesh.getVertexBuffer().data();

		indexBufferSize = mesh.getIndexBufferSize();
		indexBufferData = mesh.getIndexBuffer().data();
		indexCount		= mesh.getIndexBuffer().size();
	});


	meshInfo.vertexBuffer.allocate(vmaAllocator, vertexBufferSize);
	meshInfo.vertexBuffer.writeStaged(vkDevice, vkTransferQueue, vkCommandPool, vertexBufferData);

	meshInfo.indexCount = indexCount;
	meshInfo.indexBuffer.allocate(vmaAllocator, indexBufferSize);
	meshInfo.indexBuffer.writeStaged(vkDevice, vkTransferQueue, vkCommandPool, indexBufferData);
}

} // namespace Engine