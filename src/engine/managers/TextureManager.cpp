#include "TextureManager.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Managers {
std::vector<TextureManager::TextureInfo> TextureManager::textureInfos {};

std::vector<VmaAllocation> TextureManager::allocationInfos {};

vk::Device TextureManager::vkDevice {};
VmaAllocator TextureManager::vmaAllocator {};

vk::Queue TextureManager::vkTransferQueue {};
vk::CommandPool TextureManager::vkCommandPool {};

Engine::Graphics::StagingBuffer TextureManager::stagingBuffer {};

Engine::Graphics::DescriptorSetArray TextureManager::descriptorSetArray {};

vk::Sampler TextureManager::vkSampler {};

TextureManager::Properties TextureManager::properties {};


int TextureManager::init() {
	spdlog::info("Initializing TextureManager...");

	assert(vkDevice != vk::Device());
	assert(vmaAllocator != nullptr);
	assert(vkTransferQueue != vk::Queue());
	assert(vkCommandPool != vk::CommandPool());

	uint maxTextureSize = properties.maxTextureSize;
	if (stagingBuffer.init(vkDevice, vmaAllocator, maxTextureSize * maxTextureSize * 4)) {
		return 1;
	}


	if (ResourceManagerBase::init()) {
		return 1;
	}


	// Create image sampler

	vk::SamplerCreateInfo samplerCreateInfo {};
	samplerCreateInfo.minFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.magFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.addressModeU			  = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeV			  = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.addressModeW			  = vk::SamplerAddressMode::eRepeat;
	samplerCreateInfo.anisotropyEnable		  = properties.anisotropy > 0.0f;
	samplerCreateInfo.maxAnisotropy			  = properties.anisotropy;
	samplerCreateInfo.unnormalizedCoordinates = false;
	samplerCreateInfo.compareEnable			  = false;
	samplerCreateInfo.compareOp				  = vk::CompareOp::eAlways;
	samplerCreateInfo.mipmapMode			  = vk::SamplerMipmapMode::eLinear;
	samplerCreateInfo.mipLodBias			  = 0.0f;
	samplerCreateInfo.minLod				  = 0.0f;
	samplerCreateInfo.maxLod				  = VK_LOD_CLAMP_NONE;

	auto result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &vkSampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	descriptorSetArray.setBindingCount(2);
	descriptorSetArray.setBindingLayoutInfo(0, vk::DescriptorType::eSampler, 0);
	descriptorSetArray.setBindingLayoutInfo(1, vk::DescriptorType::eSampledImage, 0, properties.maxTextureHandles);
	descriptorSetArray.init(vkDevice, vmaAllocator);

	descriptorSetArray.updateImage(0, 0, 0, vkSampler, {});


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


	// TODO: update all at once
	for (uint i = 1; i < properties.maxTextureHandles; i++) {
		descriptorSetArray.updateImage(0, 1, i, {}, textureInfos[0].imageView);
	}


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

	uint32_t mipLevels = 1;

	apply(handle, [&](auto& texture) {
		if (texture.useMipMapping) {
			mipLevels =
				static_cast<uint32_t>(std::floor(std::log2(std::max(texture.size.width, texture.size.height)))) + 1;
		}

		imageCreateInfo.imageType	  = texture.getImageType();
		imageCreateInfo.extent		  = texture.size;
		imageCreateInfo.mipLevels	  = mipLevels;
		imageCreateInfo.arrayLayers	  = texture.layerCount;
		imageCreateInfo.format		  = texture.format;
		imageCreateInfo.tiling		  = vk::ImageTiling::eOptimal;
		imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageCreateInfo.usage		  = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc |
								vk::ImageUsageFlagBits::eTransferDst | texture.usage;
		imageCreateInfo.flags		= texture.flags;
		imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		imageCreateInfo.samples		= vk::SampleCountFlagBits::e1;


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
		imageViewCreateInfo.subresourceRange.levelCount		= mipLevels;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount		= imageCreateInfo.arrayLayers;
	});


	// Create image

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


	// Create image view

	imageViewCreateInfo.image = textureInfo.image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &textureInfo.imageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[TextureManager] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return;
	}


	// Update descriptor set

	// TODO: another way of preventing reading from texture used as an attachment
	if (!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eColorAttachment) &&
		!(imageCreateInfo.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)) {
		descriptorSetArray.updateImage(0, 1, handle.getIndex(), {}, textureInfo.imageView);
	}


	// Write texture data

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


		// Generate mip maps

		// TODO: abstract one-time command buffer
		// Begin one-time command buffer

		vk::CommandBuffer commandBuffer {};

		vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
		commandBufferAllocateInfo.level				 = vk::CommandBufferLevel::ePrimary;
		commandBufferAllocateInfo.commandPool		 = vkCommandPool;
		commandBufferAllocateInfo.commandBufferCount = 1;

		auto result = vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to allocate staging command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return;
		}

		vk::CommandBufferBeginInfo commandBufferBeginInfo {};
		commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		result = commandBuffer.begin(&commandBufferBeginInfo);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record staging command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return;
		}


		// TODO: 3D texture support
		vk::Offset3D mipSize = { static_cast<int32_t>(textureSize.width), static_cast<int32_t>(textureSize.height), 1 };

		vk::ImageMemoryBarrier imageMemoryBarrier {};
		imageMemoryBarrier.image					   = textureInfo.image;
		imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		for (uint level = 0; level < mipLevels - 1; level++) {
			imageMemoryBarrier.subresourceRange.baseMipLevel = level;
			imageMemoryBarrier.oldLayout					 = vk::ImageLayout::eTransferDstOptimal;
			imageMemoryBarrier.newLayout					 = vk::ImageLayout::eTransferSrcOptimal;
			imageMemoryBarrier.srcAccessMask				 = vk::AccessFlagBits::eTransferWrite;
			imageMemoryBarrier.dstAccessMask				 = vk::AccessFlagBits::eTransferRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
										  {}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);


			imageMemoryBarrier.subresourceRange.baseMipLevel = level + 1;
			imageMemoryBarrier.oldLayout					 = vk::ImageLayout::eUndefined;
			imageMemoryBarrier.newLayout					 = vk::ImageLayout::eTransferDstOptimal;
			imageMemoryBarrier.srcAccessMask				 = {};
			imageMemoryBarrier.dstAccessMask				 = vk::AccessFlagBits::eTransferWrite;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
										  {}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);


			vk::ImageBlit imageBlit {};

			imageBlit.srcSubresource.aspectMask		= vk::ImageAspectFlagBits::eColor;
			imageBlit.srcSubresource.mipLevel		= level;
			imageBlit.srcSubresource.baseArrayLayer = 0;
			imageBlit.srcSubresource.layerCount		= 1;
			imageBlit.srcOffsets[0]					= vk::Offset3D { 0, 0, 0 };
			imageBlit.srcOffsets[1]					= vk::Offset3D { mipSize.x, mipSize.y, 1 };

			mipSize.x = std::max(1, mipSize.x / 2);
			mipSize.y = std::max(1, mipSize.y / 2);

			imageBlit.dstSubresource.aspectMask		= vk::ImageAspectFlagBits::eColor;
			imageBlit.dstSubresource.mipLevel		= level + 1;
			imageBlit.dstSubresource.baseArrayLayer = 0;
			imageBlit.dstSubresource.layerCount		= 1;
			imageBlit.dstOffsets[0]					= vk::Offset3D { 0, 0, 0 };
			imageBlit.dstOffsets[1]					= vk::Offset3D { mipSize.x, mipSize.y, 1 };

			commandBuffer.blitImage(textureInfo.image, vk::ImageLayout::eTransferSrcOptimal, textureInfo.image,
									vk::ImageLayout::eTransferDstOptimal, 1, &imageBlit, vk::Filter::eLinear);


			imageMemoryBarrier.subresourceRange.baseMipLevel = level;
			imageMemoryBarrier.oldLayout					 = vk::ImageLayout::eTransferSrcOptimal;
			imageMemoryBarrier.newLayout					 = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageMemoryBarrier.srcAccessMask				 = vk::AccessFlagBits::eTransferRead;
			imageMemoryBarrier.dstAccessMask				 = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
										  vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1,
										  &imageMemoryBarrier);
		}

		imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
		imageMemoryBarrier.oldLayout					 = vk::ImageLayout::eTransferDstOptimal;
		imageMemoryBarrier.newLayout					 = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageMemoryBarrier.srcAccessMask				 = vk::AccessFlagBits::eTransferWrite;
		imageMemoryBarrier.dstAccessMask				 = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
									  {}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);


		// End one-time command buffer

		result = commandBuffer.end();
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record staging command buffer. Error code: {} ({})", result,
						  vk::to_string(result));
			return;
		}

		vk::SubmitInfo submitInfo {};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers	  = &commandBuffer;

		result = vkTransferQueue.submit(1, &submitInfo, nullptr);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to submit staging buffer. Error code: {} ({})", result, vk::to_string(result));
			return;
		}

		result = vkTransferQueue.waitIdle();
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to wait for staging command buffer to complete. Error code: {} ({})", result,
						  vk::to_string(result));
			return;
		}

		vkDevice.freeCommandBuffers(vkCommandPool, 1, &commandBuffer);
	}
}


void TextureManager::destroy(uint32_t index) {
	if (textureInfos[index].imageView != vk::ImageView()) {
		vkDevice.destroyImageView(textureInfos[index].imageView);
	}
	if (allocationInfos[index] != nullptr) {
		vmaDestroyImage(vmaAllocator, textureInfos[index].image, allocationInfos[index]);
	}

	textureInfos[index]	   = {};
	allocationInfos[index] = {};
}

void TextureManager::dispose() {
	for (uint i = 0; i < textureInfos.size(); i++) {
		destroy(i);
	}
	stagingBuffer.dispose();

	vkDevice.destroySampler(vkSampler);
}
} // namespace Engine::Managers