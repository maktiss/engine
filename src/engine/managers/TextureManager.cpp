#include "TextureManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Managers {
std::vector<TextureManager::TextureInfo> TextureManager::textureInfos {};

std::vector<VmaAllocation> TextureManager::allocationInfos {};

vk::Device TextureManager::vkDevice {};
VmaAllocator TextureManager::vmaAllocator {};

vk::Queue TextureManager::vkTransferQueue {};
vk::CommandPool TextureManager::vkCommandPool {};


int TextureManager::init() {
	spdlog::info("Initializing TextureManager...");

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);
	assert(vkTransferQueue != vk::Queue());
	assert(vkCommandPool != vk::CommandPool());

	return ResourceManagerBase::init();
}

void TextureManager::postCreate(Handle handle) {
	textureInfos.push_back({});
	allocationInfos.push_back({});
}

void TextureManager::update(Handle handle) {
	destroy(handle.getIndex());

	vk::ImageCreateInfo imageCreateInfo;

	apply(handle, [&imageCreateInfo](auto& texture) {
		imageCreateInfo.imageType	  = texture.getImageType();
		imageCreateInfo.extent		  = texture.size;
		imageCreateInfo.mipLevels	  = 1;
		imageCreateInfo.arrayLayers	  = texture.layerCount;
		imageCreateInfo.format		  = texture.format;
		imageCreateInfo.tiling		  = vk::ImageTiling::eOptimal;
		imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageCreateInfo.usage		  = texture.usage;
		imageCreateInfo.sharingMode	  = vk::SharingMode::eExclusive;
		imageCreateInfo.samples		  = vk::SampleCountFlagBits::e1;
	});

	VkImageCreateInfo cImageCreateInfo(imageCreateInfo);

	VmaAllocationCreateInfo vmaAllocationCreateInfo {};
	vmaAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	auto& vmaAllocation = allocationInfos[handle.getIndex()];

	VkImage vkImage;
	auto result =
		vmaCreateImage(vmaAllocator, &cImageCreateInfo, &vmaAllocationCreateInfo, &vkImage, &vmaAllocation, nullptr);

	if (result != VK_SUCCESS) {
		spdlog::error("[TextureManager] Failed to allocate image memory. Error code: {} ({})",
					  result,
					  vk::to_string(vk::Result(result)));
	}

	textureInfos[handle.getIndex()].image = vk::Image(vkImage);
}


void TextureManager::destroy(uint32_t index) {
	if (allocationInfos[index] != nullptr) {
		vmaDestroyImage(vmaAllocator, textureInfos[index].image, allocationInfos[index]);
	}
}

void TextureManager::dispose() {
	for (uint i = 0; i < textureInfos.size(); i++) {
		destroy(i);
	}
}
} // namespace Engine::Managers