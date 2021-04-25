#pragma once

#include "ResourceManagerBase.hpp"

#include "engine/graphics/Buffer.hpp"

#include "engine/graphics/meshes/Meshes.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"


namespace Engine {
class MeshManager : public ResourceManagerBase<MeshManager, StaticMesh> {
public:
	// info required for rendering
	struct MeshInfo {
		// TODO: keep vk::Buffer's only
		Buffer vertexBuffer = { vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
								VMA_MEMORY_USAGE_GPU_ONLY };
		Buffer indexBuffer	= { vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
								VMA_MEMORY_USAGE_GPU_ONLY };

		vk::Buffer vkVertexBuffer {};
		vk::Buffer vkIndexBuffer {};

		uint32_t indexCount {};
	};


private:
	static std::vector<MeshInfo> meshInfos;

	static vk::Device vkDevice;
	static VmaAllocator vmaAllocator;

	static vk::Queue vkTransferQueue;
	static vk::CommandPool vkCommandPool;


public:
	static int init();

	static void postCreate(Handle handle);
	static void update(Handle handle);


	static void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	static void setVulkanMemoryAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}

	static void setVkTransferQueue(vk::Queue queue) {
		vkTransferQueue = queue;
	}

	static void setVkCommandPool(vk::CommandPool commandPool) {
		vkCommandPool = commandPool;
	}


	static inline MeshInfo& getMeshInfo(Handle handle) {
		return meshInfos[handle.getIndex()];
	}


	static inline std::string getMeshTypeString(Handle handle) {
		std::string string;
		apply(handle, [&string](auto& mesh) {
			string = mesh.getMeshTypeString();
		});

		return string;
	}

	static inline std::string getMeshTypeString(uint32_t typeIndex) {
		return getMeshTypeStringImpl(typeIndex, std::make_index_sequence<getTypeCount()>());
	}

	template <std::size_t... Indices>
	static inline std::string getMeshTypeStringImpl(uint32_t typeIndex, std::index_sequence<Indices...>) {
		std::string meshTypeString;

		// FIXME bug
		((meshTypeString = Indices == typeIndex ? std::get<Indices>(getTypeTuple()).getMeshTypeString() : ""), ...);

		assert(!meshTypeString.empty());

		return meshTypeString;
	}


	static auto getVertexInputAttributeDescriptions(uint32_t meshTypeIndex) {
		return getVertexInputAttributeDescriptionsImpl(meshTypeIndex, std::make_index_sequence<getTypeCount()>());
	}

	template <std::size_t... Indices>
	static std::vector<vk::VertexInputAttributeDescription>
	getVertexInputAttributeDescriptionsImpl(uint32_t meshTypeIndex, std::index_sequence<Indices...>) {
		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescription;

		((vertexInputAttributeDescription =
			  Indices == meshTypeIndex ? std::get<Indices>(getTypeTuple()).getVertexInputAttributeDescriptions()
									   : vertexInputAttributeDescription),
		 ...);

		return vertexInputAttributeDescription;
	}

	static auto getVertexInputBindingDescriptions(uint32_t meshTypeIndex) {
		return getVertexInputBindingDescriptionsImpl(meshTypeIndex, std::make_index_sequence<getTypeCount()>());
	}

	template <std::size_t... Indices>
	static std::vector<vk::VertexInputBindingDescription>
	getVertexInputBindingDescriptionsImpl(uint32_t meshTypeIndex, std::index_sequence<Indices...>) {
		std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions;

		((vertexInputBindingDescriptions = Indices == meshTypeIndex
											   ? std::get<Indices>(getTypeTuple()).getVertexInputBindingDescriptions()
											   : vertexInputBindingDescriptions),
		 ...);

		return vertexInputBindingDescriptions;
	}


	static void destroy() {
		for (auto& meshInfo : meshInfos) {
			meshInfo.vertexBuffer.destroy();
			meshInfo.indexBuffer.destroy();
		}
	}


private:
	MeshManager() {};
};
} // namespace Engine