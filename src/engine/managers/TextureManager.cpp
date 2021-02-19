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

Engine::Graphics::StagingBuffer TextureManager::stagingBuffer {};


int TextureManager::init() {
	spdlog::info("Initializing TextureManager...");

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);
	assert(vkTransferQueue != vk::Queue());
	assert(vkCommandPool != vk::CommandPool());

	// TODO: not hardcoded max staging buffer size
	if (stagingBuffer.init(vkDevice, vmaAllocator, 4096 * 4096 * 4)) {
		return 1;
	}


	if (ResourceManagerBase::init()) {
		return 1;
	}


	// Update fallback textures
	// TODO: update all texture types

	auto fallbackTextureHandle = getHandle(0);
	fallbackTextureHandle.apply([](auto& texture) {
		texture.size		= vk::Extent3D(1, 1, 1);
		texture.usage		= vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		texture.imageAspect = vk::ImageAspectFlagBits::eColor;
		texture.format		= vk::Format::eR8G8B8A8Srgb;

		uint8_t textureData[4] = { 255, 0, 255, 255 };
		texture.setPixelData(textureData, 4);
	});
	fallbackTextureHandle.update();

	return 0;
}


void TextureManager::postCreate(Handle handle) {
	textureInfos.push_back({});
	allocationInfos.push_back({});
}

void TextureManager::update(Handle handle) {
	destroy(handle.getIndex());

	auto& textureInfo = textureInfos[handle.getIndex()];

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


		switch (imageCreateInfo.imageType) {
		case vk::ImageType::e2D:
			if (imageCreateInfo.arrayLayers > 1) {
				imageViewCreateInfo.viewType = vk::ImageViewType::e2DArray;
			} else {
				imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
			}
			break;

			// TODO: 1D & 3D textures support

		default:
			spdlog::error("Image type '{}' is not supported", vk::to_string(imageCreateInfo.imageType));
			return;
		}

		imageViewCreateInfo.format							= texture.format;
		imageViewCreateInfo.subresourceRange.aspectMask		= texture.imageAspect;
		imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
		imageViewCreateInfo.subresourceRange.levelCount		= 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount		= imageCreateInfo.arrayLayers;
	});


	// create image

	VkImageCreateInfo cImageCreateInfo(imageCreateInfo);

	VmaAllocationCreateInfo vmaAllocationCreateInfo {};
	vmaAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	auto& vmaAllocation = allocationInfos[handle.getIndex()];

	VkImage vkImage;
	auto result = vk::Result(
		vmaCreateImage(vmaAllocator, &cImageCreateInfo, &vmaAllocationCreateInfo, &vkImage, &vmaAllocation, nullptr));

	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to allocate image memory. Error code: {} ({})", result,
					  vk::to_string(result));
		return;
	}

	textureInfo.image = vk::Image(vkImage);


	// create image view

	imageViewCreateInfo.image = textureInfo.image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &textureInfo.imageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return;
	}


	// create image sampler

	vk::SamplerCreateInfo samplerCreateInfo {};
	samplerCreateInfo.minFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.magFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.addressModeU			  = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeV			  = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeW			  = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.anisotropyEnable		  = maxAnisotropy > 0.0f;
	samplerCreateInfo.maxAnisotropy			  = maxAnisotropy;
	samplerCreateInfo.unnormalizedCoordinates = false;
	samplerCreateInfo.compareEnable			  = false;
	samplerCreateInfo.compareOp				  = vk::CompareOp::eAlways;
	samplerCreateInfo.mipmapMode			  = vk::SamplerMipmapMode::eLinear;
	samplerCreateInfo.mipLodBias			  = 0.0f;
	samplerCreateInfo.minLod				  = 0.0f;
	samplerCreateInfo.maxLod				  = 0.0f;

	result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &textureInfo.sampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return;
	}


	// write texture data

	void* pTextureData;
	vk::DeviceSize textureDataSize;
	vk::Extent3D textureSize;
	apply(handle, [&pTextureData, &textureDataSize, &textureSize](auto& texture) {
		pTextureData	= texture.getPixelData().data();
		textureDataSize = texture.getPixelData().size();
		textureSize		= texture.size;
	});

	if (textureDataSize > 0) {
		if (stagingBuffer.update(pTextureData, textureDataSize)) {
			return;
		}
		if (stagingBuffer.transfer(vkTransferQueue, vkCommandPool, textureInfo.image, textureSize)) {
			return;
		}
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
	stagingBuffer.dispose();
}
} // namespace Engine::Managers