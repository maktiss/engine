#include "PostFxRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int PostFxRenderer::init() {
	spdlog::info("Initializing PostFxRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	// Create input sampler

	vk::SamplerCreateInfo samplerCreateInfo {};
	samplerCreateInfo.minFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.magFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.addressModeU			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.addressModeV			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.addressModeW			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.anisotropyEnable		  = false;
	samplerCreateInfo.maxAnisotropy			  = 0.0f;
	samplerCreateInfo.unnormalizedCoordinates = false;
	samplerCreateInfo.compareEnable			  = false;
	samplerCreateInfo.compareOp				  = vk::CompareOp::eAlways;
	samplerCreateInfo.mipmapMode			  = vk::SamplerMipmapMode::eLinear;
	samplerCreateInfo.mipLodBias			  = 0.0f;
	samplerCreateInfo.minLod				  = 0.0f;
	samplerCreateInfo.maxLod				  = 0.0f;

	auto result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &vkSampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[PostFxRenderer] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	// Create input imageview

	vk::ImageViewCreateInfo imageViewCreateInfo {};
	imageViewCreateInfo.viewType						= vk::ImageViewType::e2D;
	imageViewCreateInfo.format							= getInputDescriptions()[0].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 1;
	imageViewCreateInfo.image							= TextureManager::getTextureInfo(inputs[0]).image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[PostFxRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	// Define descriptor sets

	descriptorSetArrays.resize(1);

	descriptorSetArrays[0].setBindingCount(2);
	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 64);
	descriptorSetArrays[0].setBindingLayoutInfo(1, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);

	descriptorSetArrays[0].updateImage(0, 1, 0, vkSampler, vkImageView);


	mesh = MeshManager::createObject(0, "generated_screen_triangle");
	mesh.apply([](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(8);

		std::get<0>(vertexBuffer[0]) = glm::vec3(-1.0f, 1.0f, 0.5f);
		std::get<0>(vertexBuffer[1]) = glm::vec3(-1.0f, -3.0f, 0.5f);
		std::get<0>(vertexBuffer[2]) = glm::vec3(3.0f, 1.0f, 0.5f);

		std::get<1>(vertexBuffer[0]) = glm::vec2(0.0f, 0.0f);
		std::get<1>(vertexBuffer[1]) = glm::vec2(0.0f, 2.0f);
		std::get<1>(vertexBuffer[2]) = glm::vec2(2.0f, 0.0f);

		auto& indexBuffer = mesh.getIndexBuffer();

		indexBuffer = {
			0,
			1,
			2,
		};
	});
	mesh.update();

	shaderHandle = GraphicsShaderManager::getHandle<PostFxShader>(mesh);


	return GraphicsRendererBase::init();
}


void PostFxRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
												   double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	// TODO: move to function
	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(0), 0, nullptr);
	}


	const auto& meshInfo = MeshManager::getMeshInfo(mesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine