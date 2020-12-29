#include "TextureManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Managers {
std::vector<TextureManager::TextureInfo> TextureManager::textureInfos {};

std::vector<VmaAllocation> TextureManager::allocationInfos {};

vk::Device TextureManager::vkDevice {};
VmaAllocator TextureManager::vmaAllocator {};

vk::Queue TextureManager::vkTransferQueue {};
vk::CommandPool TextureManager::vkCommandPool {};

float TextureManager::maxAnisotropy {};


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

	vk::ImageCreateInfo imageCreateInfo {};
	vk::ImageViewCreateInfo imageViewCreateInfo {};

	apply(handle, [&imageCreateInfo, &imageViewCreateInfo](auto& texture) {
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

		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = texture.format;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
	});


	// create image

	VkImageCreateInfo cImageCreateInfo(imageCreateInfo);

	VmaAllocationCreateInfo vmaAllocationCreateInfo {};
	vmaAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	auto& vmaAllocation = allocationInfos[handle.getIndex()];

	VkImage vkImage;
	auto result =
		vk::Result(vmaCreateImage(vmaAllocator, &cImageCreateInfo, &vmaAllocationCreateInfo, &vkImage, &vmaAllocation, nullptr));

	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to allocate image memory. Error code: {} ({})",
					  result,
					  vk::to_string(result));
		return;
	}

	textureInfos[handle.getIndex()].image = vk::Image(vkImage);


	// create image view

	imageViewCreateInfo.image = textureInfos[handle.getIndex()].image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &textureInfos[handle.getIndex()].imageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to create image view. Error code: {} ({})",
					  result,
					  vk::to_string(vk::Result(result)));
		return;
	}


	// create image sampler

	vk::SamplerCreateInfo samplerCreateInfo {};
	samplerCreateInfo.minFilter = vk::Filter::eLinear;
	samplerCreateInfo.magFilter = vk::Filter::eLinear;
	samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.anisotropyEnable = maxAnisotropy > 0.0f;
	samplerCreateInfo.maxAnisotropy = maxAnisotropy;
	samplerCreateInfo.unnormalizedCoordinates = false;
	samplerCreateInfo.compareEnable = false;
	samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
	samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;

	result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &textureInfos[handle.getIndex()].sampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to create image sampler. Error code: {} ({})",
					  result,
					  vk::to_string(vk::Result(result)));
		return;
	}
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