#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>


namespace Engine {
class Buffer {
private:
	vk::Device vkDevice;
	vk::PhysicalDevice vkPhysicalDevice;

	vk::Buffer vkBuffer;
	vk::DeviceMemory vkDeviceMemory;

	vk::DeviceSize vkBufferSize;
	vk::BufferUsageFlags vkBufferUsageFlags;
	vk::MemoryPropertyFlags vkMemoryPropertyFlags;

public:
	Buffer() {}

	Buffer(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize bufferSize,
		   vk::BufferUsageFlags bufferUsageFlags, vk::MemoryPropertyFlags memoryPropertyFlags) :
		vkDevice(device),
		vkPhysicalDevice(physicalDevice), vkBufferSize(bufferSize), vkBufferUsageFlags(bufferUsageFlags),
		vkMemoryPropertyFlags(memoryPropertyFlags) {
	}

	[[nodiscard]] vk::Result allocate() {
		vk::BufferCreateInfo bufferCreateInfo {};
		bufferCreateInfo.size		 = vkBufferSize;
		bufferCreateInfo.usage		 = vkBufferUsageFlags;
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

		auto result = vkDevice.createBuffer(&bufferCreateInfo, nullptr, &vkBuffer);
		if (result != vk::Result::eSuccess) {
			return result;
		}

		vk::MemoryRequirements memoryRequirements = vkDevice.getBufferMemoryRequirements(vkBuffer);

		vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = vkPhysicalDevice.getMemoryProperties();

		uint32_t memoryTypeIndex = -1;
		for (uint i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
			if (memoryRequirements.memoryTypeBits & (1 << i)) {
				if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & vkMemoryPropertyFlags) ==
					vkMemoryPropertyFlags) {
					memoryTypeIndex = i;
					break;
				}
			}
		}

		if (memoryTypeIndex == -1) {
			return vk::Result::eErrorUnknown;
		}


		vk::MemoryAllocateInfo memoryAllocateInfo {};
		memoryAllocateInfo.allocationSize  = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

		result = vkDevice.allocateMemory(&memoryAllocateInfo, nullptr, &vkDeviceMemory);
		if (result != vk::Result::eSuccess) {
			return result;
		}

		result = vkDevice.bindBufferMemory(vkBuffer, vkDeviceMemory, 0);
		if (result != vk::Result::eSuccess) {
			return result;
		}

		return vk::Result::eSuccess;
	}

	[[nodiscard]] vk::Result write(void* data) {
		void* pBufferData;
		auto result = vkDevice.mapMemory(vkDeviceMemory, 0, vkBufferSize, {}, &pBufferData);
		if (result != vk::Result::eSuccess) {
			return result;
		}
		memcpy(pBufferData, data, vkBufferSize);
		vkDevice.unmapMemory(vkDeviceMemory);

		return vk::Result::eSuccess;
	}

	inline vk::Buffer getVkBuffer() const {
		return vkBuffer;
	}
};
} // namespace Buffer