#include "MipMapRenderer.hpp"


namespace Engine {
int MipMapRenderer::init() {
	spdlog::info("Initializing MipMapRenderer...");

	return TransferRendererBase::init();
}

void MipMapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
												   double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	auto initialLayout = vkOutputInitialLayouts[0];
	auto finalLayout   = vkOutputFinalLayouts[0];

	const auto textureInfo = TextureManager::getTextureInfo(outputs[0]);


	vk::ImageMemoryBarrier imageMemoryBarrier {};
	imageMemoryBarrier.image = textureInfo.image;
	// TODO: depth support?
	imageMemoryBarrier.subresourceRange.aspectMask = textureInfo.imageAspect;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	for (uint layer = 0; layer < textureInfo.arrayLayers; layer++) {
		// TODO: 3D texture support
		vk::Offset3D mipSize = { static_cast<int32_t>(outputSize.width), static_cast<int32_t>(outputSize.height), 1 };

		imageMemoryBarrier.subresourceRange.baseArrayLayer = layer;

		for (uint level = 0; level < textureInfo.mipLevels - 1; level++) {
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
			imageBlit.srcSubresource.baseArrayLayer = layer;
			imageBlit.srcSubresource.layerCount		= 1;
			imageBlit.srcOffsets[0]					= vk::Offset3D { 0, 0, 0 };
			imageBlit.srcOffsets[1]					= vk::Offset3D { mipSize.x, mipSize.y, 1 };

			mipSize.x = std::max(1, mipSize.x / 2);
			mipSize.y = std::max(1, mipSize.y / 2);

			imageBlit.dstSubresource.aspectMask		= vk::ImageAspectFlagBits::eColor;
			imageBlit.dstSubresource.mipLevel		= level + 1;
			imageBlit.dstSubresource.baseArrayLayer = layer;
			imageBlit.dstSubresource.layerCount		= 1;
			imageBlit.dstOffsets[0]					= vk::Offset3D { 0, 0, 0 };
			imageBlit.dstOffsets[1]					= vk::Offset3D { mipSize.x, mipSize.y, 1 };

			commandBuffer.blitImage(textureInfo.image, vk::ImageLayout::eTransferSrcOptimal, textureInfo.image,
									vk::ImageLayout::eTransferDstOptimal, 1, &imageBlit, vk::Filter::eLinear);


			imageMemoryBarrier.subresourceRange.baseMipLevel = level;
			imageMemoryBarrier.oldLayout					 = vk::ImageLayout::eTransferSrcOptimal;
			imageMemoryBarrier.newLayout					 = finalLayout;
			imageMemoryBarrier.srcAccessMask				 = vk::AccessFlagBits::eTransferRead;
			imageMemoryBarrier.dstAccessMask				 = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
										  vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1,
										  &imageMemoryBarrier);
		}

		imageMemoryBarrier.subresourceRange.baseMipLevel = textureInfo.mipLevels - 1;
		imageMemoryBarrier.oldLayout					 = vk::ImageLayout::eTransferDstOptimal;
		imageMemoryBarrier.newLayout					 = finalLayout;
		imageMemoryBarrier.srcAccessMask				 = vk::AccessFlagBits::eTransferWrite;
		imageMemoryBarrier.dstAccessMask				 = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
									  {}, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
}
} // namespace Engine