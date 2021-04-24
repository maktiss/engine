#include "SkymapRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine::Renderers::Graphics {
int SkymapRenderer::init() {
	spdlog::info("Initializing SkymapRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	descriptorSetArrays.resize(2);

	descriptorSetArrays[0].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 64 * 6);
	descriptorSetArrays[0].init(vkDevice, vmaAllocator);

	descriptorSetArrays[1].setBindingLayoutInfo(0, vk::DescriptorType::eUniformBuffer, 16);
	descriptorSetArrays[1].init(vkDevice, vmaAllocator);


	// Update cameras descriptor set

	struct CameraBlock {
		glm::mat4 viewProjectionMatrices[6];
	} cameraBlock;


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

	descriptorSetArrays[0].updateBuffer(0, 0, &cameraBlock, sizeof(cameraBlock));


	// Generate mesh

	const uint verticalVertexCount	 = 15;
	const uint horisontalVertexCount = 25;

	const float yStep		= 1.0f / (verticalVertexCount + 1);
	const float angularStep = (2.0f * M_PI) / horisontalVertexCount;

	skySphereMesh = Engine::Managers::MeshManager::createObject(0);
	skySphereMesh.apply([&](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(2 + verticalVertexCount * horisontalVertexCount);

		auto& indexBuffer = mesh.getIndexBuffer();
		indexBuffer.resize(verticalVertexCount * horisontalVertexCount * 3 * 2, 0);

		// 1-\left(\left(\frac{1+\sqrt[5]{0.5}}{2x+1}-1\right)^{5}+0.5\right)\cdot\left(1-\left(x\right)^{3}\right)
		auto translate = [](float value) {
			const float left  = 3.0f;
			const float right = 3.0f;
			return 1.0f - (std::pow((1.0f + std::pow(0.5f, 1.0f / left)) / (2.0f * value + 1.0f) - 1.0f, left) + 0.5f) *
							  (1.0f - std::pow(value, right));
		};


		// Generate vertices

		std::get<0>(vertexBuffer[0])					   = glm::vec3(0.0f, -1.0f, 0.0f);
		std::get<0>(vertexBuffer[vertexBuffer.size() - 1]) = glm::vec3(0.0f, 1.0f, 0.0f);

		for (uint i = 1; i <= verticalVertexCount; i++) {
			float height = translate(yStep * i) * 2.0f - 1.0f;

			for (uint j = 0; j < horisontalVertexCount; j++) {
				uint index = 1 + (i - 1) * horisontalVertexCount + j;

				float angle = angularStep * j;

				std::get<0>(vertexBuffer[index]).x = std::cos(angle) * std::sin(std::acos(height));
				std::get<0>(vertexBuffer[index]).y = height;
				std::get<0>(vertexBuffer[index]).z = std::sin(angle) * std::sin(std::acos(height));
			}
		}


		// Generate bottom strip

		for (uint i = 0; i < horisontalVertexCount; i++) {
			uint index = i * 3;

			indexBuffer[index + 0] = 0;
			indexBuffer[index + 1] = i + 1;
			indexBuffer[index + 2] = i + 2;
		}

		// Fix last triangle in bottom strip
		indexBuffer[horisontalVertexCount * 3 - 1] = 1;


		// Generate top strip

		uint topVertexIndex = vertexBuffer.size() - 1;

		for (uint i = 0; i < horisontalVertexCount; i++) {
			uint index = indexBuffer.size() - 3 - i * 3;

			indexBuffer[index + 0] = topVertexIndex - 0;
			indexBuffer[index + 1] = topVertexIndex - i - 1;
			indexBuffer[index + 2] = topVertexIndex - i - 2;
		}

		// Fix last triangle in top strip
		indexBuffer[indexBuffer.size() - horisontalVertexCount * 3 + 2] = topVertexIndex - 1;


		// Generate all other strips

		uint offset = horisontalVertexCount * 3;
		for (uint i = 1; i < verticalVertexCount; i++) {
			for (uint j = 0; j < horisontalVertexCount; j++) {
				uint vertexIndex = 1 + (i - 1) * horisontalVertexCount + j;
				uint index		 = offset + (i - 1) * horisontalVertexCount * 6 + j * 6;

				indexBuffer[index + 0] = vertexIndex;
				indexBuffer[index + 1] = vertexIndex + horisontalVertexCount;
				indexBuffer[index + 2] = vertexIndex + horisontalVertexCount + 1;

				indexBuffer[index + 3] = vertexIndex;
				indexBuffer[index + 4] = vertexIndex + horisontalVertexCount + 1;
				indexBuffer[index + 5] = vertexIndex + 1;
			}

			// Fix last triangle in a strip
			uint vertexIndex = 1 + (i - 1) * horisontalVertexCount + horisontalVertexCount - 1;
			uint index		 = offset + (i - 1) * horisontalVertexCount * 6 + (horisontalVertexCount - 1) * 6;

			indexBuffer[index + 2] = vertexIndex + 1;

			indexBuffer[index + 4] = vertexIndex + 1;
			indexBuffer[index + 5] = vertexIndex + 1 - horisontalVertexCount;
		}
	});
	skySphereMesh.update();

	shaderHandle =
		Engine::Managers::GraphicsShaderManager::getHandle<Engine::Graphics::Shaders::SkymapShader>(skySphereMesh);


	return GraphicsRendererBase::init();
}


void SkymapRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
												   double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	// TODO: move to function
	for (uint setIndex = 0; setIndex < descriptorSetArrays.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &descriptorSetArrays[setIndex].getVkDescriptorSet(0), 0, nullptr);
	}


	auto sunDirection = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Light>(
		[&](const auto& transform, auto& light) {
			if (light.castsShadows) {
				if (light.type == Engine::Components::Light::Type::DIRECTIONAL) {
					sunDirection = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * sunDirection;
					sunDirection = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * sunDirection;
					sunDirection = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * sunDirection;
				}
			}
		});

	descriptorSetArrays[1].updateBuffer(0, 0, &sunDirection, sizeof(sunDirection));


	const auto& meshInfo = Engine::Managers::MeshManager::getMeshInfo(skySphereMesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine::Renderers::Graphics