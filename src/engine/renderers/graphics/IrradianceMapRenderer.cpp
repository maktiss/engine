#include "IrradianceMapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/glm.hpp>


namespace Engine {
int IrradianceMapRenderer::init() {
	spdlog::info("Initializing IrradianceMapRenderer...");

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
		spdlog::error("[IrradianceMapRenderer] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	// Create input imageview

	auto textureInfo = TextureManager::getTextureInfo(inputs[0]);
	vk::ImageViewCreateInfo imageViewCreateInfo {};
	imageViewCreateInfo.viewType						= vk::ImageViewType::eCube;
	imageViewCreateInfo.format							= getInputDescriptions()[0].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= textureInfo.mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 6;
	imageViewCreateInfo.image							= textureInfo.image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[IrradianceMapRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	// Define descriptor sets

	descriptorSetArrays.resize(3);

	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);

	descriptorSetArrays[0].updateImage(0, 0, 0, vkSampler, vkImageView);


	descriptorSetArrays[1].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, sizeof(CameraBlock));
	descriptorSetArrays[1].init(vkDevice, vmaAllocator);


	descriptorSetArrays[2].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer,
												sizeof(glm::vec4) * irradianceMapSampleCount);
	descriptorSetArrays[2].init(vkDevice, vmaAllocator);


	// Update camera block

	CameraBlock cameraBlock;

	glm::vec3 viewVectors[6] = {
		{ 1.0f, 0.0f, 0.0f },  { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },  { 0.0f, 0.0f, -1.0f },
	};

	auto projectionMatrix = glm::perspectiveLH(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

	for (uint i = 0; i < 6; i++) {
		auto up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (i == 2) {
			up = glm::vec3(0.0f, 0.0f, -1.0f);
		} else if (i == 3) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		auto viewMatrix = glm::lookAtLH(glm::vec3(0.0f), viewVectors[i], up);

		cameraBlock.viewProjectionMatrices[i] = projectionMatrix * viewMatrix;
	}

	descriptorSetArrays[1].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));


	// Update samples block

	std::vector<glm::vec4> samples(irradianceMapSampleCount);
	Generator::fibonacciSphere(samples.data(), samples.size(), 2);

	descriptorSetArrays[2].updateBuffer(0, 0, samples.data(), sizeof(glm::vec4) * irradianceMapSampleCount);


	// Generate box mesh

	boxMesh = MeshManager::createObject(0);
	boxMesh.apply([](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(8);

		std::get<0>(vertexBuffer[0]) = glm::vec3(-1.0f, -1.0f, -1.0f);
		std::get<0>(vertexBuffer[1]) = glm::vec3(-1.0f, -1.0f, 1.0f);
		std::get<0>(vertexBuffer[2]) = glm::vec3(1.0f, -1.0f, 1.0f);
		std::get<0>(vertexBuffer[3]) = glm::vec3(1.0f, -1.0f, -1.0f);

		std::get<0>(vertexBuffer[4]) = glm::vec3(-1.0f, 1.0f, -1.0f);
		std::get<0>(vertexBuffer[5]) = glm::vec3(-1.0f, 1.0f, 1.0f);
		std::get<0>(vertexBuffer[6]) = glm::vec3(1.0f, 1.0f, 1.0f);
		std::get<0>(vertexBuffer[7]) = glm::vec3(1.0f, 1.0f, -1.0f);

		auto& indexBuffer = mesh.getIndexBuffer();

		indexBuffer = {
			0, 2, 1, 0, 3, 2, 5, 6, 7, 5, 7, 4, 0, 4, 7, 0, 7, 3, 3, 7, 6, 3, 6, 2, 2, 6, 5, 2, 5, 1, 1, 5, 4, 1, 4, 0,
		};
	});
	boxMesh.update();

	shaderHandle = GraphicsShaderManager::getHandle<IrradianceShader>(boxMesh);


	return GraphicsRendererBase::init();
}


void IrradianceMapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
														  uint layerIndex, double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	// TODO: move to function
	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(0), 0, nullptr);
	}


	const auto& meshInfo = MeshManager::getMeshInfo(boxMesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine