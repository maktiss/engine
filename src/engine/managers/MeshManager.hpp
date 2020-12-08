#pragma once

#include "ResourceManagerBase.hpp"

#include "engine/graphics/Buffer.hpp"
#include "engine/graphics/Mesh.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

namespace Engine::Managers {
class MeshManager : public ResourceManagerBase<MeshManager, Engine::Graphics::Mesh> {
public:
	// info required for rendering
	struct MeshInfo {
		// TODO: keep vk::Buffer's only
		Engine::Graphics::Buffer vertexBuffer = {
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY
		};
		Engine::Graphics::Buffer indexBuffer = {
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY
		};

		uint32_t indexCount = 0;
	};

private:
	static std::vector<MeshInfo> meshInfos;

	static vk::Device vkDevice;
	static VmaAllocator vmaAllocator;

	static vk::Queue vkTransferQueue;
	static vk::CommandPool vkCommandPool;

public:
	static int init() {
		spdlog::info("Initializing MeshManager...");

		return ResourceManagerBase::init();
	};

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

	// TODO: better names
	static void postCreate(Handle handle);
	static void postLoad(Handle handle);

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

		((meshTypeString = Indices == typeIndex ? std::get<Indices>(getTypeTuple()).getMeshTypeString() : ""), ...);

		assert(!meshTypeString.empty());

		return meshTypeString;
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
} // namespace Engine::Managers