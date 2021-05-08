#include "ReflectionRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ReflectionRenderer::init() {
	spdlog::info("Initializing ReflectionRenderer...");

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
	samplerCreateInfo.maxLod				  = VK_LOD_CLAMP_NONE;

	auto result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &vkSampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ReflectionRenderer] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	// Create input imageviews

	vk::ImageViewCreateInfo imageViewCreateInfo {};
	imageViewCreateInfo.viewType						= vk::ImageViewType::e2D;
	imageViewCreateInfo.format							= getInputDescriptions()[0].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eDepth;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 1;
	imageViewCreateInfo.image							= TextureManager::getTextureInfo(inputs[0]).image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkDepthImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ReflectionRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}

	imageViewCreateInfo.viewType						= vk::ImageViewType::e2D;
	imageViewCreateInfo.format							= getInputDescriptions()[1].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 1;
	imageViewCreateInfo.image							= TextureManager::getTextureInfo(inputs[1]).image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkNormalImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ReflectionRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	auto textureInfo = TextureManager::getTextureInfo(inputs[2]);
	imageViewCreateInfo.viewType						= vk::ImageViewType::eCube;
	imageViewCreateInfo.format							= getInputDescriptions()[2].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= textureInfo.mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 6;
	imageViewCreateInfo.image							= textureInfo.image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkEnvironmentImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ReflectionRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	// Define descriptor sets

	descriptorSetArrays.resize(1);

	descriptorSetArrays[0].setBindingCount(4);
	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, sizeof(ParamBlock));
	descriptorSetArrays[0].setBindingLayoutInfo(1, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].setBindingLayoutInfo(2, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].setBindingLayoutInfo(3, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);

	descriptorSetArrays[0].updateImage(0, 1, 0, vkSampler, vkDepthImageView);
	descriptorSetArrays[0].updateImage(0, 2, 0, vkSampler, vkNormalImageView);
	descriptorSetArrays[0].updateImage(0, 3, 0, vkSampler, vkEnvironmentImageView);


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

	shaderHandle = GraphicsShaderManager::getHandle<ReflectionShader>(mesh);


	return GraphicsRendererBase::init();
}


void ReflectionRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
													   uint layerIndex, double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	// TODO: move to function
	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(0), 0, nullptr);
	}


	ParamBlock paramBlock;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		// TODO: Check if active camera
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		auto viewMatrix =
			glm::lookAtLH(glm::vec3(0.0f), glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						  glm::vec3(0.0f, 1.0f, 0.0f));

		auto projectionMatrix = camera.getProjectionMatrix();

		paramBlock.invViewMatrix = glm::inverse(viewMatrix);
		paramBlock.invProjectionMatrix = glm::inverse(projectionMatrix);
	});

	// EntityManager::forEach<CameraComponent>([&](const auto& camera) {
	// 	// FIXME: active camera
	// 	invProjectionMatrix = glm::inverse(camera.getProjectionMatrix());
	// });

	descriptorSetArrays[0].updateBuffer(0, 0, &paramBlock, sizeof(paramBlock));


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