#include "ForwardRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ForwardRenderer::init() {
	spdlog::info("Initializing ForwardRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	vk::SamplerCreateInfo samplerCreateInfo {};
	samplerCreateInfo.minFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.magFilter				  = vk::Filter::eLinear;
	samplerCreateInfo.addressModeU			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.addressModeV			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.addressModeW			  = vk::SamplerAddressMode::eClampToEdge;
	samplerCreateInfo.anisotropyEnable		  = false;
	samplerCreateInfo.maxAnisotropy			  = 0.0f;
	samplerCreateInfo.unnormalizedCoordinates = false;
	samplerCreateInfo.compareEnable			  = true;
	samplerCreateInfo.compareOp				  = vk::CompareOp::eLessOrEqual;
	samplerCreateInfo.mipmapMode			  = vk::SamplerMipmapMode::eLinear;
	samplerCreateInfo.mipLodBias			  = 0.0f;
	samplerCreateInfo.minLod				  = 0.0f;
	samplerCreateInfo.maxLod				  = 0.0f;

	auto result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &vkShadowSampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ForwardRenderer] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	samplerCreateInfo.compareEnable = false;
	samplerCreateInfo.compareOp		= vk::CompareOp::eAlways;

	result = vkDevice.createSampler(&samplerCreateInfo, nullptr, &vkSampler);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ForwardRenderer] Failed to create image sampler. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	auto textureInfo = TextureManager::getTextureInfo(inputs[0]);
	vk::ImageViewCreateInfo imageViewCreateInfo {};
	imageViewCreateInfo.viewType						= vk::ImageViewType::e2DArray;
	imageViewCreateInfo.format							= getInputDescriptions()[0].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eDepth;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= textureInfo.mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= textureInfo.arrayLayers;
	imageViewCreateInfo.image							= textureInfo.image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkShadowImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ReflectionRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}

	textureInfo											= TextureManager::getTextureInfo(inputs[3]);
	imageViewCreateInfo.viewType						= vk::ImageViewType::eCube;
	imageViewCreateInfo.format							= getInputDescriptions()[3].format;
	imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= textureInfo.mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 6;
	imageViewCreateInfo.image							= textureInfo.image;

	result = vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkIrradianceImageView);
	if (result != vk::Result::eSuccess) {
		spdlog::error("[ReflectionRenderer] Failed to create image view. Error code: {} ({})", result,
					  vk::to_string(vk::Result(result)));
		return 1;
	}


	descriptorSetArrays.resize(3);

	descriptorSetArrays[0].setBindingCount(5);
	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 4);
	descriptorSetArrays[0].setBindingLayoutInfo(1, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].setBindingLayoutInfo(2, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].setBindingLayoutInfo(3, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].setBindingLayoutInfo(4, vk::DescriptorType::eCombinedImageSampler, 0);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);
	descriptorSetArrays[0].updateImage(0, 1, 0, vkShadowSampler, vkShadowImageView);
	descriptorSetArrays[0].updateImage(0, 2, 0, vkSampler, TextureManager::getTextureInfo(inputs[1]).imageView);
	descriptorSetArrays[0].updateImage(0, 3, 0, vkSampler, TextureManager::getTextureInfo(inputs[2]).imageView);
	descriptorSetArrays[0].updateImage(0, 4, 0, vkSampler, vkIrradianceImageView);

	descriptorSetArrays[1].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 256);
	descriptorSetArrays[1].init(vkDevice, vmaAllocator);


	uint environmentBlockSize =
		16 + (16 + 16 + 64 * directionalLightCascadeCount) +
		(2 * sizeof(EnvironmentBlockMap::LightCluster) * clusterCountX * clusterCountY * clusterCountZ);

	uint pointLightsBlockSize = maxVisiblePointLights * sizeof(PointLight);
	uint spotLightsBlockSize  = maxVisibleSpotLights * sizeof(SpotLight);

	descriptorSetArrays[2].setBindingCount(3);
	descriptorSetArrays[2].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, environmentBlockSize);
	descriptorSetArrays[2].setBindingLayoutInfo(1, vk::DescriptorType::eStorageBuffer, pointLightsBlockSize);
	descriptorSetArrays[2].setBindingLayoutInfo(2, vk::DescriptorType::eStorageBuffer, spotLightsBlockSize);
	descriptorSetArrays[2].init(vkDevice, vmaAllocator);


	return GraphicsRendererBase::init();
}


void ForwardRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
													double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(0), 0, nullptr);
	}

	const auto textureDescriptorSet = TextureManager::getVkDescriptorSet();
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, 4, 1, &textureDescriptorSet, 0,
									 nullptr);


	struct {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::mat4 invViewMatrix;
		glm::mat4 invProjectionMatrix;
	} cameraBlock;

	glm::vec3 cameraPos;
	glm::vec3 cameraViewDir;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		// TODO: Check if active camera
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		cameraPos	  = transform.position;
		cameraViewDir = glm::vec3(viewVector);

		cameraBlock.viewMatrix =
			glm::lookAtLH(transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
						  glm::vec3(0.0f, 1.0f, 0.0f));

		cameraBlock.projectionMatrix = camera.getProjectionMatrix();
	});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	descriptorSetArrays[1].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));


	void* pEnvironmentBlock;
	descriptorSetArrays[2].mapBuffer(0, 0, pEnvironmentBlock);

	EnvironmentBlockMap environmentBlockMap;

	// TODO: better/safer way of block mapping
	environmentBlockMap.useDirectionalLight = static_cast<bool*>(pEnvironmentBlock);

	environmentBlockMap.directionalLight.direction =
		reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(environmentBlockMap.useDirectionalLight) + 16);
	environmentBlockMap.directionalLight.color =
		reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.direction) + 16);
	environmentBlockMap.directionalLight.shadowMapIndex =
		reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.color) + 12);
	environmentBlockMap.directionalLight.baseLightSpaceMatrix = reinterpret_cast<glm::mat4*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.shadowMapIndex) + 4);
	environmentBlockMap.directionalLight.lightSpaceMatrices = reinterpret_cast<glm::mat4*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.baseLightSpaceMatrix) + 64);

	environmentBlockMap.pointLightClusters = reinterpret_cast<EnvironmentBlockMap::LightCluster*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.directionalLight.lightSpaceMatrices) +
		64 * directionalLightCascadeCount);
	environmentBlockMap.spotLightClusters = reinterpret_cast<EnvironmentBlockMap::LightCluster*>(
		reinterpret_cast<uint8_t*>(environmentBlockMap.pointLightClusters) +
		64 * clusterCountX * clusterCountY * clusterCountZ);


	*environmentBlockMap.useDirectionalLight = false;


	EntityManager::forEach<TransformComponent, LightComponent>([&](const auto& transform, const auto& light) {
		if (light.type == LightComponent::Type::DIRECTIONAL) {
			auto lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
			lightDirection		= glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * lightDirection;
			lightDirection		= glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * lightDirection;

			*environmentBlockMap.useDirectionalLight			 = true;
			*environmentBlockMap.directionalLight.direction		 = glm::vec3(cameraBlock.viewMatrix * lightDirection);
			*environmentBlockMap.directionalLight.color			 = light.color;
			*environmentBlockMap.directionalLight.shadowMapIndex = light.shadowMapIndex;

			// FIXME
			const float cascadeHalfSizes[] = { 2.0f, 4.0f, 8.0f };
			float cascadeHalfSize		   = cascadeHalfSizes[0];

			auto lightSpaceMatrix =
				glm::lookAtLH(cameraPos, cameraPos + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

			lightSpaceMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize,
											   -1000.0f, 1000.0f) *
							   lightSpaceMatrix;

			*environmentBlockMap.directionalLight.baseLightSpaceMatrix = lightSpaceMatrix;

			for (uint cascadeIndex = 0; cascadeIndex < 3; cascadeIndex++) {
				cascadeHalfSize = cascadeHalfSizes[cascadeIndex];

				glm::vec3 position = cameraPos + cascadeHalfSize * directionalLightCascadeOffset * cameraViewDir;

				auto lightSpaceMatrix =
					glm::lookAtLH(position, position + glm::vec3(lightDirection), glm::vec3(0.0f, 1.0f, 0.0f));

				lightSpaceMatrix = glm::orthoLH_ZO(-cascadeHalfSize, cascadeHalfSize, -cascadeHalfSize, cascadeHalfSize,
												   -1000.0f, 1000.0f) *
								   lightSpaceMatrix;

				environmentBlockMap.directionalLight.lightSpaceMatrices[cascadeIndex] = lightSpaceMatrix;
			}
		}
	});


	descriptorSetArrays[2].unmapBuffer(0, 0);


	drawObjects(pSecondaryCommandBuffers, Frustum(cameraBlock.projectionMatrix * cameraBlock.viewMatrix));
}
} // namespace Engine